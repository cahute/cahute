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
 * Obtain a 32-bit integer from raw data, if available.
 *
 * SECURITY: The raw buffer is expected to be at least 8 bytes long.
 *
 * @param buf Buffer from which to get the 32-bit integer.
 * @return Integer, or 0 if no integer could be decoded.
 */
CAHUTE_INTERNAL(unsigned long) cahute_get_long_dec(cahute_u8 const *raw) {
    unsigned long x = 0;

    if (!isdigit(raw[0]) || !isdigit(raw[1]) || !isdigit(raw[2])
        || !isdigit(raw[3]) || !isdigit(raw[4]) || !isdigit(raw[5])
        || !isdigit(raw[6]) || !isdigit(raw[7]))
        return 0;

    x = (raw[0] - '0') * 10 + raw[1] - '0';
    x = x * 10 + raw[2] - '0';
    x = x * 10 + raw[3] - '0';
    x = x * 10 + raw[4] - '0';
    x = x * 10 + raw[5] - '0';
    x = x * 10 + raw[6] - '0';
    x = x * 10 + raw[7] - '0';

    return x;
}
