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
 * Compute a checksum from a file starting at an offset.
 *
 * @param file File from which to read.
 * @param offset Offset from which to read.
 * @param size Size of the data region to read.
 * @param checksum Checksum pointer.
 * @return Error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_checksum_from_file(
    cahute_file *file,
    unsigned long offset,
    size_t size,
    unsigned int *checksump
) {
    cahute_u8 tmp_buf[1024];
    unsigned int checksum = 0;
    int err;

    while (size > sizeof(tmp_buf)) {
        err = cahute_read_from_file(file, offset, tmp_buf, sizeof(tmp_buf));
        if (err)
            return err;

        checksum += cahute_checksum(tmp_buf, sizeof(tmp_buf));
        offset += sizeof(tmp_buf);
        size -= sizeof(tmp_buf);
    }

    if (size) {
        err = cahute_read_from_file(file, offset, tmp_buf, size);
        if (err)
            return err;

        checksum += cahute_checksum(tmp_buf, size);
    }

    *checksump = checksum & 255;
    return CAHUTE_OK;
}
