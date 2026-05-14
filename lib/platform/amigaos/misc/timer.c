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

#include "../internals.h"

struct timer_data {
    struct MsgPort *msg_port;
    struct timerequest *time_request;
};

/**
 * Destroy the AmigaOS timer used in the Cahute context.
 *
 * @param context Context in which to close the AmigaOS timer.
 * @param data Timer data to free.
 */
CAHUTE_LOCAL(void)
cahute_destroy_amigaos_timer(
    cahute_context *context,
    struct timer_data *data
) {
    CloseDevice((struct IORequest *)data->time_request);
    DeleteIORequest(data->time_request);
    DeleteMsgPort(data->msg_port);

    free(data);
}

/**
 * Instantiate the AmigaOS timer data used in the Cahute context.
 *
 * @param context Cahute context to create the AmigaOS timer in.
 * @param datap Pointer to set to the data.
 * @param destroy_funcp Pointer to set to the destroy function.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_create_amigaos_timer(
    cahute_context *context,
    struct timer_data **datap,
    cahute_context_destroy_func **destroy_funcp
) {
    struct timer_data *data = NULL;
    int ret;

    data = malloc(sizeof(struct timer_data));
    if (!data)
        return CAHUTE_ERROR_ALLOC;

    data->time_request = NULL;
    data->msg_port = CreateMsgPort();
    if (!data->msg_port) {
        msg(context,
            ll_error,
            "An error has occurred while creating the port for the timer.");
        goto fail;
    }

    data->time_request =
        CreateIORequest(data->msg_port, sizeof(struct timerequest));
    if (!data->time_request) {
        msg(context,
            ll_error,
            "An error has occurred while creating the timer I/O.");
        goto fail;
    }

    ret = OpenDevice(
        (CONST_STRPTR)TIMERNAME,
        UNIT_VBLANK,
        (struct IORequest *)data->time_request,
        0L
    );
    if (ret) {
        msg(context,
            ll_error,
            "An error has occurred while creating the timer I/O.");
        goto fail;
    }

    *datap = data;
    *destroy_funcp =
        (cahute_context_destroy_func *)&cahute_destroy_amigaos_timer;
    return CAHUTE_OK;

fail:
    if (data->time_request)
        DeleteIORequest(data->time_request);
    if (data->msg_port)
        DeleteMsgPort(data->msg_port);
    free(data);
    return CAHUTE_ERROR_UNKNOWN;
}

/**
 * Get or instantiate the common AmigaOS timer for a given context.
 *
 * @param context Context for which to get the AmigaOS timer.
 * @param msg_portp Pointer to set to the timer message port, if set.
 * @param timerp Pointer to set to the timer request, if set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_get_amiga_timer(
    cahute_context *context,
    struct MsgPort **msg_portp,
    struct timerequest **timerp
) {
    struct timer_data *data;
    int err;

    err = cahute_get_context_pointer(
        context,
        (void **)&data,
        CAHUTE_CONTEXT_POINTER_AMIGAOS_TIMER,
        (cahute_context_init_func *)&cahute_create_amigaos_timer
    );
    if (err)
        return err;

    if (msg_portp)
        *msg_portp = data->msg_port;
    if (timerp)
        *timerp = data->time_request;
    return CAHUTE_OK;
}
