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

/**
 * Send data from a stream.
 *
 * Note that packet shifting is enabled only when not disabled explicitely
 * (e.g. for sensitive payloads, such as with command 0x56 "Upload and run"),
 * or when not on a reliable enough transport (i.e. not serial).
 *
 * Also note that the command code to use as data packet subtypes has already
 * been set as `link->protocol_state.seven.last_command` by
 * :c:func:`cahute_seven_send_command`, so we use that instead of requiring
 * it in the parameters.
 *
 * @param link Link with which to send the data.
 * @param flags OR'd `SEND_DATA_FLAG_*` constants.
 * @param file Stream to read data from.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_send_bytes_from_stream(
    cahute_link *link,
    unsigned long flags,
    cahute_file *file,
    unsigned long size,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 buf[264];
    size_t last_packet_size;
    unsigned long packet_count;
    unsigned long offset = 0;
    unsigned long i, loop_send_flags = 0;
    int err, shifted = 0;

    last_packet_size = size & 255;
    packet_count = (size >> 8) + !!last_packet_size;
    last_packet_size = last_packet_size ? last_packet_size : 256;
    cahute_set_ascii_hex(buf, (packet_count >> 8) & 255);
    cahute_set_ascii_hex(&buf[2], packet_count & 255);

    if (packet_count >= 3
        && link->protocol != CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN
        && (~flags & CAHUTE_SEVEN_SEND_BYTES_FLAG_DISABLE_SHIFTING)) {
        /* We are about to start packet shifting.
         * For more information, please consult the following:
         * https://cahute.org/topics/protocols/seven/flows.html
         * #packet-shifting */
        buf[4] = '0';
        buf[5] = '0';
        buf[6] = '0';
        buf[7] = '1';

        err = cahute_read_from_file(file, offset, &buf[8], 256);
        if (err)
            return err;

        offset += 256;

        err = cahute_seven_send_extended(
            link,
            CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
            PACKET_TYPE_DATA,
            link->protocol_state.seven.last_command,
            buf,
            264,
            TIMEOUT_PACKET_TIMEOUT
        );
        if (err)
            return err;

        shifted = 1;
        loop_send_flags |= CAHUTE_SEVEN_SEND_FLAG_DISABLE_CHECKSUM
                           | CAHUTE_SEVEN_SEND_FLAG_DISABLE_TIMEOUT;

        if (progress_func)
            (*progress_func)(progress_cookie, 1, packet_count);
    }

    /* General loop for all packets except the last one. */
    for (i = 1 + shifted; i < packet_count; i++) {
        cahute_set_ascii_hex(&buf[4], (i >> 8) & 255);
        cahute_set_ascii_hex(&buf[6], i & 255);

        err = cahute_read_from_file(file, offset, &buf[8], 256);
        if (err) {
            if (shifted) {
                msg(link->context,
                    ll_error,
                    "An error has occurred while we were using packet "
                    "shifting; the link is now irrecoverable.");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            }

            return err;
        }

        offset += 256;

        msg(link->context,
            ll_info,
            "Sending data packet %lu/%lu.",
            i,
            packet_count);
        err = cahute_seven_send_extended(
            link,
            loop_send_flags,
            PACKET_TYPE_DATA,
            link->protocol_state.seven.last_command,
            buf,
            264,
            TIMEOUT_PACKET_TIMEOUT
        );
        if (err) {
            if (shifted) {
                msg(link->context,
                    ll_error,
                    "An error has occurred while we were using packet "
                    "shifting; the link is now irrecoverable.");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            }

            return err;
        }

        EXPECT_BASIC_ACK;

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    /* If we have been using packet shifting, we want to normalize the
     * exchange before the last packet. */
    if (shifted) {
        if ((err = cahute_seven_receive(link, TIMEOUT_PACKET_START)))
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Send the last packet. */
    cahute_set_ascii_hex(&buf[4], (packet_count >> 8) & 255);
    cahute_set_ascii_hex(&buf[6], packet_count & 255);

    err = cahute_read_from_file(file, offset, &buf[8], last_packet_size);
    if (err)
        return err;

    msg(link->context,
        ll_info,
        "Sending data packet %lu/%lu (last).",
        packet_count,
        packet_count);
    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_DATA,
        link->protocol_state.seven.last_command,
        buf,
        8 + last_packet_size,
        TIMEOUT_PACKET_TIMEOUT
    );
    if (err)
        return err;

    if (link->protocol_state.seven.last_packet_type != PACKET_TYPE_ACK) {
        msg(link->context, ll_error, "Calculator did not answer with an ACK.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    switch (link->protocol_state.seven.last_packet_subtype) {
    case PACKET_SUBTYPE_ACK_BASIC:
        /* Basic (most common) case, where the link can resume. */
        break;

    case PACKET_SUBTYPE_ACK_TERM:
        /* The link is terminated at the end of the data exchange flow.
         * Apart from that, the packet flow went great! */
        msg(link->context,
            ll_info,
            "Calculator terminated the link following the data transfer.");

        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        break;

    default:
        msg(link->context,
            ll_error,
            "Unhandled ACK subtype %02X at the end of data transfer.",
            link->protocol_state.seven.last_packet_subtype);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (progress_func)
        (*progress_func)(progress_cookie, packet_count, packet_count);

    return CAHUTE_OK;
}
