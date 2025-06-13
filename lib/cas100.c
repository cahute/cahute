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

#define PACKET_TYPE_ACK       0x06
#define PACKET_TYPE_CORRUPTED 0x2B
#define PACKET_TYPE_HEADER    0x3A

/* The MDL1 command for Graph 100 / AFX used for initialization when
 * in sender / control mode. The speed and parity are inserted into the
 * copy of this buffer before the checksum is recomputed and placed into
 * the last byte. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
default_mdl1_payload =
    (cahute_u8 const *)":MDL1GY351\xFF" "000000N1.03\0\0\x01\0\0\0\x04\0\0\0"
    "\x01\0\x03\xFF\xFF\xFF\xFF\0";

/**
 * Send CAS100 model information.
 *
 * @param link Link on which to send CAS100 model information.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int) cahute_cas100_send_model_information(cahute_link *link) {
    cahute_u8 buf[40];
    char serial_params[7];

    /* NOTE: sprintf() adds a terminating zero, but we don't care,
     * since we do not copy serial_params[6] afterwards. */
    sprintf(serial_params, "%06lu", link->transport_serial_speed);
    switch (link->transport_serial_flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        serial_params[6] = 'E';
        break;

    case CAHUTE_SERIAL_PARITY_ODD:
        serial_params[6] = 'O';
        break;

    default:
        serial_params[6] = 'N';
    }

    memcpy(buf, default_mdl1_payload, 40);
    memcpy(&buf[11], serial_params, 7);

    buf[39] = cahute_checksub(&buf[1], 38);

    return cahute_send_on_link_transport(link, buf, 40);
}

/**
 * Expect an MDL1 header, or react to a received one.
 *
 * This can be called in two contexts:
 *
 * - We're a CAS100 receiver, have just done the CASIOLINK initiation flow,
 *   and are now expecting a MDL1 header.
 * - We're a generic receiver, and while receiving data, we have just received
 *   an MDL1 header we want to react to.
 *
 * Note that we actually answer with the same MDL1 to make the calculator
 * believe we are compatible with them.
 *
 * @param link Link for which to handle the exchange.
 * @param header MDL1 header to react to.
 *        In the first context, this is expected to be set to NULL.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_cas100_handle_mdl1(cahute_link *link, cahute_u8 const *header) {
    cahute_u8 buf[40];
    int byte, err;

    if (!header) {
        err = cahute_casiolink_receive_packet(
            link,
            buf,
            38,
            PACKET_TYPE_HEADER,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;

        header = buf;
        msg(link->context, ll_debug, "Received the following header:");
        mem(link->context, ll_debug, header, 40);
    }

    if (memcmp(header, "\x3AMDL1", 5)) {
        msg(link->context,
            ll_error,
            "Did not receive an MDL1 header as expected.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We want to store the provided information. */
    memcpy(
        link->protocol_state.casiolink.raw_device_info,
        &header[5],
        CAS100_RAW_DEVICE_INFO_SIZE
    );
    link->protocol_state.casiolink.flags |=
        CASIOLINK_FLAG_DEVICE_INFO_OBTAINED;

    /* Send the MDL1 answer now. */
    err = cahute_send_on_link_transport(link, header, 40);
    if (err)
        return err;

    /* We should actually be receiving an acknowledgement, since we are
     * sending the same packet the calculator sent. */
    err = cahute_receive_byte_on_link_transport(link, &byte, 0);
    if (err)
        return err;

    if (byte != PACKET_TYPE_ACK) {
        err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_CORRUPTED);
        if (err)
            return err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We can now send an acknowledgement.
     * The acknowledgement is already in our buffer, we can use that. */
    err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_ACK);
    if (err)
        return err;

    return CAHUTE_OK;
}

/**
 * Determine the data description for a provided CAS100 header.
 *
 * @param context Context in which the function is run.
 * @param data CAS100 header (40B).
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_cas100_determine_data_description(
    cahute_context *context,
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_HEADER;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    msg(context, ll_debug, "Raw CAS100 header is the following:");
    mem(context, ll_debug, data, 40);

    if (!memcmp(&data[1], "BKU1", 4)) {
        /* Backup packet for CAS100. */
        desc->part_sizes[0] = ((size_t)data[9] << 24)
                              | ((size_t)data[10] << 16)
                              | ((size_t)data[11] << 8) | data[12];
    } else if (!memcmp(&data[1], "END1", 4)) {
        /* End packet for CAS100. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "MCS1", 4)) {
        /* Main memory packet for CAS100. */
        desc->part_sizes[0] = ((size_t)data[8] << 8) | data[9];
        if (!desc->part_sizes[0])
            desc->part_count = 0;
    } else if (!memcmp(&data[1], "MDL1", 4)) {
        /* Initialization packet for CAS100. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_MDL;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "SET1", 4)) {
        /* TODO */
        desc->part_count = 0;
    }

    if (desc->part_count && !desc->part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        msg(context, ll_error, "Could not determine a data description.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/**
 * Decode CAS100 data, with the headers already provided and without checks.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offsetp Pointer to the offset in the file to read from, to set to
 *        the offset after the data afterwards.
 * @param header Pointer to the read header.
 * @param desc Data description to exploit.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_cas100_decode_data_direct(
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
     * - ``header_buf``, of 40 or 50 bytes as documented in ``header_size``.
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
    if (!memcmp(&header[1], "MCS1", 4)) {
        size_t size = ((size_t)header[8] << 8) | header[9];

        err = cahute_mcs_decode_data(
            file->context,
            datap,
            &header[19],
            8,
            NULL, /* MCS1 packet does not present a directory. */
            0,
            &header[11],
            8,
            file,
            offset + 1,
            size,
            header[10]
        );
        if (err && err != CAHUTE_ERROR_IMPL)
            goto fail;

        goto data_ready;
    }

    msg(file->context, ll_error, "Unhandled data with the following header:");
    mem(file->context, ll_error, header, 40);

fail:
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
 * Receive raw CAS100 data, at start or after the header.
 *
 * @param link Link.
 * @param header Buffer containing the header for the CAS100 data.
 *        NULL if the header has not been received yet.
 * @param timeout Timeout in milliseconds for the first data.
 * @param desc Data description to fill and use.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_cas100_receive_raw_data(
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
                PACKET_TYPE_HEADER,
                timeout
            );
            if (err)
                return err;
        }

        err = cahute_cas100_determine_data_description(
            link->context,
            data,
            desc
        );
        if (err)
            return err;

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_END) {
            msg(link->context, ll_debug, "CAS100 data type is an END packet.");
            link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
            return CAHUTE_ERROR_TERMINATED;
        } else if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_MDL) {
            err = cahute_cas100_handle_mdl1(link, data);
            if (err)
                return err;

            continue;
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
 * Receive CAS100 data.
 *
 * @param link Link.
 * @param datap Pointer to the data to create.
 * @param header Buffer containing the initial CAS100 header.
 *        This can be provided as NULL if the header has not been read yet.
 * @param timeout Timeout for the header, if not read yet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_EXTERN(int)
cahute_cas100_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
) {
    struct cahute_casiolink_data_description desc;
    cahute_file memory_file;
    int err;

    err = cahute_cas100_receive_raw_data(link, header, timeout, &desc);
    if (err)
        return err;

    cahute_populate_file_from_memory(
        &memory_file,
        link->context,
        link->data_buffer,
        link->data_buffer_size
    );

    return cahute_cas100_decode_data_direct(
        datap,
        &memory_file,
        0,
        link->data_buffer,
        &desc
    );
}

/**
 * Terminate the connection, for the CAS100 protocol.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_cas100_terminate(cahute_link *link) {
    cahute_u8 buf[40];
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    memset(buf, 0xFF, 40);
    buf[0] = ':';
    buf[1] = 'E';
    buf[2] = 'N';
    buf[3] = 'D';
    buf[4] = '1';
    buf[39] = cahute_checksub(&buf[1], 38);

    msg(link->context, ll_debug, "Sending the following end packet:");
    mem(link->context, ll_debug, buf, 40);

    err = cahute_send_on_link_transport(link, buf, 40);
    if (err)
        return err;

    link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
    return CAHUTE_OK;
}

/**
 * Exchange model information, as a CAS100 sender.
 *
 * @param link Link for which to initiate the connection as a receiver.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_cas100_exchange_model_information(cahute_link *link) {
    cahute_u8 *buf = link->data_buffer;
    int byte, err;

    err = cahute_cas100_send_model_information(link);
    if (err)
        return err;

    err = cahute_casiolink_receive_packet(
        link,
        buf,
        38,
        PACKET_TYPE_HEADER,
        CASIOLINK_TIMEOUT_PACKET_CONTENTS
    );
    if (err)
        return err;

    msg(link->context, ll_debug, "Received the following header:");
    mem(link->context, ll_debug, buf, 40);

    if (memcmp(buf, "\x3AMDL1", 5)) {
        int sub_err;

        msg(link->context,
            ll_error,
            "Did not receive an MDL1 header as expected.");
        sub_err =
            cahute_send_byte_on_link_transport(link, PACKET_TYPE_CORRUPTED);
        if (sub_err)
            return sub_err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We want to store the provided information. */
    memcpy(
        link->protocol_state.casiolink.raw_device_info,
        &buf[5],
        CAS100_RAW_DEVICE_INFO_SIZE
    );
    link->protocol_state.casiolink.flags |=
        CASIOLINK_FLAG_DEVICE_INFO_OBTAINED;

    /* Send the acknowledgement. */
    err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_ACK);
    if (err)
        return err;

    /* Receive the acknowledgement. */
    err = cahute_receive_byte_on_link_transport(link, &byte, 0);
    if (err)
        return err;

    if (byte != PACKET_TYPE_ACK)
        return CAHUTE_ERROR_UNKNOWN;

    return CAHUTE_OK;
}

/**
 * Produce generic device information using CAS100 device information.
 *
 * @param infop Pointer to set to the allocated device information structure.
 * @param raw_info Raw information to read from, expected to be 33 bytes long.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_cas100_make_device_info(
    cahute_device_info **infop,
    cahute_u8 const *raw_info
) {
    cahute_device_info *info;
    char *buf;

    info = malloc(sizeof(cahute_device_info) + 20);
    if (!info)
        return CAHUTE_ERROR_ALLOC;

    buf = (void *)(&info[1]);

    info->cahute_device_info_flags = CAHUTE_DEVICE_INFO_FLAG_OS;
    info->cahute_device_info_rom_capacity = 0;
    info->cahute_device_info_rom_version = "";

    info->cahute_device_info_flash_rom_capacity =
        ((unsigned long)raw_info[20] << 24)
        | ((unsigned long)raw_info[19] << 16)
        | ((unsigned long)raw_info[18] << 8) | raw_info[17];
    info->cahute_device_info_ram_capacity =
        ((unsigned long)raw_info[24] << 24)
        | ((unsigned long)raw_info[23] << 16)
        | ((unsigned long)raw_info[22] << 8) | raw_info[21];

    info->cahute_device_info_bootcode_version = "";
    info->cahute_device_info_bootcode_offset = 0;
    info->cahute_device_info_bootcode_size = 0;

    memcpy(buf, &raw_info[13], 4);
    buf[4] = 0;
    info->cahute_device_info_os_version = buf;
    buf += 5;

    info->cahute_device_info_os_offset = 0;
    info->cahute_device_info_os_size = 0;

    info->cahute_device_info_product_id = "";
    info->cahute_device_info_username = "";
    info->cahute_device_info_organisation = "";

    memcpy(buf, raw_info, 6);
    buf[6] = 0;
    info->cahute_device_info_hwid = buf;
    buf += 7;

    info->cahute_device_info_cpuid = "";

    *infop = info;
    return CAHUTE_OK;
}
