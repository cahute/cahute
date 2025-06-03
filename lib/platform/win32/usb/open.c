/* ****************************************************************************
 * Copyright (C) 2024-2025 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#include "internals.h"

CAHUTE_DECLARE_TYPE(open_cookie)
CAHUTE_DECLARE_TYPE(open_volmgr_cookie)

/**
 * Cookie for opening a device interface.
 *
 * @property context Context in which the function is run.
 * @property open_params Open parameters.
 */
struct open_cookie {
    cahute_context *context;
    cahute_usb_link_open_params *open_params;
};

/**
 * Cookie for finding a Win32 VOLMGR device interface path.
 *
 * @property context Context in which the function is run.
 * @property path Path to the device interface.
 * @property path_size Size of the path to the device interface.
 */
struct open_volmgr_cookie {
    cahute_context *context;
    char *path;
    size_t path_size;
};

/**
 * Find a volume interface associated with the provided device identifier.
 *
 * @param context Context in which the function is run.
 * @param path Path to the device interface to fill.
 * @param path_size Size of the path.
 * @param device_id Device identifier.
 * @param raw_guid Device interface GUID to look for.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
find_win32_interface(
    cahute_context *context,
    char *path,
    size_t path_size,
    char const *device_id,
    GUID const *guid
) {
    DWORD property_size = 0;
    DWORD cret;
    int err = 0;
    cahute_win32_cfgmgr32 *cfgmgr32;

    err = cahute_get_win32_cfgmgr32(context, &cfgmgr32);
    if (err)
        return err;

    cret = (*cfgmgr32->get_device_interface_list_size)(
        &property_size,
        guid,
        device_id,
        0
    );
    if (cret) {
        msg(context,
            ll_error,
            "CM_Get_Device_Interface_List_SizeA returned error "
            "0x%08lX.",
            cret);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (property_size > path_size) {
        /* Relevant device interfaces encountered in the wild do
         * not have such a big device interface name, we can skip
         * the entry. */
        return CAHUTE_ERROR_SIZE;
    }

    cret = (*cfgmgr32->get_device_interface_list)(
        guid,
        device_id,
        path,
        property_size,
        0
    );
    if (cret) {
        msg(context,
            ll_error,
            "CM_Get_Device_Interface_ListA returned error "
            "0x%08lX.",
            cret);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!path[0]) {
        /* Missing at least one interface, we want to ignore the
         * current USB device. */
        return CAHUTE_ERROR_NOT_FOUND;
    }

    return CAHUTE_OK;
}

/**
 * Match a volume.
 *
 * @param cookie Cookie.
 * @param device Device information.
 * @return Error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
match_win32_volume(
    open_volmgr_cookie *cookie,
    cahute_win32_device const *device
) {
    int err;

    err = find_win32_interface(
        cookie->context,
        cookie->path,
        cookie->path_size,
        device->device_id,
        &cahute_guid_devinterface_volume
    );
    if (!err)
        return CAHUTE_ERROR_ABORT;

    return err;
}

/**
 * Match a disk drive.
 *
 * @param cookie Cookie.
 * @param device Device information.
 * @return Error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
match_win32_disk_drive(
    open_volmgr_cookie *cookie,
    cahute_win32_device const *device
) {
    cahute_win32_device_filter filter;

    filter.related_to_device_id = NULL;
    filter.in_removal_relations_of_device_id = device->device_id;
    filter.device_class = &cahute_guid_devclass_volume;
    filter.interface_class = NULL;

    return cahute_enumerate_win32_devices(
        cookie->context,
        &filter,
        (cahute_enumerate_win32_device_func *)&match_win32_volume,
        cookie
    );
}

/**
 * Find a USB device for the provided interface type.
 *
 * For volumes, while on Windows XP the volume is a child device to the
 * USB device, on Windows 11 the volume is in a completely different tree,
 * being attached to the volume manager (volmgr). However, on both,
 * we can use Bus Relations to get the disk drive from the USB device,
 * then the volume from the disk drive.
 *
 * @param context Context in which the function is run.
 * @param device Device to open.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
match_device(open_cookie *cookie, cahute_win32_usb_device const *device) {
    cahute_win32_device_filter filter;
    open_volmgr_cookie volmgr_cookie;
    char path[300];
    size_t max_read_capacity = 0;
    int err;

    switch (device->driver) {
    case CAHUTE_WIN32_USB_DRIVER_CESG_0:
        max_read_capacity = 4096;
        /* FALLTHRU */
    case CAHUTE_WIN32_USB_DRIVER_CESG_1:
        /* For CESG502, the device interface to use is directly associated
         * with the device. */
        err = find_win32_interface(
            cookie->context,
            path,
            sizeof(path),
            device->device_id,
            &cahute_guid_devinterface_usb_device
        );
        if (err)
            return err;

        err = cahute_open_win32_cesg_link(
            cookie->context,
            cookie->open_params,
            path,
            max_read_capacity
        );
        if (!err)
            return CAHUTE_ERROR_ABORT;

        return err;

    case CAHUTE_WIN32_USB_DRIVER_VOLMGR:
        volmgr_cookie.context = cookie->context;
        volmgr_cookie.path = path;
        volmgr_cookie.path_size = sizeof(path);

        filter.related_to_device_id = device->device_id;
        filter.in_removal_relations_of_device_id = NULL;
        filter.device_class = &cahute_guid_devclass_diskdrive;
        filter.interface_class = NULL;

        err = cahute_enumerate_win32_devices(
            cookie->context,
            &filter,
            (cahute_enumerate_win32_device_func *)&match_win32_disk_drive,
            &volmgr_cookie
        );

        if (err == CAHUTE_OK)
            return CAHUTE_ERROR_NOT_FOUND;
        else if (err != CAHUTE_ERROR_ABORT)
            return err;

        err = cahute_open_win32_ums_link(
            cookie->context,
            cookie->open_params,
            path
        );
        if (!err)
            return CAHUTE_ERROR_ABORT;

        return err;

    case CAHUTE_WIN32_USB_DRIVER_WINUSB:
        if (device->entry_type != CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL)
            break;

        /* For WinUSB, the device interface to use is directly associated
         * with the device. */
        err = find_win32_interface(
            cookie->context,
            path,
            sizeof(path),
            device->device_id,
            &cahute_guid_devinterface_usb_device
        );
        if (err)
            return err;

        err = cahute_open_win32_winusb_bulk_link(
            cookie->context,
            cookie->open_params,
            path
        );
        if (!err)
            return CAHUTE_ERROR_ABORT;

        return err;
    }

    msg(cookie->context,
        ll_error,
        "For USB device at %03d:%03d:",
        device->bus,
        device->addr);
    CAHUTE_RETURN_IMPL(cookie->context, "  Unsupported USB driver / type.");
}

/**
 * Open a USB device using the Win32 interface and bus/address numbers.
 *
 * @param context
 * @param open_params
 * @param bus
 * @param address
 * @return
 */
CAHUTE_EXTERN(int)
cahute_open_win32_usb_device_from_address(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    int bus,
    int address
) {
    cahute_win32_usb_device_filter filter;
    open_cookie cookie;
    int err;

    cookie.context = context;
    cookie.open_params = open_params;

    filter.type = CAHUTE_WIN32_USB_FILTER_ADDR;
    filter.data.addr.bus = bus;
    filter.data.addr.address = address;

    err = cahute_enumerate_win32_usb_devices(
        context,
        &filter,
        (cahute_enumerate_win32_usb_device_func *)&match_device,
        &cookie
    );
    if (err == CAHUTE_ERROR_ABORT)
        return CAHUTE_OK;
    else if (err == CAHUTE_OK)
        return CAHUTE_ERROR_NOT_FOUND;

    return err;
}
