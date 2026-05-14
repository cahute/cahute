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
 * Receive a CAS300 packet.
 *
 * Note that we don't actually check that the packet identifier is ASCII-HEX
 * here, we just ensure we copy it properly when emitting the corresponding
 * acknowledgement.
 *
 * @param link Link to use.
 * @param first_byte First byte to include; -1 if no first byte is present.
 * @param timeout Timeout for the first byte.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_cas300_receive_packet(
    cahute_link *link,
    int first_byte,
    unsigned long timeout
) {
    cahute_u8 buf[CAS300_MAX_PACKET_SIZE];
    size_t payload_size;
    int packet_type, packet_subtype, err;

    /* Set the packet identifier, just so that we don't copy uninitialized
     * data later. */
    buf[1] = 0;
    buf[2] = 0;

    for (;; first_byte = -1) {
        size_t raw_payload_size;

        if (first_byte < 0) {
            err = cahute_casiolink_receive_first_byte(
                link,
                &first_byte,
                timeout
            );
            if (err)
                return err;
        }

        buf[0] = packet_type = first_byte;

        payload_size = 0;
        packet_subtype = 0;

        if (packet_type == PACKET_TYPE_ACK
            || packet_type == PACKET_TYPE_ORDER) {
            err = cahute_receive_on_link_transport(
                link,
                &buf[1],
                2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                goto fail;

            if (!cahute_is_ascii_hex(buf[1]) || !cahute_is_ascii_hex(buf[2])) {
                msg(link->context,
                    ll_error,
                    "Invalid CAS300 %s packet:",
                    packet_type == PACKET_TYPE_ACK ? "ack" : "order");
                mem(link->context, ll_error, buf, 3);
                goto fail;
            }

            msg(link->context,
                ll_debug,
                "Received the following packet from the device:");
            mem(link->context, ll_debug, buf, 3);
            break;
        }

        if (packet_type == PACKET_TYPE_TERM) {
            err = cahute_receive_on_link_transport(
                link,
                &buf[1],
                6,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                goto fail;

            if (!cahute_is_ascii_hex(buf[3]) || !cahute_is_ascii_hex(buf[4])
                || !cahute_is_ascii_hex(buf[5])
                || !cahute_is_ascii_hex(buf[6])) {
                msg(link->context,
                    ll_error,
                    "Invalid CAS300 termination packet:");
                ;
                mem(link->context, ll_error, buf, 7);
                goto fail;
            }

            msg(link->context,
                ll_debug,
                "Received the following packet from the device:");
            mem(link->context, ll_debug, buf, 7);

            packet_subtype = (cahute_ascii_hex_to_nibble(buf[3]) << 12)
                             | (cahute_ascii_hex_to_nibble(buf[4]) << 8)
                             | (cahute_ascii_hex_to_nibble(buf[5]) << 4)
                             | cahute_ascii_hex_to_nibble(buf[6]);
            break;
        }

        if ((packet_type != PACKET_TYPE_COMMAND
             && packet_type != PACKET_TYPE_DATA)) {
            msg(link->context,
                ll_error,
                "Invalid CAS300 packet type: 0x%02X",
                packet_type);
            goto fail;
        }

        /* Commands are at least 13 bytes long, data packets are at least
         * 9 bytes long; but for the sake of not corrupting the link in case
         * of command payloads being too short (less than 4 bytes), we want to
         * read the entire packet first, then check the command payload size
         * and format. */
        err = cahute_receive_on_link_transport(
            link,
            &buf[1],
            8,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            goto fail;

        if (!cahute_is_ascii_hex(buf[3]) || !cahute_is_ascii_hex(buf[4])
            || !cahute_is_ascii_hex(buf[5]) || !cahute_is_ascii_hex(buf[6])) {
            msg(link->context,
                ll_error,
                "Invalid CAS300 %s start:",
                packet_type == PACKET_TYPE_COMMAND ? "command" : "data packet"
            );
            ;
            mem(link->context, ll_error, buf, 7);
            goto fail;
        }

        raw_payload_size =
            ((cahute_ascii_hex_to_nibble(buf[3]) << 12)
             | (cahute_ascii_hex_to_nibble(buf[4]) << 8)
             | (cahute_ascii_hex_to_nibble(buf[5]) << 4)
             | cahute_ascii_hex_to_nibble(buf[6]));
        if (raw_payload_size > CAS300_MAX_ENCODED_PAYLOAD_SIZE) {
            msg(link->context,
                ll_error,
                "CAS300 %" CAHUTE_PRIuSIZE
                " payload size too big for internal buffers:",
                raw_payload_size);
            mem(link->context, ll_error, buf, 7);
            goto fail;
        }

        if (raw_payload_size) {
            err = cahute_receive_on_link_transport(
                link,
                &buf[9],
                raw_payload_size,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                goto fail;
        }

        /* Check the packet checksum before anything else. */
        if (!cahute_is_ascii_hex(buf[7 + raw_payload_size])
            || !cahute_is_ascii_hex(buf[8 + raw_payload_size])) {
            msg(link->context,
                ll_error,
                "CAS300 checksum is of invalid format:");
            mem(link->context, ll_error, buf, 9 + raw_payload_size);
            err = CAHUTE_ERROR_CORRUPT;
            goto fail;
        }

        {
            unsigned long expected_checksum;
            unsigned long obtained_checksum;

            expected_checksum =
                (cahute_ascii_hex_to_nibble(buf[7 + raw_payload_size]) << 4)
                | cahute_ascii_hex_to_nibble(buf[8 + raw_payload_size]);
            obtained_checksum = cahute_checksub(&buf[3], 4 + raw_payload_size);

            if (expected_checksum != obtained_checksum) {
                msg(link->context,
                    ll_error,
                    "Checksum 0x%02X differs from checksum 0x%02X present in "
                    "CAS300 packet:",
                    obtained_checksum,
                    expected_checksum);
                mem(link->context, ll_error, buf, 9 + raw_payload_size);
                err = CAHUTE_ERROR_CORRUPT;
                goto fail;
            }
        }

        msg(link->context,
            ll_debug,
            "Received the following packet from the device:");
        mem(link->context, ll_debug, buf, 9 + raw_payload_size);

        /* The received packet is valid, we want to acknowledge it. */
        {
            cahute_u8 const *raw_payload = &buf[7];

            if (packet_type == PACKET_TYPE_COMMAND) {
                if (raw_payload_size < 4
                    || !cahute_is_ascii_hex(raw_payload[0])
                    || !cahute_is_ascii_hex(raw_payload[1])
                    || !cahute_is_ascii_hex(raw_payload[2])
                    || !cahute_is_ascii_hex(raw_payload[3])) {
                    msg(link->context,
                        ll_error,
                        "Invalid CAS300 command packet:");
                    mem(link->context, ll_error, buf, 9 + raw_payload_size);
                    goto fail;
                }

                packet_subtype =
                    (cahute_ascii_hex_to_nibble(raw_payload[0]) << 12)
                    | (cahute_ascii_hex_to_nibble(raw_payload[1]) << 8)
                    | (cahute_ascii_hex_to_nibble(raw_payload[2]) << 4)
                    | cahute_ascii_hex_to_nibble(raw_payload[3]);

                raw_payload += 4;
                raw_payload_size -= 4;
            } else
                link->protocol_state.casiolink.cas300.packet_subtype = 0;

            if (raw_payload_size) {
                payload_size = CAS300_MAX_PAYLOAD_SIZE;
                err = cahute_unpad_data(
                    link->protocol_state.casiolink.cas300.packet_payload,
                    &payload_size,
                    raw_payload,
                    raw_payload_size
                );
                if (err)
                    goto fail;
            }
        }

        break;
    }

    link->protocol_state.casiolink.cas300.packet_type = packet_type;
    link->protocol_state.casiolink.cas300.packet_subtype = packet_subtype;
    link->protocol_state.casiolink.cas300.packet_payload_size = payload_size;
    link->protocol_state.casiolink.cas300.packet_id[0] = buf[1];
    link->protocol_state.casiolink.cas300.packet_id[1] = buf[2];

    /* Acknowledge the received packet. */
    if (packet_type != PACKET_TYPE_ACK && packet_type != PACKET_TYPE_ORDER) {
        cahute_u8 ack_buf[3];

        ack_buf[0] = PACKET_TYPE_ACK;
        ack_buf[1] = buf[1];
        ack_buf[2] = buf[2];

        msg(link->context,
            ll_debug,
            "Sending the following acknowledgement to the device:");
        mem(link->context, ll_debug, ack_buf, 3);

        err = cahute_send_on_link_transport(link, ack_buf, 3);
        if (err)
            goto fail;
    }

    payload_size = link->protocol_state.casiolink.cas300.packet_payload_size;
    switch (buf[0]) {
    case PACKET_TYPE_ORDER:
        msg(link->context, ll_debug, "Interpreted as an out-of-order signal.");
        link->protocol_state.casiolink.cas300.next_id =
            cahute_ascii_hex_to_nibble(buf[1]) << 4
            | cahute_ascii_hex_to_nibble(buf[2]);
        break;

    case PACKET_TYPE_TERM:
        msg(link->context, ll_debug, "Interpreted as termination packet.");
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        err = CAHUTE_ERROR_TERMINATED;
        goto fail;

    case PACKET_TYPE_COMMAND:
        if (payload_size) {
            msg(link->context,
                ll_debug,
                "Interpreted as command %04X with the following payload:",
                link->protocol_state.casiolink.cas300.packet_subtype);
            mem(link->context,
                ll_debug,
                link->protocol_state.casiolink.cas300.packet_payload,
                payload_size);
        } else
            msg(link->context,
                ll_debug,
                "Interpreted as command %04X with no payload.",
                link->protocol_state.casiolink.cas300.packet_subtype);

        break;

    case PACKET_TYPE_DATA:
        msg(link->context,
            ll_debug,
            "Interpreted as data packet of %" CAHUTE_PRIuSIZE "B.",
            payload_size);
        break;
    }

    return CAHUTE_OK;

fail:
    if (!err)
        return CAHUTE_ERROR_UNKNOWN;
    else if (err == CAHUTE_ERROR_TIMEOUT_START)
        return CAHUTE_ERROR_TIMEOUT;

    return err;
}
