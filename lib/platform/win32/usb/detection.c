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

CAHUTE_DECLARE_TYPE(detect_cookie)

/**
 * Cookie for USB detection.
 *
 * @property context Current context.
 * @property func User function to call.
 * @property cookie Cookie to pass to the user function on call.
 */
struct detect_cookie {
    cahute_context *context;
    cahute_detect_usb_entry_func *func;
    void *cookie;
};

/**
 * Match a USB device.
 *
 * @param cookie Cookie.
 * @param device Device information.
 * @return Error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
match_device(detect_cookie *cookie, cahute_win32_usb_device const *device) {
    cahute_usb_detection_entry entry;

    entry.cahute_usb_detection_entry_name = device->device_id;
    entry.cahute_usb_detection_entry_type = device->entry_type;
    return (*cookie->func)(cookie->cookie, &entry);
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
CAHUTE_INTERNAL(int)
cahute_win32_detect_usb(
    cahute_context *context,
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
) {
    detect_cookie internal_cookie;

    internal_cookie.context = context;
    internal_cookie.func = func;
    internal_cookie.cookie = cookie;

    return cahute_enumerate_win32_usb_devices(
        context,
        NULL,
        (cahute_enumerate_win32_usb_device_func *)&match_device,
        &internal_cookie
    );
}
