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
 * Request device information using Protocol 7.00.
 *
 * This stores the raw information in ``raw_device_info`` from the link's
 * Protocol 7.00 peer state, so that it can be exploited later if need be.
 *
 * For more information on this flow, see :ref:`seven-get-device-information`.
 *
 * @param link Link on which to request device information.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int) cahute_seven_discover(cahute_link *link) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x01,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        TIMEOUT_COMMAND_RESPONSE
    );
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_ACK, PACKET_SUBTYPE_ACK_EXTENDED);

    if (link->protocol_state.seven.last_packet_data_size
        > SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE) {
        msg(link->context,
            ll_error,
            "Could not store obtained device information (got "
            "%" CAHUTE_PRIuSIZE "/%" CAHUTE_PRIuSIZE " bytes)",
            link->protocol_state.seven.last_packet_data_size,
            SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE);
        return CAHUTE_ERROR_SIZE;
    }

    memcpy(
        link->protocol_state.seven.raw_device_info,
        link->protocol_state.seven.last_packet_data,
        link->protocol_state.seven.last_packet_data_size
    );
    link->protocol_state.seven.raw_device_info_size =
        link->protocol_state.seven.last_packet_data_size;
    link->protocol_state.seven.flags |= SEVEN_FLAG_DEVICE_INFO_REQUESTED;

    return CAHUTE_OK;
}
