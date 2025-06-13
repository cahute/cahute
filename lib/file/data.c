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

/**
 * Get data from a CASIOLINK main memory archive.
 *
 * @param file File object.
 * @param final_datap Pointer to the data to create with the read data.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_get_data_from_casiolink_file(
    cahute_file *file,
    cahute_data **final_datap
) {
    unsigned long offset = 0, file_size = 0;
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    int err;

    err = cahute_get_file_size(file, &file_size);
    if (err)
        return err;

    while (offset < file_size) {
        err = cahute_casiolink_decode_data(datap, file, &offset);
        if (err && err != CAHUTE_ERROR_IMPL)
            goto fail;

        while (*datap)
            datap = &(*datap)->cahute_data_next;
    }

    if (data) {
        *datap = *final_datap;
        *final_datap = data;
    }

    return CAHUTE_OK;

fail:
    cahute_destroy_data(data);
    return err;
}

/**
 * Get data from a standard main memory archive.
 *
 * @param file File object.
 * @param final_datap Pointer to the data to create with the read data.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_get_data_from_mainmem_file(
    cahute_file *file,
    cahute_data **final_datap
) {
    cahute_u8 header[32];
    cahute_u8 group_header[20];
    cahute_u8 file_header[24];
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    unsigned long count, group_count = 0, offset = 0;
    int err;

    err = cahute_read_from_file(file, 0, header, sizeof(header));
    if (err)
        goto fail;

    offset = 32;
    count = ((~header[30] & 255) << 8) | (~header[31] & 255);
    while (count) {
        err = cahute_read_from_file(
            file,
            offset,
            group_header,
            sizeof(group_header)
        );
        if (err)
            goto fail;

        offset += sizeof(group_header);
        group_count = (group_header[16] << 24) | (group_header[17] << 16)
                      | (group_header[18] << 8) | group_header[19];

        msg(file->context,
            ll_debug,
            "(0x%04lX) Group header:",
            offset - sizeof(group_header));
        mem(file->context, ll_debug, group_header, sizeof(group_header));

        for (; group_count; group_count--) {
            unsigned long data_size;

            err = cahute_read_from_file(
                file,
                offset,
                file_header,
                sizeof(file_header)
            );
            if (err)
                goto fail;

            offset += sizeof(file_header);
            data_size = (file_header[17] << 24) | (file_header[18] << 16)
                        | (file_header[19] << 8) | file_header[20];

            msg(file->context, ll_debug, "File header:");
            mem(file->context, ll_debug, file_header, sizeof(file_header));
            msg(file->context,
                ll_debug,
                "  Data size: %" CAHUTE_PRIuSIZE,
                data_size);

            err = cahute_mcs_decode_data(
                file->context,
                datap,
                group_header,
                16,
                file_header,
                8,
                &file_header[8],
                8,
                file,
                offset,
                data_size,
                file_header[16]
            );
            offset += data_size;
            count--;

            if (err == CAHUTE_ERROR_IMPL)
                continue;
            else if (err)
                goto fail;

            while (*datap)
                datap = &(*datap)->cahute_data_next;
        }
    }

    if (data) {
        *datap = *final_datap;
        *final_datap = data;
    }

    return CAHUTE_OK;

fail:
    cahute_destroy_data(data);
    return err;
}

/**
 * Get data from the file.
 *
 * @param file File object.
 * @param datap Pointer to the data to create with the read data.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_get_data_from_file(cahute_file *file, cahute_data **datap) {
    cahute_data *data = NULL;
    int err;

    EXAMINE(file);

    switch (file->type) {
    case CAHUTE_FILE_TYPE_CASIOLINK:
        err = cahute_get_data_from_casiolink_file(file, &data);
        if (err)
            goto fail;

        break;

    case CAHUTE_FILE_TYPE_MAINMEM:
        err = cahute_get_data_from_mainmem_file(file, &data);
        if (err)
            goto fail;

        break;

    default:
        msg(file->context,
            ll_error,
            "Invalid file type 0x%02X for extracting data from the file.",
            file->type);
        return CAHUTE_ERROR_INVALID;
    }

    *datap = data;
    return CAHUTE_OK;

fail:
    cahute_destroy_data(data);
    return err;
}
