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

#define PACKET_TYPE_COMMAND 0x01
#define PACKET_TYPE_DATA    0x02
#define PACKET_TYPE_ACK     0x06
#define PACKET_TYPE_ORDER   0x15
#define PACKET_TYPE_TERM    0x18

#define TIMEOUT_ACK             1000
#define TIMEOUT_PACKET_CONTENTS 500

#define INIT_ATTEMPTS 10

/* The 0002 command for Classpad 300 / 330 (+) used for initialization when
 * in sender / control mode. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
default_0002_payload =
    (cahute_u8 const *)"CP430\xFF\xFF\xFF" "00.00.0(0305000001.01.0016M"
    "\xFF\xFF\xFF\xFF\xFF" "8M\xFF\xFF\xFF\xFF\xFF\xFF\x81";

/* ---
 * Packet functions.
 * --- */

/**
 * Send a CAS300 command.
 *
 * @param link Link to use.
 * @param command Command to send.
 * @param payload Payload to include with the command.
 * @param payload_size Size of the payload to include with the command.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
cahute_cas300_send_command(
    cahute_link *link,
    unsigned int command,
    cahute_u8 const *payload,
    size_t payload_size
) {
    cahute_u8 buf[CAS300_MAX_PACKET_SIZE];
    size_t padded_size;
    int packet_id, err;

    if (payload_size > CAS300_MAX_PAYLOAD_SIZE)
        return CAHUTE_ERROR_SIZE;

restart_send_command:
    packet_id = link->protocol_state.casiolink.cas300.next_id;
    link->protocol_state.casiolink.cas300.next_id = (packet_id + 1) & 255;

    padded_size = 0;
    if (payload_size)
        padded_size = cahute_pad_data(&buf[11], payload, payload_size);

    buf[0] = 0x01;
    cahute_set_ascii_hex(&buf[1], packet_id);
    cahute_set_ascii_hex(&buf[3], (padded_size + 4) >> 8);
    cahute_set_ascii_hex(&buf[5], (padded_size + 4) & 255);
    cahute_set_ascii_hex(&buf[7], command >> 8);
    cahute_set_ascii_hex(&buf[9], command & 255);
    cahute_set_ascii_hex(
        &buf[11 + padded_size],
        cahute_checksub(&buf[3], padded_size + 8)
    );

    msg(link->context, ll_debug, "Sending the following packet to the device:"
    );
    mem(link->context, ll_debug, buf, padded_size + 13);

    err = cahute_send_on_link_transport(link, buf, padded_size + 13);
    if (err)
        return err;

    /* Receive the ACK with the same packet identifier. */
    do {
        err = cahute_cas300_receive_packet(link, -1, TIMEOUT_ACK);
        if (err == CAHUTE_ERROR_TIMEOUT_START) {
            msg(link->context,
                ll_debug,
                "Re-sending the following packet to the device:");
            mem(link->context, ll_debug, buf, padded_size + 13);
            err = cahute_send_on_link_transport(link, buf, padded_size + 13);
            if (err)
                return err;

            continue;
        }

        if (err)
            return err;

        switch (link->protocol_state.casiolink.cas300.packet_type) {
        case PACKET_TYPE_ACK:
            if (memcmp(
                    link->protocol_state.casiolink.cas300.packet_id,
                    &buf[1],
                    2
                )) {
                /* The packet is possibly not for us, in case we are sharing
                 * the transport. */
                continue;
            }
            break;

        case PACKET_TYPE_ORDER:
            /* Command has been sent out-of-order. next_id has been set
             * by the packet reception function, we can try to resend the
             * command. */
            goto restart_send_command;

        default:
            /* The packet is possibly not for us, in case we are sharing the
             * transport. */
            continue;
        }

        break;
    } while (1);

    return CAHUTE_OK;
}

/**
 * Send a CAS300 data packet.
 *
 * @param link Link to use.
 * @param payload Payload to include with the command.
 * @param payload_size Size of the payload to include with the command.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
cahute_cas300_send_data_packet(
    cahute_link *link,
    unsigned int command,
    cahute_u8 const *payload,
    size_t payload_size
) {
    cahute_u8 buf[CAS300_MAX_PACKET_SIZE];
    size_t padded_size;
    int packet_id, err;

    if (payload_size > CAS300_MAX_PAYLOAD_SIZE)
        return CAHUTE_ERROR_SIZE;

    packet_id = link->protocol_state.casiolink.cas300.next_id;
    link->protocol_state.casiolink.cas300.next_id = (packet_id + 1) & 255;

    padded_size = 0;
    if (payload_size)
        padded_size = cahute_pad_data(&buf[7], payload, payload_size);

    buf[0] = 0x02;
    cahute_set_ascii_hex(&buf[1], packet_id);
    cahute_set_ascii_hex(&buf[3], (padded_size + 4) >> 8);
    cahute_set_ascii_hex(&buf[5], (padded_size + 4) & 255);
    cahute_set_ascii_hex(
        &buf[7 + padded_size],
        cahute_checksub(&buf[3], padded_size + 4)
    );

    msg(link->context, ll_debug, "Sending the following packet to the device:"
    );
    mem(link->context, ll_debug, buf, padded_size + 9);

    err = cahute_send_on_link_transport(link, buf, padded_size + 9);
    if (err)
        return err;

    /* Receive the ACK with the same packet identifier. */
    /* TODO: timeouts! */
    /* TODO: can we receive invalid acknowledgements here? */
    do {
        err = cahute_cas300_receive_packet(link, -1, TIMEOUT_ACK);
        if (err)
            return err;

        if (link->protocol_state.casiolink.cas300.packet_type
                != PACKET_TYPE_ACK
            || memcmp(
                link->protocol_state.casiolink.cas300.packet_id,
                &buf[1],
                2
            ))
            continue;
    } while (0);

    return CAHUTE_OK;
}

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
CAHUTE_EXTERN(int)
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

/* ---
 * Reception functions.
 * --- */

/**
 * Receive data, optionally starting from a byte.
 *
 * @param link Link to receive data from.
 * @param datap Pointer to the data pointer to populate.
 * @param first_byte First byte to receive.
 * @param timeout Timeout to receive the first byte.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_cas300_receive_data(
    cahute_link *link,
    cahute_data **datap,
    int first_byte,
    unsigned long timeout
) {
    int err;

    for (;; first_byte = -1) {
        err = cahute_cas300_receive_packet(link, first_byte, timeout);
        if (err)
            return err;

        if (link->protocol_state.casiolink.cas300.packet_type
            != PACKET_TYPE_COMMAND) {
            msg(link->context, ll_error, "Expected a command here.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        switch (link->protocol_state.casiolink.cas300.packet_subtype) {
        case 0x0003:
            /* TODO: Find out what this command does to the link exactly,
                * as this is not yet known. */
            msg(link->context,
                ll_error,
                "Command 0003 received, communication is now corrupted.");
            link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            return CAHUTE_ERROR_IRRECOV;

        case 0x0011:
            /* We can send our dummy model information. */
            err = cahute_cas300_send_command(
                link,
                0x0002,
                default_0002_payload,
                41
            );
            if (err)
                return err;

            break;

        default:
            CAHUTE_RETURN_IMPL(
                link->context,
                "Unimplemented command for reception."
            );
        }
    }

    return CAHUTE_OK;
}

/* ---
 * Send and control functions.
 * --- */

/**
 * Initiate the connection as a sender, for CAS300.
 *
 * NOTE: This function will fail if the communication has already been
 *       initialized!
 *
 * @param link Link for which to initiate the connection, as a sender.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_cas300_initiate_as_sender(cahute_link *link) {
    int err, byte = -1, retry, attempts;

    for (retry = 1, attempts = INIT_ATTEMPTS; retry && --attempts >= 0;) {
        msg(link->context, ll_debug, "Sending start packet 0x16.");
        err = cahute_send_byte_on_link_transport(link, 0x16);
        if (err)
            return err;

        err = cahute_receive_byte_on_link_transport(link, &byte, 400);
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            continue;
        if (err)
            return err;

        if (byte == 0x13) {
            /* ClassPad 300 / 330 (+) calculators answer this over USB only,
             * and Cahute answers this over both USB and serial. */
            msg(link->context, ll_debug, "Normal init ack received (0x13).");
            retry = 0;
            break;
        }

        if (byte == 0x00) {
            /* For some reason, over serial links, ClassPad 300 / 330 (+)
             * calculators do not answer 0x13, but two 0x00 bytes.
             * There may be errors, usually found on the second byte, e.g.
             * {0x00, 0x05} or {0x00, 0x09}, so we need to check that the
             * calculator indeed returns a second 0x00 byte. */
            msg(link->context,
                ll_debug,
                "NUL byte received, checking next byte.");
            err = cahute_receive_byte_on_link_transport(link, &byte, 400);
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                continue;
            if (err)
                return err;

            if (byte == 0x00) {
                msg(link->context,
                    ll_debug,
                    "Alt init ack received (0x00, 0x00).");
                retry = 0;
                break;
            }
        }

        /* The initiation has yielded another byte, considered a notice
         * we want to log about. */
        if (byte == 0x05) {
            /* This seems to be received when the communication has not been
             * successfully initiated, which means we need to resend the 0x16
             * as it may have been skipped. */
            msg(link->context, ll_debug, "Uninitiated received (0x05).");
        } else if (byte == 0x09) {
            /* This seems to be received when wakeup mode is enabled on the
             * calculator, and the calculator, which was not in receive mode
             * yet, has spawned the menu and is more or less ready to receive
             * another start packet. */
            msg(link->context,
                ll_debug,
                "Automatic reception mode enable received (0x09)");
        } else
            msg(link->context,
                ll_warn,
                "Unknown notice received (0x%02X)",
                byte);
    }

    if (attempts < 0) {
        msg(link->context,
            ll_warn,
            "Could not initialize the communication after %d attempts.",
            INIT_ATTEMPTS);
        return CAHUTE_ERROR_TIMEOUT_START;
    }

    return CAHUTE_OK;
}

/**
 * Initiate the connection as a receiver, for CAS300.
 *
 * For some reason there seems to be an asymetry between host to device
 * communication initialization, and device to host communication
 * initialization.
 *
 * @param link Link for which to initiate the connection, as a receiver.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_cas300_initiate_as_receiver(cahute_link *link) {
    int byte = -1, err;

    while (byte != 0x16) {
        err = cahute_receive_byte_on_link_transport(link, &byte, 0);
        if (err)
            return err;
    }

    err = cahute_send_byte_on_link_transport(link, 0x13);
    if (err)
        return err;

    return CAHUTE_OK;
}

/**
 * Terminate the connection, for CAS300.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_cas300_terminate(cahute_link *link) {
    cahute_u8 buf[10];
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    buf[0] = PACKET_TYPE_TERM;
    cahute_set_ascii_hex(
        &buf[1],
        link->protocol_state.casiolink.cas300.next_id
    );
    buf[3] = '0';
    buf[4] = '0';
    buf[5] = '0';
    buf[6] = '4';

    msg(link->context, ll_debug, "Sending the following packet to the device:"
    );
    mem(link->context, ll_debug, buf, 7);

    err = cahute_send_on_link_transport(link, buf, 7);
    if (err)
        return err;

    /* TODO: Timeouts! */
    err = cahute_receive_on_link_transport(link, &buf[7], 3, 0, 0);
    if (err)
        return err;

    if (buf[7] != PACKET_TYPE_ACK || buf[8] != buf[1] || buf[9] != buf[2]) {
        msg(link->context, ll_error, "Unhandled termination response:");
        mem(link->context, ll_error, &buf[7], 3);
        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(link->context, ll_debug, "Received the following acknowledgement:");
    mem(link->context, ll_debug, &buf[7], 3);

    link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
    return CAHUTE_OK;
}

/**
 * Discover device information.
 *
 * NOTE: This function is to be called while being the sender only.
 * NOTE: For some reason, when this command is called a second time on a
 *       given session, rather than respond with a 0x0002 command, the
 *       calculator prefers to terminate the connection...
 *
 * @param link Link in which to discover device information.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_cas300_discover(cahute_link *link) {
    int err;

    err = cahute_cas300_send_command(link, 0x0011, NULL, 0);
    if (err)
        return err;

    err = cahute_cas300_receive_packet(link, -1, 0);
    if (err)
        return err;

    if (link->protocol_state.casiolink.cas300.packet_type != 0x01) {
        msg(link->context,
            ll_error,
            "Expected a CAS300 command, got 0x%02X.",
            link->protocol_state.casiolink.cas300.packet_type);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (link->protocol_state.casiolink.cas300.packet_subtype != 0x0002) {
        msg(link->context,
            ll_error,
            "Expected 0x0002 command, got 0x%04X.",
            link->protocol_state.casiolink.cas300.packet_subtype);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (link->protocol_state.casiolink.cas300.packet_payload_size != 49) {
        msg(link->context,
            ll_error,
            "Expected a 49-byte payload, got %" CAHUTE_PRIuSIZE,
            link->protocol_state.casiolink.cas300.packet_payload_size);
        return CAHUTE_ERROR_UNKNOWN;
    }

    memcpy(
        link->protocol_state.casiolink.raw_device_info,
        link->protocol_state.casiolink.cas300.packet_payload,
        link->protocol_state.casiolink.cas300.packet_payload_size
    );
    link->protocol_state.casiolink.flags |=
        (CASIOLINK_FLAG_DEVICE_INFO_OBTAINED
         | CASIOLINK_FLAG_DEVICE_INFO_CAS300);
    return CAHUTE_OK;
}

/**
 * Produce generic device information using CAS300 device information.
 *
 * @param context Context in which the function is run.
 * @param infop Pointer to set to the allocated device information structure.
 * @param raw_info Raw information to read from, expected to be 49 bytes long.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_cas300_make_device_info(
    cahute_context *context,
    cahute_device_info **infop,
    cahute_u8 const *raw_info
) {
    cahute_device_info *info = NULL;
    char *buf;
    char rawsize_buf[9], *rawsize = rawsize_buf;
    char rawver_buf[17], *rawver = rawver_buf;

    info = malloc(sizeof(cahute_device_info) + 46);
    if (!info)
        return CAHUTE_ERROR_ALLOC;

    buf = (void *)(&info[1]);

    info->cahute_device_info_flags =
        CAHUTE_DEVICE_INFO_FLAG_BOOTCODE | CAHUTE_DEVICE_INFO_FLAG_OS;
    info->cahute_device_info_rom_capacity = 0;
    info->cahute_device_info_rom_version = "";

    /* Flash ROM capacity is presented in a human-readable format,
     * we want to try to determine the machine-readable format here. */
    cahute_copy_ff_string(&rawsize, &raw_info[32], 8);
    if (!strcmp(rawsize_buf, "16M"))
        info->cahute_device_info_flash_rom_capacity = 16777216;
    else {
        msg(context, ll_error, "Unknown ROM capacity: %s", rawsize_buf);
        goto fail;
    }

    info->cahute_device_info_ram_capacity = 0;

    info->cahute_device_info_bootcode_version = buf;
    cahute_copy_ff_string(&buf, &raw_info[24], 8);

    info->cahute_device_info_bootcode_offset = 0;
    info->cahute_device_info_bootcode_size = 0;

    /* OS version seems to be presented in a strange format, being
     * "00.00.0(03050000" for OS 03.05.0000. We want to try to extract
     * the OS version from that. */
    cahute_copy_ff_string(&rawver, &raw_info[8], 16);
    if (strlen(rawver_buf) != 16) {
        msg(context,
            ll_error,
            "Unable to extract OS version from: %s",
            rawver_buf);
        goto fail;
    }

    buf[0] = rawver_buf[8];
    buf[1] = rawver_buf[9];
    buf[2] = '.';
    buf[3] = rawver_buf[10];
    buf[4] = rawver_buf[11];
    buf[5] = '.';
    buf[6] = rawver_buf[12];
    buf[7] = rawver_buf[13];
    buf[8] = rawver_buf[14];
    buf[9] = rawver_buf[15];
    buf[10] = 0;

    info->cahute_device_info_os_version = buf;
    buf += 12;

    info->cahute_device_info_os_offset = 0;
    info->cahute_device_info_os_size = 0;

    info->cahute_device_info_product_id = "";
    info->cahute_device_info_username = "";
    info->cahute_device_info_organisation = "";

    info->cahute_device_info_hwid = buf;
    cahute_copy_ff_string(&buf, &raw_info[0], 8);

    info->cahute_device_info_cpuid = "";

    *infop = info;
    return CAHUTE_OK;

fail:
    if (info)
        free(info);

    return CAHUTE_ERROR_ALLOC;
}
