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

#define PACKET_TYPE_DATA 0x3A

/* CAS50 end packet. */
CAHUTE_LOCAL_DATA(cahute_u8 const)
end_packet[] = {
    PACKET_TYPE_DATA,
    0x45,
    0x4E,
    0x44,
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
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x4C
};

/**
 * Determine the data description for a provided CAS50 header.
 *
 * @param data CAS50 header (50B).
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_cas50_determine_data_description(
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_DATA;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    msg(ll_info, "Raw CAS50 header is the following:");
    mem(ll_info, data, 50);

    if (!memcmp(&data[1], "END\xFF", 4)) {
        /* End packet for CAS50. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "VAL", 4)) {
        unsigned int height = ((unsigned int)data[7] << 8) | data[8];
        unsigned int width = ((unsigned int)data[9] << 8) | data[10];

        /* Variable data use size as W*H, or only W, or only H depending
         * on the case. */
        if (!width)
            width = 1;

        desc->part_sizes[0] = 14;
        desc->last_part_repeat = height * width;
    } else {
        /* For other packets, the size should always be located at
         * offset 6 of the header, i.e. offset 7 of the buffer. */
        desc->part_sizes[0] = ((size_t)data[7] << 24) | ((size_t)data[8] << 16)
                              | ((size_t)data[9] << 8) | data[10];

        if (desc->part_sizes[0] > 2)
            desc->part_sizes[0] -= 2;
        else
            desc->part_count = 0;

        if (!memcmp(&data[1], "MEM\0BU", 6)) {
            /* Backups are guaranteed to be the final (and only) file
             * sent in the communication. */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        }
    }

    if (desc->part_count && !desc->part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        msg(ll_error, "Could not determine a data description.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    cahute_casiolink_log_data_description(desc);
    return CAHUTE_OK;
}

/**
 * Decode CAS50 data, with the headers already provided and without checks.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offset Offset in the file at which the header is located.
 * @param header Pointer to the read header.
 * @param desc Data description to exploit.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_cas50_decode_data_direct(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long offset,
    cahute_u8 const *header,
    struct cahute_casiolink_data_description *desc
) {
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    int err = CAHUTE_ERROR_IMPL;

    /* The file type decoding can count on the following variables to be
     * available and set:
     *
     * - ``variant`` and ``desc``, if need be.
     * - ``header``, of 50 bytes as documented in ``header_size``.
     * - ``file`` and ``offset``, the offset being set to the offset right
     *   after the complete header (i.e. at the first data part, if there are
     *   some).
     *
     * From here, either the file type decoding goes along or goes to "fail",
     * which assumes "err" to be set, or it goes to "data_ready", with
     * the following variables expected to be set:
     *
     * - ``datap`` to the pointer where to set the next data, or to one of
     *   the data that could lead to the last pointer by going through the
     *   chain;
     * - ``data`` to the pointer to the first data read, to set as the
     *   result of the function. */
    if (!memcmp(&header[1], "TXT", 4)) {
        size_t data_size = ((size_t)header[7] << 24)
                           | ((size_t)header[8] << 16)
                           | ((size_t)header[9] << 8) | header[10];

        if (data_size >= 2)
            data_size -= 2;

        if (!memcmp(&header[5], "PG", 2)) {
            err = cahute_create_program_from_file(
                datap,
                CAHUTE_TEXT_ENCODING_LEGACY_8,
                &header[11],
                8,
                &header[27],
                8,
                file,
                offset + 51,
                data_size
            );

            if (err)
                goto fail;

            goto data_ready;
        }
    }

fail:
    msg(ll_error,
        "Failed to decode data: %s (%d)",
        cahute_get_error_name(err),
        err);

    cahute_destroy_data(data);
    return err;

data_ready:
    while (*datap)
        datap = &(*datap)->cahute_data_next;

    /* NOTE: ``*offsetp`` was updated earlier, to be correctly set even in
     * the case of invalid or unsupported data types. */
    *datap = *final_datap;
    *final_datap = data;

    return CAHUTE_OK;
}

/**
 * Decode CAS50 data from a file.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offsetp Pointer to the offset in the file to read from, to set to
 *        the offset after the data afterwards.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_cas50_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp
) {
    cahute_u8 header_buf[50];
    cahute_casiolink_data_description desc = {0};
    unsigned long offset = *offsetp;
    unsigned int obtained_checksum;
    unsigned int expected_checksum;
    int err;

    /* Read the header from the buffer, and extract the variant if need be. */
    err = cahute_read_from_file(file, offset, header_buf, 50);
    if (err)
        return err;

    /* Only check the header if not already checked, i.e. in the case of files
     * and not in the case of links. */
    if (header_buf[0] != PACKET_TYPE_DATA) {
        msg(ll_error,
            "Header type 0x%02X is not the expected 0x%02X.",
            header_buf[0],
            PACKET_TYPE_DATA);
        return CAHUTE_ERROR_CORRUPT;
    }

    obtained_checksum = header_buf[49];
    expected_checksum = cahute_checksub(&header_buf[1], 48);
    if (obtained_checksum != expected_checksum) {
        msg(ll_error,
            "Header checksum 0x%02X is different from expected checksum "
            "%02X.",
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
    err = cahute_cas50_determine_data_description(header_buf, &desc);
    if (err)
        return err;

    *offsetp =
        offset + 50 + cahute_casiolink_compute_data_description_size(&desc);

    /* Only check data if not already checked, i.e. in the case of files and
     * not in the case of links (where all data is not available in one go,
     * i.e. the receiver must determine the data description, check the packet
     * type and checksums and acknowledge every part of the data). */
    err = cahute_casiolink_check_file_data(file, offset + 50, &desc);
    if (err)
        return err;

    return cahute_cas50_decode_data_direct(
        datap,
        file,
        offset,
        header_buf,
        &desc
    );
}

/**
 * Receive raw CAS50 data, at start or after the header.
 *
 * @param link Link.
 * @param header Buffer containing the header for the CAS40 data.
 *        NULL if the header has not been received yet.
 * @param timeout Timeout in milliseconds for the first data.
 * @param desc Data description to fill and use.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_cas50_receive_raw_data(
    cahute_link *link,
    cahute_u8 const *header,
    unsigned long timeout,
    struct cahute_casiolink_data_description *desc
) {
    cahute_u8 *data = link->data_buffer;
    size_t data_capacity = link->data_buffer_capacity;
    int err;

    if (data_capacity < 50) {
        msg(ll_error, "Data capacity was expected to be at least 50 bytes.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (header)
        memcpy(data, header, 50);
    else {
        err = cahute_casiolink_receive_packet(
            link,
            data,
            48,
            PACKET_TYPE_DATA,
            timeout
        );
        if (err)
            return err;
    }

    err = cahute_cas50_determine_data_description(data, desc);
    if (err)
        return err;

    if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_END) {
        msg(ll_info, "CAS50 data type is an END packet.");
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        return CAHUTE_ERROR_TERMINATED;
    } else if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_FINAL) {
        msg(ll_info, "CAS50 data type is final.");
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
    }

    data_capacity -= 50;
    err = cahute_casiolink_receive_raw_data(
        link,
        desc,
        &data[50],
        &data_capacity
    );
    link->data_buffer_size = 50 + data_capacity;

    return err;
}

/**
 * Receive CAS50 data.
 *
 * @param link Link.
 * @param datap Pointer to the data to create.
 * @param header Buffer containing the initial CAS40 header.
 *        This can be provided as NULL if the header has not been read yet.
 * @param timeout Timeout for the header, if not read yet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_EXTERN(int)
cahute_cas50_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
) {
    struct cahute_casiolink_data_description desc = {0};
    cahute_file memory_file;
    int err;

    for (;; header = NULL) {
        err = cahute_cas50_receive_raw_data(link, header, timeout, &desc);
        if (err)
            return err;

        cahute_populate_file_from_memory(
            &memory_file,
            link->data_buffer,
            link->data_buffer_size
        );

        err = cahute_cas50_decode_data_direct(
            datap,
            &memory_file,
            0,
            link->data_buffer,
            &desc
        );
        if (err && err != CAHUTE_ERROR_IMPL)
            break;

        /* Data may have been final (e.g. in case of backup), we want to stop
         * the communication in that case. */
        if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
            return CAHUTE_ERROR_TERMINATED;
    }

    return err;
}

/* ---
 * Send and control.
 * --- */

/**
 * Terminate the connection, for the CAS50 protocol.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_cas50_terminate(cahute_link *link) {
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    msg(ll_info, "Sending the following end packet:");
    mem(ll_info, end_packet, sizeof(end_packet));

    err = cahute_send_on_link_medium(
        &link->medium,
        end_packet,
        sizeof(end_packet)
    );
    if (err)
        return err;

    link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
    return CAHUTE_OK;
}
