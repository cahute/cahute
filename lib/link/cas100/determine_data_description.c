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
 * Determine the data description for a provided CAS100 header.
 *
 * @param context Context in which the function is run.
 * @param data CAS100 header (40B).
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_cas100_determine_data_description(
    cahute_context *context,
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_HEADER;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    msg(context, ll_debug, "Raw CAS100 header is the following:");
    mem(context, ll_debug, data, 40);

    if (!memcmp(&data[1], "BKU1", 4)) {
        /* Backup packet for CAS100. */
        desc->part_sizes[0] = ((size_t)data[9] << 24)
                              | ((size_t)data[10] << 16)
                              | ((size_t)data[11] << 8) | data[12];
    } else if (!memcmp(&data[1], "END1", 4)) {
        /* End packet for CAS100. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "MCS1", 4)) {
        /* Main memory packet for CAS100. */
        desc->part_sizes[0] = ((size_t)data[8] << 8) | data[9];
        if (!desc->part_sizes[0])
            desc->part_count = 0;
    } else if (!memcmp(&data[1], "MDL1", 4)) {
        /* Initialization packet for CAS100. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_MDL;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "SET1", 4)) {
        /* TODO */
        desc->part_count = 0;
    }

    if (desc->part_count && !desc->part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        msg(context, ll_error, "Could not determine a data description.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}
