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

/* 1-character program names for the PZ CAS40 data.
 * \xCD is ro and \xCE is theta. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
pz_program_names =
    (cahute_u8 const *)"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\xCD\xCE";

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
CAHUTE_INTERNAL(int)
cahute_cas40_decode_data_using_description(
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
