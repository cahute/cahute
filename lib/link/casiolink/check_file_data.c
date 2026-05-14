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
 * Check a file based on a data description.
 *
 * @param file File to check.
 * @param offset Offset from which to check the file.
 * @param desc Data description based on which to check the file.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_casiolink_check_file_data(
    cahute_file *file,
    unsigned long offset,
    struct cahute_casiolink_data_description const *desc
) {
    cahute_u8 buf[4];
    unsigned int checksum, checksum_alt;
    size_t i, total_parts, part_size;
    int err;

    if (!desc->part_count)
        return CAHUTE_OK;

    total_parts = desc->part_count - 1 + desc->last_part_repeat;
    for (i = 0; i < total_parts; i++) {
        part_size =
            desc->part_sizes[i >= desc->part_count ? desc->part_count - 1 : i];

        err = cahute_read_from_file(file, offset++, buf, 2);
        if (err)
            return err;

        if (buf[0] != desc->packet_type) {
            msg(file->context,
                ll_error,
                "In part %" CAHUTE_PRIuSIZE "/%" CAHUTE_PRIuSIZE
                ": invalid "
                "type 0x%02X (expected: 0x%02X)",
                i + 1,
                total_parts,
                buf[0],
                desc->packet_type);
            return CAHUTE_ERROR_CORRUPT;
        }

        /* We apply the same checksum logics as in
         * `cahute_casiolink_receive_packet()`, with the alt checksum
         * for CAS40 screenshots. */
        if (part_size > 1) {
            err = cahute_checksum_from_file(
                file,
                offset + 1,
                part_size - 1,
                &checksum_alt
            );
            if (err)
                return err;

            checksum = (checksum_alt + buf[1]) & 255;
        } else if (part_size) {
            checksum = buf[1];
            checksum_alt = 0;
        } else {
            checksum = 0;
            checksum_alt = 0;
        }

        checksum = cahute_checksub_from_checksum(checksum);
        checksum_alt = cahute_checksub_from_checksum(checksum_alt);

        if (err)
            return err;

        offset += part_size;
        err = cahute_read_from_file(file, offset++, &buf[1], 1);
        if (err)
            return err;

        if (buf[1] != checksum && buf[1] != checksum_alt) {
            msg(file->context,
                ll_error,
                "In part %" CAHUTE_PRIuSIZE "/%" CAHUTE_PRIuSIZE
                ": invalid checksum (obtained: 0x%02X, computed: 0x%02X)",
                i + 1,
                total_parts,
                buf[1],
                checksum);
            return CAHUTE_ERROR_CORRUPT;
        }
    }

    return CAHUTE_OK;
}
