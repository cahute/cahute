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
 * Receive from a libusb serial over bulk link.
 *
 * @param context Context on which the function is called.
 * @param cookie Cookie.
 * @param buf Buffer in which to receive.
 * @param capacity Capacity of the buffer.
 * @param receivedp Pointer to the received bytes count to set.
 * @param timeout Timeout; 0 for infinite.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_receive_on_libusb_bulk_link(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    int libusberr, received;

    libusberr = libusb_bulk_transfer(
        cookie->handle,
        cookie->bulk_in,
        buf,
        capacity,
        &received,
        timeout
    );

    switch (libusberr) {
    case 0:
        break;

    case LIBUSB_ERROR_PIPE:
    case LIBUSB_ERROR_NO_DEVICE:
    case LIBUSB_ERROR_IO:
        msg(context, ll_error, "USB device is no longer available.");
        return CAHUTE_ERROR_GONE;

    case LIBUSB_ERROR_TIMEOUT:
        return CAHUTE_ERROR_TIMEOUT;

    default:
        msg(context,
            ll_error,
            "libusb_bulk_transfer returned %d: %s",
            libusberr,
            libusb_error_name(libusberr));
        if (libusberr == LIBUSB_ERROR_OVERFLOW)
            msg(context, ll_error, "Required buffer size was %d.", received);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *receivedp = (size_t)received;
    return CAHUTE_OK;
}

/**
 * Send on a libusb serial over bulk link.
 *
 * @param context Context in which the function is called.
 * @param cookie Cookie.
 * @param buf Buffer to send.
 * @param size Size of the buffer to send.
 * @param sentp Pointer to the written bytes count to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_send_on_libusb_bulk_link(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
) {
    int libusberr, sent;

    libusberr = libusb_bulk_transfer(
        cookie->handle,
        cookie->bulk_out,
        (cahute_u8 *)buf,
        size,
        &sent,
        0 /* Unlimited timeout. */
    );

    switch (libusberr) {
    case 0:
        break;

    case LIBUSB_ERROR_PIPE:
    case LIBUSB_ERROR_NO_DEVICE:
    case LIBUSB_ERROR_IO:
        msg(context, ll_error, "USB device is no longer available.");
        return CAHUTE_ERROR_GONE;

    default:
        msg(context,
            ll_error,
            "libusb_bulk_transfer returned %d: %s",
            libusberr,
            libusb_error_name(libusberr));
        return CAHUTE_ERROR_UNKNOWN;
    }

    *sentp = (size_t)sent;
    return CAHUTE_OK;
}
