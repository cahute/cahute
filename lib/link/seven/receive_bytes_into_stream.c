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
 * Accept and receive data to a FILE stream.
 *
 * This command starts by sending ACK in order to accept the command that
 * is accompanied with data, then receives and acknowledges all data packets
 * with the exception of the last one. This way, the caller can send a
 * different acknowledgement (e.g. with subtype '03'), or check that it
 * receives a roleswap or another command.
 *
 * @param link Link with which to receive the data.
 * @param file File to write data to.
 * @param size Size of the data to receive.
 * @param code Command code of the corresponding flow.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_receive_bytes_into_stream(
    cahute_link *link,
    cahute_file *file,
    size_t size,
    int command_code,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 const *buf = link->protocol_state.seven.last_packet_data;
    unsigned long packet_count = 0;
    unsigned long offset = 0;
    unsigned int i;
    size_t current_size;
    int err;

    for (i = 1; size; i++) {
        unsigned int read_packet_count, read_packet_i;

        /* On the first iteration, ``packet_count`` is set to 0, as we do not
         * know yet how much packets the splitting algorithm on the other
         * end has produced. In order to avoid logging "packet 1/0", we
         * emit a specific log for the first packet. */
        if (!packet_count)
            msg(link->context, ll_info, "Requesting first data packet.");
        else
            msg(link->context,
                ll_info,
                "Requesting packet %u/%u.",
                i,
                packet_count);

        err = cahute_seven_send_basic(
            link,
            0,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;

        EXPECT_PACKET(PACKET_TYPE_DATA, command_code);
        if (link->protocol_state.seven.last_packet_data_size < 9) {
            msg(link->context,
                ll_error,
                "Data packet doesn't contain metadata and at least one byte.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!cahute_is_ascii_hex(buf[0]) || !cahute_is_ascii_hex(buf[1])
            || !cahute_is_ascii_hex(buf[2]) || !cahute_is_ascii_hex(buf[3])
            || !cahute_is_ascii_hex(buf[4]) || !cahute_is_ascii_hex(buf[5])
            || !cahute_is_ascii_hex(buf[6]) || !cahute_is_ascii_hex(buf[7])) {
            msg(link->context, ll_error, "Data packet has invalid format.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_i =
            ((cahute_ascii_hex_to_nibble(buf[4]) << 12)
             | (cahute_ascii_hex_to_nibble(buf[5]) << 8)
             | (cahute_ascii_hex_to_nibble(buf[6]) << 4)
             | cahute_ascii_hex_to_nibble(buf[7]));
        if (read_packet_i != i) {
            msg(link->context,
                ll_error,
                "Unexpected sequence number (expected %u, got %u)",
                i,
                read_packet_i);
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_count =
            ((cahute_ascii_hex_to_nibble(buf[0]) << 12)
             | (cahute_ascii_hex_to_nibble(buf[1]) << 8)
             | (cahute_ascii_hex_to_nibble(buf[2]) << 4)
             | cahute_ascii_hex_to_nibble(buf[3]));
        if (i == 1)
            packet_count = read_packet_count;
        else if (read_packet_count != packet_count) {
            msg(link->context,
                ll_error,
                "Packet count was not consistent between packets "
                "(initial: 1/%u, current: %u/%u)",
                packet_count,
                i,
                read_packet_count);
            return CAHUTE_ERROR_UNKNOWN;
        }

        current_size = link->protocol_state.seven.last_packet_data_size - 8;
        if (i < packet_count) {
            if (current_size >= size) {
                msg(link->context,
                    ll_error,
                    "Packet too much data for the expected total size of "
                    "the data flow (expected: %" CAHUTE_PRIuSIZE
                    ", got: %" CAHUTE_PRIuSIZE ")",
                    size,
                    current_size);
                return CAHUTE_ERROR_UNKNOWN;
            }
        } else if (current_size < size) {
            msg(link->context,
                ll_error,
                "Last packet did not contain enough bytes to finish the "
                "data flow (expected: %" CAHUTE_PRIuSIZE
                ", got: %" CAHUTE_PRIuSIZE ").",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        } else if (current_size > size) {
            msg(link->context,
                ll_error,
                "Last packet contained too many bytes to finish the data "
                "flow (expected: %" CAHUTE_PRIuSIZE ", got: %" CAHUTE_PRIuSIZE
                " )",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Write what is in the current packet. */
        err = cahute_write_to_file(file, offset, &buf[8], current_size);
        if (err)
            return err;

        size -= current_size;
        offset += current_size;

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    return CAHUTE_OK;
}
