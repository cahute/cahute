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
 * Expect an MDL1 header, or react to a received one.
 *
 * This can be called in two contexts:
 *
 * - We're a CAS100 receiver, have just done the CASIOLINK initiation flow,
 *   and are now expecting a MDL1 header.
 * - We're a generic receiver, and while receiving data, we have just received
 *   an MDL1 header we want to react to.
 *
 * Note that we actually answer with the same MDL1 to make the calculator
 * believe we are compatible with them.
 *
 * @param link Link for which to handle the exchange.
 * @param header MDL1 header to react to.
 *        In the first context, this is expected to be set to NULL.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_cas100_handle_mdl1(cahute_link *link, cahute_u8 const *header) {
    cahute_u8 buf[40];
    int byte, err;

    if (!header) {
        err = cahute_casiolink_receive_packet(
            link,
            buf,
            38,
            PACKET_TYPE_HEADER,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;

        header = buf;
        msg(link->context, ll_debug, "Received the following header:");
        mem(link->context, ll_debug, header, 40);
    }

    if (memcmp(header, "\x3AMDL1", 5)) {
        msg(link->context,
            ll_error,
            "Did not receive an MDL1 header as expected.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We want to store the provided information. */
    memcpy(
        link->protocol_state.casiolink.raw_device_info,
        &header[5],
        CAS100_RAW_DEVICE_INFO_SIZE
    );
    link->protocol_state.casiolink.flags |=
        CASIOLINK_FLAG_DEVICE_INFO_OBTAINED;

    /* Send the MDL1 answer now. */
    err = cahute_send_on_link_transport(link, header, 40);
    if (err)
        return err;

    /* We should actually be receiving an acknowledgement, since we are
     * sending the same packet the calculator sent. */
    err = cahute_receive_byte_on_link_transport(link, &byte, 0);
    if (err)
        return err;

    if (byte != PACKET_TYPE_ACK) {
        err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_CORRUPTED);
        if (err)
            return err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We can now send an acknowledgement.
     * The acknowledgement is already in our buffer, we can use that. */
    err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_ACK);
    if (err)
        return err;

    return CAHUTE_OK;
}
