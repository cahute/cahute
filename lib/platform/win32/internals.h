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

#ifndef PLATFORM_WIN32_INTERNALS_H
#define PLATFORM_WIN32_INTERNALS_H 1

/* For Microsoft Windows, we want to explicitely select the target system to
 * avoid breaking compatibility if possible.
 * See the following for more information:
 *
 * https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt */
#define WINVER 0x0501 /* Windows XP */

#include "../../internals.h"
#include <windows.h>

CAHUTE_DECLARE_TYPE(cahute_win32_file_cookie)
CAHUTE_DECLARE_TYPE(cahute_win32_serial_link_cookie)
CAHUTE_DECLARE_TYPE(cahute_win32_ums_link_cookie)

CAHUTE_EXTERN(void)
cahute_win32_log_error(
    cahute_context *context,
    char const *func_name,
    char const *win_func,
    DWORD code
);

#define log_windows_error(CTX, FUNC, CODE) \
    cahute_win32_log_error(CTX, CAHUTE_LOGFUNC, FUNC, CODE)

/* ---
 * Link-related internals for Win32.
 * --- */

/**
 * Win32 serial / CESG link cookie.
 *
 * @property handle
 * @property overlapped
 * @property received
 * @property read_in_progress
 */
struct cahute_win32_serial_link_cookie {
    HANDLE handle;
    OVERLAPPED overlapped;
    DWORD received;
    DWORD read_in_progress;
};

CAHUTE_EXTERN(void)
cahute_close_win32_serial_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie
);

CAHUTE_EXTERN(int)
cahute_receive_on_win32_serial_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_send_on_win32_serial_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
);

CAHUTE_EXTERN(int)
cahute_set_win32_serial_link_params(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
);

/**
 * Win32 UMS link cookie.
 *
 * @property handle
 */
struct cahute_win32_ums_link_cookie {
    HANDLE handle;
};

CAHUTE_EXTERN(void)
cahute_close_win32_ums_link(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie
);

CAHUTE_EXTERN(int)
cahute_scsi_request_to_win32_device(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);

CAHUTE_EXTERN(int)
cahute_scsi_request_from_win32_device(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

/* ---
 * File-related internals for Win32.
 * --- */

/**
 * Win32 file state.
 *
 * @property handle
 * @property close
 */
struct cahute_win32_file_cookie {
    HANDLE handle;
    int close;
};

CAHUTE_EXTERN(void)
cahute_close_win32_file(
    cahute_context *context,
    cahute_win32_file_cookie *cookie
);

CAHUTE_EXTERN(int)
cahute_read_from_win32_file(
    cahute_context *context,
    cahute_win32_file_cookie *cookie,
    cahute_u8 *buf,
    size_t size,
    size_t *readp
);

CAHUTE_EXTERN(int)
cahute_write_to_win32_file(
    cahute_context *context,
    cahute_win32_file_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *writtenp
);

CAHUTE_EXTERN(int)
cahute_move_in_win32_file(
    cahute_context *context,
    cahute_win32_file_cookie *cookie,
    unsigned long offset,
    unsigned long *offsetp
);

#endif /* PLATFORM_WIN32_INTERNALS_H */
