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
#define INIT_ATTEMPTS 10

/**
 * Initiate the connection as a sender, for CAS300.
 *
 * NOTE: This function will fail if the communication has already been
 *       initialized!
 *
 * @param link Link for which to initiate the connection, as a sender.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int) cahute_cas300_initiate_as_sender(cahute_link *link) {
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
