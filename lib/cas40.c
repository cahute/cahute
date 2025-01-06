/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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

#define PACKET_TYPE_ACK  0x06
#define PACKET_TYPE_DATA 0x3A

/* CAS40 end packet. */
CAHUTE_LOCAL_DATA(cahute_u8)
end_packet[] = {
    PACKET_TYPE_DATA,
    0x17,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x0E,
};

/* 1-character program names for the PZ CAS40 data.
 * \xCD is ro and \xCE is theta. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
pz_program_names =
    (cahute_u8 const *)"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\xCD\xCE";

/**
 * Determine the data description for a provided CAS40 header.
 *
 * @param context Context in which the function is run.
 * @param data CAS40 header (40B).
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_cas40_determine_data_description(
    cahute_context *context,
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_DATA;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    msg(context, ll_info, "Raw CAS40 header is the following:");
    mem(context, ll_info, data, 40);

    if (!memcmp(&data[1], "\x17\x17", 2)) {
        /* CAS40 AL End */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_AL_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "\x17\xFF", 2)) {
        /* CAS40 End */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "A1", 2)) {
        /* CAS40 Dynamic Graph */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] > 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "AA", 2)) {
        /* CAS40 Dynamic Graph in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] > 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "AD", 2)) {
        /* CAS40 All Memories */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    } else if (!memcmp(&data[1], "AL", 2)) {
        /* CAS40 All */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_AL;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "AM", 2)) {
        /* CAS40 Variable Memories */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    } else if (!memcmp(&data[1], "BU", 2)) {
        /* CAS40 Backup */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        if (!memcmp(&data[3], "TYPEA00", 7) || !memcmp(&data[3], "TYPEA02", 7))
            desc->part_sizes[0] = 32768;
    } else if (!memcmp(&data[1], "DC", 2)) {
        /* CAS40 Color Screenshot. */
        unsigned int width = data[3], height = data[4];

        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL
                       | CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG;
        if (!memcmp(&data[5], "\x11UWF\x03", 4)) {
            desc->last_part_repeat = 3;
            desc->part_sizes[0] = 1 + ((width >> 3) + !!(width & 7)) * height;
        }
    } else if (!memcmp(&data[1], "DD", 2)) {
        /* CAS40 Monochrome Screenshot. */
        unsigned int width = data[3], height = data[4];

        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL
                       | CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG;
        if (!memcmp(&data[5], "\x10\x44WF", 4))
            desc->part_sizes[0] = ((width >> 3) + !!(width & 7)) * height;
    } else if (!memcmp(&data[1], "DM", 2)) {
        /* CAS40 Defined Memories */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    } else if (!memcmp(&data[1], "EN", 2)) {
        /* CAS40 Single Editor Program */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "EP", 2)) {
        /* CAS40 Single Password Protected Editor Program */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "F1", 2)) {
        /* CAS40 Single Function */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "F6", 2)) {
        /* CAS40 Multiple Functions */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "FN", 2)) {
        /* CAS40 Single Editor Program in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "FP", 2)) {
        /* CAS40 Single Password Protected Editor Program in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "G1", 2)) {
        /* CAS40 Graph Function */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "GA", 2)) {
        /* CAS40 Graph Function in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "GF", 2)) {
        /* CAS40 Factor */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = 2 + data[6] * 10;
    } else if (!memcmp(&data[1], "GR", 2)) {
        /* CAS40 Range */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = 92;
    } else if (!memcmp(&data[1], "GT", 2)) {
        /* CAS40 Function Table */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_count = 3;
        desc->last_part_repeat = ((size_t)data[7] << 8) | data[8];
        desc->part_sizes[0] = data[6];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;

        desc->part_sizes[1] = 32;
        desc->part_sizes[2] = 22;
    } else if (!memcmp(&data[1], "M1", 2)) {
        /* CAS40 Single Matrix */
        unsigned int width = data[5], height = data[6];

        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = 14;
        desc->last_part_repeat = width * height + 1; /* Sentinel data part. */
    } else if (!memcmp(&data[1], "MA", 2)) {
        /* CAS40 Single Matrix in Bulk */
        unsigned int width = data[5], height = data[6];

        desc->flags |= 0;
        desc->part_sizes[0] = 14;
        desc->last_part_repeat = width * height;
    } else if (!memcmp(&data[1], "P1", 2)) {
        /* CAS40 Single Numbered Program. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "PD", 2)) {
        unsigned int deg = data[5] * 10 + data[6];

        /* CAS40 Polynomial Equation */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        if (!deg)
            desc->part_count = 0;
        else if (deg == 2)
            desc->part_sizes[0] = 32;
        else if (deg == 3)
            desc->part_sizes[0] = 42;
    } else if (!memcmp(&data[1], "PZ", 2)) {
        /* CAS40 Multiple Numbered Programs */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_count = 2;
        desc->part_sizes[0] = 190;
        desc->part_sizes[1] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[1] >= 2)
            desc->part_sizes[1] -= 2;
    } else if (!memcmp(&data[1], "RT", 2)) {
        /* CAS40 Recursion Table */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_count = 3;
        desc->last_part_repeat = ((size_t)data[7] << 8) | data[8];
        desc->part_sizes[0] = data[6];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;

        desc->part_sizes[1] = 22;
        desc->part_sizes[2] = 32;
    } else if (!memcmp(&data[1], "SD", 2)) {
        /* CAS40 Simultaneous Equations */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = data[5] * data[6] + 1;
        desc->part_sizes[0] = 14;
    } else if (!memcmp(&data[1], "SR", 2)) {
        /* CAS40 Paired Variable Data */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 32;
    } else if (!memcmp(&data[1], "SS", 2)) {
        /* CAS40 Single Variable Data */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    }

    if (desc->part_count && !desc->part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        msg(context, ll_error, "Could not determine a data description.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    cahute_casiolink_log_data_description(context, desc);
    return CAHUTE_OK;
}

/**
 * Decode CAS40 data, with the headers already provided and without checks.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offset Offset in the file at which the header is located.
 * @param header Pointer to the read header.
 * @param desc Data description to exploit.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_cas40_decode_data_direct(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long offset,
    cahute_u8 const *header,
    struct cahute_casiolink_data_description *desc
) {
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    int err = CAHUTE_ERROR_IMPL;

    if (!memcmp(&header[1], "P1", 2)) {
        size_t program_size = ((size_t)header[4] << 8) | header[5];

        /* CAS40 Single Numbered Program. */
        err = cahute_create_program_from_file(
            datap,
            CAHUTE_TEXT_ENCODING_LEGACY_8,
            NULL, /* No program name, this is anonymous. */
            0,
            NULL, /* No password. */
            0,
            file,
            offset + 1,
            program_size
        );
        if (err)
            goto fail;

        goto data_ready;
    }

    if (!memcmp(&header[1], "PZ", 2)) {
        cahute_u8 programs_header[190];
        cahute_u8 const *buf = programs_header;
        cahute_u8 const *names = pz_program_names;
        int i = 0;

        /* CAS40 Multiple Numbered Programs
         * This is made of 38 programs, with all 5-byte headers placed
         * consecutively in a first data part, then all contents placed
         * consecutively in a second data part. */
        err = cahute_read_from_file(file, offset + 41, programs_header, 190);
        if (err)
            goto fail;

        offset += 233; /* Header, content, 2 packet types, 1 checksum. */
        for (i = 1; i < 39; i++) {
            size_t program_length = ((size_t)buf[1] << 8) | buf[2];

            if (program_length >= 2)
                program_length -= 2;

            err = cahute_create_program_from_file(
                datap,
                CAHUTE_TEXT_ENCODING_LEGACY_8,
                names++,
                1,
                NULL, /* No password. */
                0,
                file,
                offset,
                program_length
            );
            if (err)
                goto fail;

            datap = &(*datap)->cahute_data_next;
            buf += 5;
            offset += program_length;
        }

        goto data_ready;
    }

fail:
    msg(file->context,
        ll_error,
        "Failed to decode data: %s (%d)",
        cahute_get_error_name(err),
        err);

    cahute_destroy_data(data);
    return err;

data_ready:
    while (*datap)
        datap = &(*datap)->cahute_data_next;

    *datap = *final_datap;
    *final_datap = data;

    return CAHUTE_OK;
}

/**
 * Decode CAS40 data from a file.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offsetp Pointer to the offset in the file to read from, to set to
 *        the offset after the data afterwards.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_cas40_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp
) {
    cahute_u8 header_buf[40];
    cahute_casiolink_data_description desc = {0};
    unsigned long offset = *offsetp;
    unsigned int obtained_checksum;
    unsigned int expected_checksum;
    int err;

    /* Read the header. */
    err = cahute_read_from_file(file, offset, header_buf, 40);
    if (err)
        return err;

    if (header_buf[0] != PACKET_TYPE_DATA) {
        msg(file->context,
            ll_error,
            "Header type 0x%02X is not the expected 0x%02X.",
            header_buf[0],
            PACKET_TYPE_DATA);
        return CAHUTE_ERROR_CORRUPT;
    }

    obtained_checksum = header_buf[39];
    expected_checksum = cahute_checksub(&header_buf[1], 38);
    if (obtained_checksum != expected_checksum) {
        msg(file->context,
            ll_error,
            "Invalid checksum in header (obtained: 0x%02X, computed: 0x%02X)",
            obtained_checksum,
            expected_checksum);
        return CAHUTE_ERROR_CORRUPT;
    }

    /* We need to get the data description in order to at least place the
     * offset after the current header and data part, even if it is not
     * implemented, in order for file reading to use CAHUTE_ERROR_IMPL errors
     * to skip unimplemented file types.
     *
     * NOTE: We only update ``*offsetp`` here and NOT ``offset``, because
     * ``offset`` is actually used in data decoding later on in the
     * function. */
    err = cahute_cas40_determine_data_description(
        file->context,
        header_buf,
        &desc
    );
    if (err)
        return err;

    *offsetp =
        offset + 40 + cahute_casiolink_compute_data_description_size(&desc);

    /* Only check data if not already checked, i.e. in the case of files and
     * not in the case of links (where all data is not available in one go,
     * i.e. the receiver must determine the data description, check the packet
     * type and checksums and acknowledge every part of the data). */
    err = cahute_casiolink_check_file_data(file, offset, &desc);
    if (err)
        return err;

    return cahute_cas40_decode_data_direct(
        datap,
        file,
        offset,
        header_buf,
        &desc
    );
}

/**
 * Receive raw CAS40 data, at start or after the header.
 *
 * @param link Link.
 * @param header Buffer containing the header for the CAS40 data.
 *        NULL if the header has not been received yet.
 * @param timeout Timeout in milliseconds for the first data.
 * @param desc Data description to fill and use.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_cas40_receive_raw_data(
    cahute_link *link,
    cahute_u8 const *header,
    unsigned long timeout,
    struct cahute_casiolink_data_description *desc
) {
    cahute_u8 *data = link->data_buffer;
    size_t data_capacity = link->data_buffer_capacity;
    int err;

    if (data_capacity < 40) {
        msg(link->context,
            ll_error,
            "Data capacity was expected to be at least 40 bytes.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    for (;; header = NULL) {
        if (header)
            memcpy(data, header, 40);
        else {
            err = cahute_casiolink_receive_packet(
                link,
                data,
                38,
                PACKET_TYPE_DATA,
                timeout
            );
            if (err)
                return err;
        }

        err =
            cahute_cas40_determine_data_description(link->context, data, desc);
        if (err)
            return err;

        /* We can acknowledge the header. */
        err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_ACK);
        if (err)
            return err;

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_AL) {
            if (link->protocol_state.casiolink.flags
                & CASIOLINK_FLAG_DEVICE_INFO_CAS40_AL) {
                msg(link->context,
                    ll_error,
                    "Calculator sends us an AL header when already in AL "
                    "mode; there are shenanigans happening here.");
                return CAHUTE_ERROR_UNKNOWN;
            }

            msg(link->context, ll_info, "Calculator has started CAS40 AL mode."
            );
            link->protocol_state.casiolink.flags |=
                CASIOLINK_FLAG_DEVICE_INFO_CAS40_AL;
            continue;
        } else if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_AL_END) {
            if (~link->protocol_state.casiolink.flags
                & CASIOLINK_FLAG_DEVICE_INFO_CAS40_AL) {
                msg(link->context,
                    ll_error,
                    "Calculator sends us an AL END header when not already in "
                    "AL mode; there are shenanigans happening here.");
                return CAHUTE_ERROR_UNKNOWN;
            }

            /* The communication is ending. */
            msg(link->context,
                ll_info,
                "Calculator has terminated CAS40 AL mode.");
            link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
            return CAHUTE_ERROR_TERMINATED;
        } else if (~link->protocol_state.casiolink.flags & CASIOLINK_FLAG_DEVICE_INFO_CAS40_AL) {
            /* Final and end data data types are not that while AL mode is
             * active, hence the condition. */
            if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_END) {
                /* The communication is ending. */
                msg(link->context, ll_info, "CAS40 data type is an END packet."
                );
                link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
                return CAHUTE_ERROR_TERMINATED;
            } else if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_FINAL) {
                /* Communication will end after reception of current data.
                 * Note that we still want to receive data. */
                msg(link->context, ll_info, "CAS40 data type is final.");
                link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
            }
        }

        break;
    }

    data_capacity -= 40;
    err = cahute_casiolink_receive_raw_data(
        link,
        desc,
        &data[40],
        &data_capacity
    );
    link->data_buffer_size = 40 + data_capacity;

    return err;
}

/**
 * Receive CAS40 data.
 *
 * @param link Link.
 * @param datap Pointer to the data to create.
 * @param header Buffer containing the initial CAS40 header.
 *        This can be provided as NULL if the header has not been read yet.
 * @param timeout Timeout for the header, if not read yet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_EXTERN(int)
cahute_cas40_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
) {
    struct cahute_casiolink_data_description desc;
    cahute_file memory_file;
    int err;

    for (;; header = NULL) {
        err = cahute_cas40_receive_raw_data(link, header, timeout, &desc);
        if (err)
            break;

        cahute_populate_file_from_memory(
            &memory_file,
            link->context,
            link->data_buffer,
            link->data_buffer_size
        );

        err = cahute_cas40_decode_data_direct(
            datap,
            &memory_file,
            0,
            link->data_buffer,
            &desc
        );
        if (err && err != CAHUTE_ERROR_IMPL)
            break;

        /* Data may have been final, we want to stop the communication
         * in that case. */
        if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
            return CAHUTE_ERROR_TERMINATED;
    }

    return err;
}

/**
 * Receive a frame through screen capture.
 *
 * @param link Link for which to receive screens.
 * @param frame Function to call back.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
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

/* ---
 * Send and control.
 * --- */

/**
 * Terminate the connection, for CAS40 variant.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_cas40_terminate(cahute_link *link) {
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    msg(link->context, ll_info, "Sending the following end packet:");
    mem(link->context, ll_info, end_packet, 40);

    err = cahute_send_on_link_transport(link, end_packet, 40);
    if (err)
        return err;

    link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
    return CAHUTE_OK;
}
