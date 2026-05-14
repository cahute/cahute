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
 * Initiate the connection as a sender, for any CASIOLINK variant.
 *
 * @param link Link for which to initiate the connection as a sender.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int) cahute_casiolink_initiate_as_sender(cahute_link *link) {
    int initial_attempts = 6, attempts, err, byte;

    msg(link->context,
        ll_info,
        "Making the initial handshake (%d attempts, %lums for each).",
        initial_attempts,
        TIMEOUT_INIT);
    for (attempts = initial_attempts; attempts > 0; attempts--) {
        msg(link->context,
            ll_debug,
            "Sending 0x%02X start packet.",
            PACKET_TYPE_START);

        err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_START);
        if (err)
            return err;

        /* On CAS300 serial links, the calculator may send invalid 0x00 bytes
         * until it sends something else, so we want to ignore such cases. */
        for (byte = -1; !err && byte <= 0;) {
            err = cahute_receive_byte_on_link_transport(
                link,
                &byte,
                TIMEOUT_INIT
            );
        }

        if (err == CAHUTE_ERROR_TIMEOUT_START)
            continue;

        if (byte != PACKET_TYPE_ESTABLISHED) {
            msg(link->context,
                ll_error,
                "Expected ESTABLISHED packet (0x%02X), got 0x%02X.",
                PACKET_TYPE_ESTABLISHED,
                byte);

            return CAHUTE_ERROR_UNKNOWN;
        }

        break;
    }

    if (attempts <= 0) {
        msg(link->context,
            ll_error,
            "No response after %d attempts.",
            initial_attempts);
        return CAHUTE_ERROR_TIMEOUT_START;
    }

    return CAHUTE_OK;
}
