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

/* Protocol 7.00 packets for detection. */
CAHUTE_LOCAL_DATA(cahute_u8)
seven_check_packet[] = {5, '0', '0', '0', '7', '0'};
CAHUTE_LOCAL_DATA(cahute_u8)
seven_ack_packet[] = {6, '0', '0', '0', '7', '0'};
CAHUTE_LOCAL_DATA(cahute_u8)
seven_nak_packet[] = {21, '0', '4', '0', '6', 'C'};

/* CAS300 discover packet for detection. */
CAHUTE_LOCAL_DATA(cahute_u8)
cas300_discover_packet[] =
    {1, '0', '0', '0', '0', '0', '4', '0', '0', '1', '1', '7', 'A'};

/* Map the linkopen protocol to the actual protocol. */
CAHUTE_LOCAL(int) get_protocol_value(cahute_context *context, int protocol) {
    switch (protocol) {
    case PROTOCOL_SERIAL_NONE:
        return CAHUTE_LINK_PROTOCOL_SERIAL_NONE;
    case PROTOCOL_SERIAL_CAS:
        return CAHUTE_LINK_PROTOCOL_SERIAL_CAS;
    case PROTOCOL_SERIAL_CAS40:
        return CAHUTE_LINK_PROTOCOL_SERIAL_CAS40;
    case PROTOCOL_SERIAL_CAS50:
        return CAHUTE_LINK_PROTOCOL_SERIAL_CAS50;
    case PROTOCOL_SERIAL_CAS100:
        return CAHUTE_LINK_PROTOCOL_SERIAL_CAS100;
    case PROTOCOL_SERIAL_CAS300:
        return CAHUTE_LINK_PROTOCOL_SERIAL_CAS300;
    case PROTOCOL_SERIAL_SEVEN:
        return CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN;
    case PROTOCOL_SERIAL_SEVEN_OHP:
        return CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP;
    case PROTOCOL_USB_NONE:
        return CAHUTE_LINK_PROTOCOL_USB_NONE;
    case PROTOCOL_USB_CAS300:
        return CAHUTE_LINK_PROTOCOL_USB_CAS300;
    case PROTOCOL_USB_SEVEN:
        return CAHUTE_LINK_PROTOCOL_USB_SEVEN;
    case PROTOCOL_USB_SEVEN_OHP:
        return CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP;
    case PROTOCOL_USB_MASS_STORAGE:
        return CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE;
    default:
        msg(context,
            ll_error,
            "Could not map linkopen protocol to actual protocol: %d",
            protocol);
        return 0;
    }
}

/**
 * Determine the protocol for a serial link as a receiver.
 *
 * @param link Link to initialize.
 * @param protocolp Pointer to the protocol to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
determine_protocol_as_receiver(cahute_link *link, int *protocolp) {
    cahute_u8 buf[6];
    size_t received = 1;
    int protocol = *protocolp;
    int err;

    msg(link->context, ll_info, "Waiting for input to determine the protocol."
    );

    do {
        err = cahute_receive_on_link_transport(link, buf, 1, 0, 0);
        if (err)
            goto fail;

        if (buf[0] == 0x05) {
            /* This is the beginning of a Protocol 7.00 check packet.
             * We want to read the rest of the packet to ensure that
             * everything is correct. */
            err = cahute_receive_on_link_transport(link, &buf[1], 5, 0, 0);
            if (err)
                goto fail;

            received = 6;
            if (!memcmp(buf, seven_check_packet, 6)) {
                /* That's a check packet! We can answer with an ACK, then
                 * set the protocol to Protocol 7.00. */
                err = cahute_send_on_link_transport(link, seven_ack_packet, 6);
                if (err)
                    goto fail;

                switch (protocol) {
                case PROTOCOL_SERIAL_AUTO:
                case PROTOCOL_SERIAL_AUTO_CAS40:
                case PROTOCOL_SERIAL_AUTO_CAS50:
                case PROTOCOL_SERIAL_AUTO_CAS100:
                case PROTOCOL_SERIAL_AUTO_CAS300:
                    protocol = PROTOCOL_SERIAL_SEVEN;
                    break;

                case PROTOCOL_USB_AUTO:
                    protocol = PROTOCOL_USB_SEVEN;
                    break;

                default:
                    msg(link->context,
                        ll_error,
                        "No SEVEN detected equiv. for protocol: %d",
                        protocol);
                    err = CAHUTE_ERROR_UNKNOWN;
                    goto fail;
                }

                goto found;
            }
        } else if (buf[0] == 0x0B) {
            /* This is the beginning of a Protocol 7.00 Screenstreaming packet.
             * We don't want to read the rest of the packet, the receiving
             * routine for Protocol 7.00 screenstreaming will realign
             * itself. */
            switch (protocol) {
            case PROTOCOL_SERIAL_AUTO:
            case PROTOCOL_SERIAL_AUTO_CAS40:
            case PROTOCOL_SERIAL_AUTO_CAS50:
            case PROTOCOL_SERIAL_AUTO_CAS100:
            case PROTOCOL_SERIAL_AUTO_CAS300:
                protocol = PROTOCOL_SERIAL_SEVEN_OHP;
                break;

            case PROTOCOL_USB_AUTO:
                protocol = PROTOCOL_USB_SEVEN_OHP;
                break;

            default:
                msg(link->context,
                    ll_error,
                    "No SEVEN_OHP detected equiv. for protocol: %d",
                    protocol);
                err = CAHUTE_ERROR_UNKNOWN;
                goto fail;
            }

            goto found;
        } else if (buf[0] == 0x10) {
            /* This is an unknown protocol that is tried by the calculator
             * on both USB and serial when transmitting. If we just ignore
             * this, the calculator will try Protocol 7.00 and CASIOLINK
             * eventually. */
            continue;
        } else if (buf[0] == 0x16) {
            /* This is a CASIOLINK start packet.
             * We can answer with an 'established' packet and set the protocol
             * to CASIOLINK. */
            buf[0] = 0x13;

            err = cahute_send_on_link_transport(link, buf, 1);
            if (err)
                goto fail;

            switch (protocol) {
            case PROTOCOL_SERIAL_AUTO:
                protocol = PROTOCOL_SERIAL_CAS;
                break;

            case PROTOCOL_SERIAL_AUTO_CAS40:
                protocol = PROTOCOL_SERIAL_CAS40;
                break;

            case PROTOCOL_SERIAL_AUTO_CAS50:
                protocol = PROTOCOL_SERIAL_CAS50;
                break;

            case PROTOCOL_SERIAL_AUTO_CAS100:
                protocol = PROTOCOL_SERIAL_CAS100;
                break;

            case PROTOCOL_SERIAL_AUTO_CAS300:
                protocol = PROTOCOL_SERIAL_CAS300;
                break;

            case PROTOCOL_USB_AUTO:
                protocol = PROTOCOL_USB_CAS300;
                break;

            default:
                msg(link->context,
                    ll_error,
                    "No CASIOLINK detected equiv. for protocol: %d",
                    protocol);
                err = CAHUTE_ERROR_UNKNOWN;
                goto fail;
            }

            goto found;
        }

        break;
    } while (1);

    msg(link->context,
        ll_error,
        "Unable to determine a protocol out of the following:");
    mem(link->context, ll_error, buf, received);

    err = CAHUTE_ERROR_UNKNOWN;
fail:
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        err = CAHUTE_ERROR_TIMEOUT;
    return err;

found:
    *protocolp = protocol;
    return CAHUTE_OK;
}

/**
 * Determine the protocol for a serial link as a sender or control.
 *
 * @param link Link to initialize.
 * @param protocolp Pointer to the protocol to set.
 * @param cas300_discoveredp Pointer to the boolean to set if the protocol
 *        detection has caused CAS300 model information to be discovered.
 * @param cas300_next_idp Pointer to the CAS300 next packet identifier to
 *        set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
determine_protocol_as_sender(
    cahute_link *link,
    int *protocolp,
    int *cas300_discoveredp,
    int *cas300_next_idp
) {
    cahute_u8 buf[48];
    size_t received = 1;
    int err, attempts;
    int protocol = *protocolp;

    for (attempts = 3; attempts; attempts--) {
        /* We want to complete the Protocol 7.00 check packet. */
        msg(link->context, ll_debug, "Sending the Protocol 7.00 check packet:"
        );
        mem(link->context,
            ll_debug,
            seven_check_packet,
            sizeof(seven_check_packet));

        err = cahute_send_on_link_transport(
            link,
            seven_check_packet,
            sizeof(seven_check_packet)
        );
        if (err)
            return err;

        err = cahute_receive_on_link_transport(link, buf, 1, 800, 0);
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_TIMEOUT_START)
            return err;

        /* Try writing a CASIOLINK start packet to see if we get an answer. */
        msg(link->context, ll_debug, "Sending the CASIOLINK check packet.");
        err = cahute_send_byte_on_link_transport(link, 0x16);
        if (err)
            return err;

        err = cahute_receive_on_link_transport(link, buf, 1, 200, 0);
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_TIMEOUT_START)
            return err;

        /* It is possible we have a ClassPad 300 / 330 (+) for which the
         * communication has already been initiated somehow. We want to try
         * sending a command to find model information.
         *
         * In order to be able to terminate the communication if an active
         * Protocol 7.00 device is listening, we want to send the command in
         * two parts. */
        msg(link->context,
            ll_debug,
            "Sending the start of the CAS300 discovery command:");
        mem(link->context, ll_debug, cas300_discover_packet, 6);

        err = cahute_send_on_link_transport(link, cas300_discover_packet, 6);
        if (err)
            return err;

        err = cahute_receive_on_link_transport(link, buf, 1, 100, 0);
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_TIMEOUT_START)
            return err;

        msg(link->context,
            ll_debug,
            "Sending the rest of the CAS300 discovery command:");
        mem(link->context,
            ll_debug,
            &cas300_discover_packet[6],
            sizeof(cas300_discover_packet) - 6);

        err = cahute_send_on_link_transport(
            link,
            &cas300_discover_packet[6],
            sizeof(cas300_discover_packet) - 6
        );
        if (err)
            return err;

        err = cahute_receive_on_link_transport(link, buf, 1, 400, 0);
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_TIMEOUT_START)
            return err;
    }

    if (!attempts) {
        msg(link->context,
            ll_error,
            "No answer detected, protocol could not be determined.");
        return CAHUTE_ERROR_NOT_FOUND;
    }

    if (buf[0] == 0x00) {
        /* This is a CAS300 serial status packet.
         * It may have another byte to indicate the status. */
        err = cahute_cas300_initiate_as_sender(link);
        if (err)
            goto fail;

        switch (protocol) {
        case PROTOCOL_SERIAL_AUTO:
        case PROTOCOL_SERIAL_AUTO_CAS300:
        case PROTOCOL_SERIAL_CAS300:
            protocol = PROTOCOL_SERIAL_CAS300;
            break;

        case PROTOCOL_USB_AUTO:
            protocol = PROTOCOL_USB_CAS300;
            break;

        default:
            msg(link->context,
                ll_error,
                "No CAS300 detected equiv. for protocol: %d",
                protocol);
            err = CAHUTE_ERROR_UNKNOWN;
            goto fail;
        }

        goto found;
    } else if (buf[0] == 0x05) {
        /* This is likely a ClassPad 300 / 330 (+) answering our Protocol 7.00
         * with the length amount of 0x05 bytes, in order to indicate to us
         * that the communication was not initialized. We want to read all of
         * the 0x05 bytes, then initialize the communication. */
        while (1) {
            int byte;

            err = cahute_receive_byte_on_link_transport(link, &byte, 200);
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                break;
            else if (err)
                return err;
            else if (byte != 0x05) {
                msg(link->context,
                    ll_error,
                    "One of the bytes in the received ones is not 0x05!");
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        err = cahute_cas300_initiate_as_sender(link);
        if (err)
            return err;

        switch (protocol) {
        case PROTOCOL_SERIAL_AUTO:
        case PROTOCOL_SERIAL_AUTO_CAS300:
        case PROTOCOL_SERIAL_CAS300:
            protocol = PROTOCOL_SERIAL_CAS300;
            break;

        case PROTOCOL_USB_AUTO:
            protocol = PROTOCOL_USB_CAS300;
            break;

        default:
            msg(link->context,
                ll_error,
                "No CAS300 detected equiv. for protocol: %d",
                protocol);
            err = CAHUTE_ERROR_UNKNOWN;
            goto fail;
        }

        goto found;
    } else if (buf[0] == 0x06) {
        /* This can either be the beginning of a Protocol 7.00 ack packet,
         * or the answer to the CAS300 discovery command. In the first case,
         * the packet is 6 bytes long, in the second case, the packet is
         * 3 bytes long, but is immediately followed with a command, so
         * we want to check the 4th byte. */
        err = cahute_receive_on_link_transport(link, &buf[1], 3, 0, 0);
        if (err)
            goto fail;

        received = 4;

        if (buf[3] == 0x01) {
            /* This is the packet identifier we have set on the discovery
             * command, which means it is a CAS300 acknowledgement.
             * We want to receive an acknowledge the information. */
            err = cahute_cas300_receive_packet(link, 0x01, 0);
            if (err)
                return err;

            if (link->protocol_state.casiolink.cas300.packet_subtype != 2
                || link->protocol_state.casiolink.cas300.packet_payload_size
                       != 49) {
                msg(link->context,
                    ll_error,
                    "Answer to discovery command was unexpected!");
                return CAHUTE_ERROR_UNKNOWN;
            }

            switch (protocol) {
            case PROTOCOL_SERIAL_AUTO:
            case PROTOCOL_SERIAL_AUTO_CAS300:
            case PROTOCOL_SERIAL_CAS300:
                protocol = PROTOCOL_SERIAL_CAS300;
                break;

            case PROTOCOL_USB_AUTO:
                protocol = PROTOCOL_USB_CAS300;
                break;

            default:
                msg(link->context,
                    ll_error,
                    "No CAS300 detected equiv. for protocol: %d",
                    protocol);
                err = CAHUTE_ERROR_UNKNOWN;
                goto fail;
            }

            memcpy(
                link->protocol_state.casiolink.raw_device_info,
                link->protocol_state.casiolink.cas300.packet_payload,
                link->protocol_state.casiolink.cas300.packet_payload_size
            );
            *cas300_discoveredp = 1;
            goto found;
        }

        /* This is the beginning of a Protocol 7.00 ack packet.
         * We want to read the rest of the packet to ensure that
         * everything is correct. */
        err = cahute_receive_on_link_transport(link, &buf[4], 2, 0, 0);
        if (err)
            goto fail;

        received = 6;
        if (!memcmp(buf, seven_ack_packet, 6)) {
            /* That's a check packet! We can answer with an ACK, then
             * set the protocol to Protocol 7.00. */
            switch (protocol) {
            case PROTOCOL_SERIAL_AUTO:
            case PROTOCOL_SERIAL_AUTO_CAS40:
            case PROTOCOL_SERIAL_AUTO_CAS50:
            case PROTOCOL_SERIAL_AUTO_CAS100:
            case PROTOCOL_SERIAL_AUTO_CAS300:
                protocol = PROTOCOL_SERIAL_SEVEN;
                break;

            case PROTOCOL_USB_AUTO:
                protocol = PROTOCOL_USB_SEVEN;
                break;

            default:
                msg(link->context,
                    ll_error,
                    "No SEVEN detected equiv. for protocol: %d",
                    protocol);
                err = CAHUTE_ERROR_UNKNOWN;
                goto fail;
            }

            goto found;
        }
    } else if (buf[0] == 0x13) {
        /* This is a CASIOLINK start packet.
         * We can answer with an 'established' packet and set the protocol
         * to CASIOLINK. */
        switch (protocol) {
            /* NOTE: No support for PROTOCOL_SERIAL_AUTO here, since we need to
         * know in which format to send headers as the sender/active side. */

        case PROTOCOL_SERIAL_AUTO_CAS40:
            protocol = PROTOCOL_SERIAL_CAS40;
            break;

        case PROTOCOL_SERIAL_AUTO_CAS50:
            protocol = PROTOCOL_SERIAL_CAS50;
            break;

        case PROTOCOL_SERIAL_AUTO_CAS100:
            protocol = PROTOCOL_SERIAL_CAS100;
            break;

        case PROTOCOL_SERIAL_AUTO_CAS300:
            protocol = PROTOCOL_SERIAL_CAS300;
            break;

        case PROTOCOL_USB_AUTO:
            protocol = PROTOCOL_USB_CAS300;
            break;

        default:
            msg(link->context,
                ll_error,
                "No CAS detected equiv. for protocol: %d",
                protocol);
            err = CAHUTE_ERROR_UNKNOWN;
            goto fail;
        }

        goto found;
    } else if (buf[0] == 0x15) {
        /* This is either a ClassPad 300 / 330 (+) answering our discovery
         * command to tell us the packet is out-of-order, or a past-initiation
         * Protocol 7.00 device signaling an invalid packet.
         *
         * The difficulty here is that the ClassPad out-of-order packet is
         * 3-bytes long, and the Protocol 7.00 NAK packet is 6-bytes
         * long, and there is no actual way to distinguish between both based
         * on the first 3 bytes because the Protocol 7.00 packet subtype is
         * '04', not '00', since the error is generic. So we have to resort
         * to reading the first 3 bytes, then trying to read a fourth one,
         * and if we fail, we assume it's a ClassPad out-of-order packet. */
        err = cahute_receive_on_link_transport(link, &buf[1], 2, 0, 0);
        if (err)
            goto fail;
        received = 3;

        err = cahute_receive_on_link_transport(link, &buf[3], 3, 50, 50);
        if (!err) {
            /* We have a Protocol 7.00 NAK. */
            received = 6;

            if (!memcmp(buf, seven_nak_packet, 6)) {
                msg(link->context,
                    ll_debug,
                    "Received Protocol 7.00 NAK packet:");
                mem(link->context, ll_debug, buf, 6);

                switch (protocol) {
                case PROTOCOL_SERIAL_AUTO:
                case PROTOCOL_SERIAL_AUTO_CAS40:
                case PROTOCOL_SERIAL_AUTO_CAS50:
                case PROTOCOL_SERIAL_AUTO_CAS100:
                case PROTOCOL_SERIAL_AUTO_CAS300:
                    protocol = PROTOCOL_SERIAL_SEVEN;
                    break;

                case PROTOCOL_USB_AUTO:
                    protocol = PROTOCOL_USB_SEVEN;
                    break;

                default:
                    msg(link->context,
                        ll_error,
                        "No SEVEN detected equiv. for protocol: %d",
                        protocol);
                    err = CAHUTE_ERROR_UNKNOWN;
                    goto fail;
                }

                goto found;
            }
        } else if (err != CAHUTE_ERROR_TIMEOUT_START)
            goto fail;
        else {
            msg(link->context,
                ll_debug,
                "Received possible CAS300 out-of-order error is the following:"
            );
            mem(link->context, ll_debug, buf, 3);

            if (!cahute_is_ascii_hex(buf[1]) || !cahute_is_ascii_hex(buf[2])) {
                msg(link->context,
                    ll_error,
                    "Packet identifier is not ASCII-HEX!");
                err = CAHUTE_ERROR_UNKNOWN;
                goto fail;
            }

            *cas300_next_idp = (cahute_ascii_hex_to_nibble(buf[1]) << 4)
                               | cahute_ascii_hex_to_nibble(buf[2]);

            switch (protocol) {
            case PROTOCOL_SERIAL_AUTO:
            case PROTOCOL_SERIAL_AUTO_CAS300:
            case PROTOCOL_SERIAL_CAS300:
                protocol = PROTOCOL_SERIAL_CAS300;
                break;

            case PROTOCOL_USB_AUTO:
                protocol = PROTOCOL_USB_CAS300;
                break;

            default:
                msg(link->context,
                    ll_error,
                    "No CAS300 detected equiv. for protocol: %d",
                    protocol);
                err = CAHUTE_ERROR_UNKNOWN;
                goto fail;
            }

            goto found;
        }
    }

    /* We want to receive as much bytes as possible to fit the buffer. */
    err = CAHUTE_OK;
    for (; received < sizeof(buf); received++) {
        err = cahute_receive_on_link_transport(link, &buf[received], 1, 50, 0);
        if (err)
            break;
    }

    msg(link->context,
        ll_error,
        "Unable to determine a protocol out of the received data:");
    mem(link->context, ll_error, buf, received);
    if (err && err != CAHUTE_ERROR_TIMEOUT_START)
        msg(link->context,
            ll_info,
            "Error %s (%d) was encountered while receiving more bytes.",
            cahute_get_error_name(err),
            err);

    err = CAHUTE_ERROR_UNKNOWN;

fail:
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        err = CAHUTE_ERROR_TIMEOUT;
    return err;

found:
    *protocolp = protocol;
    return CAHUTE_OK;
}

/**
 * Initialize protocol for a link.
 *
 * @param link Link to initialize.
 * @param protocol Protocol to initialize the link with.
 * @param flags Flags to tweak the function's behaviour.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_initialize_link_protocol(
    cahute_link *link,
    int protocol,
    unsigned long flags
) {
    int err, cas300_discovered = 0, cas300_next_id = 0;
    int exchange_cas100_model_info = 0;
    struct cahute_casiolink_state *casiolink_state;
    struct cahute_seven_state *seven_state;
    struct cahute_seven_ohp_state *seven_ohp_state;

    if (~flags & PROTOCOL_FLAG_NOTERM)
        link->flags |= CAHUTE_LINK_FLAG_TERMINATE;
    if (flags & PROTOCOL_FLAG_RECEIVER)
        link->flags |= CAHUTE_LINK_FLAG_RECEIVER;

    if (protocol & PROTOCOL_AUTO_FLAG) {
        if (flags & PROTOCOL_FLAG_RECEIVER)
            err = determine_protocol_as_receiver(link, &protocol);
        else
            err = determine_protocol_as_sender(
                link,
                &protocol,
                &cas300_discovered,
                &cas300_next_id
            );

        if (err)
            return err;

        /* The protocol has been found using automatic discovery, by tweaking
         * the check handshake! It should not be re-done.
         * If the protocol is CAS100, the model information has not yet been
         * exchanged, and must be for the communication to be successful. */
        flags |= PROTOCOL_FLAG_NOCHECK;
        exchange_cas100_model_info = 1;
    }

    /* Map the linkopen protocol to the actual link protocol. */
    link->protocol = get_protocol_value(link->context, protocol);
    msg(link->context,
        ll_info,
        "Using %s over %s.",
        cahute_get_protocol_name(link->protocol),
        link->transport_name);
    msg(link->context,
        ll_info,
        "Playing the role of %s.",
        flags & PROTOCOL_FLAG_RECEIVER ? "receiver / passive side"
                                       : "sender / active side");

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_NONE:
    case CAHUTE_LINK_PROTOCOL_USB_NONE:
        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS:
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS40:
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS50:
        casiolink_state = &link->protocol_state.casiolink;
        casiolink_state->flags = 0;
        casiolink_state->cas300.next_id = 0;

        if (link->data_buffer_capacity < CASIOLINK_MINIMUM_BUFFER_SIZE) {
            msg(link->context,
                ll_fatal,
                "CASIOLINK implementation expected a minimum data "
                "buffer capacity of %" CAHUTE_PRIuSIZE
                ", got %" CAHUTE_PRIuSIZE ".",
                CASIOLINK_MINIMUM_BUFFER_SIZE,
                link->data_buffer_capacity);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (flags & PROTOCOL_FLAG_NOCHECK)
            err = CAHUTE_OK;
        else if (link->flags & CAHUTE_LINK_FLAG_RECEIVER)
            err = cahute_casiolink_initiate_as_receiver(link);
        else
            err = cahute_casiolink_initiate_as_sender(link);

        if (err)
            return err;

        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS100:
        if (link->data_buffer_capacity < CASIOLINK_MINIMUM_BUFFER_SIZE) {
            msg(link->context,
                ll_fatal,
                "CASIOLINK implementation expected a minimum data "
                "buffer capacity of %" CAHUTE_PRIuSIZE
                ", got %" CAHUTE_PRIuSIZE ".",
                CASIOLINK_MINIMUM_BUFFER_SIZE,
                link->data_buffer_capacity);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (flags & PROTOCOL_FLAG_NOCHECK)
            err = CAHUTE_OK;
        else if (link->flags & CAHUTE_LINK_FLAG_RECEIVER) {
            err = cahute_casiolink_initiate_as_receiver(link);
            exchange_cas100_model_info = 1;
        } else {
            err = cahute_casiolink_initiate_as_sender(link);
            exchange_cas100_model_info = 1;
        }

        if (err)
            return err;

        /* The following occurs occurs in two possible situations:
         * - We have just made the CASIOLINK check flow (0x16 / 0x13);
         * - We have made the CASIOLINK check flow in the protocol discovery
         *   step, and must either exchange model information (sender), or
         *   expect model information to be exchanged (receiver). */
        if (!exchange_cas100_model_info)
            err = CAHUTE_OK;
        else if (link->flags & CAHUTE_LINK_FLAG_RECEIVER)
            err = cahute_cas100_handle_mdl1(link, NULL);
        else
            err = cahute_cas100_exchange_model_information(link);

        if (err)
            return err;
        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS300:
    case CAHUTE_LINK_PROTOCOL_USB_CAS300:
        casiolink_state = &link->protocol_state.casiolink;
        casiolink_state->flags = 0;
        casiolink_state->cas300.next_id = cas300_next_id;

        if (link->data_buffer_capacity < CASIOLINK_MINIMUM_BUFFER_SIZE) {
            msg(link->context,
                ll_fatal,
                "CASIOLINK implementation expected a minimum data "
                "buffer capacity of %" CAHUTE_PRIuSIZE
                ", got %" CAHUTE_PRIuSIZE ".",
                CASIOLINK_MINIMUM_BUFFER_SIZE,
                link->data_buffer_capacity);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (cas300_discovered) {
            /* CAS300 device information has already been found and stored
             * in the context of automatic protocol detection as a sender,
             * we want to set the flags representing this fact. */
            casiolink_state->flags |=
                (CASIOLINK_FLAG_DEVICE_INFO_OBTAINED
                 | CASIOLINK_FLAG_DEVICE_INFO_CAS300);
        }

        if (flags & PROTOCOL_FLAG_NOCHECK)
            err = CAHUTE_OK;
        else if (link->flags & CAHUTE_LINK_FLAG_RECEIVER)
            err = cahute_cas300_initiate_as_receiver(link);
        else
            err = cahute_cas300_initiate_as_sender(link);

        if (err)
            return err;

        if ((~link->flags & CAHUTE_LINK_FLAG_RECEIVER)
            && (~flags & PROTOCOL_FLAG_NODISC)
            && (~casiolink_state->flags & CASIOLINK_FLAG_DEVICE_INFO_OBTAINED
            )) {
            err = cahute_cas300_discover(link);
            if (err)
                return err;
        }

        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        seven_state = &link->protocol_state.seven;
        seven_state->flags = 0;
        seven_state->last_packet_type = -1;
        seven_state->last_packet_subtype = -1;
        seven_state->last_packet_data_size = 0;
        seven_state->raw_device_info_size = 0;

        if (~flags & PROTOCOL_FLAG_NOCHECK) {
            err = cahute_seven_initiate(link);
            if (err)
                return err;
        }

        if (~flags & PROTOCOL_FLAG_RECEIVER
            && (~flags & PROTOCOL_FLAG_NODISC)) {
            err = cahute_seven_discover(link);
            if (err)
                return err;
        }

        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP:
        seven_ohp_state = &link->protocol_state.seven_ohp;

        if (link->data_buffer_capacity < DEFAULT_PICTURE_BUFFER_SIZE) {
            msg(link->context,
                ll_error,
                "Expected at least %" CAHUTE_PRIuSIZE
                " bytes in the data "
                "buffer for Protocol 7.00 Screenstreaming.",
                DEFAULT_PICTURE_BUFFER_SIZE);
            return CAHUTE_ERROR_SIZE;
        }

        link->data_buffer_capacity -= DEFAULT_PICTURE_BUFFER_SIZE;
        seven_ohp_state->picture_buf =
            &link->data_buffer[link->data_buffer_capacity];
        seven_ohp_state->picture_capacity = DEFAULT_PICTURE_BUFFER_SIZE;
        seven_ohp_state->picture_size = 0;

        /* No need to guarantee a minimum data buffer size here;
         * all writes to the data buffer will check for its capacity! */
        seven_ohp_state->last_packet_type = -1;
        memset(seven_ohp_state->last_packet_subtype, 0, 5);
        seven_ohp_state->picture_format = -1;
        seven_ohp_state->picture_width = -1;
        seven_ohp_state->picture_height = -1;
        break;

    default:
        CAHUTE_RETURN_IMPL(
            link->context,
            "No initialization routine for the protocol."
        );
    }

    return CAHUTE_OK;
}
