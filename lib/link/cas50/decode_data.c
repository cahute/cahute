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
 * Decode CAS50 data from a file.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offsetp Pointer to the offset in the file to read from, to set to
 *        the offset after the data afterwards.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_cas50_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp
) {
    cahute_u8 header_buf[50];
    cahute_casiolink_data_description desc = {0};
    unsigned long offset = *offsetp;
    unsigned int obtained_checksum;
    unsigned int expected_checksum;
    int err;

    /* Read the header from the buffer, and extract the variant if need be. */
    err = cahute_read_from_file(file, offset, header_buf, 50);
    if (err)
        return err;

    /* Only check the header if not already checked, i.e. in the case of files
     * and not in the case of links. */
    if (header_buf[0] != PACKET_TYPE_DATA) {
        msg(file->context,
            ll_error,
            "Header type 0x%02X is not the expected 0x%02X.",
            header_buf[0],
            PACKET_TYPE_DATA);
        return CAHUTE_ERROR_CORRUPT;
    }

    obtained_checksum = header_buf[49];
    expected_checksum = cahute_checksub(&header_buf[1], 48);
    if (obtained_checksum != expected_checksum) {
        msg(file->context,
            ll_error,
            "Header checksum 0x%02X is different from expected checksum "
            "%02X.",
            obtained_checksum,
            expected_checksum);
        return CAHUTE_ERROR_CORRUPT;
    }

    /* We need to get the data description in order to at least place the
     * offset after the current header and data part, even if it is not
     * implemented, in order for file reading to use CAHUTE_ERROR_IMPL errors
     * to skip unimplemented file types.
     *
     * NOTE: We only update ``*offsetp`` here and NOT ``offset``, because
     * ``offset`` is actually used in data decoding later on in the
     * function. */
    err = cahute_cas50_determine_data_description(
        file->context,
        header_buf,
        &desc
    );
    if (err)
        return err;

    *offsetp =
        offset + 50 + cahute_casiolink_compute_data_description_size(&desc);

    /* Only check data if not already checked, i.e. in the case of files and
     * not in the case of links (where all data is not available in one go,
     * i.e. the receiver must determine the data description, check the packet
     * type and checksums and acknowledge every part of the data). */
    err = cahute_casiolink_check_file_data(file, offset + 50, &desc);
    if (err)
        return err;

    return cahute_cas50_decode_data_using_description(
        datap,
        file,
        offset,
        header_buf,
        &desc
    );
}
