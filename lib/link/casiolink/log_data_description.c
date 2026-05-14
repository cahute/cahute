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
 * Show a data description in a logging context.
 *
 * @param desc Data description to show.
 */
CAHUTE_INTERNAL(void)
cahute_casiolink_log_data_description(
    cahute_context *context,
    struct cahute_casiolink_data_description const *desc
) {
    msg(context, ll_debug, "Data description was the following:");

    {
        char flags_buf[50], *p = flags_buf;

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_END) {
            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';
            *p++ = 'E';
            *p++ = 'N';
            *p++ = 'D';
        }

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_FINAL) {
            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';
            *p++ = 'F';
            *p++ = 'I';
            *p++ = 'N';
            *p++ = 'A';
            *p++ = 'L';
        }

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_AL) {
            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';
            *p++ = 'A';
            *p++ = 'L';
        }

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_AL_END) {
            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';
            *p++ = 'A';
            *p++ = 'L';
            *p++ = '_';
            *p++ = 'E';
            *p++ = 'N';
            *p++ = 'D';
        }

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG) {
            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';
            *p++ = 'N';
            *p++ = 'O';
            *p++ = '_';
            *p++ = 'L';
            *p++ = 'O';
            *p++ = 'G';
        }

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_MDL) {
            *p++ = ' ';
            *p++ = '|';
            *p++ = ' ';
            *p++ = 'M';
            *p++ = 'D';
            *p++ = 'L';
        }

        *p = '\0';

        msg(context,
            ll_debug,
            "  Flags: %s",
            flags_buf[0] ? &flags_buf[3] : "(none)");
    }

    {
        size_t part_count = desc->part_count,
               last_part_repeat = desc->last_part_repeat;

        if (part_count && !last_part_repeat) {
            part_count--;
            last_part_repeat = 1;
        }

        if (!part_count)
            msg(context, ll_debug, "  Part count: 0");
        else {
            char sizes[60], *p = sizes;
            size_t i;

            for (i = 0; i < part_count - 1; i++) {
                sprintf(p, "%" CAHUTE_PRIuSIZE "o, ", desc->part_sizes[i]);
                for (; *p; p++)
                    ;
            }

            if (last_part_repeat > 1)
                sprintf(
                    p,
                    "%" CAHUTE_PRIuSIZE "o (x%" CAHUTE_PRIuSIZE ")",
                    desc->part_sizes[i],
                    last_part_repeat
                );
            else
                sprintf(p, "%" CAHUTE_PRIuSIZE "o", desc->part_sizes[i]);

            msg(context,
                ll_debug,
                "  Part count: %" CAHUTE_PRIuSIZE,
                part_count);
            msg(context, ll_debug, "  Part sizes: %s", sizes);
        }
    }
}
