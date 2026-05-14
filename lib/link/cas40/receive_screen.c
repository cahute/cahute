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
 * Receive a frame through screen capture.
 *
 * @param link Link for which to receive screens.
 * @param frame Function to call back.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_cas40_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    cahute_u8 const *header,
    unsigned long timeout
) {
    struct cahute_casiolink_data_description desc;
    cahute_u8 *buf = link->data_buffer;
    size_t sheet_size;
    int err;

    do {
        err = cahute_cas40_receive_raw_data(link, header, timeout, &desc);
        if (err == CAHUTE_ERROR_TIMEOUT_START) {
            msg(link->context,
                ll_error,
                "No data received in a timely matter, exiting.");
            break;
        }

        if (err)
            return err;

        if (!memcmp(&buf[1], "DD", 2)) {
            if (!memcmp(&buf[5], "\x10\x44WF", 4))
                frame->cahute_frame_format =
                    CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50;
            else
                continue;

            frame->cahute_frame_height = buf[3];
            frame->cahute_frame_width = buf[4];
            frame->cahute_frame_data = &buf[40];
        } else if (!memcmp(&buf[1], "DC", 2)) {
            if (!memcmp(&buf[5], "\x11UWF\x03", 5)) {
                size_t expected_size;
                int first_color, second_color, third_color;

                sheet_size = buf[3] * ((buf[4] >> 3) + !!(buf[4] & 7));
                expected_size = 40 + (sheet_size + 3) * 3;
                if (link->data_buffer_size != expected_size) {
                    msg(link->context,
                        ll_error,
                        "Invalid data size %" CAHUTE_PRIuSIZE
                        " (expected: %" CAHUTE_PRIuSIZE ")",
                        link->data_buffer_size,
                        expected_size);
                    continue;
                }

                /* Check that the color codes are all known, i.e. that
                 * they all are between 1 and 4 included. */
                first_color = buf[41];
                second_color = buf[44 + sheet_size];
                third_color = buf[47 + sheet_size + sheet_size];

                if (first_color < 1 || first_color > 4) {
                    msg(link->context,
                        ll_warn,
                        "Unknown color code 0x%02X for sheet 1, skipping.",
                        first_color);
                    continue;
                }
                if (second_color < 1 || second_color > 4) {
                    msg(link->context,
                        ll_warn,
                        "Unknown color code 0x%02X for sheet 2, skipping.",
                        second_color);
                    continue;
                }
                if (third_color < 1 || third_color > 4) {
                    msg(link->context,
                        ll_warn,
                        "Unknown color code 0x%02X for sheet 3, skipping.",
                        third_color);
                    continue;
                }

                frame->cahute_frame_format =
                    CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50;
            } else
                continue;

            frame->cahute_frame_height = buf[3];
            frame->cahute_frame_width = buf[4];
            frame->cahute_frame_data = &buf[40];
        } else
            continue;

        /* Frame is ready! */
        break;
    } while (1);

    /* We actually unset the fact that the link is terminated here, since
     * every screen is actually its own exchange. */
    link->flags &= ~CAHUTE_LINK_FLAG_TERMINATED;

    return CAHUTE_OK;
}
