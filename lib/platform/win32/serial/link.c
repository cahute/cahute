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

CAHUTE_DECLARE_TYPE(cahute_win32_serial_link_cookie)

/**
 * Win32 serial cookie.
 *
 * @property handle Handle to use for receiving and sending.
 * @property read_overlapped Overlapped object for receiving.
 * @property write_overlapped Overlapped object for sending.
 * @property received Number of received bytes in an asynchronous read
 *           or write.
 * @property read_in_progress Whether a read operation is currently in
 *           progress.
 */
struct cahute_win32_serial_link_cookie {
    HANDLE handle;
    OVERLAPPED read_overlapped;
    OVERLAPPED write_overlapped;
    DWORD received;
    DWORD read_in_progress;
};

/**
 * Close a Win32 serial or CESG link.
 *
 * @param context
 * @param cookie
 */
CAHUTE_LOCAL(void)
close_link(cahute_context *context, cahute_win32_serial_link_cookie *cookie) {
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
CAHUTE_LOCAL(int)
receive_on_link(
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
CAHUTE_LOCAL(int)
send_on_link(
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

/**
 * Set serial params on a Win32 serial link.
 *
 * @param context
 * @param cookie
 * @param flags Flags on which to set
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
set_serial_params_on_link(
    cahute_context *context,
    cahute_win32_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
) {
    DCB dcb;
    DWORD dcb_speed;

    switch (speed) {
    case 300:
        dcb_speed = CBR_300;
        break;
    case 600:
        dcb_speed = CBR_600;
        break;
    case 1200:
        dcb_speed = CBR_1200;
        break;
    case 2400:
        dcb_speed = CBR_2400;
        break;
    case 4800:
        dcb_speed = CBR_4800;
        break;
    case 9600:
        dcb_speed = CBR_9600;
        break;
    case 19200:
        dcb_speed = CBR_19200;
        break;
    case 38400:
        dcb_speed = CBR_38400;
        break;
    case 57600:
        dcb_speed = CBR_57600;
        break;
    case 115200:
        dcb_speed = CBR_115200;
        break;

    default:
        msg(context, ll_error, "Speed unsupported by Windows API: %lu", speed);
        return CAHUTE_ERROR_UNKNOWN;
    }

    memset(&dcb, 0, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(cookie->handle, &dcb)) {
        log_windows_error(context, "GetCommState", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    dcb.BaudRate = dcb_speed;
    dcb.ByteSize = 8;
    dcb.fOutxCtsFlow = 0;
    dcb.fOutxDsrFlow = 0;
    dcb.fDsrSensitivity = 0;
    dcb.fNull = 0;

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        dcb.fParity = 1;
        dcb.Parity = EVENPARITY;
        break;

    case CAHUTE_SERIAL_PARITY_ODD:
        dcb.fParity = 1;
        dcb.Parity = ODDPARITY;
        break;

    default:
        dcb.fParity = 0;
        break;
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case CAHUTE_SERIAL_STOP_ONE:
        dcb.StopBits = ONESTOPBIT;
        break;

    case CAHUTE_SERIAL_STOP_TWO:
        dcb.StopBits = TWOSTOPBITS;
        break;
    }

    dcb.fTXContinueOnXoff = 0;
    dcb.XonChar = 0x13;
    dcb.XoffChar = 0x11;
    dcb.XonLim = 0;
    dcb.XoffLim = 0;

    switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
    case CAHUTE_SERIAL_XONXOFF_ENABLE:
        dcb.fInX = 1;
        dcb.fOutX = 1;
        break;

    default:
        dcb.fInX = 0;
        dcb.fOutX = 0;
    }

    switch (flags & CAHUTE_SERIAL_DTR_MASK) {
    case CAHUTE_SERIAL_DTR_DISABLE:
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        break;

    case CAHUTE_SERIAL_DTR_ENABLE:
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
    }

    switch (flags & CAHUTE_SERIAL_RTS_MASK) {
    case CAHUTE_SERIAL_RTS_DISABLE:
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        break;

    case CAHUTE_SERIAL_RTS_ENABLE:
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        break;

    case CAHUTE_SERIAL_RTS_HANDSHAKE:
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    }

    if (!SetCommState(cookie->handle, &dcb)) {
        log_windows_error(context, "SetCommState", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/* Win32 serial link callbacks. */
CAHUTE_LOCAL_DATA(cahute_serial_link_interface)
win32_serial_link_interface = {
    "Serial (Win32)",
    (cahute_link_close_func *)&close_link,
    (cahute_link_receive_func *)&receive_on_link,
    (cahute_link_send_func *)&send_on_link,
    (cahute_link_set_serial_params_func *)&set_serial_params_on_link
};

/**
 * Open a Win32 serial link.
 *
 * @param context
 * @param open_params
 * @param name_or_path
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_open_win32_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
) {
    cahute_win32_serial_link_cookie cookie;
    HANDLE handle = INVALID_HANDLE_VALUE;
    HANDLE read_overlapped_event_handle = INVALID_HANDLE_VALUE;
    HANDLE write_overlapped_event_handle = INVALID_HANDLE_VALUE;
    COMMTIMEOUTS timeouts;
    DWORD werr;
    int err = CAHUTE_ERROR_UNKNOWN;

    handle = CreateFile(
        name_or_path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (handle == INVALID_HANDLE_VALUE)
        switch (werr = GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_DEV_NOT_EXIST:
            return CAHUTE_ERROR_NOT_FOUND;

        case ERROR_ACCESS_DENIED:
            return CAHUTE_ERROR_PRIV;

        default:
            log_windows_error(context, "CreateFile", werr);
            return CAHUTE_ERROR_UNKNOWN;
        }

    /* Read timeouts will be managed by using WaitForMultipleObjects().
     * Here we only need to configure the timeouts to return
     * immediately. */
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(handle, &timeouts)) {
        log_windows_error(context, "SetCommTimeouts", GetLastError());
        goto fail;
    }

    /* We only want events to be set if we are receiving a byte. */
    if (!SetCommMask(handle, EV_RXCHAR)) {
        log_windows_error(context, "SetCommMask", GetLastError());
        goto fail;
    }

    if (!PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        log_windows_error(context, "PurgeComm", GetLastError());
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

    memset(&cookie.read_overlapped, 0, sizeof(OVERLAPPED));
    memset(&cookie.write_overlapped, 0, sizeof(OVERLAPPED));
    cookie.handle = handle;
    cookie.read_overlapped.hEvent = read_overlapped_event_handle;
    cookie.write_overlapped.hEvent = write_overlapped_event_handle;
    cookie.read_in_progress = 0;
    cookie.received = 0;
    return cahute_open_serial_link_from_interface(
        open_params,
        &win32_serial_link_interface,
        &cookie,
        sizeof(cookie)
    );

fail:
    if (read_overlapped_event_handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);

    return err;
}
