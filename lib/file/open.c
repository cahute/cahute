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
 * Open a file, using an interface.
 *
 * @param open_params Parameters to the function, passed through by
 *        ``cahute_open_file()``.
 * @param interface Native interface to use.
 * @param cookie Cookie to pass to the functions in the interface.
 * @param cookie_size Size of the cookie to pass.
 * @param file_size Size of the file.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_open_file_from_interface(
    cahute_file_open_params *open_params,
    cahute_file_open_interface const *interface,
    void *cookie,
    size_t cookie_size,
    unsigned long file_size
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

    file->flags =
        CAHUTE_FILE_FLAG_READ | CAHUTE_FILE_FLAG_SEEK | CAHUTE_FILE_FLAG_SIZE;
    file->file_size = file_size;
    file->offset = 0;
    file->read_offset = 0;
    file->read_size = 0;
    file->context = context;
    file->cookie = &data[CAHUTE_FILE_READ_BUFFER_SIZE];
    file->read_buffer = data;
    file->close_func = interface->close_func;
    file->read_func = interface->read_func;
    file->write_func = NULL;
    file->seek_func = interface->seek_func;
    file->type = 0;
    file->extension[0] = '\0';

    if (!cookie || !cookie_size)
        file->cookie = NULL;
    else
        memcpy(file->cookie, cookie, cookie_size);

    /* Try to find the path extension.
     * If this method fails, the error is just ignored. */
    cahute_find_path_extension(
        context,
        file->extension,
        sizeof(file->extension),
        open_params->path,
        open_params->path_type
    );

    *open_params->filep = file;
    return CAHUTE_OK;
}

/**
 * Open a file on the current system, for reading.
 *
 * @param context Context within which to create the file.
 * @param filep Pointer to the file to open.
 * @param flags Flags for opening the file.
 * @param path Path to the file to open.
 * @param path_type Type of the path.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_open_file(
    cahute_context CAHUTE_NNPTR(context),
    cahute_file **filep,
    unsigned long flags,
    void const *path,
    int path_type
) {
    cahute_file_open_params params;

    if (flags)
        CAHUTE_RETURN_IMPL(
            context,
            "Unsupported flags passed to cahute_open_file()."
        );

    params.context = context;
    params.filep = filep;
    params.path = path;
    params.path_type = path_type;

#if CAHUTE_PLATFORM_POSIX
    return cahute_open_posix_file(context, &params, path, path_type);
#elif CAHUTE_PLATFORM_WIN32
    return cahute_open_win32_file(context, &params, path, path_type);
#else
    (void)params;
    CAHUTE_RETURN_IMPL(context, "No file opening method available.");
#endif
}
