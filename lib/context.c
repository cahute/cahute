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
 * Create a context.
 *
 * @param contextp Pointer to set to the created context.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_EXTERN(int) cahute_create_context(cahute_context **contextp) {
    cahute_context *context;

    *contextp = NULL;
    context = malloc(sizeof(cahute_context));
    if (!context)
        return CAHUTE_ERROR_ALLOC;

    cahute_reset_log_func(context);
    context->log_level = CAHUTE_DEFAULT_LOGLEVEL;

#if LIBUSB_ENABLED
    context->libusb_context = NULL;
#endif

#if AMIGAOS_ENABLED
    context->amiga_timer_msg_port = NULL;
    context->amiga_timer_request = NULL;
#endif

    *contextp = context;
    return CAHUTE_OK;
}

/**
 * Destroy a context.
 *
 * @param context Context to destroy.
 */
CAHUTE_EXTERN(void) cahute_destroy_context(cahute_context *context) {
    if (!context)
        return;

#if LIBUSB_ENABLED
    if (context->libusb_context)
        libusb_exit(context->libusb_context);
#endif

#if AMIGAOS_ENABLED
    if (context->amiga_timer_msg_port) {
        AbortIO((struct IORequest *)context->amiga_timer_request);
        WaitIO((struct IORequest *)context->amiga_timer_request);
        CloseDevice((struct IORequest *)context->amiga_timer_request);
        DeleteIORequest(context->amiga_timer_request);
        DeleteMsgPort(context->amiga_timer_msg_port);
    }
#endif

    free(context);
}

#if LIBUSB_ENABLED
/**
 * Get or instantiate the libusb context for a given context.
 *
 * @param context Context for which to get the libusb context.
 * @param contextp Pointer to set to the libusb context.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_get_libusb_context(cahute_context *context, libusb_context **contextp) {
    int err;

    *contextp = NULL;
    if (!context->libusb_context) {
        err = libusb_init(&context->libusb_context);
        if (err) {
            context->libusb_context = NULL;
            msg(context,
                ll_fatal,
                "Could not create a libusb context: %s (%d)",
                libusb_error_name(err),
                err);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    *contextp = context->libusb_context;
    return CAHUTE_OK;
}
#endif

#if AMIGAOS_ENABLED
/**
 * Get or instantiate the common AmigaOS timer for a given context.
 *
 * @param context Context for which to get the AmigaOS timer.
 * @param msg_portp Pointer to set to the timer message port, if set.
 * @param timerp Pointer to set to the timer request, if set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_get_amiga_timer(
    cahute_context *context,
    struct MsgPort **msg_portp,
    struct timerequest **timerp
) {
    struct MsgPort *msg_port;
    struct timerequest *timer_io;
    int ret;

    if (context->amiga_timer_request)
        goto end;

    msg_port = CreateMsgPort();
    if (!msg_port) {
        msg(context,
            ll_error,
            "An error has occurred while creating the port for the timer.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    timer_io = CreateIORequest(msg_port, sizeof(struct timerequest));
    if (!timer_io) {
        msg(context,
            ll_error,
            "An error has occurred while creating the timer I/O.");
        DeleteMsgPort(msg_port);
        return CAHUTE_ERROR_UNKNOWN;
    }

    ret = OpenDevice(
        (CONST_STRPTR)TIMERNAME,
        UNIT_VBLANK,
        (struct IORequest *)timer_io,
        0L
    );
    if (ret) {
        msg(context,
            ll_error,
            "An error has occurred while creating the timer I/O.");
        DeleteIORequest(timer_io);
        DeleteMsgPort(msg_port);
        return CAHUTE_ERROR_UNKNOWN;
    }

    context->amiga_timer_msg_port = msg_port;
    context->amiga_timer_request = timer_io;

end:
    if (msg_portp)
        *msg_portp = context->amiga_timer_msg_port;
    if (timerp)
        *timerp = context->amiga_timer_request;
    return CAHUTE_OK;
}
#endif
