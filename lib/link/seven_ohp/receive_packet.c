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

/* Recognized packet headers for alignment. */
CAHUTE_LOCAL_DATA(char const *)
alignment_sequences[] = {
    "\x0BTYP01",
    "\x0BTYPZ1",
    "\x0BTYPZ2",
    "\x0BTYPB1",
    ("\x16"
     "CAL00")
};
CAHUTE_LOCAL_DATA(size_t)
alignment_sequence_count = sizeof(alignment_sequences) / sizeof(char const *);

/**
 * Receive and decode a Protocol 7.00 screenstreaming packet, and store it
 * into the link.
 *
 * Note that if we receive a frame packet, we store its content directly
 * into the data buffer if we have enough capacity in it.
 *
 * @param link Link to use to receive the Protocol 7.00 packet.
 * @param align Whether we should align ourselves. to the beginning of the next
 *        packet, to avoid missing bytes from corrupting the whole connection.
 * @param timeout Timeout before the first byte.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_seven_ohp_receive(cahute_link *link, int align, unsigned long timeout) {
    struct cahute_seven_ohp_state *state = &link->protocol_state.seven_ohp;
    cahute_u8 buf[50], *data_buf = state->picture_buf;
    size_t data_capacity = state->picture_capacity;
    size_t *data_sizep = &state->picture_size;
    size_t packet_size;
    int err, is_blit = 0;
    unsigned long blit_x = 0, blit_y = 0;

    if (align) {
        size_t to_complete = 6;

        /* We're aligning ourselves to receive a known packet.
         *
         * This is useful, if not necessary, because the calculator seems to
         * skip a few bytes sometimes, desynchronizing the input, meaning we
         * can't get frames from the desynchronization onward.
         *
         * With screenstreaming packet alignment, we only skip two frames
         * (the one cut short and the next one which has started too early
         * and for which the start was considered as part of the last packet),
         * then are able to recover.
         *
         * The sequences we can be expecting */
        while (1) {
            err = cahute_receive_on_link_transport(
                link,
                &buf[6 - to_complete],
                to_complete,
                timeout,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                return err;

            /* We look at every alignment sequence for every completion. */
            for (to_complete = 0; to_complete < 6; to_complete++) {
                size_t count = alignment_sequence_count;
                char const * const *p;

                for (p = alignment_sequences; count--; p++)
                    if (!memcmp(&buf[to_complete], *p, 6 - to_complete))
                        goto sequence_found;
            }

sequence_found:
            if (!to_complete)
                break;

            /* If we have found 2 matching bytes at the end of the buffer,
             * then we have 4 chars to complete.
             * This means we must move 6 - 4 = 2 bytes from index 4 onwards
             * to the beginning of the buffer. */
            if (to_complete < 6)
                memmove(buf, &buf[to_complete], 6 - to_complete);
        }
    } else {
        /* We just need to fill the initial 6 bytes in the buffer. */
        err = cahute_receive_on_link_transport(
            link,
            buf,
            6,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;
    }

    state->last_packet_type = buf[0];
    memcpy(state->last_packet_subtype, &buf[1], 5);
    link->data_buffer_size = 0;

    packet_size = 6;
    if (buf[0] == PACKET_TYPE_CHECK || buf[0] == PACKET_TYPE_ACK) {
        /* Nothing to process, we just need the packet type and subtype to
         * be set, and that's already done above. */
    } else if (buf[0] == PACKET_TYPE_FRAME) {
        int width, height, format = -1;
        size_t frame_length = 0, expected_size;

        /* The subtype represents the kind of frame we have. */
        if (!memcmp(&buf[1], "TYP01", 5)) {
            width = 128;
            height = 64;
            format = CAHUTE_PICTURE_FORMAT_1BIT_MONO;
            frame_length = 1024;
        } else if (!memcmp(&buf[1], "TYPZ1", 5) || !memcmp(&buf[1], "TYPZ2", 5)) {
            if (buf[5] == '1') {
                /* The Frame Length (FL) field is 6 bytes long. */
                err = cahute_receive_on_link_transport(
                    link,
                    &buf[6],
                    18,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                if (!cahute_is_ascii_hex(buf[6])
                    || !cahute_is_ascii_hex(buf[7])
                    || !cahute_is_ascii_hex(buf[8])
                    || !cahute_is_ascii_hex(buf[9])
                    || !cahute_is_ascii_hex(buf[10])
                    || !cahute_is_ascii_hex(buf[11]))
                    return CAHUTE_ERROR_CORRUPT;

                packet_size += 18;
                frame_length =
                    ((cahute_ascii_hex_to_nibble(buf[6]) << 20)
                     | (cahute_ascii_hex_to_nibble(buf[7]) << 16)
                     | (cahute_ascii_hex_to_nibble(buf[8]) << 12)
                     | (cahute_ascii_hex_to_nibble(buf[9]) << 8)
                     | (cahute_ascii_hex_to_nibble(buf[10]) << 4)
                     | cahute_ascii_hex_to_nibble(buf[11]));
            } else {
                /* The Frame Length (FL) field is 8 bytes long. */
                err = cahute_receive_on_link_transport(
                    link,
                    &buf[6],
                    20,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                if (!cahute_is_ascii_hex(buf[6])
                    || !cahute_is_ascii_hex(buf[7])
                    || !cahute_is_ascii_hex(buf[8])
                    || !cahute_is_ascii_hex(buf[9])
                    || !cahute_is_ascii_hex(buf[10])
                    || !cahute_is_ascii_hex(buf[11])
                    || !cahute_is_ascii_hex(buf[12])
                    || !cahute_is_ascii_hex(buf[13]))
                    return CAHUTE_ERROR_CORRUPT;

                packet_size += 20;
                frame_length =
                    ((cahute_ascii_hex_to_nibble(buf[6]) << 28)
                     | (cahute_ascii_hex_to_nibble(buf[7]) << 24)
                     | (cahute_ascii_hex_to_nibble(buf[8]) << 20)
                     | (cahute_ascii_hex_to_nibble(buf[9]) << 16)
                     | (cahute_ascii_hex_to_nibble(buf[10]) << 12)
                     | (cahute_ascii_hex_to_nibble(buf[11]) << 8)
                     | (cahute_ascii_hex_to_nibble(buf[12]) << 4)
                     | cahute_ascii_hex_to_nibble(buf[13]));
            }

            if (!cahute_is_ascii_hex(buf[packet_size - 12])
                || !cahute_is_ascii_hex(buf[packet_size - 11])
                || !cahute_is_ascii_hex(buf[packet_size - 10])
                || !cahute_is_ascii_hex(buf[packet_size - 9])
                || !cahute_is_ascii_hex(buf[packet_size - 8])
                || !cahute_is_ascii_hex(buf[packet_size - 7])
                || !cahute_is_ascii_hex(buf[packet_size - 6])
                || !cahute_is_ascii_hex(buf[packet_size - 5])) {
                /* The header is corrupted.
                 * We however still want to skip the frame length and the
                 * checksum in order to fall back on our feet on next
                 * packet reception. */
                err = cahute_receive_on_link_transport(
                    link,
                    NULL,
                    frame_length + 2,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                return CAHUTE_ERROR_CORRUPT;
            }

            height =
                ((cahute_ascii_hex_to_nibble(buf[packet_size - 12]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[packet_size - 11]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[packet_size - 10]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[packet_size - 9]));
            width =
                ((cahute_ascii_hex_to_nibble(buf[packet_size - 8]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[packet_size - 7]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[packet_size - 6]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[packet_size - 5]));

            if (!memcmp(&buf[packet_size - 4], "1RC2", 4)) {
                format = CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5;
                expected_size = width * height * 2;
            } else if (!memcmp(&buf[packet_size - 4], "1RC3", 4)) {
                format = CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED;
                expected_size = width * height;
                expected_size = (expected_size >> 1) + (expected_size & 1);
            } else if (!memcmp(&buf[packet_size - 4], "1RM2", 4)) {
                format = CAHUTE_PICTURE_FORMAT_1BIT_DUAL;
                expected_size = ((width >> 3) + !!(width & 7)) * height << 1;
            } else {
                msg(link->context,
                    ll_warn,
                    "The following Frame Format was unknown:");
                mem(link->context, ll_warn, &buf[packet_size - 4], 4);
            }
        } else if (!memcmp(&buf[1], "TYPB1", 5)) {
            /* The Frame Length (FL) field is 6 bytes long. */
            err = cahute_receive_on_link_transport(
                link,
                &buf[6],
                24,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            packet_size += 24;
            if (!cahute_is_ascii_hex(buf[6]) || !cahute_is_ascii_hex(buf[7])
                || !cahute_is_ascii_hex(buf[8]) || !cahute_is_ascii_hex(buf[9])
                || !cahute_is_ascii_hex(buf[10])
                || !cahute_is_ascii_hex(buf[11])
                || !cahute_is_ascii_hex(buf[12])
                || !cahute_is_ascii_hex(buf[13]))
                return CAHUTE_ERROR_CORRUPT;

            frame_length =
                ((cahute_ascii_hex_to_nibble(buf[6]) << 28)
                 | (cahute_ascii_hex_to_nibble(buf[7]) << 24)
                 | (cahute_ascii_hex_to_nibble(buf[8]) << 20)
                 | (cahute_ascii_hex_to_nibble(buf[9]) << 16)
                 | (cahute_ascii_hex_to_nibble(buf[10]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[11]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[12]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[13]));

            if (!cahute_is_ascii_hex(buf[14]) || !cahute_is_ascii_hex(buf[15])
                || !cahute_is_ascii_hex(buf[16])
                || !cahute_is_ascii_hex(buf[17])
                || !cahute_is_ascii_hex(buf[18])
                || !cahute_is_ascii_hex(buf[19])
                || !cahute_is_ascii_hex(buf[20])
                || !cahute_is_ascii_hex(buf[21])
                || !cahute_is_ascii_hex(buf[22])
                || !cahute_is_ascii_hex(buf[23])
                || !cahute_is_ascii_hex(buf[24])
                || !cahute_is_ascii_hex(buf[25])
                || !cahute_is_ascii_hex(buf[26])
                || !cahute_is_ascii_hex(buf[27])
                || !cahute_is_ascii_hex(buf[28])
                || !cahute_is_ascii_hex(buf[29])) {
                /* The header is corrupted.
                 * We however still want to skip the frame length and the
                 * checksum in order to fall back on our feet on next
                 * packet reception. */
                err = cahute_receive_on_link_transport(
                    link,
                    NULL,
                    frame_length + 2,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                return CAHUTE_ERROR_CORRUPT;
            }

            format = state->picture_format;
            is_blit = 1;
            blit_x =
                ((cahute_ascii_hex_to_nibble(buf[14]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[15]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[16]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[17]));
            blit_y =
                ((cahute_ascii_hex_to_nibble(buf[18]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[19]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[20]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[21]));
            width =
                ((cahute_ascii_hex_to_nibble(buf[22]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[23]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[24]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[25]));
            height =
                ((cahute_ascii_hex_to_nibble(buf[26]) << 12)
                 | (cahute_ascii_hex_to_nibble(buf[27]) << 8)
                 | (cahute_ascii_hex_to_nibble(buf[28]) << 4)
                 | cahute_ascii_hex_to_nibble(buf[29]));
        } else {
            msg(link->context, ll_error, "The following subtype was unknown:");
            mem(link->context, ll_error, &buf[1], 5);
            msg(link->context,
                ll_error,
                "The format and length could not be determined.");
            msg(link->context, ll_error, "This will likely break the link.");
        }

        /* If we are to blit the obtained data, we want to copy the received
         * data to the link data buffer, then call a function later to
         * blit the result. */
        if (is_blit) {
            data_buf = link->data_buffer;
            data_capacity = link->data_buffer_capacity;
            data_sizep = &link->data_buffer_size;
        }

        /* We now have the following data:
         * - A format (that may be undefined, by being < 0).
         * - A frame length (that may be undefined, by being set to 0).
         * - A width and a height (that is unset if any of the two properties
         *   above is undefined).
         *
         * We must do the following checks to ensure that we can store the
         * packet correctly for later processing:
         * - The frame length is known. Otherwise, at least skip the checksum
         *   and return a CAHUTE_ERROR_UNKNOWN.
         * - The format is known. Otherwise, skip the frame length and the
         *   checksum and return a CAHUTE_ERROR_UNKNOWN.
         * - The size matches one the one we expect from a frame with
         *   the provided format and dimensions. Otherwise, skip the frame
         *   length and the checksum and return a CAHUTE_ERROR_UNKNOWN.
         * - The frame length fits within our data buffer. Otherwise,
         *   skip the frame length and the checksum and return a
         *   CAHUTE_ERROR_UNKNOWN.
         *
         * We can then read the frame data into the data buffer and
         * compute our checksum. If it does not match the checksum present
         * at the end of the frame, we return a CAHUTE_ERROR_CORRUPT. */

        if (!frame_length) {
            /* The message has likely already been displayed here, we don't
             * need to print another one. */
            err = cahute_receive_on_link_transport(
                link,
                NULL,
                2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (format < 0) {
            /* Same as above, the message has likely already been displayed. */
            err = cahute_receive_on_link_transport(
                link,
                NULL,
                frame_length + 2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (format == CAHUTE_PICTURE_FORMAT_1BIT_MONO)
            expected_size = ((width >> 3) + !!(width & 7)) * height;
        else if (format == CAHUTE_PICTURE_FORMAT_1BIT_DUAL)
            expected_size = ((width >> 3) + !!(width & 7)) * height * 2;
        else if (format == CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED) {
            expected_size = width * height;
            expected_size = (expected_size >> 1) + (expected_size & 1);
        } else if (format == CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5)
            expected_size = width * height * 2;
        else {
            /* This may be an implementation oversight, it's targeted towards
             * contributors to this function / protocol :-) */
            msg(link->context, ll_debug, "Picture type is: %d", format);
            CAHUTE_RETURN_IMPL(
                link->context,
                "No size estimation method for found format."
            );
        }

        if (expected_size != frame_length) {
            msg(link->context,
                ll_error,
                "Frame length %" CAHUTE_PRIuSIZE
                "o did not match expected "
                "size %" CAHUTE_PRIuSIZE "o for a %dx%d picture (format: %d).",
                frame_length,
                expected_size,
                width,
                height,
                format);

            err = cahute_receive_on_link_transport(
                link,
                NULL,
                frame_length + 2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (frame_length > data_capacity) {
            msg(link->context,
                ll_warn,
                "Frame length %" CAHUTE_PRIuSIZE
                "o exceeded data buffer "
                "capacity %" CAHUTE_PRIuSIZE "o.",
                frame_length,
                data_capacity);

            /* We still want to skip the frame length and the
             * checksum in order to fall back on our feet on next
             * packet reception. */
            err = cahute_receive_on_link_transport(
                link,
                NULL,
                frame_length + 2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_SIZE;
        }

        /* We are now able to read the data from the link to the protocol
         * buffer! */
        err = cahute_receive_on_link_transport(
            link,
            data_buf,
            frame_length,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        /* Store the frame length for checksum computation. */
        *data_sizep = frame_length;

        /* Apply the image.
         * NOTE: We apply the image even if the checksum is incorrect, we just
         * don't report it as complete. */
        if (!is_blit) {
            state->picture_width = width;
            state->picture_height = height;
            state->picture_format = format;
        } else {
            /* We want to copy the obtained image at (blit_x, blit_y) on the
             * existing image. */
            if (!state->picture_format) {
                msg(link->context,
                    ll_error,
                    "Cannot blit on no previous image.");
                return CAHUTE_ERROR_CORRUPT;
            }

            msg(link->context,
                ll_info,
                "Blitting %dx%d image to y=%d, x=%d.",
                width,
                height,
                blit_y,
                blit_x);

            err = cahute_blit_picture(
                link->context,
                state->picture_buf,
                state->picture_format,
                state->picture_width,
                state->picture_height,
                data_buf,
                format,
                width,
                height,
                blit_y,
                blit_x
            );
            if (err)
                return err;
        }
    } else {
        msg(link->context,
            ll_error,
            "Unknown packet type %d (0x%02X).",
            buf[0],
            buf[0]);

        /* Skip the checksum. */
        err = cahute_receive_on_link_transport(
            link,
            NULL,
            2,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(link->context, ll_debug, "Received the following packet header:");
    mem(link->context, ll_debug, buf, packet_size);

    /* We can now compute the checksum.
     * Note that adding checksums works, i.e.
     * checksum(A) + checksum(B) == checksum(AB). */
    err = cahute_receive_on_link_transport(
        link,
        &buf[packet_size],
        2,
        TIMEOUT_PACKET_CONTENTS,
        TIMEOUT_PACKET_CONTENTS
    );
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        return CAHUTE_ERROR_TIMEOUT;
    if (err)
        return err;

    if (!cahute_is_ascii_hex(buf[packet_size])
        || !cahute_is_ascii_hex(buf[packet_size + 1]))
        return CAHUTE_ERROR_CORRUPT;

    {
        unsigned int obtained_checksum =
            ((cahute_ascii_hex_to_nibble(buf[packet_size]) << 4)
             | cahute_ascii_hex_to_nibble(buf[packet_size + 1]));
        unsigned int computed_checksum =
            cahute_checksub(&buf[1], packet_size - 1);

        if (*data_sizep) {
            computed_checksum += cahute_checksub(data_buf, *data_sizep);
            computed_checksum &= 255;
        }

        if (obtained_checksum != computed_checksum) {
            msg(link->context,
                ll_error,
                "Obtained checksum 0x%02X does not match computed "
                "checksum 0x%02X.",
                obtained_checksum,
                computed_checksum);

            return CAHUTE_ERROR_CORRUPT;
        }
    }

    return CAHUTE_OK;
}
