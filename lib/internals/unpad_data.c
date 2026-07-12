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
 * Apply reverse 0x5C padding to source data and write to a destination buffer.
 *
 * This functions reads the original buffer size by using ``*buf_sizep``, and
 * sets ``*buf_sizep`` to the number of actual bytes at the end.
 *
 * @param buf Destination buffer.
 * @param buf_size Maximum capacity in the destination buffer.
 * @param data Source data to apply reverse padding to.
 * @param data_size Size of the source data to apply padding to.
 * @return Error code, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_unpad_data(
    cahute_u8 *buf,
    size_t *buf_sizep,
    cahute_u8 const *data,
    size_t data_size
) {
    cahute_u8 *orig = buf;
    cahute_u8 const *p;
    size_t buf_size = *buf_sizep;

    for (p = data; buf_size && data_size; p++, data_size--, buf_size--) {
        int byte = *p;

        if (byte == '\\') {
            /* If we've arrived at the end, we ignore the char. */
            if (data_size <= 1)
                break;

            byte = *++p;
            data_size--;

            *buf++ = byte == '\\' ? '\\' : byte - 32;
        } else
            *buf++ = byte;
    }

    if (data_size) {
        /* ``data_size`` characters could not be converted because the
         * destination buffer was full.
         * Note that ``*buf_sizep`` does not need to be changed here, because
         * it actually represents both the capacity and the actual used
         * space in the destination buffer, since it's full. */
        return CAHUTE_ERROR_SIZE;
    }

    *buf_sizep = (size_t)(buf - orig);
    return CAHUTE_OK;
}
