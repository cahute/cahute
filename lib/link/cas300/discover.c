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
CAHUTE_INTERNAL(int) cahute_cas300_discover(cahute_link *link) {
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
