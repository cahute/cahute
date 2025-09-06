/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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

/* USB serial over USB bulk link interface. */
CAHUTE_LOCAL_DATA(cahute_serial_over_usb_bulk_link_interface)
cahute_libusb_serial_over_usb_bulk_link_interface = {
    "Serial over USB bulk (libusb)",
    (cahute_link_close_func *)cahute_close_libusb_link,
    (cahute_link_receive_func *)cahute_receive_on_libusb_bulk_link,
    (cahute_link_send_func *)cahute_send_on_libusb_bulk_link
};

CAHUTE_LOCAL_DATA(cahute_ums_link_interface)
cahute_libusb_ums_link_interface = {
    "UMS (libusb)",
    (cahute_link_close_func *)&cahute_close_libusb_link,
    (cahute_link_scsi_request_to_func *)&cahute_scsi_request_to_libusb_device,
    (cahute_link_scsi_request_from_func
         *)&cahute_scsi_request_from_libusb_device
};

/**
 * Extract bus and device addresses from a given path.
 *
 * The expected format is "%u:%u", where the number on the left is the
 * bus number the number on the right is the address number.
 * Leading zeroes are allowed, and do not change the interpretation of
 * the numbers (decimal).
 *
 * As an example, "004:010" leads to a bus number of 4, and an address of 10.
 *
 * @param path Device path.
 * @param busp Bus pointer.
 * @param addrp Address pointer.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_LOCAL(int)
cahute_parse_libusb_device_path(char const *path, int *busp, int *addrp) {
    int bus = 0, addr = 0;

    if (*path < '0' || *path > '9')
        return CAHUTE_ERROR_NOT_FOUND;

    do {
        bus = bus * 10 + *path++ - '0';
    } while (*path >= '0' && *path <= '9');

    if (*path++ != ':')
        return CAHUTE_ERROR_NOT_FOUND;

    if (*path < '0' || *path > '9')
        return CAHUTE_ERROR_NOT_FOUND;

    do {
        addr = addr * 10 + *path++ - '0';
    } while (*path >= '0' && *path <= '9');

    if (*path)
        return CAHUTE_ERROR_NOT_FOUND;

    *busp = bus;
    *addrp = addr;
    return CAHUTE_OK;
}

/**
 * Open a USB link using libusb.
 *
 * @param context Context in which the link is opened.
 * @param open_params Parameters to pass to the underlying open function.
 * @param path Path to the USB device to open.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_libusb_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path
) {
    libusb_context *lu_context;
    libusb_device **device_list = NULL;
    struct libusb_config_descriptor *config_descriptor = NULL;
    libusb_device_handle *device_handle = NULL;
    cahute_libusb_link_cookie cookie;
    cahute_ssize device_count;
    int i, libusberr, bulk_in = -1, bulk_out = -1;
    int bus, address, is_ums = 0;
    int err = CAHUTE_ERROR_UNKNOWN;

    err = cahute_parse_libusb_device_path(path, &bus, &address);
    if (err)
        goto fail;

    err = cahute_get_libusb_context(context, &lu_context);
    if (err)
        goto fail;

    device_count = libusb_get_device_list(lu_context, &device_list);
    if (device_count < 0) {
        msg(context, ll_fatal, "Could not get a device list.");
        goto fail;
    }

    for (i = 0; i < device_count; i++) {
        struct libusb_device_descriptor device_descriptor;
        struct libusb_interface_descriptor const *interface_descriptor;
        struct libusb_endpoint_descriptor const *endpoint_descriptor;
        int interface_class = 0, interface_subclass = 0, interface_proto = 0;
        int j;

        if (libusb_get_bus_number(device_list[i]) != bus
            || libusb_get_device_address(device_list[i]) != address)
            continue;

        err = CAHUTE_ERROR_INCOMPAT;
        if (libusb_get_device_descriptor(device_list[i], &device_descriptor))
            goto fail;

        if (device_descriptor.idVendor != 0x07cf)
            goto fail;
        if (device_descriptor.idProduct != 0x6101
            && device_descriptor.idProduct != 0x6102
            && device_descriptor.idProduct != 0x6103)
            goto fail;

        /* We want to check the interface class of the default configuration:
         *
         * - If it's 8 (Mass Storage), then we are facing an SCSI device.
         * - If it's 255 (Vendor-Specific), then we are facing a P7 device. */
        libusberr = libusb_get_active_config_descriptor(
            device_list[i],
            &config_descriptor
        );
        if (libusberr)
            goto fail;

        if (config_descriptor->bNumInterfaces != 1
            || config_descriptor->interface[0].num_altsetting != 1)
            goto fail;

        interface_descriptor = config_descriptor->interface[0].altsetting;
        interface_class = interface_descriptor->bInterfaceClass;
        interface_subclass = interface_descriptor->bInterfaceSubClass;
        interface_proto = interface_descriptor->bInterfaceProtocol;

        /* By default the protocol is USB_SEVEN.
         * We need to distinguish here between SEVEN, SEVEN_OHP,
         * USB_MASS_STORAGE and CASIOLINK with variant CAS300.
         *
         * Note that both CASIOLINK with variant CAS300 and USB_SEVEN
         * over bulk-only both present themselves with 07cf:6101 and
         * interface class 0xff (255). While they both present two different
         * iManufacturer strings, we can't use this here because it requires
         * opening the device using libusb_open(), which may result in
         * LIBUSB_ERROR_NOT_SUPPORTED on some platforms including Win32,
         * for which we need to use either CESG502 or SCSI system functions
         * directly.
         *
         * There some other differences that are less reliable, such as bcdUSB
         * being 0x0100 on Classpads, and 0x0110 on fx-9860G and derivatives,
         * so we try to use that for now. */

        if (interface_class == 8 && interface_subclass == 6
            && interface_proto == 80)
            is_ums = 1;
        else if (interface_class == 255 && interface_subclass == 0 && interface_proto == 255)
            is_ums = 0;
        else {
            msg(context,
                ll_error,
                "Unsupported interface class %d and interface subclass %d",
                interface_class,
                interface_subclass);
            goto fail;
        }

        /* Find bulk in and out endpoints.
         * This search is in case they vary between host platforms. */
        for ((void)(endpoint_descriptor = interface_descriptor->endpoint),
             j = interface_descriptor->bNumEndpoints;
             j;
             endpoint_descriptor++, j--) {
            if (IS_LIBUSB_BULK_ENDPOINT(endpoint_descriptor))
                continue;

            switch (endpoint_descriptor->bEndpointAddress & 128) {
            case LIBUSB_ENDPOINT_OUT:
                bulk_out = endpoint_descriptor->bEndpointAddress;
                break;

            case LIBUSB_ENDPOINT_IN:
                bulk_in = endpoint_descriptor->bEndpointAddress;
                break;

            default:
                /* This should not be reached. */
                break;
            }
        }

        if (bulk_in < 0) {
            msg(context, ll_error, "Bulk in endpoint could not be found.");
            goto fail;
        }

        if (bulk_out < 0) {
            msg(context, ll_error, "Bulk out endpoint could not be found.");
            goto fail;
        }

        libusberr = libusb_open(device_list[i], &device_handle);

        switch (libusberr) {
        case 0:
            break;

        case LIBUSB_ERROR_ACCESS:
            err = CAHUTE_ERROR_PRIV;
            goto fail;

        default:
            msg(context,
                ll_error,
                "libusb_open returned %d: %s",
                libusberr,
                libusb_error_name(libusberr));
            goto fail;
        }

        break;
    }

    /* We can arrive here with or without a device handle.
     * The device list could have been empty, or at least one case could
     * have been encountered, but not matched. */

    err = CAHUTE_ERROR_UNKNOWN;
    if (device_list) {
        libusb_free_device_list(device_list, 1);
        device_list = NULL;
    }

    if (config_descriptor) {
        libusb_free_config_descriptor(config_descriptor);
        config_descriptor = NULL;
    }

    if (!device_handle) {
        err = CAHUTE_ERROR_NOT_FOUND;
        goto fail;
    }

    /* Disconnect any kernel driver, if any. */
    libusberr = libusb_detach_kernel_driver(device_handle, 0);

    switch (libusberr) {
    case 0:
    case LIBUSB_ERROR_NOT_SUPPORTED:
    case LIBUSB_ERROR_NOT_FOUND:
        break;

    case LIBUSB_ERROR_ACCESS:
        /* On MacOS / OS X, we actually require an entitlement guaranteed
         * by code signing, and that costs money, so we just don't
         * detach the kernel driver and try to use the device directly. */
        msg(context,
            ll_warn,
            "Kernel driver could not be detached due to access.");
        break;

    case LIBUSB_ERROR_NO_DEVICE:
        err = CAHUTE_ERROR_NOT_FOUND;
        goto fail;

    default:
        msg(context,
            ll_fatal,
            "libusb_detach_kernel_driver returned %d: %s",
            libusberr,
            libusb_error_name(libusberr));
        goto fail;
    }

    /* Claim the interface. */
    libusberr = libusb_claim_interface(device_handle, 0);
    switch (libusberr) {
    case 0:
        break;

    case LIBUSB_ERROR_NO_DEVICE:
    case LIBUSB_ERROR_NOT_FOUND:
        err = CAHUTE_ERROR_NOT_FOUND;
        goto fail;

    case LIBUSB_ERROR_ACCESS:
        /* Same entitlement problems on MacOS / OS X. */
        msg(context, ll_warn, "Interface could not be claimed due to access.");
        break;

    case LIBUSB_ERROR_BUSY:
        msg(context,
            ll_info,
            "Another program/driver has claimed the interface.");
        err = CAHUTE_ERROR_PRIV;
        goto fail;

    default:
        msg(context,
            ll_fatal,
            "libusb_claim_interface returned %d: %s",
            libusberr,
            libusb_error_name(libusberr));
        goto fail;
    }

    if (!is_ums) {
        /* Calculators running 1.x OSes with Protocol 7.00 support may need a
         * push to enable communicating using Protocol 7.00, in the form of
         * a vendor-specific request documented in fxReverse. */
        msg(context, ll_info, "Running vendor-specific interface request 0x01."
        );
        libusberr = libusb_control_transfer(
            device_handle,
            0x41,   /* Vendor-specific interface request. */
            0x01,   /* Request code 0x01 */
            0x0000, /* wValue is unused */
            0x0000, /* wIndex is unused also */
            NULL,
            0,  /* No data transfer. */
            300 /* 300ms should be more than enough. */
        );

        switch (libusberr) {
        case 0:
            break;

        default:
            msg(context,
                ll_fatal,
                "libusb_control_transfer with vendor-specific interface "
                "request 0x01 caused error %d: %s",
                libusberr,
                libusb_error_name(libusberr));
            goto fail;
        }
    }

    cookie.handle = device_handle;
    cookie.bulk_in = bulk_in;
    cookie.bulk_out = bulk_out;

    msg(context, ll_debug, "Bulk in endpoint address is: 0x%02X", bulk_in);
    msg(context, ll_debug, "Bulk out endpoint address is: 0x%02X", bulk_out);

    if (is_ums)
        return cahute_open_ums_link_from_interface(
            open_params,
            &cahute_libusb_ums_link_interface,
            &cookie,
            sizeof(cookie)
        );

    return cahute_open_serial_over_usb_bulk_link_from_interface(
        open_params,
        &cahute_libusb_serial_over_usb_bulk_link_interface,
        &cookie,
        sizeof(cookie)
    );

fail:
    if (config_descriptor)
        libusb_free_config_descriptor(config_descriptor);
    if (device_list)
        libusb_free_device_list(device_list, 1);
    if (device_handle)
        libusb_close(device_handle);

    return err;
}
