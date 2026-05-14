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
 * Receive raw CAS100 data, at start or after the header.
 *
 * @param link Link.
 * @param header Buffer containing the header for the CAS100 data.
 *        NULL if the header has not been received yet.
 * @param timeout Timeout in milliseconds for the first data.
 * @param desc Data description to fill and use.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_cas100_receive_raw_data(
    cahute_link *link,
    cahute_u8 const *header,
    unsigned long timeout,
    struct cahute_casiolink_data_description *desc
) {
    cahute_u8 *data = link->data_buffer;
    size_t data_capacity = link->data_buffer_capacity;
    int err;

    if (data_capacity < 40) {
        msg(link->context,
            ll_error,
            "Data capacity was expected to be at least 40 bytes.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    for (;; header = NULL) {
        if (header)
            memcpy(data, header, 40);
        else {
            err = cahute_casiolink_receive_packet(
                link,
                data,
                38,
                PACKET_TYPE_HEADER,
                timeout
            );
            if (err)
                return err;
        }

        err = cahute_cas100_determine_data_description(
            link->context,
            data,
            desc
        );
        if (err)
            return err;

        if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_END) {
            msg(link->context, ll_debug, "CAS100 data type is an END packet.");
            link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
            return CAHUTE_ERROR_TERMINATED;
        } else if (desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_MDL) {
            err = cahute_cas100_handle_mdl1(link, data);
            if (err)
                return err;

            continue;
        }

        break;
    }

    data_capacity -= 40;
    err = cahute_casiolink_receive_raw_data(
        link,
        desc,
        &data[40],
        &data_capacity
    );
    link->data_buffer_size = 40 + data_capacity;

    return err;
}
