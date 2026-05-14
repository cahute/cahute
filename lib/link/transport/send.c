/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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
 * Send data synchronously on the transport associated with the given link.
 *
 * There is no send buffering specific to Cahute: the buffer is directly
 * written to the underlying transport, meaning there is no guarantee of any
 * alignment.
 *
 * @param link Link with the transport to send data on.
 * @param buf Buffer to send on the link's transport.
 * @param size Size of the buffer to send.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_send_on_link_transport(
    cahute_link *link,
    cahute_u8 const *buf,
    size_t size
) {
    size_t bytes_sent;
    int err;

    if (!size)
        return CAHUTE_OK;

    do {
        bytes_sent = size;

        err = (*link->transport_send_func)(
            link->context,
            link->transport_stream_cookie,
            buf,
            size,
            &bytes_sent
        );
        switch (err) {
        case CAHUTE_OK:
            break;

        case CAHUTE_ERROR_GONE:
            link->flags |= CAHUTE_LINK_FLAG_GONE;
            /* FALLTHRU */

        default:
            return err;
        }

        if (bytes_sent >= size)
            break;

        buf += bytes_sent;
        size -= bytes_sent;
    } while (size);

    return CAHUTE_OK;
}
