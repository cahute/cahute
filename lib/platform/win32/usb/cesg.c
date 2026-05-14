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

CAHUTE_DECLARE_TYPE(cahute_win32_cesg_link_cookie)

/**
 * Win32 CESG link cookie.
 *
 * @property handle Handle to use for receiving and sending.
 * @property read_overlapped Overlapped object for receiving.
 * @property write_overlapped Overlapped object for sending.
 * @property received Number of received bytes in an asynchronous read
 *           or write.
 * @property read_in_progress Whether a read operation is currently in
 *           progress.
 * @property max_read_capacity Maximum read capacity.
 */
struct cahute_win32_cesg_link_cookie {
    HANDLE handle;
    OVERLAPPED read_overlapped;
    OVERLAPPED write_overlapped;
    DWORD received;
    DWORD read_in_progress;
    size_t max_read_capacity;
};

/**
 * Close a Win32 CESG link.
 *
 * @param context
 * @param cookie
 */
CAHUTE_LOCAL(void)
close_link(cahute_context *context, cahute_win32_cesg_link_cookie *cookie) {
    if (!CancelIo(cookie->handle)) {
        DWORD werr = GetLastError();
        log_windows_error(context, "CancelIo", werr);
    }

    CloseHandle(cookie->read_overlapped.hEvent);
    CloseHandle(cookie->write_overlapped.hEvent);
    CloseHandle(cookie->handle);
}

/**
 * Receive from a Win32 CESG link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer in which to receive.
 * @param capacity Capacity of the buffer.
 * @param receivedp Pointer to the received bytes count to set.
 * @param timeout Timeout; 0 for infinite.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
receive_on_link(
    cahute_context *context,
    cahute_win32_cesg_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    BOOL ret;

    /* If a read operation is not already in progress, we want to
     * initiate it now. */
    if (!cookie->read_in_progress) {
        size_t max_capacity = cookie->max_read_capacity;

        /* Requesting with a capacity too high (e.g. 32768 bytes) results in a
         * 0x00000057 (ERROR_INVALID_PARAMETER) error with legacy CESG502.
         * Therefore, we want to limit to that size. */
        if (max_capacity && capacity > max_capacity)
            capacity = max_capacity;

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
 * Send on a Win32 CESG link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer to send.
 * @param size Size of the buffer to send.
 * @param sentp Pointer to the written bytes count to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
send_on_link(
    cahute_context *context,
    cahute_win32_cesg_link_cookie *cookie,
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

CAHUTE_LOCAL_DATA(cahute_serial_over_usb_bulk_link_interface)
cesg_link_interface = {
    "Serial over USB bulk (Win32 CESG)",
    (cahute_link_close_func *)close_link,
    (cahute_link_receive_func *)receive_on_link,
    (cahute_link_send_func *)send_on_link
};


/**
 * Open a Win32 CESG link.
 *
 * @param context
 * @param open_params
 * @param path Path to the device interface.
 * @param max_read_capacity Maximum buffer size to use when reading.
 *        This parameter is necessary since older versions of the driver
 *        do not support the read capacity of Cahute (CESG 1.0.0.0 does not
 *        support reading 32768 bytes at once). Set to 0 for no limit.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_open_win32_cesg_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path,
    size_t max_read_capacity
) {
    HANDLE handle = INVALID_HANDLE_VALUE;
    HANDLE read_overlapped_event_handle = INVALID_HANDLE_VALUE;
    HANDLE write_overlapped_event_handle = INVALID_HANDLE_VALUE;
    cahute_win32_cesg_link_cookie cookie;
    int err = CAHUTE_ERROR_UNKNOWN;

    msg(context,
        ll_info,
        "Opening a handle to the following device interface: %s",
        path);

    /* The device is a USB device opened using CESG502. */
    handle = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD werr = GetLastError();

        if (werr == ERROR_ACCESS_DENIED)
            err = CAHUTE_ERROR_PRIV;
        else
            log_windows_error(context, "CreateFileA", werr);

        goto fail;
    }

    /* Create the overlapped events. */
    read_overlapped_event_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (read_overlapped_event_handle == INVALID_HANDLE_VALUE) {
        log_windows_error(context, "CreateEvent", GetLastError());
        goto fail;
    }

    write_overlapped_event_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (write_overlapped_event_handle == INVALID_HANDLE_VALUE) {
        log_windows_error(context, "CreateEvent", GetLastError());
        goto fail;
    }

    cookie.handle = handle;
    cookie.read_in_progress = 0;
    cookie.received = 0;
    memset(&cookie.read_overlapped, 0, sizeof(OVERLAPPED));
    memset(&cookie.write_overlapped, 0, sizeof(OVERLAPPED));
    cookie.read_overlapped.hEvent = read_overlapped_event_handle;
    cookie.write_overlapped.hEvent = write_overlapped_event_handle;
    cookie.max_read_capacity = max_read_capacity;

    return cahute_open_serial_over_usb_bulk_link_from_interface(
        open_params,
        &cesg_link_interface,
        &cookie,
        sizeof(cookie)
    );

fail:
    if (read_overlapped_event_handle != INVALID_HANDLE_VALUE)
        CloseHandle(read_overlapped_event_handle);
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);

    return err;
}
