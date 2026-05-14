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

#include "internals.h"

CAHUTE_DECLARE_TYPE(cahute_win32_winusb_bulk_link_cookie)

/**
 * Win32 WinUSB serial over bulk link cookie.
 *
 * @property handle WinUSB handle.
 * @property device_handle Device handle to close.
 * @property winusb WinUSB interface.
 * @property read_overlapped OVERLAPPED object for read operations.
 * @property write_overlapped OVERLAPPED object for write operations.
 * @property received Where to store the number of received bytes.
 * @property read_in_progress Whether a read operation is currently in
 *           progress.
 * @property read_pipe Pipe from which to read data.
 * @property write_pipe Pipe to which to write data.
 */
struct cahute_win32_winusb_bulk_link_cookie {
    void *handle;
    HANDLE device_handle;
    cahute_win32_winusb *winusb;
    OVERLAPPED read_overlapped;
    OVERLAPPED write_overlapped;
    ULONG received;
    int read_in_progress;
    UCHAR read_pipe;
    UCHAR write_pipe;
};

/**
 * Close a Win32 WinUSB link.
 *
 * @param context
 * @param cookie
 */
CAHUTE_LOCAL(void)
close_link(
    cahute_context *context,
    cahute_win32_winusb_bulk_link_cookie *cookie
) {
    (*cookie->winusb->abort_pipe)(cookie->handle, cookie->read_pipe);
    CloseHandle(cookie->read_overlapped.hEvent);
    CloseHandle(cookie->write_overlapped.hEvent);
    (*cookie->winusb->free)(cookie->handle);
    CloseHandle(cookie->device_handle);
}

/**
 * Receive from a Win32 WinUSB link.
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
    cahute_win32_winusb_bulk_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    BOOL ret;
    DWORD dreceived;

    /* If a read operation is not already in progress, we want to
     * initiate it now. */
    if (!cookie->read_in_progress) {
        cookie->received = 0;
        ret = (*cookie->winusb->read_pipe)(
            cookie->handle,
            cookie->read_pipe,
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
                log_windows_error(context, "WinUsb_ReadPipe", werr);
                return CAHUTE_ERROR_UNKNOWN;
            }
        } else {
            *receivedp = (size_t)cookie->received;
            return CAHUTE_OK;
        }
    }

    /* A read operation is in progress, i.e. either if it has been
     * initiated in a previous read or if it has been initiated before
     * and has not returned immediately, we want to check on it. */
    ret = WaitForSingleObject(
        cookie->read_overlapped.hEvent,
        timeout ? timeout : INFINITE
    );
    switch (ret) {
    case WAIT_OBJECT_0:
        cookie->read_in_progress = 0;
        ret = (*cookie->winusb->get_overlapped_result)(
            cookie->handle,
            &cookie->read_overlapped,
            &dreceived,
            FALSE
        );

        if (!ret) {
            DWORD werr = GetLastError();
            if (werr == ERROR_GEN_FAILURE)
                return CAHUTE_ERROR_GONE;

            log_windows_error(context, "WinUsb_GetOverlappedResult", werr);
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

    *receivedp = (size_t)dreceived;
    return CAHUTE_OK;
}

/**
 * Send on a Win32 WinUSB link.
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
    cahute_win32_winusb_bulk_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
) {
    ULONG lsent;
    BOOL ret;

    ret = (*cookie->winusb->write_pipe)(
        cookie->handle,
        cookie->write_pipe,
        buf,
        size,
        &lsent,
        &cookie->write_overlapped
    );
    if (ret)
        *sentp = (size_t)lsent;
    else {
        DWORD werr = GetLastError();
        DWORD dsent;

        if (werr == ERROR_IO_PENDING) {
            ret =
                WaitForSingleObject(cookie->write_overlapped.hEvent, INFINITE);
            switch (ret) {
            case WAIT_OBJECT_0:
                ret = (*cookie->winusb->get_overlapped_result)(
                    cookie->handle,
                    &cookie->write_overlapped,
                    &dsent,
                    FALSE
                );
                if (!ret) {
                    werr = GetLastError();
                    if (werr == ERROR_GEN_FAILURE)
                        return CAHUTE_ERROR_GONE;

                    log_windows_error(
                        context,
                        "WinUsb_GetOverlappedResult",
                        werr
                    );
                    return CAHUTE_ERROR_UNKNOWN;
                }

                *sentp = (size_t)dsent;
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
            log_windows_error(context, "WinUsb_WritePipe", werr);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    return CAHUTE_OK;
}

CAHUTE_LOCAL_DATA(cahute_serial_over_usb_bulk_link_interface)
winusb_link_interface = {
    "Serial over USB bulk (Win32 WinUSB)",
    (cahute_link_close_func *)close_link,
    (cahute_link_receive_func *)receive_on_link,
    (cahute_link_send_func *)send_on_link
};

/**
 * Find BULK IN and BULK OUT pipe identifiers for a WinUSB device.
 *
 * @param context Context in which the function is run.
 * @param handle WinUSB handle for which to get the pipe identifiers.
 * @param winusb WinUSB interface.
 * @param read_pipep Pointer to set to the found read pipe.
 * @param write_pipep Pointer to set to the found write pipe.
 */
CAHUTE_LOCAL(int)
find_bulk_pipes(
    cahute_context *context,
    void *handle,
    cahute_win32_winusb *winusb,
    UCHAR *read_pipep,
    UCHAR *write_pipep
) {
    USB_INTERFACE_DESCRIPTOR interface_descriptor;
    struct cahute_winusb_pipe_information pipe;
    BOOL ret;
    int i, read_pipe = -1, write_pipe = -1;

    ret =
        (*winusb->query_interface_settings)(handle, 0, &interface_descriptor);
    if (!ret) {
        log_windows_error(
            context,
            "WinUsb_QueryInterfaceSettings",
            GetLastError()
        );
        return CAHUTE_ERROR_UNKNOWN;
    }

    for (i = 0; i < interface_descriptor.bNumEndpoints; i++) {
        ret = (*winusb->query_pipe)(handle, 0, i, &pipe);
        if (!ret) {
            msg(context, ll_error, "Unable to query pipe #%d:", i);
            log_windows_error(context, "WinUsb_QueryPipe", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (pipe.PipeType != 2 /* UsbdPipeTypeBulk */)
            continue;

        if (pipe.PipeId & 0x80) { /* IN */
            if (read_pipe >= 0) {
                msg(context,
                    ll_error,
                    "At least 2 read pipes found: 0x%02X and 0x%02X",
                    read_pipe,
                    pipe.PipeId);
                return CAHUTE_ERROR_UNKNOWN;
            }

            read_pipe = pipe.PipeId;
        } else { /* OUT */
            if (write_pipe >= 0) {
                msg(context,
                    ll_error,
                    "At least 2 write pipes found: 0x%02X and 0x%02X",
                    write_pipe,
                    pipe.PipeId);
                return CAHUTE_ERROR_UNKNOWN;
            }

            write_pipe = pipe.PipeId;
        }
    }

    if (read_pipe < 0) {
        msg(context, ll_error, "No read pipe found.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (write_pipe < 0) {
        msg(context, ll_error, "No write pipe found.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    *read_pipep = read_pipe;
    *write_pipep = write_pipe;
    return CAHUTE_OK;
}

/**
 * Open a Win32 CESG link.
 *
 * @param context
 * @param open_params
 * @param path Path to the device interface.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_open_win32_winusb_bulk_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path
) {
    cahute_win32_winusb_bulk_link_cookie cookie;
    void *handle = NULL;
    HANDLE dev_handle = INVALID_HANDLE_VALUE;
    HANDLE read_overlapped_event_handle = INVALID_HANDLE_VALUE;
    HANDLE write_overlapped_event_handle = INVALID_HANDLE_VALUE;
    cahute_win32_winusb *winusb = NULL;
    int err = 0;

    err = cahute_get_win32_winusb(context, &winusb);
    if (err)
        return err;

    dev_handle = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );
    if (dev_handle == INVALID_HANDLE_VALUE) {
        DWORD werr = GetLastError();

        if (werr == ERROR_ACCESS_DENIED)
            err = CAHUTE_ERROR_PRIV;
        else
            log_windows_error(context, "CreateFileA", werr);

        goto fail;
    }

    if (!(*winusb->initialize)(dev_handle, &handle)) {
        log_windows_error(context, "WinUSB_Initialize", GetLastError());
        goto fail;
    }

    err = find_bulk_pipes(
        context,
        handle,
        winusb,
        &cookie.read_pipe,
        &cookie.write_pipe
    );
    if (err)
        goto fail;

    /* Device enabling control flow. */
    {
        struct cahute_winusb_setup_packet setup_packet;
        BYTE buf[10];
        ULONG transferred = 0;
        BOOL ret;

        setup_packet.bmRequestType = 0x41;
        setup_packet.bRequest = 0x01;
        setup_packet.wValue = 0;
        setup_packet.wIndex = 0;
        setup_packet.wLength = 0;

        ret = (*winusb->control_transfer)(
            handle,
            setup_packet,
            buf,
            sizeof(buf),
            &transferred,
            NULL
        );
        if (!ret) {
            log_windows_error(
                context,
                "WinUsb_ControlTransfer",
                GetLastError()
            );
            goto fail;
        }
    }

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
    cookie.device_handle = dev_handle;
    cookie.winusb = winusb;

    memset(&cookie.read_overlapped, 0, sizeof(OVERLAPPED));
    memset(&cookie.write_overlapped, 0, sizeof(OVERLAPPED));
    cookie.read_overlapped.hEvent = read_overlapped_event_handle;
    cookie.write_overlapped.hEvent = write_overlapped_event_handle;
    cookie.read_in_progress = 0;

    return cahute_open_serial_over_usb_bulk_link_from_interface(
        open_params,
        &winusb_link_interface,
        &cookie,
        sizeof(cookie)
    );

fail:
    if (read_overlapped_event_handle != INVALID_HANDLE_VALUE)
        CloseHandle(read_overlapped_event_handle);
    if (handle)
        (*winusb->free)(handle);
    if (dev_handle != INVALID_HANDLE_VALUE)
        CloseHandle(dev_handle);

    if (!err)
        err = CAHUTE_ERROR_UNKNOWN;
    return err;
}
