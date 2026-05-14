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

#include "../internals.h"

/**
 * Log a Windows API error.
 *
 * This is implemented as a separate function to the rest, because gathering
 * an error message for a given error code is quite lengthy.
 *
 * @param context Context to use for logging.
 * @param func_name Name of the function from which the log is emitted.
 * @param win_func Name of the Windows API function that returned the
 *        error.
 * @param code Windows API error code that was actually returned.
 */
CAHUTE_INTERNAL(void)
cahute_win32_log_error(
    cahute_context *context,
    char const *func_name,
    char const *win_func,
    DWORD code
) {
    char buf[1024];
    DWORD buf_size;

    buf_size = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf,
        1023,
        NULL
    );
    if (!buf_size) {
        cahute_log_message(
            context,
            30,
            func_name,
            "Error 0x%08lX occurred in %s.",
            code,
            win_func
        );
        return;
    }

    if (buf_size && buf[buf_size] == '\n')
        buf_size--;
    if (buf_size && buf[buf_size] == '\r')
        buf_size--;

    buf[buf_size] = '\0';
    cahute_log_message(
        context,
        30,
        func_name,
        "Error 0x%08lX occurred in %s: %s",
        code,
        win_func,
        buf
    );
}
