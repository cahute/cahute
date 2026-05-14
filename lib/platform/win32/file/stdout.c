/* ****************************************************************************
 * Copyright (C) 2024-2025 Thomas Touhey <thomas@touhey.fr>
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

CAHUTE_LOCAL_DATA(cahute_stdout_open_interface)
win32_stdout_interface = {
    (cahute_file_close_func *)&cahute_close_win32_file,
    (cahute_file_write_func *)&cahute_write_to_win32_file
};

/**
 * Open standard output.
 *
 * @param context
 * @param open_params
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_open_win32_stdout(
    cahute_context *context,
    cahute_stdout_open_params *open_params
) {
    cahute_win32_file_cookie cookie;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (handle == INVALID_HANDLE_VALUE) {
        log_windows_error(context, "GetStdHandle", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    cookie.handle = handle;
    cookie.close = 0;
    return cahute_open_stdout_from_interface(
        open_params,
        &win32_stdout_interface,
        &cookie,
        sizeof(cookie)
    );
}
