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
 * Receive and decode a Protocol 7.00 packet, and store it into the link.
 *
 * This function should not be used directly, but with
 * ``cahute_seven_send_and_receive``.
 *
 * @param link Link to use to receive the Protocol 7.00 packet.
 * @param timeout Timeout of the packet start.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_seven_receive(cahute_link *link, unsigned long timeout) {
    struct cahute_seven_state *state = &link->protocol_state.seven;
    cahute_u8 buf[SEVEN_MAX_PACKET_SIZE];
    size_t packet_size, data_size = 0;
    size_t to_complete = 6;
    int err;

    do {
        /* The packet is at least 6 bytes long: type (1 B)
         * + subtype (2 B) + ex (1 B) + checksum (2 B).
         *
         * In TRANSMIT mode, the calculator may start sending a bunch of
         * 0x10 bytes; in this case, we want to ignore them until we have
         * at least the base of a packet.
         *
         * If we have 4 bytes to complete, we are to read 4 bytes starting
         * at &buf[2], since the first 2 bytes are already present. */
        err = cahute_receive_on_link_transport(
            link,
            &buf[6 - to_complete],
            to_complete,
            timeout,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;

        for (to_complete = 0; to_complete < 6 && buf[to_complete] == 0x10;
             to_complete++)
            ;

        if (!to_complete)
            break;

        /* In case we have 4 bytes to complete, we are to copy the 2 bytes
         * at the end of the buffer to the beginning. */
        if (to_complete < 6)
            memmove(buf, &buf[to_complete], 6 - to_complete);
    } while (1);

    /* We assume the packet is of basic or extended format from here.
     * We want to check the basic format of the packet, complete the raw
     * packet, and check the checksum before any other treatment. */
    if (!cahute_is_ascii_hex(buf[1]) || !cahute_is_ascii_hex(buf[2])
        || (buf[3] != '0' && buf[3] != '1')) {
        msg(link->context,
            ll_error,
            "Invalid format for the usual packet header.");
        msg(link->context, ll_debug, "Data read so far is the following:");
        mem(link->context, ll_debug, buf, 6);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (buf[3] == '0')
        packet_size = 6;
    else {
        /* Packet is extended, there is at least 10 bytes: Type (1 B)
         * + Subtype (2 B) + Extended (1 B) + Data size (4 B)
         * + Checksum (2 B). */
        err = cahute_receive_on_link_transport(
            link,
            &buf[6],
            4,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        if (!cahute_is_ascii_hex(buf[4]) || !cahute_is_ascii_hex(buf[5])
            || !cahute_is_ascii_hex(buf[6]) || !cahute_is_ascii_hex(buf[7])) {
            msg(link->context, ll_error, "Invalid format for the data size.");
            msg(link->context, ll_debug, "Data read so far is the following:");
            mem(link->context, ll_debug, buf, 10);
            return CAHUTE_ERROR_UNKNOWN;
        }

        data_size =
            ((cahute_ascii_hex_to_nibble(buf[4]) << 12)
             | (cahute_ascii_hex_to_nibble(buf[5]) << 8)
             | (cahute_ascii_hex_to_nibble(buf[6]) << 4)
             | cahute_ascii_hex_to_nibble(buf[7]));

        if (data_size == 0 || data_size > SEVEN_MAX_ENCODED_PACKET_DATA_SIZE) {
            msg(link->context,
                ll_error,
                "Invalid data size %" CAHUTE_PRIuSIZE
                " for the extended packet.",
                data_size);
            msg(link->context, ll_debug, "Data read so far is the following:");
            mem(link->context, ll_debug, buf, 10);

            if (data_size)
                cahute_receive_on_link_transport(
                    link,
                    NULL,
                    data_size,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );

            return CAHUTE_ERROR_SIZE;
        }

        /* We want to read the rest of the packet here, with the rest of the
         * data (since we've already read 2 bytes of data) and the checksum. */
        err = cahute_receive_on_link_transport(
            link,
            &buf[10],
            data_size,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        packet_size = 10 + data_size;
    }

    msg(link->context, ll_debug, "Received packet data is the following:");
    mem(link->context, ll_debug, buf, packet_size);

    if (!cahute_is_ascii_hex(buf[packet_size - 2])
        || !cahute_is_ascii_hex(buf[packet_size - 1])) {
        msg(link->context,
            ll_error,
            "Invalid checksum format for the following packet:");
        mem(link->context, ll_error, buf, packet_size);
        return CAHUTE_ERROR_CORRUPT;
    }

    /* We want to compute the checksum and check if it's valid or not. */
    {
        unsigned int obtained_checksum =
            ((cahute_ascii_hex_to_nibble(buf[packet_size - 2]) << 4)
             | cahute_ascii_hex_to_nibble(buf[packet_size - 1]));
        unsigned int computed_checksum =
            cahute_checksub(&buf[1], packet_size - 3);

        if (obtained_checksum != computed_checksum) {
            msg(link->context,
                ll_error,
                "Obtained checksum 0x%02X does not match computed checksum "
                "0x%02X.",
                obtained_checksum,
                computed_checksum);
            return CAHUTE_ERROR_CORRUPT;
        }
    }

    /* Now that we've decoded data, we're able to parse it a bit better. */
    state->last_packet_type = buf[0];
    state->last_packet_subtype =
        ((cahute_ascii_hex_to_nibble(buf[1]) << 4)
         | cahute_ascii_hex_to_nibble(buf[2]));

    if (data_size) {
        state->last_packet_data_size = SEVEN_MAX_PACKET_DATA_SIZE;
        return cahute_unpad_data(
            state->last_packet_data,
            &state->last_packet_data_size,
            &buf[8],
            data_size
        );
    }

    state->last_packet_data_size = 0;
    return CAHUTE_OK;
}
