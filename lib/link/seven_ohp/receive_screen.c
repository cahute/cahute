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
 * Receive a frame through screenstreaming.
 *
 * @param link Link for which to receive screens.
 * @param frame Function to call back.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_seven_ohp_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
) {
    struct cahute_seven_ohp_state *state = &link->protocol_state.seven_ohp;
    int err;

    while (1) {
        err = cahute_seven_ohp_receive(link, 1, timeout);
        switch (err) {
        case CAHUTE_OK:
            /* Continue. */
            break;

        case CAHUTE_ERROR_CORRUPT:
            /* In case of checksum error, we just continue receiving
             * packets. */
            msg(link->context, ll_warn, "Missed a frame due to corruption.");
            continue;

        default:
            return err;
        }

        switch (state->last_packet_type) {
        case PACKET_TYPE_FRAME:
            frame->cahute_frame_width = state->picture_width;
            frame->cahute_frame_height = state->picture_height;
            frame->cahute_frame_format = state->picture_format;
            frame->cahute_frame_data = state->picture_buf;

            return CAHUTE_OK;

        case PACKET_TYPE_CHECK:
            err = cahute_seven_ohp_send_basic(
                link,
                PACKET_TYPE_ACK,
                (cahute_u8 *)"02001"
            );
            if (err)
                return err;

            break;

        default:
            msg(link->context,
                ll_error,
                "Unexpected packet of type %d (0x%02X), exiting.",
                state->last_packet_type,
                state->last_packet_type);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    /* We shouldn't have arrived here. */
    return CAHUTE_ERROR_UNKNOWN;
}
