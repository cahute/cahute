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
    int i;

    *contextp = NULL;
    context = malloc(sizeof(cahute_context));
    if (!context)
        return CAHUTE_ERROR_ALLOC;

    cahute_reset_log_func(context);
    context->log_level = CAHUTE_DEFAULT_LOGLEVEL;
    context->log_prefix = "";

#if CAHUTE_PLATFORM_POSIX
    if (cahute_posix_is_stderr_tty())
        context->log_prefix = "\r";
#endif

    for (i = 0; i < CAHUTE_CONTEXT_POINTER_COUNT; i++)
        context->pointers[i].flags = 0;

    *contextp = context;
    return CAHUTE_OK;
}

/**
 * Destroy a context.
 *
 * @param context Context to destroy.
 */
CAHUTE_EXTERN(void) cahute_destroy_context(cahute_context *context) {
    int i;

    if (!context)
        return;

    for (i = 0; i < CAHUTE_CONTEXT_POINTER_COUNT; i++) {
        cahute_context_pointer *p = &context->pointers[i];

        if (~p->flags & CAHUTE_CONTEXT_POINTER_FLAG_INIT || !p->destroy_func)
            continue;

        (*p->destroy_func)(context, p->value);
    }

    free(context);
}

/**
 * Get or instantiate a context pointer.
 *
 * @param context Context for which to get the pointer.
 * @param valuep Pointer to the value to set.
 * @param key Key of the pointer to get.
 * @param init_func Initialization function, if the value is not yet
 *        initialized.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_get_context_pointer(
    cahute_context *context,
    void **valuep,
    int key,
    cahute_context_init_func *init_func
) {
    cahute_context_pointer *p = &context->pointers[key];
    cahute_context_destroy_func *destroy_func = NULL;
    void *value = NULL;
    int err;

    if (p->flags & CAHUTE_CONTEXT_POINTER_FLAG_INIT)
        goto end;

    err = (*init_func)(context, &value, &destroy_func);
    if (err)
        return err;

    p->flags = CAHUTE_CONTEXT_POINTER_FLAG_INIT;
    p->value = value;
    p->destroy_func = destroy_func;

end:
    if (valuep)
        *valuep = p->value;
    return CAHUTE_OK;
}
