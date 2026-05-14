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
 * Terminate the connection, for CAS300.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int) cahute_cas300_terminate(cahute_link *link) {
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
