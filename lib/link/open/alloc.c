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
 * Allocate a link and set its base properties.
 *
 * @param context Context in which the function is called.
 * @param linkp Pointer to the link to set to the allocated one.
 * @param cookie Cookie to set.
 * @param cookie_size Size of the cookie to set.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_alloc_link(
    cahute_context *context,
    cahute_link **linkp,
    void *cookie,
    size_t cookie_size
) {
    cahute_link *link = NULL;
    cahute_u8 *data;

    if (cookie_size && !cookie) {
        msg(context, ll_error, "Cookie size set with a NULL cookie, bug?");
        return CAHUTE_ERROR_UNKNOWN;
    } else if (cookie && !cookie_size) {
        msg(context, ll_error, "Cookie set with a null cookie size, bug?");
        return CAHUTE_ERROR_UNKNOWN;
    }

    link = malloc(
        sizeof(cahute_link) + 32 + CAHUTE_LINK_RECEIVE_BUFFER_SIZE
        + DEFAULT_DATA_BUFFER_SIZE + cookie_size
    );
    if (!link)
        return CAHUTE_ERROR_ALLOC;

    memset(link, 0, sizeof(cahute_link));

    data = (cahute_u8 *)&link[1];
    data += (~(cahute_uintptr)data & 31) + 1;

    link->transport_receive_buffer = data;
    data += CAHUTE_LINK_RECEIVE_BUFFER_SIZE;
    link->data_buffer = data;
    data += DEFAULT_DATA_BUFFER_SIZE;

    if (cookie_size) {
        link->transport_cookie = data;
        memcpy(link->transport_cookie, cookie, cookie_size);
    } else
        link->transport_cookie = NULL;

    link->transport_stream_cookie = link->transport_cookie;
    link->context = context;
    link->data_buffer_capacity = DEFAULT_DATA_BUFFER_SIZE;
    link->transport_close_func = (cahute_link_close_func *)0;
    link->transport_receive_func = (cahute_link_receive_func *)0;
    link->transport_send_func = (cahute_link_send_func *)0;
    link->transport_set_serial_params_func =
        (cahute_link_set_serial_params_func *)0;
    link->transport_scsi_request_to_func =
        (cahute_link_scsi_request_to_func *)0;
    link->transport_scsi_request_from_func =
        (cahute_link_scsi_request_from_func *)0;

    *linkp = link;
    return CAHUTE_OK;
}
