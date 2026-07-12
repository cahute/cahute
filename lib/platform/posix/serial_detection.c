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
 * Check if all characters in the strings are digits between 0 and 9.
 *
 * @param s String to check.
 * @return 1 if all characters are decimal digits, 0 otherwise. */
CAHUTE_LOCAL(int) all_numbers(char const *s) {
    for (; *s; s++)
        if (*s < '0' || *s > '9')
            return 0;

    return 1;
}

CAHUTE_INTERNAL(int)
cahute_posix_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
) {
    DIR *dp;
    struct dirent *dr;
    cahute_serial_detection_entry entry;
    size_t path_max = cahute_get_posix_path_max(context);
    char *fullbuf = malloc(path_max * 2);
    int err = CAHUTE_OK;

    if (!fullbuf)
        return CAHUTE_ERROR_ALLOC;

    /* Fallback method: iterate over known USB serial devices in "/dev"
     * directly:
     *
     * - On MacOS / OS X: "cu.*" (e.g. "cu.usbmodem621").
     * - On FreeBSD: "cuadX" (e.g. "cuad0") or "cuanX" (e.g. "cuan0").
     * - On NetBSD: "dtyX" (e.g. "dty01").
     * - On Linux: "ttyUSBX" (e.g. "ttyUSB1"). */
    dp = opendir("/dev/");
    if (dp) {
        char *buf = fullbuf;
        char *end = &buf[5];

        strcpy(buf, "/dev/");

        while (1) {
            if (!(dr = readdir(dp)))
                break;

            if (strncmp(dr->d_name, "cu.", 3)
                && (strncmp(dr->d_name, "cuad", 4)
                    || !all_numbers(&dr->d_name[4]))
                && (strncmp(dr->d_name, "cuan", 4)
                    || !all_numbers(&dr->d_name[4]))
                && (strncmp(dr->d_name, "dty", 4)
                    || !all_numbers(&dr->d_name[3]))
                && (strncmp(dr->d_name, "ttyUSB", 6)
                    || !all_numbers(&dr->d_name[6])))
                continue;

            strcpy(end, dr->d_name);
            entry.cahute_serial_detection_entry_name = buf;
            if (func(cookie, &entry)) {
                err = CAHUTE_ERROR_INT;
                break;
            }
        }

        closedir(dp);
        free(fullbuf);

        return err;
    }

    free(fullbuf);
    return CAHUTE_OK;
}
