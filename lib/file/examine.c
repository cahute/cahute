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
 * Determine the file type and store it within the file.
 *
 * @param file File object.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int) cahute_examine_file(cahute_file *file) {
    cahute_u8 buf[32], *p, *q;
    cahute_u8 std[32];
    int err;

    /* Read the first 32 (0x20) bytes to try to find a StandardHeader. */
    err = cahute_read_from_file(file, 0, buf, 32);
    if (err == CAHUTE_ERROR_TRUNC)
        goto examined;
    else if (err)
        return err;

    /* We can try to determine a standard header. */
    for (p = &buf[31], q = &std[31]; p >= buf; p--, q--)
        *q = ~*p & 255;

    if (!memcmp(std, "USBPower\x62\0\x10\0\x10\0", 14)    /* G1M, G1R */
        || !memcmp(std, "USBPower\x31\0\x10\0\x10\0", 14) /* G2M, G2R */
        || !memcmp(std, "USBPower\x75\0\x10\0\x10\0", 14) /* G3M, G3R */
    ) {
        file->type = CAHUTE_FILE_TYPE_MAINMEM;
        goto examined;
    }

    /* We can try to see if we have a CASIOLINK archive. */
    if (buf[0] == 0x3A) {
        file->type = CAHUTE_FILE_TYPE_CASIOLINK;
        goto examined;
    }

    /* TODO: There are many, many more file types to test for. */

    /* TODO: By compatibility with CaS, the following extensions must be
     * matched:
     *
     * - '.ctf', '.txt': CTF;
     * - '.fxp': FX-Program;
     * - '.bmp': Bitmap;
     * - '.gif': GIF. */

examined:
    file->flags |= CAHUTE_FILE_FLAG_EXAMINED;
    return CAHUTE_OK;
}

/**
 * Guess the file type.
 *
 * @param file File object.
 * @param typep Type to set with the found file type.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_guess_file_type(cahute_file *file, unsigned long *typep) {
    *typep = CAHUTE_FILE_TYPE_UNKNOWN;

    EXAMINE(file);

    if (!file->type)
        return CAHUTE_ERROR_NOT_FOUND;

    *typep = file->type;
    return CAHUTE_OK;
}
