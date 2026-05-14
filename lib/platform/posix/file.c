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
 * Close a POSIX file.
 *
 * @param context
 * @param cookie
 */
CAHUTE_INTERNAL(void)
cahute_close_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie
) {
    if (cookie->close)
        close(cookie->fd);
}

/**
 * Read from the current offset using a POSIX file.
 *
 * @param context
 * @param cookie
 * @param buf
 * @param size
 * @param readp
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_read_from_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie,
    cahute_u8 *buf,
    size_t size,
    size_t *readp
) {
    cahute_ssize ret;

    ret = read(cookie->fd, buf, size);
    if (ret < 0)
        switch (errno) {
        default:
            msg(context,
                ll_error,
                "An error occurred while calling read(): %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

    *readp = (size_t)ret;
    return CAHUTE_OK;
}

/**
 * Write to the current offset using a POSIX file.
 *
 * @param context
 * @param cookie
 * @param buf
 * @param size
 * @param writtenp
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_write_to_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *writtenp
) {
    cahute_ssize ret;

    ret = write(cookie->fd, buf, size);
    if (ret < 0)
        switch (errno) {
        default:
            msg(context,
                ll_error,
                "An error occurred while calling write(): %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

    *writtenp = (size_t)ret;
    return CAHUTE_OK;
}

/**
 * Move to the provided offset on a POSIX file.
 *
 * @param context
 * @param cookie
 * @param offset
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_move_in_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie,
    unsigned long offset,
    unsigned long *offsetp
) {
    off_t loff = (off_t)offset;
    off_t new_loff;

    new_loff = lseek(cookie->fd, loff, SEEK_SET);
    if (new_loff == (off_t)-1)
        switch (errno) {
        case EOVERFLOW:
            /* off_t may be 16-bits on some platforms, but we support
             * up to 32-bit offsets here, so we can safely ignore this. */
            break;

        default:
            msg(context,
                ll_error,
                "An error occurred while calling lseek(): %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
    else
        *offsetp = new_loff;

    return CAHUTE_OK;
}
