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
 * Destroy the libusb context from the Cahute context.
 *
 * @param context Context in which to close the libusb context.
 * @param lu_context Libusb context to destroy.
 */
CAHUTE_LOCAL(void)
cahute_destroy_libusb_context(
    cahute_context *context,
    libusb_context *lu_context
) {
    libusb_exit(lu_context);
}

/**
 * Instantiate the libusb context with the Cahute context.
 *
 * @param context Cahute context to create the libusb context in.
 * @param lu_contextp Pointer to set to the context.
 * @param close_funcp Pointer to set the close function.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_create_libusb_context(
    cahute_context *context,
    libusb_context **lu_contextp,
    cahute_context_destroy_func **close_funcp
) {
    libusb_context *lu_context;
    int err;

    err = libusb_init(&lu_context);
    if (err) {
        msg(context,
            ll_fatal,
            "Could not create a libusb context: %s (%d)",
            libusb_error_name(err),
            err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *lu_contextp = lu_context;
    *close_funcp =
        (cahute_context_destroy_func *)&cahute_destroy_libusb_context;
    return CAHUTE_OK;
}


/**
 * Get or instantiate the libusb context for a given context.
 *
 * @param context Context for which to get the libusb context.
 * @param lu_contextp Pointer to set to the libusb context.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_get_libusb_context(
    cahute_context *context,
    libusb_context **lu_contextp
) {
    return cahute_get_context_pointer(
        context,
        (void **)lu_contextp,
        CAHUTE_CONTEXT_POINTER_LIBUSB_CONTEXT,
        (cahute_context_init_func *)&cahute_create_libusb_context
    );
}
