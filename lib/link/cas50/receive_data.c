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
 * Receive CAS50 data.
 *
 * @param link Link.
 * @param datap Pointer to the data to create.
 * @param header Buffer containing the initial CAS40 header.
 *        This can be provided as NULL if the header has not been read yet.
 * @param timeout Timeout for the header, if not read yet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_cas50_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
) {
    struct cahute_casiolink_data_description desc = {0};
    cahute_file memory_file;
    int err;

    for (;; header = NULL) {
        err = cahute_cas50_receive_raw_data(link, header, timeout, &desc);
        if (err)
            return err;

        cahute_populate_file_from_memory(
            &memory_file,
            link->context,
            link->data_buffer,
            link->data_buffer_size
        );

        err = cahute_cas50_decode_data_using_description(
            datap,
            &memory_file,
            0,
            link->data_buffer,
            &desc
        );
        if (err && err != CAHUTE_ERROR_IMPL)
            break;

        /* Data may have been final (e.g. in case of backup), we want to stop
         * the communication in that case. */
        if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
            return CAHUTE_ERROR_TERMINATED;
    }

    return err;
}
