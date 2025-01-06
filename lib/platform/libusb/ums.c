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
 * Emit an SCSI request to a libusb device.
 *
 * @param context
 * @param cookie
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param buf Optional data buffer to either send or receive.
 * @param buf_size Size of the data or capacity of the data buffer.
 * @param is_send Whether the data should be sent or received.
 * @param statusp Pointer to the SCSI status to set to the received one.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
libusb_scsi_request(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int is_send,
    int *statusp
) {
    cahute_u8 cbw_buf[32], csw_buf[13];
    cahute_u8 *csw = csw_buf;
    size_t csw_size = 13;
    int libusberr, recv = 0, sent = 0;

    /* Produce a CBW as described in Bulk-Only Transport, and send it. */
    memset(cbw_buf, 0, 31);
    memcpy(cbw_buf, "USBCABCD", 8);
    memcpy(&cbw_buf[15], command, command_size);
    cbw_buf[8] = buf_size & 0xFF;
    cbw_buf[9] = (buf_size >> 8) & 0xFF;
    cbw_buf[10] = (buf_size >> 16) & 0xFF;
    cbw_buf[11] = (buf_size >> 24) & 0xFF;
    cbw_buf[14] = command_size;

    if (!is_send)
        cbw_buf[12] |= 128;

    libusberr = libusb_bulk_transfer(
        cookie->handle,
        cookie->bulk_out,
        (cahute_u8 *)cbw_buf,
        31,
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

    if (!buf_size) {
        /* Nothing to be sent, nothing to be received. */
    } else if (is_send) {
        libusberr = libusb_bulk_transfer(
            cookie->handle,
            cookie->bulk_out,
            buf,
            (int)buf_size,
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
    } else {
        do {
            libusberr = libusb_bulk_transfer(
                cookie->handle,
                cookie->bulk_in,
                buf,
                (int)buf_size,
                &recv,
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

            if (!recv)
                continue;

            buf += recv;
            buf_size -= recv;
        } while (buf_size);
    }

    /* Receive the CSW (status). */
    do {
        libusberr = libusb_bulk_transfer(
            cookie->handle,
            cookie->bulk_in,
            csw,
            (int)csw_size,
            &recv,
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

        csw += recv;
        csw_size -= recv;
    } while (csw_size);

    if (memcmp(csw_buf, "USBSABCD", 8)) {
        msg(context, ll_error, "Unknown or unrecognized UMS CSW:");
        mem(context, ll_error, csw_buf, 13);
        return CAHUTE_ERROR_CORRUPT;
    }

    *statusp = csw_buf[12];
    return CAHUTE_OK;
}

/**
 * Emit an SCSI request with outgoing data to a libusb device.
 *
 * @param context
 * @param cookie
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param data
 * @param data_size
 * @param statusp Pointer to the SCSI status to set to the received one.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_scsi_request_to_libusb_device(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
) {
    return libusb_scsi_request(
        context,
        cookie,
        command,
        command_size,
        (cahute_u8 *)data, /* Explicit removal of const. */
        data_size,
        1,
        statusp
    );
}

/**
 * Emit an SCSI request with incoming data to a libusb device.
 *
 * @param context
 * @param cookie
 * @param command
 * @param command_size
 * @param buf
 * @param buf_size
 * @param statusp
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_scsi_request_from_libusb_device(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
) {
    return libusb_scsi_request(
        context,
        cookie,
        command,
        command_size,
        buf,
        buf_size,
        0,
        statusp
    );
}
