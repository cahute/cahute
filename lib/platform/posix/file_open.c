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

CAHUTE_LOCAL_DATA(cahute_file_open_interface)
posix_open_file_interface = {
    (cahute_file_close_func *)&cahute_close_posix_file,
    (cahute_file_read_func *)&cahute_read_from_posix_file,
    (cahute_file_seek_func *)&cahute_move_in_posix_file
};

/**
 * Open a file.
 *
 * @param context
 * @param open_params
 * @param path
 * @param path_type
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_open_posix_file(
    cahute_context *context,
    cahute_file_open_params *open_params,
    void const *path,
    int path_type
) {
    cahute_posix_file_cookie cookie;
    unsigned long file_size;
    int fd;
    off_t off;

    if (path_type != CAHUTE_PATH_TYPE_POSIX)
        CAHUTE_RETURN_IMPL(context, "Path type must be POSIX.");

    fd = open(path, O_RDONLY | O_NOCTTY);
    if (fd < 0) {
        switch (errno) {
        case ENOENT:
            msg(context, ll_error, "Could not open file: %s", strerror(errno));
            return CAHUTE_ERROR_NOT_FOUND;

        case EACCES:
            return CAHUTE_ERROR_PRIV;

        default:
            msg(context,
                ll_error,
                "Unknown error: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    off = lseek(fd, 0, SEEK_END);

    if (off == (off_t)-1) {
        close(fd);
        switch (errno) {
        default:
            msg(context,
                ll_error,
                "An error occurred while calling lseek(): %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    if (off > CAHUTE_MAX_FILE_OFFSET) {
        msg(context,
            ll_warn,
            "File size %lu is longer than maximum offset %lu",
            (unsigned long)off,
            CAHUTE_MAX_FILE_OFFSET);
        close(fd);
        return CAHUTE_ERROR_SIZE;
    }

    file_size = (unsigned long)off;

    off = lseek(fd, 0, SEEK_SET);
    if (off == (off_t)-1) {
        close(fd);
        switch (errno) {
        default:
            msg(context,
                ll_error,
                "An error occurred while calling lseek(): %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    cookie.fd = fd;
    cookie.close = 1;

    return cahute_open_file_from_interface(
        open_params,
        &posix_open_file_interface,
        &cookie,
        sizeof(cookie),
        file_size
    );
}
