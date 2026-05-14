/* ****************************************************************************
 * Copyright (C) 2025 Thomas Touhey <thomas@touhey.fr>
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

/**
 * Load a DLL from the Windows system library.
 *
 * @param context Context in which the function is run.
 * @param dllp Pointer to the DLL to set.
 * @param name Name of the library to load.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_load_win32_system_library(
    cahute_context *context,
    HMODULE *dllp,
    char const *name
) {
    char path[MAX_PATH];
    size_t len, name_len;
    HMODULE dll;

    *dllp = NULL;

    name_len = strlen(name);
    len = GetSystemDirectoryA(path, sizeof(path));
    if (!len || len >= (UINT)sizeof(path)) {
        log_windows_error(context, "GetSystemDirectoryA", GetLastError());
        return CAHUTE_ERROR_SIZE;
    }

    /* 5 is the length of the backslash and ``.dll`` combined. */
    if (len + 5 + name_len >= sizeof(path)) {
        msg(context, ll_error, "System directory path too long: %s", path);
        return CAHUTE_ERROR_SIZE;
    }

    sprintf(&path[len], "\\%s.dll", name);
    dll = LoadLibraryA(path);
    if (!dll) {
        log_windows_error(context, "LoadLibraryA", GetLastError());
        return CAHUTE_ERROR_SIZE;
    }

    *dllp = dll;
    return CAHUTE_OK;
}

/**
 * Load a function from a DLL.
 *
 * @param context Context in which the function is run.
 * @param funcp Pointer to the function to set.
 * @param dll DLL to get the function from.
 * @param name Name of the function to extract from the library.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_get_win32_library_function(
    cahute_context *context,
    FARPROC *funcp,
    HMODULE dll,
    char const *name
) {
    FARPROC func;

    func = GetProcAddress(dll, name);
    if (!func) {
        msg(context, ll_error, "Could not load function `%s`:", name);
        log_windows_error(context, "GetProcAddress", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    *funcp = func;
    return CAHUTE_OK;
}
