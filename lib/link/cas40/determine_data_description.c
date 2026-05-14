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
 * Determine the data description for a provided CAS40 header.
 *
 * @param context Context in which the function is run.
 * @param data CAS40 header (40B).
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_cas40_determine_data_description(
    cahute_context *context,
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_DATA;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    msg(context, ll_debug, "Raw CAS40 header is the following:");
    mem(context, ll_debug, data, 40);

    if (!memcmp(&data[1], "\x17\x17", 2)) {
        /* CAS40 AL End */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_AL_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "\x17\xFF", 2)) {
        /* CAS40 End */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "A1", 2)) {
        /* CAS40 Dynamic Graph */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] > 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "AA", 2)) {
        /* CAS40 Dynamic Graph in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] > 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "AD", 2)) {
        /* CAS40 All Memories */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    } else if (!memcmp(&data[1], "AL", 2)) {
        /* CAS40 All */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_AL;
        desc->part_count = 0;
    } else if (!memcmp(&data[1], "AM", 2)) {
        /* CAS40 Variable Memories */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    } else if (!memcmp(&data[1], "BU", 2)) {
        /* CAS40 Backup */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        if (!memcmp(&data[3], "TYPEA00", 7) || !memcmp(&data[3], "TYPEA02", 7))
            desc->part_sizes[0] = 32768;
    } else if (!memcmp(&data[1], "DC", 2)) {
        /* CAS40 Color Screenshot. */
        unsigned int width = data[3], height = data[4];

        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL
                       | CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG;
        if (!memcmp(&data[5], "\x11UWF\x03", 4)) {
            desc->last_part_repeat = 3;
            desc->part_sizes[0] = 1 + ((width >> 3) + !!(width & 7)) * height;
        }
    } else if (!memcmp(&data[1], "DD", 2)) {
        /* CAS40 Monochrome Screenshot. */
        unsigned int width = data[3], height = data[4];

        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL
                       | CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG;
        if (!memcmp(&data[5], "\x10\x44WF", 4))
            desc->part_sizes[0] = ((width >> 3) + !!(width & 7)) * height;
    } else if (!memcmp(&data[1], "DM", 2)) {
        /* CAS40 Defined Memories */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
    } else if (!memcmp(&data[1], "EN", 2)) {
        /* CAS40 Single Editor Program */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "EP", 2)) {
        /* CAS40 Single Password Protected Editor Program */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "F1", 2)) {
        /* CAS40 Single Function */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "F6", 2)) {
        /* CAS40 Multiple Functions */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "FN", 2)) {
        /* CAS40 Single Editor Program in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "FP", 2)) {
        /* CAS40 Single Password Protected Editor Program in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "G1", 2)) {
        /* CAS40 Graph Function */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "GA", 2)) {
        /* CAS40 Graph Function in Bulk */
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "GF", 2)) {
        /* CAS40 Factor */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = 2 + data[6] * 10;
    } else if (!memcmp(&data[1], "GR", 2)) {
        /* CAS40 Range */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = 92;
    } else if (!memcmp(&data[1], "GT", 2)) {
        /* CAS40 Function Table */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_count = 3;
        desc->last_part_repeat = ((size_t)data[7] << 8) | data[8];
        desc->part_sizes[0] = data[6];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;

        desc->part_sizes[1] = 32;
        desc->part_sizes[2] = 22;
    } else if (!memcmp(&data[1], "M1", 2)) {
        /* CAS40 Single Matrix */
        unsigned int width = data[5], height = data[6];

        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = 14;
        desc->last_part_repeat = width * height + 1; /* Sentinel data part. */
    } else if (!memcmp(&data[1], "MA", 2)) {
        /* CAS40 Single Matrix in Bulk */
        unsigned int width = data[5], height = data[6];

        desc->flags |= 0;
        desc->part_sizes[0] = 14;
        desc->last_part_repeat = width * height;
    } else if (!memcmp(&data[1], "P1", 2)) {
        /* CAS40 Single Numbered Program. */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;
    } else if (!memcmp(&data[1], "PD", 2)) {
        unsigned int deg = data[5] * 10 + data[6];

        /* CAS40 Polynomial Equation */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        if (!deg)
            desc->part_count = 0;
        else if (deg == 2)
            desc->part_sizes[0] = 32;
        else if (deg == 3)
            desc->part_sizes[0] = 42;
    } else if (!memcmp(&data[1], "PZ", 2)) {
        /* CAS40 Multiple Numbered Programs */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_count = 2;
        desc->part_sizes[0] = 190;
        desc->part_sizes[1] = ((size_t)data[4] << 8) | data[5];
        if (desc->part_sizes[1] >= 2)
            desc->part_sizes[1] -= 2;
    } else if (!memcmp(&data[1], "RT", 2)) {
        /* CAS40 Recursion Table */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->part_count = 3;
        desc->last_part_repeat = ((size_t)data[7] << 8) | data[8];
        desc->part_sizes[0] = data[6];
        if (desc->part_sizes[0] >= 2)
            desc->part_sizes[0] -= 2;

        desc->part_sizes[1] = 22;
        desc->part_sizes[2] = 32;
    } else if (!memcmp(&data[1], "SD", 2)) {
        /* CAS40 Simultaneous Equations */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = data[5] * data[6] + 1;
        desc->part_sizes[0] = 14;
    } else if (!memcmp(&data[1], "SR", 2)) {
        /* CAS40 Paired Variable Data */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 32;
    } else if (!memcmp(&data[1], "SS", 2)) {
        /* CAS40 Single Variable Data */
        desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
        desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
        desc->part_sizes[0] = 22;
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
