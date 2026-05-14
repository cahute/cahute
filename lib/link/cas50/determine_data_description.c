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
 * Determine the data description for a provided CAS50 header.
 *
 * @param context Context in which the function is run.
 * @param data CAS50 header (50B).
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_cas50_determine_data_description(
    cahute_context *context,
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_DATA;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    msg(context, ll_debug, "Raw CAS50 header is the following:");
    mem(context, ll_debug, data, 50);

    if (!memcmp(&data[1], "END\xFF", 4)) {
        /* End packet for CAS50. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "VAL", 4)) {
        unsigned int height = ((unsigned int)data[7] << 8) | data[8];
        unsigned int width = ((unsigned int)data[9] << 8) | data[10];

        /* Variable data use size as W*H, or only W, or only H depending
         * on the case. */
        if (!width)
            width = 1;

        desc->part_sizes[0] = 14;
        desc->last_part_repeat = height * width;
    } else {
        /* For other packets, the size should always be located at
         * offset 6 of the header, i.e. offset 7 of the buffer. */
        desc->part_sizes[0] = ((size_t)data[7] << 24) | ((size_t)data[8] << 16)
                              | ((size_t)data[9] << 8) | data[10];

        if (desc->part_sizes[0] > 2)
            desc->part_sizes[0] -= 2;
        else
            desc->part_count = 0;

        if (!memcmp(&data[1], "MEM\0BU", 6)) {
            /* Backups are guaranteed to be the final (and only) file
             * sent in the communication. */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        }
    }

    if (desc->part_count && !desc->part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        msg(context, ll_error, "Could not determine a data description.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    cahute_casiolink_log_data_description(context, desc);
    return CAHUTE_OK;
}
