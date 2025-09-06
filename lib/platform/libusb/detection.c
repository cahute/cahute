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

/**
 * Detect USB entries available to Cahute.
 *
 * @param func User function to call back with every USB entry.
 * @param cookie Cookie to pass to the user function.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_libusb_detect_usb(
    cahute_context *context,
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
) {
    libusb_context *lu_context = NULL;
    libusb_device **device_list = NULL;
    cahute_usb_detection_entry entry;
    cahute_ssize device_count;
    int id, err;
    char buf[20];

    err = cahute_get_libusb_context(context, &lu_context);
    if (err)
        return err;

    device_count = libusb_get_device_list(lu_context, &device_list);
    if (device_count < 0) {
        msg(context, ll_fatal, "Could not get a device list.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    for (id = 0; id < device_count; id++) {
        struct libusb_device_descriptor device_descriptor;
        struct libusb_config_descriptor *config_descriptor;
        struct libusb_interface_descriptor const *interface_descriptor;
        int interface_class = 0, interface_subclass = 0, interface_proto = 0;

        if (libusb_get_device_descriptor(device_list[id], &device_descriptor))
            continue;

        if (device_descriptor.idVendor != 0x07cf
            || (device_descriptor.idProduct != 0x6101
                && device_descriptor.idProduct != 0x6102
                && device_descriptor.idProduct != 0x6103))
            continue;

        if (libusb_get_active_config_descriptor(
                device_list[id],
                &config_descriptor
            ))
            continue;

        if (config_descriptor->bNumInterfaces == 1
            && config_descriptor->interface[0].num_altsetting == 1) {
            interface_descriptor = config_descriptor->interface[0].altsetting;
            interface_class = interface_descriptor->bInterfaceClass;
            interface_subclass = interface_descriptor->bInterfaceSubClass;
            interface_proto = interface_descriptor->bInterfaceProtocol;
        }

        libusb_free_config_descriptor(config_descriptor);

        if (interface_class == 8 && interface_subclass == 6
            && interface_proto == 80)
            entry.cahute_usb_detection_entry_type =
                CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI;
        else if (interface_class == 255 && interface_subclass == 0 && interface_proto == 255)
            entry.cahute_usb_detection_entry_type =
                CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL;
        else
            continue;

        sprintf(
            buf,
            "%03d:%03d",
            libusb_get_bus_number(device_list[id]),
            libusb_get_device_address(device_list[id])
        );
        entry.cahute_usb_detection_entry_name = buf;

        if (func(cookie, &entry)) {
            err = CAHUTE_ERROR_INT;
            break;
        }
    }

    libusb_free_device_list(device_list, 1);
    return err;
}
