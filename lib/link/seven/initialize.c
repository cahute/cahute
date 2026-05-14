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

/* Raw initial check packet to send. */
CAHUTE_LOCAL_DATA(cahute_u8)
initial_check_packet[] = {5, '0', '0', '0', '7', '0'};

/**
 * Initiate the Protocol 7.00 communication.
 *
 * For more information on this flow, see :ref:`seven-init-link`.
 *
 * @param link Link on which to initiate the Protocol 7.00 communication.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int) cahute_seven_initiate(cahute_link *link) {
    int err, attempts = 8;

    if (link->flags & CAHUTE_LINK_FLAG_RECEIVER) {
        err = cahute_seven_receive(link, TIMEOUT_PACKET_START);
        if (err)
            return err;

        EXPECT_PACKET(PACKET_TYPE_CHECK, PACKET_SUBTYPE_CHECK_INIT);

        err = cahute_seven_send_basic(
            link,
            CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        return err;
    }

    for (; attempts > 0; attempts--) {
        err = cahute_seven_send_and_receive(
            link,
            CAHUTE_SEVEN_SEND_FLAG_DISABLE_TIMEOUT,
            initial_check_packet,
            6,
            TIMEOUT_PACKET_INIT
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            continue;
        else if (err)
            return err;

        if (link->protocol_state.seven.last_packet_type != PACKET_TYPE_ACK
            || link->protocol_state.seven.last_packet_subtype
                   != PACKET_SUBTYPE_ACK_BASIC) {
            msg(link->context,
                ll_error,
                "Calculator did not answer a basic ACK.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        return CAHUTE_OK;
    }

    msg(link->context, ll_error, "Link did not respond to the initial check.");
    return CAHUTE_ERROR_TIMEOUT_START;
}
