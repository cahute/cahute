/* ****************************************************************************
 * Copyright (C) 2024-2025 Thomas Touhey <thomas@touhey.fr>
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
 * Read the first 40 bytes of a CASIOLINK header to determine the type.
 *
 * @param data First 40 bytes of the CASIOLINK header, including the 0x3A.
 * @return Variant.
 */
CAHUTE_INTERNAL(int)
cahute_casiolink_determine_header_variant(cahute_u8 const *data) {
    /* We want to try to determine the currently selected variant based
     * on the header's content. */
    if (!memcmp(&data[1], "ADN1", 4) || !memcmp(&data[1], "ADN2", 4)
        || !memcmp(&data[1], "BKU1", 4) || !memcmp(&data[1], "END1", 4)
        || !memcmp(&data[1], "FCL1", 4) || !memcmp(&data[1], "FMV1", 4)
        || !memcmp(&data[1], "MCS1", 4) || !memcmp(&data[1], "MDL1", 4)
        || !memcmp(&data[1], "REQ1", 4) || !memcmp(&data[1], "REQ2", 4)
        || !memcmp(&data[1], "SET1", 4)) {
        /* The type seems to be a CAS100 header type we can use. */
        return VARIANT_CAS100;
    }

    if (!memcmp(&data[1], "END\xFF", 4) || !memcmp(&data[1], "FNC", 4)
        || !memcmp(&data[1], "IMG", 4) || !memcmp(&data[1], "MEM", 4)
        || !memcmp(&data[1], "REQ", 4) || !memcmp(&data[1], "TXT", 4)
        || !memcmp(&data[1], "VAL", 4)) {
        /* The type seems to be a CAS50 header type.
         * This means that we actually have 10 more bytes to read for
         * a full header.
         *
         * NOTE: The '4' in the memcmp() calls above are intentional,
         * as the NUL character ('\0) is actually considered as part of
         * the CAS50 header type. */
        return VARIANT_CAS50;
    }

    /* By default, we consider the header to be a CAS40 header. */
    return VARIANT_CAS40;
}
