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

/* The 0002 command for Classpad 300 / 330 (+) used for initialization when
 * in sender / control mode. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
default_0002_payload =
    (cahute_u8 const *)"CP430\xFF\xFF\xFF" "00.00.0(0305000001.01.0016M"
    "\xFF\xFF\xFF\xFF\xFF" "8M\xFF\xFF\xFF\xFF\xFF\xFF\x81";

/**
 * Receive data, optionally starting from a byte.
 *
 * @param link Link to receive data from.
 * @param datap Pointer to the data pointer to populate.
 * @param first_byte First byte to receive.
 * @param timeout Timeout to receive the first byte.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
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
