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
 * Receive data synchronously from the transport associated with the link.
 *
 * This function is guaranteed to fill the buffer completely, or return
 * an error.
 *
 * If any timeout is provided as 0, the corresponding timeout will be
 * unlimited, i.e. the function will wait indefinitely.
 *
 * @param link Link on which to receive.
 * @param buf Buffer in which to write the received data.
 *        Can be NULL if we only want to skip data received on the link.
 * @param size Size to receive into the buffer.
 * @param first_timeout Timeout before the first byte is received,
 *        in milliseconds.
 * @param next_timeout Timeout in-between any byte past the first one,
 *        in milliseconds.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_receive_on_link_transport(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    unsigned long first_timeout,
    unsigned long next_timeout
) {
    size_t original_size = size; /* For logging. */
    size_t bytes_received;
    unsigned long timeout = first_timeout;
    unsigned long iteration_timeout = first_timeout; /* For logging. */
    unsigned long start_time, first_time = 0, last_time;
    int timeout_error = CAHUTE_ERROR_TIMEOUT_START;
    int err;

    if (!size)
        return CAHUTE_OK;

    /* We first need to empty the link's transport receive buffer.
     * Note that this may fully satisfy the need presented by the caller. */
    {
        size_t left =
            link->transport_receive_size - link->transport_receive_start;

        if (size <= left) {
            if (buf)
                memcpy(
                    buf,
                    &link->transport_receive_buffer
                         [link->transport_receive_start],
                    size
                );

            link->transport_receive_start += size;
            return CAHUTE_OK;
        }

        if (left) {
            if (buf) {
                memcpy(
                    buf,
                    &link->transport_receive_buffer
                         [link->transport_receive_start],
                    left
                );
                buf += left;
            }

            size -= left;
        }

        /* The transport receive buffer is empty! We need to reset it. */
        link->transport_receive_start = 0;
        link->transport_receive_size = 0;
    }

    /* Set ``bytes_received`` to 1 so the first round of the loop actually
     * attempts at receiving and does not remove time from the time. */
    bytes_received = 1;

    err = cahute_monotonic(link->context, &start_time);
    if (err)
        return err;

    last_time = start_time;

    /* We need to complete the buffer here, by doing multiple passes on the
     * transport implementation specific receive code until the caller's need
     * is fully satisfied.
     *
     * At each pass, we actually want to ensure that we always have
     * ``CAHUTE_LINK_RECEIVE_BUFFER_SIZE`` bytes available in the target
     * buffer.
     *
     * In order to accomplish this, for each pass, we actually determine
     * whether we want to write in the caller's buffer directly or in
     * the link's transport receive buffer. */
    while (size) {
        /* If no bytes have been received since last time, we actually need to
         * remove the difference using the monotonic clock! */
        if (!bytes_received && timeout) {
            unsigned long current_time;

            err = cahute_monotonic(link->context, &current_time);
            if (err)
                return err;

            if (current_time - last_time >= timeout)
                goto time_out;

            timeout -= current_time - last_time;
            last_time = current_time;
        }

        bytes_received = CAHUTE_LINK_RECEIVE_BUFFER_SIZE;

        /* NOTE: Historically here, we used to write directly to the
         * destination buffer if the output was big enough. However, the
         * transport sometimes requires aligned buffers, and the transport
         * receive buffer is guaranteed to be aligned on 32 bytes. */
        err = (*link->transport_receive_func)(
            link->context,
            link->transport_stream_cookie,
            link->transport_receive_buffer,
            CAHUTE_LINK_RECEIVE_BUFFER_SIZE,
            &bytes_received,
            timeout
        );
        switch (err) {
        case CAHUTE_OK:
            break;

        case CAHUTE_ERROR_TIMEOUT:
        case CAHUTE_ERROR_TIMEOUT_START:
            goto time_out;

        case CAHUTE_ERROR_GONE:
            link->flags |= CAHUTE_LINK_FLAG_GONE;
            /* FALLTHRU */

        default:
            return err;
        }

        if (!bytes_received)
            continue;

        /* At least one byte has been received in this iteration; we can reset
         * the timeout to the next timeout. */
        timeout = next_timeout;
        iteration_timeout = next_timeout;
        timeout_error = CAHUTE_ERROR_TIMEOUT;

        if (!first_time) {
            err = cahute_monotonic(link->context, &first_time);
            if (err)
                return err;

            last_time = first_time;
        } else {
            err = cahute_monotonic(link->context, &last_time);
            if (err)
                return err;
        }

        if (bytes_received >= size) {
            if (buf)
                memcpy(buf, link->transport_receive_buffer, size);

            link->transport_receive_start = size;
            link->transport_receive_size = bytes_received;

            break;
        }

        if (buf) {
            memcpy(buf, link->transport_receive_buffer, bytes_received);
            buf += bytes_received;
        }

        size -= bytes_received;
    }

    if (!cahute_monotonic(link->context, &last_time)) {
        if (first_time > start_time + 20) {
            msg(link->context,
                ll_debug,
                "Received %" CAHUTE_PRIuSIZE
                " bytes in %lums (after waiting %lums).",
                original_size + link->transport_receive_size
                    - link->transport_receive_start,
                last_time - first_time,
                first_time - start_time);
        } else {
            msg(link->context,
                ll_debug,
                "Received %" CAHUTE_PRIuSIZE " bytes in %lums.",
                original_size + link->transport_receive_size
                    - link->transport_receive_start,
                last_time - start_time);
        }
    }

    return CAHUTE_OK;

time_out:
    msg(link->context,
        ll_error,
        "Hit a timeout of %lums after receiving %" CAHUTE_PRIuSIZE
        "/%" CAHUTE_PRIuSIZE " bytes.",
        iteration_timeout,
        original_size - size,
        original_size);
    return timeout_error;
}
