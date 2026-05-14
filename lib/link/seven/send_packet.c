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

/* Raw check packet to send in case of timeout. */
CAHUTE_LOCAL_DATA(cahute_u8)
timeout_check_packet[] = {5, '0', '1', '0', '6', 'F'};

/**
 * Send a raw Protocol 7.00 packet, receive a response and store it into
 * the link.
 *
 * As opposed to the basic send or receive functions, this function supports
 * receiving invalid checksums and re-sending packets in such cases.
 *
 * This function should not be used directly, but with either
 * ``cahute_seven_send_basic`` or ``cahute_seven_send_extended``.
 *
 * @param link Link to use to send and receive the Protocol 7.00 packet.
 * @param flags Flags, as or'd `CAHUTE_SEVEN_SEND_FLAG_*` constants.
 * @param raw_packet Raw packet data to send.
 * @param raw_packet_size Size of the raw packet data to send.
 * @param timeout Timeout to use for start of packet.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_seven_send_and_receive(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *raw_packet,
    size_t raw_packet_size,
    unsigned long timeout
) {
    int err, correct = 0;
    int attempts, initial_attempts = 3;

    if (flags & CAHUTE_SEVEN_SEND_FLAG_DISABLE_CHECKSUM) {
        /* No retries and no checksum flow if this flag is on, we directly
         * return CAHUTE_ERROR_CORRUPT! */
        initial_attempts = 1;
    }

    for (attempts = initial_attempts; attempts > 0; attempts--) {
        msg(link->context,
            ll_debug,
            "Sending the following packet to the device:");
        mem(link->context, ll_debug, raw_packet, raw_packet_size);

        err = cahute_send_on_link_transport(link, raw_packet, raw_packet_size);
        if (err)
            return err;

        if (flags & CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE) {
            /* We don't want to receive the response here, so we consider the
             * flow to be correct! */
            correct = 1;
            break;
        }

        msg(link->context,
            ll_debug,
            "Packet sent successfully, now waiting for response.");
        err = cahute_seven_receive(link, timeout);
        if (err == CAHUTE_ERROR_TIMEOUT_START
            && (~flags & CAHUTE_SEVEN_SEND_FLAG_DISABLE_TIMEOUT)) {
            /* We are about to continue, but if the timeout recovery flow
             * succeeds, we want to restore the number of attempts. */
            msg(link->context,
                ll_debug,
                "Link did not respond in a timely manner; sending timeout "
                "check:");
            mem(link->context, ll_info, timeout_check_packet, 6);

            err = cahute_send_on_link_transport(link, timeout_check_packet, 6);
            if (err)
                return err;

            err = cahute_seven_receive(link, TIMEOUT_PACKET_TIMEOUT);
            if (err == CAHUTE_ERROR_TIMEOUT_START) {
                msg(link->context,
                    ll_debug,
                    "Link did not respond on sent packet nor timeout check.");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
                return CAHUTE_ERROR_TIMEOUT_START;
            }

            if (err)
                return err;

            /* From fxReverse: "If the passive side receives the check packet,
             * an error packet is returned as a retransmission request,
             * the active side resends the packet and communication
             * continues". */
            if (link->protocol_state.seven.last_packet_type != PACKET_TYPE_NAK
                || link->protocol_state.seven.last_packet_subtype
                       != PACKET_SUBTYPE_NAK_RESEND) {
                msg(link->context,
                    ll_debug,
                    "Expected a resend error on timeout check, got a packet "
                    "of type %02X and subtype %02X.",
                    link->protocol_state.seven.last_packet_type,
                    link->protocol_state.seven.last_packet_subtype);
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
                return CAHUTE_ERROR_TIMEOUT_START;
            }

            attempts = initial_attempts;
            continue;
        }

        if (err)
            return err;

        if (link->protocol_state.seven.last_packet_type == PACKET_TYPE_NAK
            && link->protocol_state.seven.last_packet_subtype
                   == PACKET_SUBTYPE_NAK_RESEND) {
            /* The checksum may have been invalidated by the transport, we want
             * to try to resend. */
            continue;
        }

        correct = 1;
        break;
    }

    if (!correct) {
        /* All attempts at sending the packet have unfortunately failed. */
        return CAHUTE_ERROR_CORRUPT;
    }

    return CAHUTE_OK;
}
