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
CAHUTE_INTERNAL(int)
cahute_cas100_decode_data_using_description(
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
