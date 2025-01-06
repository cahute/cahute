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
 * Get the name of a given error.
 *
 * @param code Code of the error to give the name for.
 * @return Name of the error.
 */
CAHUTE_EXTERN(char const *) cahute_get_error_name(int code) {
    switch (code) {
    case CAHUTE_OK:
        return "CAHUTE_OK";
    case CAHUTE_ERROR_UNKNOWN:
        return "CAHUTE_ERROR_UNKNOWN";
    case CAHUTE_ERROR_ABORT:
        return "CAHUTE_ERROR_ABORT";
    case CAHUTE_ERROR_IMPL:
        return "CAHUTE_ERROR_IMPL";
    case CAHUTE_ERROR_ALLOC:
        return "CAHUTE_ERROR_ALLOC";
    case CAHUTE_ERROR_PRIV:
        return "CAHUTE_ERROR_PRIV";
    case CAHUTE_ERROR_BUSY:
        return "CAHUTE_ERROR_BUSY";
    case CAHUTE_ERROR_INT:
        return "CAHUTE_ERROR_INT";
    case CAHUTE_ERROR_SIZE:
        return "CAHUTE_ERROR_SIZE";
    case CAHUTE_ERROR_TRUNC:
        return "CAHUTE_ERROR_TRUNC";
    case CAHUTE_ERROR_INVALID:
        return "CAHUTE_ERROR_INVALID";
    case CAHUTE_ERROR_INCOMPAT:
        return "CAHUTE_ERROR_INCOMPAT";
    case CAHUTE_ERROR_TERMINATED:
        return "CAHUTE_ERROR_TERMINATED";
    case CAHUTE_ERROR_NOT_FOUND:
        return "CAHUTE_ERROR_NOT_FOUND";
    case CAHUTE_ERROR_TOO_MANY:
        return "CAHUTE_ERROR_TOO_MANY";
    case CAHUTE_ERROR_GONE:
        return "CAHUTE_ERROR_GONE";
    case CAHUTE_ERROR_TIMEOUT_START:
        return "CAHUTE_ERROR_TIMEOUT_START";
    case CAHUTE_ERROR_TIMEOUT:
        return "CAHUTE_ERROR_TIMEOUT";
    case CAHUTE_ERROR_CORRUPT:
        return "CAHUTE_ERROR_CORRUPT";
    case CAHUTE_ERROR_IRRECOV:
        return "CAHUTE_ERROR_IRRECOV";
    case CAHUTE_ERROR_NOOW:
        return "CAHUTE_ERROR_NOOW";
    default:
        return "(unknown)";
    }
}

/**
 * Apply 0x5C padding to source data and write to a destination buffer.
 *
 * SECURITY: The destination buffer is assumed to have at least data_size*2
 * bytes available. Assertions regarding the data size must be done in the
 * caller.
 *
 * @param buf Destination buffer.
 * @param data Source data to apply padding to.
 * @param data_size Size of the source data to apply padding to.
 * @return Size of the unpadded data.
 */
CAHUTE_EXTERN(int)
cahute_pad_data(cahute_u8 *buf, cahute_u8 const *data, size_t data_size) {
    cahute_u8 *orig = buf;
    cahute_u8 const *p;

    for (p = data; data_size--; p++) {
        int byte = *p;

        if (byte < 32) {
            *buf++ = '\\';
            *buf++ = 32 + byte;
        } else if (byte == '\\') {
            *buf++ = '\\';
            *buf++ = '\\';
        } else
            *buf++ = byte;
    }

    return (size_t)(buf - orig);
}

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
CAHUTE_EXTERN(int)
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
