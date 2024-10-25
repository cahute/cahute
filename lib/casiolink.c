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

#include <string.h>
#include "internals.h"

#define TIMEOUT_INIT 500

#define PACKET_TYPE_ACK          0x06
#define PACKET_TYPE_ESTABLISHED  0x13
#define PACKET_TYPE_START        0x16
#define PACKET_TYPE_INVALID_DATA 0x24

/* ---
 * Reception.
 * --- */

#define VARIANT_CAS40  1
#define VARIANT_CAS50  2
#define VARIANT_CAS100 3

/**
 * Read the first 40 bytes of a CASIOLINK header to determine the type.
 *
 * @param data First 40 bytes of the CASIOLINK header, including the 0x3A.
 * @return Variant.
 */
CAHUTE_LOCAL(int)
cahute_casiolink_determine_header_variant(cahute_u8 const *data) {
    /* We want to try to determine the currently selected variant based
     * on the header's content. */
    if (!memcmp(&data[1], "ADN1", 4) || !memcmp(&data[1], "ADN2", 4)
        || !memcmp(&data[1], "BKU1", 4) || !memcmp(&data[1], "END1", 4)
        || !memcmp(&data[1], "FCL1", 4) || !memcmp(&data[1], "FMV1", 4)
        || !memcmp(&data[1], "MCS1", 4) || !memcmp(&data[1], "MDL1", 4)
        || !memcmp(&data[1], "REQ1", 4) || !memcmp(&data[1], "REQ2", 4)
        || !memcmp(&data[1], "SET1", 4)) {
        /* The type seems to be a CAS100 header type we can use. */
        return VARIANT_CAS100;
    }

    if (!memcmp(&data[1], "END\xFF", 4) || !memcmp(&data[1], "FNC", 4)
        || !memcmp(&data[1], "IMG", 4) || !memcmp(&data[1], "MEM", 4)
        || !memcmp(&data[1], "REQ", 4) || !memcmp(&data[1], "TXT", 4)
        || !memcmp(&data[1], "VAL", 4)) {
        /* The type seems to be a CAS50 header type.
         * This means that we actually have 10 more bytes to read for
         * a full header.
         *
         * NOTE: The '4' in the memcmp() calls above are intentional,
         * as the NUL character ('\0) is actually considered as part of
         * the CAS50 header type. */
        return VARIANT_CAS50;
    }

    /* By default, we consider the header to be a CAS40 header. */
    return VARIANT_CAS40;
}

/**
 * Receive the first byte of a CASIOLINK packet, whatever the variant is.
 *
 * This also manages the initialization if need be.
 *
 * @param link Link to receive the first byte on.
 * @param first_bytep Pointer to the first byte to define.
 * @param timeout Timeout, in ms.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_casiolink_receive_first_byte(
    cahute_link *link,
    int *first_bytep,
    unsigned long timeout
) {
    int byte = -1, err;

    do {
        err =
            cahute_receive_byte_on_link_medium(&link->medium, &byte, timeout);
        if (err)
            return err;

        if (byte == 0x16) {
            /* The sender is either re-initializing the connection, or
             * starting the connection and we did not check when creating
             * the link, either way we can just answer here and restart. */
            err = cahute_send_byte_on_link_medium(&link->medium, 0x13);
            if (err)
                return err;

            link->flags &= ~CAHUTE_LINK_FLAG_TERMINATED;
            continue;
        }

        break;
    } while (1);

    *first_bytep = byte;
    return CAHUTE_OK;
}

/**
 * Receive a CASIOLINK packet, whatever the variant is.
 *
 * This is useful for CAS40, CAS50 and CAS100.
 *
 * WARNING: The buffer is expected to have a minimum capacity
 *          of ``size + 2`` bytes.
 *
 * @param link Link to receive the CASIOLINK packet on.
 * @param buf Buffer in which to place the packet.
 * @param size Expected packet size.
 * @param expected_type Expected type of the packet.
 * @param timeout Timeout for the first byte of the packet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_receive_packet(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    int expected_type,
    unsigned long timeout
) {
    unsigned int checksum, checksum_alt;
    int err, first_byte;

    err = cahute_casiolink_receive_first_byte(link, &first_byte, timeout);
    if (err == CAHUTE_ERROR_TIMEOUT_START) {
        msg(ll_error, "Timeout received while reading the packet type.");
        return CAHUTE_ERROR_TIMEOUT;
    } else if (err)
        return err;

    if (first_byte != expected_type) {
        msg(ll_error,
            "Expected 0x%02X packet type, got 0x%02X.",
            expected_type,
            first_byte);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *buf = first_byte;
    if (size) {
        size_t size_left = size + 1;
        cahute_u8 *p = &buf[1];

        while (size_left) {
            size_t to_read = size_left > 512 ? 512 : size_left;

            err = cahute_receive_on_link_medium(
                &link->medium,
                p,
                to_read,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            else if (err)
                return err;

            size_left -= to_read;
            p += to_read;
        }

        /* For color screenshots, sometimes the first byte is not
         * taken into account in the checksum calculation, as it's
         * metadata for the sheet and not the "actual data" of the
         * sheet. But sometimes it also gets the checksum right!
         * In any case, we want to compute and check both checksums
         * to see if at least one matches. */
        checksum = cahute_checksub(&buf[1], size);
        checksum_alt = cahute_checksub(&buf[2], size - 1);
    } else {
        checksum = 0;
        checksum_alt = 0;
    }

    /* Check the checksum. */
    if (checksum != buf[1 + size] && checksum_alt != buf[1 + size]) {
        msg(ll_warn,
            "Invalid checksum (obtained: 0x%02X, computed: "
            "0x%02X).",
            buf[1 + size],
            checksum);
        mem(ll_info, buf, size);

        return CAHUTE_ERROR_CORRUPT;
    }

    return CAHUTE_OK;
}

/**
 * Decode CASIOLINK data from a file.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offsetp Pointer to the offset in the file to read from, to set to
 *        the offset after the data afterwards.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp
) {
    cahute_u8 header_buf[40];
    int err;

    /* Read the header start. */
    msg(ll_info, "Reading new header at offset %lu.", *offsetp);
    err = cahute_read_from_file(file, *offsetp, header_buf, 40);
    if (err)
        return err;

    switch (cahute_casiolink_determine_header_variant(header_buf)) {
    case VARIANT_CAS50:
        return cahute_cas50_decode_data(datap, file, offsetp);

    case VARIANT_CAS40:
        return cahute_cas40_decode_data(datap, file, offsetp);
    }

    CAHUTE_RETURN_IMPL("Cannot decode CAS100 data from a CASIOLINK file.");
}

/**
 * Receive raw data.
 *
 * This is used by the generic CASIOLINK raw data reception function, as well
 * as CAS40, CAS50 and CAS100 raw data reception function.
 *
 * @param link Link to use.
 * @param desc Data description to receive data from.
 * @param buf Buffer to receive raw data into.
 * @param buf_sizep Pointer to the buffer capacity. Set to the buffer size
 *        at the end of the process.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_receive_raw_data(
    cahute_link *link,
    struct cahute_casiolink_data_description const *desc,
    cahute_u8 *buf,
    size_t *buf_sizep
) {
    size_t received = 0;
    size_t i, nparts;
    int err;

    if (!desc->part_count)
        goto end;

    /* Before doing anything, check that the buffer is big enough.
     * Otherwise, we want to send invalid data here. */
    {
        size_t total_size =
            cahute_casiolink_compute_data_description_size(desc);

        if (total_size > *buf_sizep) {
            msg(ll_error,
                "Cannot get %" CAHUTE_PRIuSIZE "o into a %" CAHUTE_PRIuSIZE
                "o data buffer.",
                total_size,
                *buf_sizep);

            /* We actually send like we don't recognize the data, in
             * order not to make the link irrecoverable. */
            err = cahute_send_byte_on_link_medium(
                &link->medium,
                PACKET_TYPE_INVALID_DATA
            );
            if (err)
                return err;

            return CAHUTE_ERROR_SIZE;
        }
    }

    /* We can acknowledge the header so we can actually receive it. */
    err = cahute_send_byte_on_link_medium(&link->medium, PACKET_TYPE_ACK);
    if (err)
        return err;

    nparts = desc->part_count - 1 + desc->last_part_repeat;
    for (i = 0; i < nparts; i++) {
        size_t part_size =
            desc->part_sizes[i >= desc->part_count ? desc->part_count - 1 : i];

        msg(ll_info,
            "Reading data part %d/%d (%" CAHUTE_PRIuSIZE "o).",
            i + 1,
            nparts,
            part_size);

        err = cahute_casiolink_receive_packet(
            link,
            buf,
            part_size,
            desc->packet_type,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_CORRUPT) {
            int sub_err;

            msg(ll_error, "Transfer will abort.");
            link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;

            sub_err = cahute_send_byte_on_link_medium(
                &link->medium,
                PACKET_TYPE_INVALID_DATA
            );
            if (sub_err)
                return sub_err;

            return err;
        } else if (err)
            return err;

        /* Acknowledge the data. */
        err = cahute_send_byte_on_link_medium(&link->medium, PACKET_TYPE_ACK);
        if (err)
            return err;

        msg(ll_info,
            "Data part %d/%d received and acknowledged.",
            i + 1,
            nparts);
        if ((~desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG)
            && part_size <= 4096) /* Let's not flood the terminal. */
            mem(ll_info, buf, part_size);

        buf += part_size + 2;
        received += part_size + 2;
    }

    /* TODO */

end:
    *buf_sizep = received;
    return CAHUTE_OK;
}

/**
 * Receive data.
 *
 * @param link Link to the device.
 * @param datap Data to allocate.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
) {
    cahute_u8 buf[50];
    int byte, err;

    do {
        err = cahute_casiolink_receive_first_byte(link, &byte, timeout);
        if (err)
            return err;

        if (byte == 0x01 || byte == 0x02 || byte == 0x18) {
            err = cahute_cas300_receive_data(link, datap, byte, timeout);
            goto data_decoded;
        }

        if (byte != 0x3A) {
            msg(ll_error, "Unknown packet type 0x%02X.", byte);
            return CAHUTE_ERROR_UNKNOWN;
        }

        buf[0] = byte;
        err = cahute_receive_on_link_medium(
            &link->medium,
            &buf[1],
            39,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;

        switch (cahute_casiolink_determine_header_variant(buf)) {
        case VARIANT_CAS100:
            err = cahute_cas100_receive_data(link, datap, buf, 0);
            break;

        case VARIANT_CAS50:
            err = cahute_receive_on_link_medium(
                &link->medium,
                &buf[40],
                10,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                return err;

            err = cahute_cas50_receive_data(link, datap, buf, 0);
            break;

        default:
            err = cahute_cas40_receive_data(link, datap, buf, 0);
        }

data_decoded:
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_IMPL)
            return err;
    } while (1);

    return CAHUTE_OK;
}

/* ---
 * Send and control functions.
 * --- */

/**
 * Initiate the connection as a receiver, for any CASIOLINK variant.
 *
 * @param link Link for which to initiate the connection as a receiver.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_casiolink_initiate_as_receiver(cahute_link *link) {
    int byte = -1, err;

    while (byte < 0) {
        err = cahute_receive_byte_on_link_medium(&link->medium, &byte, 0);
        if (err)
            return err;
    }

    if (byte != PACKET_TYPE_START) {
        msg(ll_error,
            "Expected START packet (0x%02X), got 0x%02X.",
            PACKET_TYPE_START,
            byte);

        return CAHUTE_ERROR_UNKNOWN;
    }

    err = cahute_send_byte_on_link_medium(
        &link->medium,
        PACKET_TYPE_ESTABLISHED
    );
    if (err)
        return err;

    msg(ll_info, "CASIOLINK initiation successful as receiver!");
    return CAHUTE_OK;
}

/**
 * Initiate the connection as a sender, for any CASIOLINK variant.
 *
 * @param link Link for which to initiate the connection as a sender.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_casiolink_initiate_as_sender(cahute_link *link) {
    cahute_u8 *buf = link->data_buffer;
    int initial_attempts = 6, attempts, err;

    msg(ll_info,
        "Making the initial handshake (%d attempts, %lums for each).",
        initial_attempts,
        TIMEOUT_INIT);
    for (attempts = initial_attempts; attempts > 0; attempts--) {
        buf[0] = PACKET_TYPE_START;
        msg(ll_info, "Sending the following start packet:");
        mem(ll_info, buf, 1);

        err = cahute_send_on_link_medium(&link->medium, buf, 1);
        if (err)
            return err;

        err = cahute_receive_on_link_medium(
            &link->medium,
            buf,
            1,
            TIMEOUT_INIT,
            0
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            continue;

        if (err)
            return err;

        if (buf[0] != PACKET_TYPE_ESTABLISHED) {
            msg(ll_error,
                "Expected ESTABLISHED packet (0x%02X), got 0x%02X.",
                PACKET_TYPE_ESTABLISHED,
                buf[0]);

            return CAHUTE_ERROR_UNKNOWN;
        }

        break;
    }

    if (attempts <= 0) {
        msg(ll_error, "No response after %d attempts.");
        return CAHUTE_ERROR_TIMEOUT_START;
    }

    return CAHUTE_OK;
}
