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

CAHUTE_DECLARE_TYPE(cahute_libusb_mapping)

/* ---
 * Global mapping.
 * --- */

/**
 * libusb context to Cahute context mapping.
 */
struct cahute_libusb_mapping {
    cahute_libusb_mapping *next;
    libusb_context *lu_context;
    cahute_context *local_context;
};

CAHUTE_LOCAL_MUTABLE_DATA(cahute_libusb_mapping *)
cahute_libusb_global_context_mapping = NULL;

/* ---
 * Logging callback.
 * --- */

/**
 * Log the libusb message.
 *
 * @param lu_context libusb context from which to retrieve the Cahute context.
 * @param level Logging level from libusb, to convert into the Cahute logging
 *        level.
 * @param str Formatted message.
 */
static void LIBUSB_CALL cahute_log_libusb_message(
    libusb_context *lu_context,
    enum libusb_log_level level,
    char const *str
) {
    cahute_libusb_mapping *ent;
    int loglevel;
    char const *result;
    size_t len;

    switch (level) {
    case LIBUSB_LOG_LEVEL_ERROR:
        loglevel = CAHUTE_LOGLEVEL_ERROR;
        break;

    case LIBUSB_LOG_LEVEL_WARNING:
        loglevel = CAHUTE_LOGLEVEL_WARNING;
        break;

    case LIBUSB_LOG_LEVEL_INFO:
        loglevel = CAHUTE_LOGLEVEL_INFO;
        break;

    case LIBUSB_LOG_LEVEL_DEBUG:
        loglevel = CAHUTE_LOGLEVEL_DEBUG;
        break;

    default:
        loglevel = CAHUTE_LOGLEVEL_NONE;
    }

    for (ent = cahute_libusb_global_context_mapping; ent; ent = ent->next) {
        if (ent->lu_context == lu_context)
            break;
    }

    if (!ent)
        return;

    /* libusb actually includes a lot of information we already have in our
     * metadata, such as the context, the monotonic time, and so on.
     * We want to try to remove these. */
    result = strstr(str, "libusb: ");
    if (result)
        str = &result[8];

    /* It actually also includes a final newline. We want to remove it. */
    len = strlen(str);
    if (len && str[len - 1] == '\n')
        len--;
    if (len && str[len - 1] == '\r')
        len--;
    if (!len)
        return;

    cahute_log_external_message(
        ent->local_context,
        loglevel,
        "libusb",
        NULL,
        str,
        len
    );
}

/* ---
 * Context pointer management.
 * --- */

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
    cahute_libusb_mapping **mapping_entryp, *mapping_entry;

    /* Destroy the context. */
    libusb_exit(lu_context);

    /* Unregister the context from the mapping. */
    for (mapping_entryp = &cahute_libusb_global_context_mapping;
         (mapping_entry = *mapping_entryp);
         mapping_entryp = &(*mapping_entryp)->next) {
        if (mapping_entry->lu_context != lu_context)
            continue;

        *mapping_entryp = mapping_entry->next;
        free(mapping_entry);
        break;
    }
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
    libusb_context *lu_context = NULL;
    cahute_libusb_mapping *mapping_entry = NULL;
    int err = CAHUTE_ERROR_UNKNOWN, lu_err;

    lu_err = libusb_init(&lu_context);
    if (lu_err) {
        msg(context,
            ll_fatal,
            "Could not create a libusb context: %s (%d)",
            libusb_error_name(lu_err),
            lu_err);
        goto fail;
    }

    /* Register the logging callback.
     * `libusb_set_log_cb()` appeared in v1.0.23, whereas
     * `LIBUSB_OPTION_LOG_CB` appeared in v1.0.27, so we use the former
     * for compatibility. */
    libusb_set_option(
        lu_context,
        LIBUSB_OPTION_LOG_LEVEL,
        LIBUSB_LOG_LEVEL_INFO
    );
    libusb_set_log_cb(
        lu_context,
        &cahute_log_libusb_message,
        LIBUSB_LOG_CB_CONTEXT
    );

    /* Register the context mapping. */
    mapping_entry = malloc(sizeof(cahute_libusb_mapping));
    if (!mapping_entry) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    mapping_entry->lu_context = lu_context;
    mapping_entry->local_context = context;
    mapping_entry->next = cahute_libusb_global_context_mapping;
    cahute_libusb_global_context_mapping = mapping_entry;

    *lu_contextp = lu_context;
    *close_funcp =
        (cahute_context_destroy_func *)&cahute_destroy_libusb_context;
    return CAHUTE_OK;

fail:
    if (lu_context)
        libusb_exit(lu_context);

    return err;
}


/**
 * Get or instantiate the libusb context for a given context.
 *
 * @param context Context for which to get the libusb context.
 * @param lu_contextp Pointer to set to the libusb context.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
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
