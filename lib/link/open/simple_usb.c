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
 * Cookie for detection in the context of simple USB link opening.
 *
 * @property context Context in which the USB detection is run.
 * @property found_bus Bus of the found USB device; -1 if no device was found.
 * @property found_address Address relative to the bus of the found USB device;
 *           -1 if no device was found.
 * @property found_type Type of the found address, -1 if not found.
 * @property multiple Flag that, if set to 1, signifies that multiple devices have
 *           already been found.
 */
struct simple_usb_detection_cookie {
    cahute_context *context;
    int found_bus;
    int found_address;
    int found_type;
    int multiple;
    int filter;
};

/**
 * Get the name of a USB detection entry type.
 *
 * @param type Type identifier.
 * @return Name of the type.
 */
CAHUTE_INLINE(char const *) get_usb_detection_type_name(int type) {
    switch (type) {
    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL:
        return "Serial over bulk transfers";

    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI:
        return "USB Mass Storage";

    default:
        return "unknown";
    }
}

/**
 * Simple USB detection callback.
 *
 * @param cookie Simple USB detection cookie.
 * @param entry USB entry.
 */
CAHUTE_LOCAL(int)
cahute_find_simple_usb_device(
    struct simple_usb_detection_cookie *cookie,
    cahute_usb_detection_entry const *entry
) {
    if (cookie->filter)
        switch (entry->cahute_usb_detection_entry_type) {
        case CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL:
            if (!(cookie->filter & CAHUTE_USB_FILTER_SERIAL))
                goto filtered_out;
            break;

        case CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI:
            if (!(cookie->filter & CAHUTE_USB_FILTER_UMS))
                goto filtered_out;
            break;
        }

    if (cookie->found_bus >= 0) {
        /* A device was already found, which means there are at least two
         * connected devices! */
        if (!cookie->multiple) {
            cookie->multiple = 1;
            msg(cookie->context, ll_error, "Multiple devices were found:");
            msg(cookie->context,
                ll_error,
                "- %03d:%03d: %s",
                cookie->found_bus,
                cookie->found_address,
                get_usb_detection_type_name(cookie->found_type));
        }

        msg(cookie->context,
            ll_error,
            "- %03d:%03d: %s",
            entry->cahute_usb_detection_entry_bus,
            entry->cahute_usb_detection_entry_address,
            get_usb_detection_type_name(entry->cahute_usb_detection_entry_type)
        );

        return 0;
    }

    cookie->found_bus = entry->cahute_usb_detection_entry_bus;
    cookie->found_address = entry->cahute_usb_detection_entry_address;
    cookie->found_type = entry->cahute_usb_detection_entry_type;
    return 0;

filtered_out:
    msg(cookie->context, ll_info, "Device was filtered out:");
    msg(cookie->context,
        ll_info,
        "  %03d:%03d: %s",
        entry->cahute_usb_detection_entry_bus,
        entry->cahute_usb_detection_entry_address,
        get_usb_detection_type_name(entry->cahute_usb_detection_entry_type));
    return 0;
}

/**
 * Open a link over a detected USB transport.
 *
 * @param context Context in which the link is opened.
 * @param linkp Pointer to the link to set with the opened link.
 * @param flags Flags to open the link and underlying transport with.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_simple_usb_link(
    cahute_context *context,
    cahute_link **linkp,
    unsigned long flags
) {
    struct simple_usb_detection_cookie cookie;
    int attempts_left, err;

    cookie.filter = flags & CAHUTE_USB_FILTER_MASK;
    flags &= ~CAHUTE_USB_FILTER_MASK;
    switch (cookie.filter) {
    case CAHUTE_USB_FILTER_ANY:
    case CAHUTE_USB_FILTER_SERIAL:
    case CAHUTE_USB_FILTER_UMS:
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported simple USB filter.");
    }

    /* If any filter is provided that does not contain serial devices,
     * we want to set the SEVEN flag for cahute_open_usb_link() not to
     * raise an error if NOCHECK is set. */
    if ((flags & CAHUTE_USB_NOCHECK)
        && !(
            flags & (CAHUTE_USB_SEVEN | CAHUTE_USB_CAS300 | CAHUTE_USB_OHP)
        )) {
        if (!cookie.filter || (cookie.filter & CAHUTE_USB_FILTER_SERIAL)) {
            msg(context,
                ll_error,
                "SEVEN or CAS300 USB flag must be set if check is disabled "
                "and serial devices are candidates.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        flags |= CAHUTE_USB_SEVEN;
    }

    for (attempts_left = 20; attempts_left; attempts_left--) {
        if (attempts_left < 20) {
            msg(context, ll_warn, "Calculator not found, retrying in 250ms.");

            err = cahute_sleep(context, 250);
            if (err)
                return err;
        }

        cookie.context = context;
        cookie.found_bus = -1;
        cookie.found_address = -1;
        cookie.found_type = -1;
        cookie.multiple = 0;

        err = cahute_detect_usb(
            context,
            (cahute_detect_usb_entry_func *)&cahute_find_simple_usb_device,
            &cookie
        );
        if (err)
            return err;

        if (cookie.multiple)
            return CAHUTE_ERROR_TOO_MANY;

        if (cookie.found_bus < 0)
            continue;

        return cahute_open_usb_link(
            context,
            linkp,
            flags,
            cookie.found_bus,
            cookie.found_address
        );
    }

    return CAHUTE_ERROR_NOT_FOUND;
}
