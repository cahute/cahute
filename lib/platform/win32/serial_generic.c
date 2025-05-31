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
 * Close a Win32 serial or CESG link.
 *
 * @param context
 * @param cookie
 */
CAHUTE_EXTERN(void)
cahute_close_win32_serial_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie
) {
    if (!CancelIo(cookie->handle)) {
        DWORD werr = GetLastError();
        log_windows_error(context, "CancelIo", werr);
    }

    CloseHandle(cookie->read_overlapped.hEvent);
    CloseHandle(cookie->write_overlapped.hEvent);
    CloseHandle(cookie->handle);
}

/**
 * Receive from a Win32 serial link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer in which to receive.
 * @param capacity Capacity of the buffer.
 * @param receivedp Pointer to the received bytes count to set.
 * @param timeout Timeout; 0 for infinite.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_receive_on_win32_serial_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    BOOL ret;

    /* If a read operation is not already in progress, we want to
     * initiate it now. */
    if (!cookie->read_in_progress) {
        cookie->received = 0;
        ret = ReadFile(
            cookie->handle,
            buf,
            capacity,
            &cookie->received,
            &cookie->read_overlapped
        );

        if (!ret) {
            DWORD werr = GetLastError();

            if (werr == ERROR_IO_PENDING)
                cookie->read_in_progress = 1;
            else {
                log_windows_error(context, "ReadFile", werr);
                return CAHUTE_ERROR_UNKNOWN;
            }
        }
    }

    /* If a read operation is in progress, i.e. either if it has been
     * initiated in a previous read or if it has been initiated before
     * and has not returned immediately, we want to check on it. */
    if (cookie->read_in_progress) {
        ret = WaitForSingleObject(
            cookie->read_overlapped.hEvent,
            timeout ? timeout : INFINITE
        );
        switch (ret) {
        case WAIT_OBJECT_0:
            cookie->read_in_progress = 0;
            ret = GetOverlappedResult(
                cookie->handle,
                &cookie->read_overlapped,
                &cookie->received,
                FALSE
            );

            if (!ret) {
                DWORD werr = GetLastError();
                if (werr == ERROR_GEN_FAILURE)
                    return CAHUTE_ERROR_GONE;

                log_windows_error(context, "GetOverlappedResult", werr);
                return CAHUTE_ERROR_UNKNOWN;
            }
            break;

        case WAIT_TIMEOUT:
            /* Read will still be in progress for next time we come
             * back to this function. */
            return CAHUTE_ERROR_TIMEOUT;

        default:
            log_windows_error(context, "WaitForSingleObject", GetLastError());

            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    *receivedp = (size_t)cookie->received;
    return CAHUTE_OK;
}

/**
 * Send on a Win32 serial link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer to send.
 * @param size Size of the buffer to send.
 * @param sentp Pointer to the written bytes count to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_send_on_win32_serial_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
) {
    DWORD sent;
    BOOL ret;

    ret =
        WriteFile(cookie->handle, buf, size, &sent, &cookie->write_overlapped);
    if (!ret) {
        DWORD werr = GetLastError();

        if (werr == ERROR_IO_PENDING) {
            ret =
                WaitForSingleObject(cookie->write_overlapped.hEvent, INFINITE);
            switch (ret) {
            case WAIT_OBJECT_0:
                ret = GetOverlappedResult(
                    cookie->handle,
                    &cookie->write_overlapped,
                    &sent,
                    FALSE
                );
                if (!ret) {
                    werr = GetLastError();
                    if (werr == ERROR_GEN_FAILURE)
                        return CAHUTE_ERROR_GONE;

                    log_windows_error(context, "GetOverlappedResult", werr);
                    return CAHUTE_ERROR_UNKNOWN;
                }
                break;

            default:
                log_windows_error(
                    context,
                    "WaitForSingleObject",
                    GetLastError()
                );
                return CAHUTE_ERROR_UNKNOWN;
            }
        } else {
            log_windows_error(context, "WriteFile", werr);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    *sentp = (size_t)sent;
    return CAHUTE_OK;
}
