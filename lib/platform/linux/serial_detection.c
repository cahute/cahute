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

CAHUTE_INTERNAL(int)
cahute_linux_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
) {
    DIR *dp;
    struct dirent *dr;
    struct stat st;
    cahute_serial_detection_entry entry;
    size_t path_max = cahute_get_posix_path_max(context);
    char *fullbuf = malloc(path_max * 2);
    int err = CAHUTE_OK;

    if (!fullbuf)
        return CAHUTE_ERROR_ALLOC;

    /* Iterate over links in ``/dev/serial/by-id/``, which is
     * populated by a system udev rule if present. */
    dp = opendir("/dev/serial/by-id/");
    if (!dp)
        return CAHUTE_ERROR_NOT_FOUND;

    char *buf = fullbuf, *devbuf = &fullbuf[path_max];
    char *end = &buf[18];
    cahute_ssize rl;

    strcpy(buf, "/dev/serial/by-id/");

    while (1) {
        if (!(dr = readdir(dp)))
            break;

        /* The entry is expected to be a link.
            * We want to check that and, if it's the case, get the absolute
            * path of the linked serial device. */
        strcpy(end, dr->d_name);
        if (lstat(buf, &st) || (st.st_mode & S_IFMT) != S_IFLNK)
            continue;

        rl = readlink(buf, devbuf, path_max);
        if (rl < 0)
            continue;

        if (devbuf[0] != '/') {
            /* Device path is relative to the directory, concatenate the
                * original device and return. */
            strcpy(end, devbuf);
            if (!(realpath(buf, devbuf)))
                continue;
        }

        entry.cahute_serial_detection_entry_name = devbuf;
        if (func(cookie, &entry)) {
            err = CAHUTE_ERROR_INT;
            break;
        }
    }

    closedir(dp);
    free(fullbuf);

    return err;
}
