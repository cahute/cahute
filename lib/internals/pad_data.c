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
CAHUTE_INTERNAL(int)
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
