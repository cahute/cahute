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
#include <ntddscsi.h>

CAHUTE_DECLARE_TYPE(cahute_win32_ums_link_cookie)

/**
 * Win32 UMS link cookie.
 *
 * @property handle
 */
struct cahute_win32_ums_link_cookie {
    HANDLE handle;
};

/**
 * Close a Win32 UMS link.
 *
 * @param context
 * @param cookie
 */
CAHUTE_LOCAL(void)
close_link(cahute_context *context, cahute_win32_ums_link_cookie *cookie) {
    CloseHandle(cookie->handle);
}

/**
 * Get the storage device properties for the device.
 *
 * @param context
 * @param cookie
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
get_storage_device_properties(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie
) {
    STORAGE_PROPERTY_QUERY req;
    STORAGE_DESCRIPTOR_HEADER header_desc;
    cahute_u8 *desc_buf = NULL,
              short_desc_buf[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 128];
    char const *raw_desc_buf;
    STORAGE_DEVICE_DESCRIPTOR *desc = NULL;
    DWORD werr, wret, wcnt;
    int err = CAHUTE_ERROR_UNKNOWN;

    req.PropertyId = StorageDeviceProperty;
    req.QueryType = PropertyStandardQuery;

    wret = DeviceIoControl(
        cookie->handle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &req,
        sizeof(req),
        &header_desc,
        sizeof(header_desc),
        &wcnt,
        NULL
    );

    /* Here, we expect the call to be successful even though the
     * buffer is technically to small. */
    if (!wret) {
        werr = GetLastError();

        if (werr == ERROR_INVALID_CATEGORY)
            CAHUTE_RETURN_IMPL(
                context,
                "Cannot request storage device properties."
            );

        msg(context,
            ll_error,
            "Failed obtaining storage device properties' header:");
        log_windows_error(context, "DeviceIoControl", werr);
        goto fail;
    }

    if (wcnt != sizeof(header_desc)) {
        msg(context,
            ll_error,
            "Size of obtained header (%luo) doesn't match expected (%luo)",
            wcnt,
            sizeof(header_desc));
        goto fail;
    }

    if (header_desc.Size <= sizeof(*desc)) {
        msg(context,
            ll_error,
            "Size of descriptor (%luo) is less than expected (at least %luo)",
            header_desc.Size,
            sizeof(*desc));
        goto fail;
    }

    if (header_desc.Size <= sizeof(short_desc_buf)) {
        desc = (void *)short_desc_buf;
        raw_desc_buf = (void *)short_desc_buf;
    } else {
        desc_buf = malloc(header_desc.Size);
        if (!desc_buf) {
            err = CAHUTE_ERROR_ALLOC;
            goto fail;
        }

        desc = (void *)desc_buf;
        raw_desc_buf = (void *)desc_buf;
    }

    wret = DeviceIoControl(
        cookie->handle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &req,
        sizeof(req),
        desc,
        header_desc.Size,
        &wcnt,
        NULL
    );

    if (!wret) {
        werr = GetLastError();

        msg(context, ll_error, "Failed obtaining storage device properties:");
        log_windows_error(context, "DeviceIoControl", werr);
        goto fail;
    }

    msg(context, ll_debug, "Obtained storage device properties:");
    msg(context, ll_debug, "  Device Type: %d", desc->DeviceType);
    msg(context,
        ll_debug,
        "  Device Type Modifier: %d",
        desc->DeviceTypeModifier);
    msg(context,
        ll_debug,
        "  Removable Media: %s",
        desc->RemovableMedia ? "yes" : "no");
    msg(context,
        ll_debug,
        "  Command Queueing: %s",
        desc->CommandQueueing ? "yes" : "no");
    if (desc->VendorIdOffset)
        msg(context,
            ll_debug,
            "  Vendor ID: %s",
            &raw_desc_buf[desc->VendorIdOffset]);
    if (desc->ProductIdOffset)
        msg(context,
            ll_debug,
            "  Product ID: %s",
            &raw_desc_buf[desc->ProductIdOffset]);
    if (desc->ProductRevisionOffset)
        msg(context,
            ll_debug,
            "  Product Revision: %s",
            &raw_desc_buf[desc->ProductRevisionOffset]);
    if (desc->SerialNumberOffset)
        msg(context,
            ll_debug,
            "  Serial Number: %s",
            &raw_desc_buf[desc->SerialNumberOffset]);

    err = CAHUTE_OK;

fail:
    if (desc_buf)
        free(desc_buf);

    return err;
}

/**
 * Get the storage adapter properties for the device.
 *
 * @param context
 * @param cookie
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
get_storage_adapter_properties(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie
) {
    STORAGE_PROPERTY_QUERY req;
    STORAGE_ADAPTER_DESCRIPTOR desc;
    DWORD wret, wcnt, werr;

    req.PropertyId = StorageAdapterProperty;
    req.QueryType = PropertyStandardQuery;

    wret = DeviceIoControl(
        cookie->handle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &req,
        sizeof(req),
        &desc,
        sizeof(desc),
        &wcnt,
        NULL
    );

    if (!wret) {
        werr = GetLastError();

        if (werr == ERROR_INVALID_CATEGORY)
            CAHUTE_RETURN_IMPL(
                context,
                "Cannot request storage device properties."
            );

        msg(context, ll_error, "Failed obtaining storage adapter properties:");
        log_windows_error(context, "DeviceIoControl", werr);
        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(context, ll_debug, "Obtained storage adapter properties:");
    msg(context,
        ll_debug,
        "  Maximum Transfer Length: %lu",
        desc.MaximumTransferLength);
    msg(context, ll_debug, "  Alignment mask: 0x%08lX", desc.AlignmentMask);

    return CAHUTE_OK;
}

/**
 * Emit an SCSI request on a Win32 link.
 *
 * @param context
 * @param cookie
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param buf Optional data buffer to either send or receive.
 * @param buf_size Size of the data or capacity of the data buffer.
 * @param is_send Whether the data should be sent or received.
 * @param statusp Pointer to the SCSI status to set to the received one.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
scsi_request(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int is_send,
    int *statusp
) {
    SCSI_PASS_THROUGH_DIRECT req;
    DWORD wret, werr, wcnt;

    memset(&req, 0, sizeof(SCSI_PASS_THROUGH_DIRECT));
    req.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    req.TimeOutValue = 30;
    req.CdbLength = command_size;
    memcpy(req.Cdb, command, command_size);

    if (!is_send) {
        req.DataIn = SCSI_IOCTL_DATA_IN;
        req.DataBuffer = buf;
        req.DataTransferLength = buf_size;
    } else if (buf_size) {
        req.DataIn = SCSI_IOCTL_DATA_OUT;
        req.DataBuffer = buf;
        req.DataTransferLength = buf_size;
    } else {
        req.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
        req.DataBuffer = NULL;
        req.DataTransferLength = 0;
    }

    wret = DeviceIoControl(
        cookie->handle,
        IOCTL_SCSI_PASS_THROUGH_DIRECT,
        &req,
        sizeof(req),
        &req,
        sizeof(req),
        &wcnt,
        NULL
    );

    if (!wret) {
        werr = GetLastError();

        if (werr == ERROR_SEM_TIMEOUT || werr == ERROR_DEV_NOT_EXIST
            || werr == ERROR_NO_SUCH_DEVICE)
            return CAHUTE_ERROR_GONE;

        log_windows_error(context, "DeviceIoControl", werr);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *statusp = (int)req.ScsiStatus;
    return CAHUTE_OK;
}

/**
 * Emit an SCSI request with outgoing data on a Win32 link.
 *
 * @param context
 * @param cookie
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param data
 * @param data_size
 * @param statusp Pointer to the SCSI status to set to the received one.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
scsi_request_to(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
) {
    return scsi_request(
        context,
        cookie,
        command,
        command_size,
        (cahute_u8 *)data, /* Explicit removal of const. */
        data_size,
        1,
        statusp
    );
}

/**
 * Emit an SCSI request with incoming data on a Win32 link.
 *
 * @param context
 * @param cookie
 * @param command
 * @param command_size
 * @param buf
 * @param buf_size
 * @param statusp
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
scsi_request_from(
    cahute_context *context,
    cahute_win32_ums_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
) {
    return scsi_request(
        context,
        cookie,
        command,
        command_size,
        buf,
        buf_size,
        0,
        statusp
    );
}

/* Win32 UMS link interface. */
CAHUTE_LOCAL_DATA(cahute_ums_link_interface)
ums_link_interface = {
    "UMS (Win32)",
    (cahute_link_close_func *)&close_link,
    (cahute_link_scsi_request_to_func *)&scsi_request_to,
    (cahute_link_scsi_request_from_func *)&scsi_request_from
};


/**
 * Open a Win32 UMS link.
 *
 * @param context
 * @param open_params
 * @param path Path to the device interface.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_open_win32_ums_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path
) {
    HANDLE handle = INVALID_HANDLE_VALUE;
    struct cahute_win32_ums_link_cookie cookie;
    int err = CAHUTE_ERROR_UNKNOWN;

    /* The device is a volume on which we should make synchronous
     * SCSI requests using DeviceIoControl(). */
    handle = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD werr = GetLastError();

        if (werr == ERROR_ACCESS_DENIED)
            err = CAHUTE_ERROR_PRIV;
        else
            log_windows_error(context, "CreateFileA", werr);

        return err;
    }

    cookie.handle = handle;

    err = get_storage_device_properties(context, &cookie);
    if (err && err != CAHUTE_ERROR_IMPL) {
        close_link(context, &cookie);
        return err;
    }

    err = get_storage_adapter_properties(context, &cookie);
    if (err && err != CAHUTE_ERROR_IMPL) {
        close_link(context, &cookie);
        return err;
    }

    return cahute_open_ums_link_from_interface(
        open_params,
        &ums_link_interface,
        &cookie,
        sizeof(cookie)
    );
}
