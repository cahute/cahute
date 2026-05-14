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
 * Open standard output as a file, using an interface.
 *
 * @param context Context within which to open the standard output.
 * @param open_params Parameters to the function, passed through by
 *        ``cahute_open_stdout()``.
 * @param interface Native interface to use.
 * @param cookie Cookie to pass to the functions in the interface.
 * @param cookie_size Size of the cookie to pass.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_open_stdout_from_interface(
    cahute_stdout_open_params *open_params,
    cahute_stdout_open_interface const *interface,
    void *cookie,
    size_t cookie_size
) {
    cahute_context *context = open_params->context;
    cahute_file *file = NULL;
    cahute_u8 *data;

    file = malloc(
        sizeof(cahute_file) + 32 + CAHUTE_FILE_READ_BUFFER_SIZE + cookie_size
    );
    if (!file) {
        if (interface->close_func)
            (*interface->close_func)(context, cookie);

        return CAHUTE_ERROR_ALLOC;
    }

    data = (cahute_u8 *)&file[1];
    data += (~(cahute_uintptr)data & 31) + 1;

    file->flags = CAHUTE_FILE_FLAG_WRITE;
    file->file_size = 0;
    file->offset = 0;
    file->read_offset = 0;
    file->read_size = 0;
    file->context = context;
    file->cookie = &data[CAHUTE_FILE_READ_BUFFER_SIZE];
    file->read_buffer = data;
    file->close_func = interface->close_func;
    file->read_func = NULL;
    file->write_func = interface->write_func;
    file->seek_func = NULL;
    file->type = 0;
    file->extension[0] = '\0';

    if (!cookie || !cookie_size)
        file->cookie = NULL;
    else
        memcpy(file->cookie, cookie, cookie_size);

    *open_params->filep = file;
    return CAHUTE_OK;
}

/**
 * Open standard output as a file.
 *
 * @param context Context within which to open the standard output.
 * @param filep Pointer to the file to create.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_open_stdout(cahute_context CAHUTE_NNPTR(context), cahute_file **filep) {
    cahute_stdout_open_params params;

    params.context = context;
    params.filep = filep;

#if CAHUTE_PLATFORM_POSIX
    return cahute_open_posix_stdout(context, &params);
#elif CAHUTE_PLATFORM_WIN32
    return cahute_open_win32_stdout(context, &params);
#else
    (void)params;
    CAHUTE_RETURN_IMPL(context, "No file opening method available.");
#endif
}
