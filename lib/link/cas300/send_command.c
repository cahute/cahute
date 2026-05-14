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
 * Send a CAS300 command.
 *
 * @param link Link to use.
 * @param command Command to send.
 * @param payload Payload to include with the command.
 * @param payload_size Size of the payload to include with the command.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
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
