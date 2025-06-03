/* ****************************************************************************
 * Copyright (C) 2025 Thomas Touhey <thomas@touhey.fr>
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
#define MAX_HUB_COUNT 255

CAHUTE_DECLARE_TYPE(dev_cookie)
CAHUTE_DECLARE_TYPE(cahute_usb_hub)
CAHUTE_DECLARE_TYPE(cahute_usb_hub_ref)

/**
 * Cookie for USB device enumeration.
 *
 * @property context Current context.
 * @property filter Current filter.
 * @property hub_ref Hub ID reference to use or add to.
 * @property func User function to call.
 * @property cookie Cookie to pass to the user function on call.
 */
struct dev_cookie {
    cahute_context *context;
    cahute_win32_usb_device_filter const *filter;
    cahute_win32_cfgmgr32 *cfgmgr32;
    cahute_usb_hub_ref *hub_ref;
    cahute_enumerate_win32_usb_device_func *func;
    void *cookie;
};

/**
 * Hub related data.
 *
 * @property device_id Device identifier.
 * @property device_instance Device instance for the hub.
 * @property handle Handle to the hub, if already opened.
 */
struct cahute_usb_hub {
    char device_id[100];
    DWORD device_instance;
    HANDLE handle;
};

/**
 * Hub identifier / number reference.
 *
 * @property hubs Hub number to device instance number mapping.
 * @property count Number of assigned hub numbers.
 * @property capacity Maximum number of hub numbers.
 */
struct cahute_usb_hub_ref {
    cahute_usb_hub hubs[MAX_HUB_COUNT];
    size_t count;
    size_t capacity;
};

/* ---
 * Hub ID reference.
 * --- */

/**
 * Close USB hub handles.
 *
 * @param context Context in which to close the handles.
 * @param ref Reference for which to close the handles.
 */
CAHUTE_LOCAL(void)
close_hub_ref_handles(cahute_context *context, cahute_usb_hub_ref *ref) {
    size_t i;

    for (i = 0; i < ref->count; i++) {
        if (ref->hubs[i].handle == INVALID_HANDLE_VALUE)
            continue;

        CloseHandle(ref->hubs[i].handle);
        ref->hubs[i].handle = INVALID_HANDLE_VALUE;
    }
}

/**
 * Destroy the USB hub ID reference.
 *
 * @param context Context for which to destroy the reference.
 * @param ref Reference to destroy.
 */
CAHUTE_LOCAL(void)
free_hub_ref(cahute_context *context, cahute_usb_hub_ref *ref) {
    close_hub_ref_handles(context, ref);
    free(ref);
}

/**
 * Allocate the USB hub ID reference.
 *
 * @param context Context for which to create the reference.
 * @param refp Pointer to set to the allocated reference.
 * @param close_funcp Pointer to set to the close function.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
alloc_hub_ref(
    cahute_context *context,
    cahute_usb_hub_ref **refp,
    cahute_context_destroy_func **close_funcp
) {
    cahute_usb_hub_ref *ref;

    ref = malloc(sizeof(cahute_usb_hub_ref));
    if (!ref)
        return CAHUTE_ERROR_ALLOC;

    ref->count = 0;
    ref->capacity = MAX_HUB_COUNT;

    *refp = ref;
    *close_funcp = (cahute_context_destroy_func *)&free_hub_ref;
    return CAHUTE_OK;
}

/**
 * Get the USB hub ID reference for a context.
 *
 * @param context Context for which to get the reference.
 * @param refp Pointer to set to the hub ID reference.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
get_hub_ref(cahute_context *context, cahute_usb_hub_ref **refp) {
    return cahute_get_context_pointer(
        context,
        (void **)refp,
        CAHUTE_CONTEXT_POINTER_WIN32_HUB_REF_ID,
        (cahute_context_init_func *)&alloc_hub_ref
    );
}

/* ---
 * USB device enumeration.
 * --- */

/**
 * Match a USB hub.
 *
 * We want to enumerate USB hubs in order to assign them a number.
 *
 * @param cookie Cookie.
 * @param device Device information.
 * @return Error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
match_win32_usb_hub(dev_cookie *cookie, cahute_win32_device const *device) {
    cahute_usb_hub *hub;
    size_t i, id_len;

    id_len = strlen(device->device_id) + 1;
    if (id_len > sizeof(cookie->hub_ref->hubs[0].device_id)) {
        msg(cookie->context,
            ll_error,
            "USB hub device id too long (%" CAHUTE_PRIuSIZE
            " > %" CAHUTE_PRIuSIZE ")",
            id_len,
            sizeof(cookie->hub_ref->hubs[0].device_id));
        return CAHUTE_ERROR_SIZE;
    }

    /* Check if a bus number has already been assigned to the device. */
    for (i = 0; i < cookie->hub_ref->count; i++) {
        if (device->device_instance
            == cookie->hub_ref->hubs[i].device_instance)
            return CAHUTE_OK;
    }

    if (cookie->hub_ref->count >= cookie->hub_ref->capacity) {
        msg(cookie->context,
            ll_error,
            "Could not assign a bus number (capacity of %" CAHUTE_PRIuSIZE
            " reached)",
            cookie->hub_ref->capacity);
        return CAHUTE_ERROR_SIZE;
    }

    hub = &cookie->hub_ref->hubs[cookie->hub_ref->count++];
    hub->handle = INVALID_HANDLE_VALUE;
    hub->device_instance = device->device_instance;
    memcpy(hub->device_id, device->device_id, id_len);
    return CAHUTE_OK;
}

/**
 * Match a USB device.
 *
 * From the obtained device information, we need to find the bus number,
 * device address and VID/PID to ensure it is a CASIO calculator we know of,
 * as well as the driver we need to use to interact with the device.
 *
 * The driver can be obtained from the data provided in the device.
 * We can use the service as well as the driver string to get our information.
 *
 * The bus number is a Cahute/libusb construct that does not exist on Windows.
 * What Windows has is a set of USB hub devices with device instances, which
 * we have mapped to indexes on a first exploration pass of USB hubs.
 * So we go up the device tree from the USB device to find the closest hub
 * we know of, and get the bus number we have computed of it.
 *
 * The device address and VID/PID can be obtained by opening a handle to the
 * hub and making an IOCTL_USB_GET_NODE_CONNECTION_INFORMATION request to it,
 * with the connection index corresponding to the port on which the device is
 * present. libusb looks through a bunch of things to get this device port
 * (see ``get_dev_port_number()`` in ``windows_winusb.c``), we will use the
 * address property obtained on the Cfgmgr32 device.
 *
 * @param cookie Cookie.
 * @param device Device information.
 * @return Error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
match_win32_usb_device(dev_cookie *cookie, cahute_win32_device const *device) {
    cahute_win32_usb_device usb_device;
    cahute_usb_hub *hub;
    int hub_number = 0, addr = 0, entry_type;
    int err = CAHUTE_ERROR_UNKNOWN;

    /* Get the hub number and data for the parent device. */
    {
        DWORD devinst = device->device_instance;

        for (hub = NULL; !hub;) {
            DWORD cerr;
            size_t i = 0;

            cerr = (*cookie->cfgmgr32->get_parent)(&devinst, devinst, 0);
            if (cerr == 0x25 /* CR_NO_SUCH_VALUE */)
                break;
            else if (cerr) {
                msg(cookie->context,
                    ll_error,
                    "CM_Get_Parent returned error 0x%08lX.",
                    cerr);
                goto fail;
            }

            /* Check if it corresponds to one of the known hub devices. */
            for (i = 0; i < cookie->hub_ref->count; i++) {
                if (cookie->hub_ref->hubs[i].device_instance == devinst) {
                    hub = &cookie->hub_ref->hubs[i];
                    hub_number = i;
                    break;
                }
            }
        }
    }

    if (!hub) {
        /* No hub has been found, we want to ignore the device. */
        msg(cookie->context, ll_warn, "No hub was found for device.");
        err = CAHUTE_OK;
        goto fail;
    }

    /* We have found the parent hub device.
     * We need to obtain it, if not already done. */
    if (hub->handle == INVALID_HANDLE_VALUE) {
        DWORD cret;
        char path[300];

        cret = (*cookie->cfgmgr32->get_device_interface_list)(
            &cahute_guid_devinterface_usb_hub,
            hub->device_id,
            path,
            sizeof(path),
            0
        );
        if (cret) {
            msg(cookie->context,
                ll_error,
                "CM_Get_Device_Interface_ListA returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (!path[0]) {
            msg(cookie->context,
                ll_error,
                "No device interface for hub %d.",
                hub_number);
            goto fail;
        }

        hub->handle = CreateFileA(
            path,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        if (hub->handle == INVALID_HANDLE_VALUE) {
            msg(cookie->context, ll_error, "Could not open hub %d.", hub_number
            );
            err = CAHUTE_ERROR_PRIV;
            goto fail;
        }
    }

    /* We can now obtain the device descriptor for the device,
     * and check for the vendor and product identifier, as well as the
     * device address. */
    {
        unsigned int vid, pid;
        USB_NODE_CONNECTION_INFORMATION conn_info;
        DWORD size = sizeof(conn_info);

        conn_info.ConnectionIndex = device->address;
        err = CAHUTE_ERROR_UNKNOWN;
        if (!DeviceIoControl(
                hub->handle,
                IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                &conn_info,
                size,
                &conn_info,
                size,
                &size,
                NULL
            )) {
            log_windows_error(
                cookie->context,
                "DeviceIoControl",
                GetLastError()
            );
            goto fail;
        }

        if (conn_info.ConnectionStatus != DeviceConnected) {
            msg(cookie->context, ll_warn, "Device not marked as connected.");
            err = CAHUTE_ERROR_NOT_FOUND;
            goto fail;
        }

        /* NOTE: While bDeviceClass, bDeviceSubClass and bDeviceProtocol
         * are present in the USB_DEVICE_DESCRIPTOR instance, they are
         * set to 0, meaning we need to make a request for the interface
         * descriptor specifically. */
        addr = conn_info.DeviceAddress;
        vid = conn_info.DeviceDescriptor.idVendor;
        pid = conn_info.DeviceDescriptor.idProduct;

        msg(cookie->context, ll_info, "Data obtained from device descriptor:");
        msg(cookie->context, ll_info, "  idVendor: %04X", vid);
        msg(cookie->context, ll_info, "  idProduct: %04X", pid);

        err = CAHUTE_OK;
        if (vid != 0x07cf || (pid != 0x6101 && pid != 0x6102 && pid != 0x6103))
            goto fail;

        if (conn_info.DeviceDescriptor.bNumConfigurations != 1) {
            msg(cookie->context, ll_warn, "More than 1 configs, unsupported.");
            goto fail;
        }
    }

    /* If need be to apply a filter, now is the time. */
    err = CAHUTE_OK;
    if (!cookie->filter) {
    } else if (cookie->filter->type == CAHUTE_WIN32_USB_FILTER_ADDR) {
        if (hub_number != cookie->filter->data.addr.bus
            || addr != cookie->filter->data.addr.address) {
            msg(cookie->context,
                ll_info,
                "Address mismatch (obtained: %03d:%03d, expected: %03d:%03d)",
                hub_number,
                addr,
                cookie->filter->data.addr.bus,
                cookie->filter->data.addr.address);
            goto fail;
        }
    }

    /* In order to check for the protocol, we now need to make another request
     * for the interface descriptor specifically. */
    {
        UCHAR short_req_buf
            [sizeof(USB_DESCRIPTOR_REQUEST)
             + sizeof(USB_CONFIGURATION_DESCRIPTOR)];
        DWORD size = sizeof(short_req_buf), ret_size = size;
        USHORT total_length;
        USB_DESCRIPTOR_REQUEST *req = NULL;
        USB_CONFIGURATION_DESCRIPTOR *desc;
        USB_INTERFACE_DESCRIPTOR *intf;
        unsigned int interface_class, interface_subclass, interface_proto;

        err = CAHUTE_ERROR_UNKNOWN;

        /* We start by making a dummy request to get the total length of
         * the data. */
        req = (void *)short_req_buf;
        desc = (void *)req->Data;

        req->ConnectionIndex = device->address;
        req->SetupPacket.bmRequest = 0x80;
        req->SetupPacket.bRequest = 0x06;
        req->SetupPacket.wValue = 0x0200;
        req->SetupPacket.wIndex = 0;
        req->SetupPacket.wLength = (USHORT)sizeof(*desc);

        if (!DeviceIoControl(
                hub->handle,
                IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                req,
                size,
                req,
                size,
                &ret_size,
                NULL
            )) {
            log_windows_error(
                cookie->context,
                "DeviceIoControl",
                GetLastError()
            );
            goto fail;
        }

        if (ret_size != size) {
            msg(cookie->context,
                ll_error,
                "Unexpected descriptor size (obtained: %lu, expected: %lu)",
                ret_size,
                size);
            goto fail;
        }

        total_length = desc->wTotalLength;
        if (total_length < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
            msg(cookie->context,
                ll_error,
                "Obtained total length (%lu) is shorter than expected (%lu)",
                total_length,
                sizeof(USB_CONFIGURATION_DESCRIPTOR));
            goto fail;
        }

        if (desc->bNumInterfaces != 1) {
            msg(cookie->context,
                ll_error,
                "More than 1 interface, unsupported.");
            goto fail;
        }

        if (total_length < sizeof(USB_CONFIGURATION_DESCRIPTOR)
                               + sizeof(USB_INTERFACE_DESCRIPTOR)) {
            msg(cookie->context,
                ll_error,
                "Total length not enough to contain interface descriptor.");
            goto fail;
        }

        size = sizeof(USB_DESCRIPTOR_REQUEST) + total_length;
        req = malloc(size);
        if (!req) {
            err = CAHUTE_ERROR_ALLOC;
            goto fail;
        }

        /* Now we make the request to get all of the data. */
        req->ConnectionIndex = device->address;
        req->SetupPacket.bmRequest = 0x80;
        req->SetupPacket.bRequest = 0x06;
        req->SetupPacket.wValue = 0x200;
        req->SetupPacket.wIndex = 0;
        req->SetupPacket.wLength = total_length;

        ret_size = size;
        if (!DeviceIoControl(
                hub->handle,
                IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                req,
                size,
                req,
                size,
                &ret_size,
                NULL
            )) {
            log_windows_error(
                cookie->context,
                "DeviceIoControl",
                GetLastError()
            );
            free(req);
            goto fail;
        }

        desc = (void *)req->Data;
        if ((size != ret_size) || (desc->wTotalLength != total_length)) {
            msg(cookie->context,
                ll_error,
                "Unexpected configuration descriptor size on full request");
            free(req);
            goto fail;
        }

        intf = (void *)&desc[1];

        interface_class = intf->bInterfaceClass;
        interface_subclass = intf->bInterfaceSubClass;
        interface_proto = intf->bInterfaceProtocol;
        free(req);

        msg(cookie->context,
            ll_info,
            "Data obtained from interface descriptor:");
        msg(cookie->context, ll_info, "  bInterfaceClass: %u", interface_class
        );
        msg(cookie->context,
            ll_info,
            "  bInterfaceSubClass: %u",
            interface_subclass);
        msg(cookie->context,
            ll_info,
            "  bInterfaceProtocol: %u",
            interface_proto);

        if (interface_class == 8 && interface_subclass == 6
            && interface_proto == 80)
            entry_type = CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI;
        else if (interface_class == 255 && interface_subclass == 0 && interface_proto == 255)
            entry_type = CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL;
        else {
            msg(cookie->context,
                ll_error,
                "Unsupported interface class %d and interface subclass %d",
                interface_class,
                interface_subclass);
            goto fail;
        }
    }

    usb_device.driver = CAHUTE_WIN32_USB_DRIVER_UNKNOWN;
    usb_device.entry_type = entry_type;
    usb_device.bus = hub_number;
    usb_device.addr = addr;
    usb_device.device_id = device->device_id;

    /* We use the obtained data to identify the driver. */
    if (!device->service) {
    } else if (!strcmp(device->service, "WinUSB"))
        usb_device.driver = CAHUTE_WIN32_USB_DRIVER_WINUSB;
    else if (!strcmp(device->service, "USBSTOR"))
        usb_device.driver = CAHUTE_WIN32_USB_DRIVER_VOLMGR;
    else if (!strcmp(device->service, "PVUSB")) {
        if (device->driver_name
            && !strcmp(device->driver_name, "CESG502 USB")) {
            if (device->driver_version
                && strcmp(device->driver_version, "1.0.0.0") != 0)
                usb_device.driver = CAHUTE_WIN32_USB_DRIVER_CESG_1;

            /* If no driver version is provided, we want to be conservative. */
            usb_device.driver = CAHUTE_WIN32_USB_DRIVER_CESG_0;
        }
    }

    return (*cookie->func)(cookie->cookie, &usb_device);

fail:
    return err;
}

/**
 * Detect USB entries available to Cahute.
 *
 * The full extent of the Unified Device Property Model is not available until
 * Windows Vista, and we aim at keeping Windows 2000 and XP compatibility,
 * so we use registry properties on devices.
 *
 * @param func User function to call back with every USB entry.
 * @param cookie Cookie to pass to the user function.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_enumerate_win32_usb_devices(
    cahute_context *context,
    cahute_win32_usb_device_filter const *filter,
    cahute_enumerate_win32_usb_device_func *func,
    void *cookie
) {
    cahute_win32_device_filter dev_filter;
    dev_cookie internal_cookie;
    int err;

    err = cahute_get_win32_cfgmgr32(context, &internal_cookie.cfgmgr32);
    if (err)
        return err;

    err = get_hub_ref(context, &internal_cookie.hub_ref);
    if (err)
        return err;

    internal_cookie.context = context;
    internal_cookie.func = func;
    internal_cookie.cookie = cookie;
    internal_cookie.filter = filter;

    dev_filter.related_to_device_id = NULL;
    dev_filter.in_removal_relations_of_device_id = NULL;
    dev_filter.device_class = &cahute_guid_devclass_usb;
    dev_filter.interface_class = NULL;

    /* First, we want to enumerate USB hubs. */
    msg(context, ll_info, "Enumerating USB hubs.");
    dev_filter.interface_class = &cahute_guid_devinterface_usb_hub;
    err = cahute_enumerate_win32_devices(
        context,
        &dev_filter,
        (cahute_enumerate_win32_device_func *)&match_win32_usb_hub,
        &internal_cookie
    );
    if (err)
        goto fail;

    /* Now, we enumerate devices.
     * There are two device classes for USB devices, we need to go over both
     * to ensure we have all of them. */
    msg(context, ll_info, "Enumerating USB devices.");
    dev_filter.interface_class = &cahute_guid_devinterface_usb_device;
    err = cahute_enumerate_win32_devices(
        context,
        &dev_filter,
        (cahute_enumerate_win32_device_func *)&match_win32_usb_device,
        &internal_cookie
    );
    if (err)
        goto fail;

    dev_filter.device_class = &cahute_guid_devclass_usb_device;
    err = cahute_enumerate_win32_devices(
        context,
        &dev_filter,
        (cahute_enumerate_win32_device_func *)&match_win32_usb_device,
        &internal_cookie
    );

fail:
    close_hub_ref_handles(context, internal_cookie.hub_ref);
    return err;
}
