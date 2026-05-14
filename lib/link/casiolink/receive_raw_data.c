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
 * Receive raw data.
 *
 * This is used by the generic CASIOLINK raw data reception function, as well
 * as CAS40, CAS50 and CAS100 raw data reception function.
 *
 * @param link Link to use.
 * @param desc Data description to receive data from.
 * @param buf Buffer to receive raw data into.
 * @param buf_sizep Pointer to the buffer capacity. Set to the buffer size
 *        at the end of the process.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_casiolink_receive_raw_data(
    cahute_link *link,
    struct cahute_casiolink_data_description const *desc,
    cahute_u8 *buf,
    size_t *buf_sizep
) {
    size_t received = 0;
    size_t i, nparts;
    int err;

    if (!desc->part_count)
        goto end;

    /* Before doing anything, check that the buffer is big enough.
     * Otherwise, we want to send invalid data here. */
    {
        size_t total_size =
            cahute_casiolink_compute_data_description_size(desc);

        if (total_size > *buf_sizep) {
            msg(link->context,
                ll_error,
                "Cannot get %" CAHUTE_PRIuSIZE "o into a %" CAHUTE_PRIuSIZE
                "o data buffer.",
                total_size,
                *buf_sizep);

            /* We actually send like we don't recognize the data, in
             * order not to make the link irrecoverable. */
            err = cahute_send_byte_on_link_transport(
                link,
                PACKET_TYPE_INVALID_DATA
            );
            if (err)
                return err;

            return CAHUTE_ERROR_SIZE;
        }
    }

    /* We can acknowledge the header so we can actually receive it. */
    err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_ACK);
    if (err)
        return err;

    nparts = desc->part_count - 1 + desc->last_part_repeat;
    for (i = 0; i < nparts; i++) {
        size_t part_size =
            desc->part_sizes[i >= desc->part_count ? desc->part_count - 1 : i];

        msg(link->context,
            ll_info,
            "Reading data part %d/%d (%" CAHUTE_PRIuSIZE "o).",
            i + 1,
            nparts,
            part_size);

        err = cahute_casiolink_receive_packet(
            link,
            buf,
            part_size,
            desc->packet_type,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_CORRUPT) {
            int sub_err;

            msg(link->context, ll_error, "Transfer will abort.");
            link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;

            sub_err = cahute_send_byte_on_link_transport(
                link,
                PACKET_TYPE_INVALID_DATA
            );
            if (sub_err)
                return sub_err;

            return err;
        } else if (err)
            return err;

        /* Acknowledge the data. */
        err = cahute_send_byte_on_link_transport(link, PACKET_TYPE_ACK);
        if (err)
            return err;

        msg(link->context,
            ll_info,
            "Data part %d/%d received and acknowledged.",
            i + 1,
            nparts);
        if ((~desc->flags & CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG)
            && part_size <= 4096) /* Let's not flood the terminal. */
            mem(link->context, ll_debug, buf, part_size);

        buf += part_size + 2;
        received += part_size + 2;
    }

    /* TODO */

end:
    *buf_sizep = received;
    return CAHUTE_OK;
}
