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
#define HUB_DEFAULT_CACHE_ENTRY_COUNT    1024
#define HUB_ADDITIONAL_CACHE_ENTRY_COUNT 1024

CAHUTE_DECLARE_TYPE(hub_cache_entry)
CAHUTE_DECLARE_TYPE(dev_cookie)

/**
 * Hub cache.
 *
 * This is either a positive cache, with an opened handle in such case,
 * or a negative hub, in which case the handle is INVALID_HANDLE_VALUE,
 *
 * @property handle Handle.
 * @property devinst Device instance for the hub.
 * @property is_hub Whether the device is a hub, or not.
 */
struct hub_cache_entry {
    HANDLE handle;
    DWORD devinst;
    int is_hub;
};

/**
 * Cookie for USB device enumeration.
 *
 * @property context Current context.
 * @property filter Current filter.
 * @property func User function to call.
 * @property cookie Cookie to pass to the user function on call.
 * @property cfgmgr32 Opened Cfgmgr32 library.
 * @property hub_cache Hub cache.
 * @property hub_cache_length Hub cache length.
 * @property hub_cache_capacity Hub cache capacity.
 * @property small_hub_cache Small hub cache, to avoid allocating dynamically
 *           if we can.
 */
struct dev_cookie {
    cahute_context *context;
    cahute_enumerate_win32_usb_device_func *func;
    void *cookie;
    cahute_win32_cfgmgr32 *cfgmgr32;
    hub_cache_entry *hub_cache;
    size_t hub_cache_length;
    size_t hub_cache_capacity;
    hub_cache_entry small_hub_cache[HUB_DEFAULT_CACHE_ENTRY_COUNT];
};

/**
 * Data obtained from a connection information.
 *
 * @property address Device address.
 * @property vendor_id Vendor ID.
 * @property product_id Product ID.
 * @property num_configurations Number of configurations.
 */
struct conn_info {
    int device_address;
    int vendor_id;
    int product_id;
    int num_configurations;
};

/**
 * Data obtained from a configuration descriptor.
 *
 * @property interface_class
 * @property interface_subclass
 * @property interface_protocol
 */
struct config_descriptor_data {
    int interface_class;
    int interface_subclass;
    int interface_protocol;
};

/* ---
 * USB device enumeration.
 * --- */

/**
 * Get a hub for a given device.
 *
 * @param cookie Cookie on which to access to the hub cache entries.
 */
CAHUTE_LOCAL(int)
cahute_open_win32_usb_hub_for_device(
    dev_cookie *cookie,
    cahute_win32_device const *device,
    HANDLE *handlep
) {
    DWORD devinst = device->device_instance;
    hub_cache_entry *entry = NULL;
    DWORD cret;

    /* Access the device parent recursively, until we reach a hub.
     * Every entry is either:
     * - A hub we've already identified and opened.
     * - A non-hub we've already identified.
     * - A device for which we need to determine the nature. */
    while (1) {
        char *device_id = NULL, small_device_id[300];
        char *interface_list = NULL, small_interface_list[300];
        ULONG device_id_len, interface_list_len;
        DWORD cerr;
        size_t i = 0;

        cerr = (*cookie->cfgmgr32->get_parent)(&devinst, devinst, 0);
        if (cerr == 0x0D /* CR_NO_SUCH_DEVNODE */)
            return CAHUTE_ERROR_NOT_FOUND;
        else if (cerr) {
            msg(cookie->context,
                ll_error,
                "CM_Get_Parent returned error 0x%08lX.",
                cerr);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Check if it corresponds to a known device. */
        entry = NULL;
        for (i = 0; i < cookie->hub_cache_length; i++) {
            hub_cache_entry *i_entry = &cookie->hub_cache[i];

            if (i_entry->devinst != devinst)
                continue;

            entry = i_entry;
            break;
        }

        if (entry) {
            if (!entry->is_hub)
                continue;

            break;
        }

        /* No cache entry; we need to create the new entry and populate it
         * with the obtained data. */
        {
            hub_cache_entry *new_cache;
            size_t new_capacity =
                cookie->hub_cache_capacity + HUB_ADDITIONAL_CACHE_ENTRY_COUNT;

            if (cookie->hub_cache_length < cookie->hub_cache_capacity) {
            } else if (cookie->hub_cache == cookie->small_hub_cache) {
                /* First dynamic allocation of the hub cache. */
                new_cache = malloc(sizeof(hub_cache_entry) * new_capacity);
                if (!cookie->hub_cache)
                    return CAHUTE_ERROR_ALLOC;

                memcpy(
                    new_cache,
                    cookie->hub_cache,
                    sizeof(hub_cache_entry) * cookie->hub_cache_capacity
                );

                cookie->hub_cache = new_cache;
                cookie->hub_cache_capacity = new_capacity;
            } else {
                /* Next dynamic allocation, need re-allocation. */
                new_cache = realloc(
                    cookie->hub_cache,
                    sizeof(hub_cache_entry) * new_capacity
                );
                if (!new_cache)
                    return CAHUTE_ERROR_ALLOC;

                cookie->hub_cache = new_cache;
                cookie->hub_cache_capacity = new_capacity;
            }
        }

        entry = &cookie->hub_cache[cookie->hub_cache_length++];
        entry->devinst = devinst;
        entry->handle = INVALID_HANDLE_VALUE;
        entry->is_hub = 0;

        /* Here, we need to determine whether the entry is a hub, and if it
         * is the case, open the hub interface.
         * In order to do this, we need to get the hub interface from the
         * device, and if it doesn't exist, consider the device as not a hub.
         * In order to do this, we need to get the device ID from the
         * device instance. How fun! */
        device_id = NULL;
        interface_list = NULL;

        cret = (*cookie->cfgmgr32
                     ->get_device_id_size)(&device_id_len, devinst, 0);
        if (cret) {
            msg(cookie->context,
                ll_warn,
                "CM_Get_Device_ID_Size returned error 0x%08lX.",
                cret);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (device_id_len < sizeof(small_device_id))
            device_id = small_device_id;
        else if (!(device_id = malloc(device_id_len + 1)))
            return CAHUTE_ERROR_ALLOC;

        cret =
            (*cookie->cfgmgr32
                  ->get_device_id)(devinst, device_id, device_id_len + 1, 0);
        if (cret) {
            msg(cookie->context,
                ll_warn,
                "CM_Get_Device_IDA returned error 0x%08lX.",
                cret);
            if (device_id != small_device_id)
                free(device_id);

            return CAHUTE_ERROR_UNKNOWN;
        }

        cret = (*cookie->cfgmgr32->get_device_interface_list_size)(
            &interface_list_len,
            &cahute_guid_devinterface_usb_hub,
            device_id,
            0
        );
        if (cret) {
            msg(cookie->context,
                ll_warn,
                "CM_Get_Device_Interface_List_SizeA returned error "
                "0x%08lX.",
                cret);
            if (device_id != small_device_id)
                free(device_id);

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (interface_list_len <= 1) {
            /* The device does not have USB hub interfaces; it is therefore
             * not a hub. */
            if (device_id != small_device_id)
                free(device_id);

            continue;
        }

        /* The device has at least one hub interface; we want to open it. */
        if (interface_list_len < sizeof(small_interface_list))
            interface_list = small_interface_list;
        else if (!(interface_list = malloc(interface_list_len + 1))) {
            if (device_id != small_device_id)
                free(device_id);

            return CAHUTE_ERROR_ALLOC;
        }

        cret = (*cookie->cfgmgr32->get_device_interface_list)(
            &cahute_guid_devinterface_usb_hub,
            device_id,
            interface_list,
            interface_list_len + 1,
            0
        );
        if (cret) {
            msg(cookie->context,
                ll_warn,
                "CM_Get_Device_Interface_ListA returned error 0x%08lX.",
                cret);
            if (interface_list != small_interface_list)
                free(interface_list);
            if (device_id != small_device_id)
                free(device_id);

            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Open the hub! */
        entry->handle = CreateFileA(
            interface_list,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        /* In any case, we don't need those anymore. */
        if (interface_list != small_interface_list)
            free(interface_list);
        if (device_id != small_device_id)
            free(device_id);

        entry->is_hub = 1;
        if (entry->handle == INVALID_HANDLE_VALUE) {
            log_windows_error(cookie->context, "CreateFileA", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        break;
    }

    *handlep = entry->handle;
    return CAHUTE_OK;
}

/**
 * Get connection information from a Win32 USB hub.
 *
 * @param cookie Cookie.
 * @param hub_handle Handle to the hub.
 * @param connection_index Index of the connection.
 * @param info Information to fill.
 */
CAHUTE_LOCAL(int)
cahute_get_win32_usb_connection_information(
    dev_cookie *cookie,
    HANDLE *hub_handle,
    DWORD connection_index,
    struct conn_info *info
) {
    USB_NODE_CONNECTION_INFORMATION conn_info;
    DWORD size = sizeof(conn_info);

    conn_info.ConnectionIndex = connection_index;
    if (!DeviceIoControl(
            hub_handle,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
            &conn_info,
            size,
            &conn_info,
            size,
            &size,
            NULL
        )) {
        log_windows_error(cookie->context, "DeviceIoControl", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (conn_info.ConnectionStatus != DeviceConnected) {
        msg(cookie->context, ll_warn, "Device not marked as connected.");
        return CAHUTE_ERROR_NOT_FOUND;
    }

    /* NOTE: While bDeviceClass, bDeviceSubClass and bDeviceProtocol
     * are present in the USB_DEVICE_DESCRIPTOR instance, they are
     * set to 0, meaning we need to make a request for the interface
     * descriptor specifically. */
    msg(cookie->context, ll_debug, "Data obtained from device descriptor:");
    msg(cookie->context,
        ll_debug,
        "  idVendor: %04X",
        conn_info.DeviceDescriptor.idVendor);
    msg(cookie->context,
        ll_debug,
        "  idProduct: %04X",
        conn_info.DeviceDescriptor.idProduct);

    info->device_address = conn_info.DeviceAddress;
    info->vendor_id = conn_info.DeviceDescriptor.idVendor;
    info->product_id = conn_info.DeviceDescriptor.idProduct;
    info->num_configurations = conn_info.DeviceDescriptor.bNumConfigurations;
    return CAHUTE_OK;
}

/**
 * Get data from a configuration descriptor.
 *
 * @param cookie Cookie.
 * @param hub_handle Handle to the hub.
 * @param connection_index Connection index.
 * @param info Information to fill.
 */
CAHUTE_LOCAL(int)
cahute_get_win32_usb_config_descriptor(
    dev_cookie *cookie,
    HANDLE *hub_handle,
    DWORD connection_index,
    struct config_descriptor_data *info
) {
    UCHAR short_req_buf
        [sizeof(USB_DESCRIPTOR_REQUEST)
         + sizeof(USB_CONFIGURATION_DESCRIPTOR)];
    DWORD size = sizeof(short_req_buf), ret_size = size;
    USHORT total_length;
    USB_DESCRIPTOR_REQUEST *req = NULL;
    USB_CONFIGURATION_DESCRIPTOR *desc;
    USB_INTERFACE_DESCRIPTOR *intf;
    int err = CAHUTE_ERROR_UNKNOWN, req_allocated = 0;

    /* We start by making a dummy request to get the total length of
     * the data. */
    req = (void *)short_req_buf;
    desc = (void *)req->Data;

    req->ConnectionIndex = connection_index;
    req->SetupPacket.bmRequest = 0x80;
    req->SetupPacket.bRequest = 0x06;
    req->SetupPacket.wValue = 0x0200;
    req->SetupPacket.wIndex = 0;
    req->SetupPacket.wLength = (USHORT)sizeof(*desc);

    if (!DeviceIoControl(
            hub_handle,
            IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
            req,
            size,
            req,
            size,
            &ret_size,
            NULL
        )) {
        log_windows_error(cookie->context, "DeviceIoControl", GetLastError());
        goto fail;
    }

    if (ret_size != size && ret_size + 1 != size) {
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
        msg(cookie->context, ll_error, "More than 1 interface, unsupported.");
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

    req_allocated = 1;

    /* Now we make the request to get all of the data. */
    req->ConnectionIndex = connection_index;
    req->SetupPacket.bmRequest = 0x80;
    req->SetupPacket.bRequest = 0x06;
    req->SetupPacket.wValue = 0x200;
    req->SetupPacket.wIndex = 0;
    req->SetupPacket.wLength = total_length;

    ret_size = size;
    if (!DeviceIoControl(
            hub_handle,
            IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
            req,
            size,
            req,
            size,
            &ret_size,
            NULL
        )) {
        log_windows_error(cookie->context, "DeviceIoControl", GetLastError());
        free(req);
        goto fail;
    }

    desc = (void *)req->Data;
    if (size != ret_size && ret_size + 1 != size) {
        msg(cookie->context,
            ll_error,
            "Unexpected configuration descriptor size on full request"
            "(obtained: %lu, expected: %lu)",
            ret_size,
            size);
        free(req);
        goto fail;
    }

    if (desc->wTotalLength != total_length) {
        msg(cookie->context,
            ll_error,
            "Unexpected total length change (obtained: %lu, previously "
            "obtained: %lu)",
            desc->wTotalLength,
            total_length);
        goto fail;
    }

    intf = (void *)&desc[1];

    info->interface_class = intf->bInterfaceClass;
    info->interface_subclass = intf->bInterfaceSubClass;
    info->interface_protocol = intf->bInterfaceProtocol;

    msg(cookie->context, ll_debug, "Data obtained from interface descriptor:");
    msg(cookie->context,
        ll_debug,
        "  bInterfaceClass: %u",
        info->interface_class);
    msg(cookie->context,
        ll_debug,
        "  bInterfaceSubClass: %u",
        info->interface_subclass);
    msg(cookie->context,
        ll_debug,
        "  bInterfaceProtocol: %u",
        info->interface_protocol);

    err = CAHUTE_OK;
fail:
    if (req_allocated)
        free(req);
    return err;
}

/**
 * Match a USB device.
 *
 * From the obtained device information, we need to find the device address
 * and VID/PID to ensure it is a CASIO calculator we know of,
 * as well as the driver we need to use to interact with the device.
 *
 * The driver can be obtained from the data provided in the device.
 * We can use the service as well as the driver string to get our information.
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
    struct conn_info conn_info;
    struct config_descriptor_data cd_data;
    HANDLE hub_handle;
    char const *driver_name;
    int entry_type;
    int err = CAHUTE_ERROR_UNKNOWN;

    /* Open the hub corresponding to the device.
     * If we cannot find one, it may just be a hub itself, so we continue. */
    err = cahute_open_win32_usb_hub_for_device(cookie, device, &hub_handle);
    if (err) {
        if (err == CAHUTE_ERROR_NOT_FOUND)
            return CAHUTE_OK;

        msg(cookie->context,
            ll_warn,
            "Could not open the parent hub (%s).",
            cahute_get_error_name(err));
        return err;
    }

    /* We can now obtain the device descriptor for the device,
     * and check for the vendor and product identifier, as well as the
     * device address. */
    err = cahute_get_win32_usb_connection_information(
        cookie,
        hub_handle,
        device->address,
        &conn_info
    );
    if (err)
        goto fail;

    if (conn_info.vendor_id != 0x07cf
        || (conn_info.product_id != 0x6101 && conn_info.product_id != 0x6102
            && conn_info.product_id != 0x6103))
        goto fail;

    if (conn_info.num_configurations != 1) {
        msg(cookie->context, ll_warn, "More than 1 configs, unsupported.");
        goto fail;
    }

    /* In order to check for the protocol, we now need to make another request
     * for the interface descriptor specifically. */
    err = cahute_get_win32_usb_config_descriptor(
        cookie,
        hub_handle,
        device->address,
        &cd_data
    );
    if (err)
        goto fail;

    if (cd_data.interface_class == 8 && cd_data.interface_subclass == 6
        && cd_data.interface_protocol == 80)
        entry_type = CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI;
    else if (cd_data.interface_class == 255 && cd_data.interface_subclass == 0 && cd_data.interface_protocol == 255)
        entry_type = CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL;
    else {
        msg(cookie->context,
            ll_error,
            "Unsupported interface class %d, subclass %d, protocol %d",
            cd_data.interface_class,
            cd_data.interface_subclass,
            cd_data.interface_protocol);
        goto fail;
    }

    usb_device.driver = CAHUTE_WIN32_USB_DRIVER_UNKNOWN;
    usb_device.entry_type = entry_type;
    usb_device.device_id = device->device_id;

    /* We use the obtained data to identify the driver. */
    if (!device->service) {
    } else if (!strcmp(device->service, "WinUSB"))
        usb_device.driver = CAHUTE_WIN32_USB_DRIVER_WINUSB;
    else if (!strcmp(device->service, "libusb0"))
        usb_device.driver = CAHUTE_WIN32_USB_DRIVER_LIBUSB_WIN32;
    else if (!strcmp(device->service, "libusbK"))
        usb_device.driver = CAHUTE_WIN32_USB_DRIVER_LIBUSBK;
    else if (!strcmp(device->service, "USBSTOR"))
        usb_device.driver = CAHUTE_WIN32_USB_DRIVER_VOLMGR;
    else if (!strcmp(device->service, "PVUSB")) {
        if (device->driver_name
            && !strcmp(device->driver_name, "CESG502 USB")) {
            if (device->driver_version
                && strcmp(device->driver_version, "1.0.0.0"))
                usb_device.driver = CAHUTE_WIN32_USB_DRIVER_CESG_1;
            else {
                /* If no driver version is provided, we want to be conservative. */
                usb_device.driver = CAHUTE_WIN32_USB_DRIVER_CESG_0;
            }
        }
    }

    switch (usb_device.driver) {
    case CAHUTE_WIN32_USB_DRIVER_VOLMGR:
        driver_name = "VOLMGR / USBSTOR";
        break;
    case CAHUTE_WIN32_USB_DRIVER_CESG_0:
        driver_name = "CESG 1.0.0.0";
        break;
    case CAHUTE_WIN32_USB_DRIVER_CESG_1:
        driver_name = "CESG 1.0.0.1+";
        break;
    case CAHUTE_WIN32_USB_DRIVER_WINUSB:
        driver_name = "WinUSB";
        break;
    case CAHUTE_WIN32_USB_DRIVER_LIBUSB_WIN32:
        driver_name = "libusb-win32";
        break;
    case CAHUTE_WIN32_USB_DRIVER_LIBUSBK:
        driver_name = "libusbK";
        break;
    default:
        driver_name = "(unknown)";
    }

    msg(cookie->context, ll_debug, "Interpreted driver is %s.", driver_name);

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
 * @param context Context.
 * @param filter Path to look for.
 * @param func User function to call back with every USB entry.
 * @param cookie Cookie to pass to the user function.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_enumerate_win32_usb_devices(
    cahute_context *context,
    char const *filter,
    cahute_enumerate_win32_usb_device_func *func,
    void *cookie
) {
    cahute_win32_device_filter dev_filter;
    dev_cookie internal_cookie;
    int err;
    size_t i;

    err = cahute_get_win32_cfgmgr32(context, &internal_cookie.cfgmgr32);
    if (err)
        return err;

    internal_cookie.context = context;
    internal_cookie.func = func;
    internal_cookie.cookie = cookie;
    internal_cookie.hub_cache = internal_cookie.small_hub_cache;
    internal_cookie.hub_cache_length = 0;
    internal_cookie.hub_cache_capacity = HUB_DEFAULT_CACHE_ENTRY_COUNT;

    dev_filter.related_to_device_id = NULL;
    dev_filter.in_removal_relations_of_device_id = NULL;
    dev_filter.device_class = &cahute_guid_devclass_usb;
    dev_filter.interface_class = NULL;
    dev_filter.path = filter;

    /* Now, we enumerate devices.
     * There are two device classes for USB devices, we need to go over both
     * to ensure we have all of them. */
    msg(context, ll_info, "Enumerating USB devices.");
    dev_filter.device_class = &cahute_guid_devclass_usb;
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
    if (err)
        goto fail;

    dev_filter.device_class = &cahute_guid_devclass_libusb_win32_device;
    err = cahute_enumerate_win32_devices(
        context,
        &dev_filter,
        (cahute_enumerate_win32_device_func *)&match_win32_usb_device,
        &internal_cookie
    );
    if (err)
        goto fail;

    dev_filter.device_class = &cahute_guid_devclass_libusbk_device;
    err = cahute_enumerate_win32_devices(
        context,
        &dev_filter,
        (cahute_enumerate_win32_device_func *)&match_win32_usb_device,
        &internal_cookie
    );

fail:
    /* Close the opened hub entries, and free the hub cache if need be. */
    if (internal_cookie.hub_cache) {
        for (i = 0; i < internal_cookie.hub_cache_length; i++) {
            hub_cache_entry *entry = &internal_cookie.hub_cache[i];

            if (entry->handle != INVALID_HANDLE_VALUE)
                CloseHandle(entry->handle);
        }

        if (internal_cookie.hub_cache != internal_cookie.small_hub_cache)
            free(internal_cookie.hub_cache);
    }

    return err;
}
