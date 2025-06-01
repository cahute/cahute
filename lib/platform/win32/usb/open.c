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
#include <cfgmgr32.h>
#include <initguid.h>
#if defined(__MINGW32__) || defined(__MINGW64__)
# include <ddk/wdmguid.h>
#else
# include <wdmguid.h>
#endif
#include <usbiodef.h>
#include <devguid.h>
#define HEXDIGIT(C) \
    ((C) >= 'a' ? (C) - 'a' + 10 : (C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')

#define TYPE_CESG 1
#define TYPE_UMS  2

/**
 * Decode a GUID from a string.
 *
 * An example string is "{4d36e967-e325-11ce-bfc1-08002be10318}".
 *
 * @param context Context in which the function is run.
 * @param guid Pointer to the GUID to set.
 * @param raw Raw GUID to parse.
 * @return 1 if parsing has failed, 0 otherwise.
 */
CAHUTE_INLINE(int)
decode_guid(cahute_context *context, GUID *guid, char const *raw) {
    if (raw[0] != '{' || !isxdigit(raw[1]) || !isxdigit(raw[2])
        || !isxdigit(raw[3]) || !isxdigit(raw[4]) || !isxdigit(raw[5])
        || !isxdigit(raw[6]) || !isxdigit(raw[7]) || !isxdigit(raw[8])
        || raw[9] != '-' || !isxdigit(raw[10]) || !isxdigit(raw[11])
        || !isxdigit(raw[12]) || !isxdigit(raw[13]) || raw[14] != '-'
        || !isxdigit(raw[15]) || !isxdigit(raw[16]) || !isxdigit(raw[17])
        || !isxdigit(raw[18]) || raw[19] != '-' || !isxdigit(raw[20])
        || !isxdigit(raw[21]) || !isxdigit(raw[22]) || !isxdigit(raw[23])
        || raw[24] != '-' || !isxdigit(raw[25]) || !isxdigit(raw[26])
        || !isxdigit(raw[27]) || !isxdigit(raw[28]) || !isxdigit(raw[29])
        || !isxdigit(raw[30]) || !isxdigit(raw[31]) || !isxdigit(raw[32])
        || !isxdigit(raw[33]) || !isxdigit(raw[34]) || !isxdigit(raw[35])
        || !isxdigit(raw[36]) || raw[37] != '}') {
        msg(context, ll_error, "Unable to decode GUID: %s", raw);
        return 1;
    }

    guid->Data1 = cahute_htole32(
        (HEXDIGIT(raw[1]) << 28) | (HEXDIGIT(raw[2]) << 24)
        | (HEXDIGIT(raw[3]) << 20) | (HEXDIGIT(raw[4]) << 16)
        | (HEXDIGIT(raw[5]) << 12) | (HEXDIGIT(raw[6]) << 8)
        | (HEXDIGIT(raw[7]) << 4) | HEXDIGIT(raw[8])
    );
    guid->Data2 = cahute_htole16(
        (HEXDIGIT(raw[10]) << 12) | (HEXDIGIT(raw[11]) << 8)
        | (HEXDIGIT(raw[12]) << 4) | HEXDIGIT(raw[13])
    );
    guid->Data3 = cahute_htole16(
        (HEXDIGIT(raw[15]) << 12) | (HEXDIGIT(raw[16]) << 8)
        | (HEXDIGIT(raw[17]) << 4) | HEXDIGIT(raw[18])
    );
    guid->Data4[0] = (HEXDIGIT(raw[20]) << 4) | HEXDIGIT(raw[21]);
    guid->Data4[1] = (HEXDIGIT(raw[22]) << 4) | HEXDIGIT(raw[23]);

    /* We skip ``raw[24]`` because it's a dash. */
    guid->Data4[2] = (HEXDIGIT(raw[25]) << 4) | HEXDIGIT(raw[26]);
    guid->Data4[3] = (HEXDIGIT(raw[27]) << 4) | HEXDIGIT(raw[28]);
    guid->Data4[4] = (HEXDIGIT(raw[29]) << 4) | HEXDIGIT(raw[30]);
    guid->Data4[5] = (HEXDIGIT(raw[31]) << 4) | HEXDIGIT(raw[32]);
    guid->Data4[6] = (HEXDIGIT(raw[33]) << 4) | HEXDIGIT(raw[34]);
    guid->Data4[7] = (HEXDIGIT(raw[35]) << 4) | HEXDIGIT(raw[36]);
    return 0;
}

/**
 * Find a volume interface associated with the provided device identifier.
 *
 * @param context Context in which the function is run.
 * @param path Path to the device interface to fill.
 * @param path_size Size of the path.
 * @param device_id Device identifier.
 * @param guid Device interface GUID to look for.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
find_win32_interface(
    cahute_context *context,
    char *path,
    size_t path_size,
    char *device_id,
    LPGUID guid
) {
    DWORD property_size = 0;
    CONFIGRET cret;

    cret = CM_Get_Device_Interface_List_SizeA(
        &property_size,
        guid,
        device_id,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
    );
    if (cret != CR_SUCCESS) {
        msg(context,
            ll_error,
            "CM_Get_Device_Interface_List_SizeA returned error "
            "0x%08lX.",
            cret);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (property_size > path_size) {
        /* Relevant device interfaces encountered in the wild do
         * not have such a big device interface name, we can skip
         * the entry. */
        return CAHUTE_ERROR_SIZE;
    }

    cret = CM_Get_Device_Interface_ListA(
        guid,
        device_id,
        path,
        property_size,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
    );
    if (cret != CR_SUCCESS) {
        msg(context,
            ll_error,
            "CM_Get_Device_Interface_ListA returned error "
            "0x%08lX.",
            cret);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!path[0]) {
        /* Missing at least one interface, we want to ignore the
         * current USB device. */
        return CAHUTE_ERROR_NOT_FOUND;
    }

    return CAHUTE_OK;
}

/**
 * Find a USB device for the provided interface type.
 *
 * We use the more portable CfgMgr32 API rather than SetupApi.
 *
 * The full extent of the Unified Device Property Model is not available
 * until Windows Vista, and we aim at keeping Windows XP compatibility,
 * so we use registry properties on devices, and start of by querying devices
 * first rather than device interfaces first, since querying a device from
 * a device interface is a device interface property, which is not available
 * on Windows XP.
 *
 * The provided physical port was obtained using ``libusb_get_port_number``,
 * which is set to the device address on Windows. We can find out the USB
 * device by checking that the bus type (``CM_DRP_BUSTYPEGUID``) is USB,
 * and that the device address (``CM_DRP_ADDRESS``) corresponds to the
 * libusb port number.
 *
 * For volumes, while on Windows XP the volume is a child device to the
 * USB device, on Windows 11 the volume is in a completely different tree,
 * being attached to the volume manager (volmgr). However, on both,
 * we can use Bus Relations to get the disk drive from the USB device,
 * then the volume from the disk drive.
 *
 * This function has the following steps:
 *
 *   Step 1. Find the USB device corresponding to the calculator.
 *   Step 2. If the loaded driver is CESG, get the USB device interface path
 *           corresponding to the USB device.
 *   Step 3. Get the disk drive device corresponding to the USB device through
 *           Bus Relations.
 *   Step 4. Get the volume device corresponding to the disk drive device
 *           through Bus Relations.
 *   Step 5. Get the volume device interface path corresponding to the
 *           volume device.
 *
 * @param context Context in which the function is run.
 * @param path Path to the device interface to fill.
 * @param path_size Size of the path.
 * @param typep Type of transport to establish on the interface.
 * @param addr Physical port of the device.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
find_win32_usb_device(
    cahute_context *context,
    char *path,
    size_t path_size,
    int *typep,
    DWORD addr
) {
    DEVINST device_instance;
    char *usb_device_id_list = NULL, *usb_device_id;
    BYTE property[64];
    char disk_drive_id_buf[300], *disk_drive_id;
    char volume_id_buf[300], *volume_id;
    ULONG property_type = 0;
    ULONG property_size = 0;
    CONFIGRET cret;
    GUID guid;
    int err = CAHUTE_ERROR_UNKNOWN;

    /* ---
     * Step 1. Find the USB device corresponding to the calculator.
     * ---
     * The final device identifier will be accessible through
     * ``usb_device_id``.
     *
     * Since the device was found using libusb, not finding it here results
     * rightfully in a CAHUTE_ERROR_UNKNOWN. */

    cret = CM_Get_Device_ID_List_SizeA(
        &property_size,
        NULL,
        CM_GETIDLIST_FILTER_NONE
    );
    if (cret != CR_SUCCESS) {
        msg(context,
            ll_error,
            "CM_Get_Device_ID_List_SizeA returned error 0x%08lX.",
            cret);
        goto fail;
    }

    usb_device_id_list =
        (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, property_size);
    if (!usb_device_id_list) {
        log_windows_error(context, "HeapAlloc", GetLastError());
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    cret = CM_Get_Device_ID_ListA(
        NULL,
        usb_device_id_list,
        property_size,
        CM_GETIDLIST_FILTER_NONE
    );
    if (cret != CR_SUCCESS) {
        msg(context,
            ll_error,
            "CM_Get_Device_ID_ListA returned error 0x%08lX.",
            cret);
        goto fail;
    }

    for (usb_device_id = usb_device_id_list; *usb_device_id;
         usb_device_id += strlen(usb_device_id) + 1) {
        /* Get the device behind the interface. */
        cret = CM_Locate_DevNodeA(
            &device_instance,
            usb_device_id,
            CM_LOCATE_DEVNODE_NORMAL
        );
        if (cret == CR_NO_SUCH_DEVINST)
            continue;

        if (cret != CR_SUCCESS) {
            msg(context,
                ll_error,
                "CM_Locate_DevNodeA returned error 0x%08lX.",
                cret);
            goto fail;
        }

        /* We want to check that the device or any of its parents is actually
         * the USB device we're looking for. */
        property_size = sizeof(property);
        cret = CM_Get_DevNode_Registry_PropertyA(
            device_instance,
            CM_DRP_BUSTYPEGUID,
            &property_type,
            (PBYTE)property,
            &property_size,
            0
        );
        if (cret == CR_NO_SUCH_VALUE)
            continue; /* Virtual device, we need to check the parent. */

        if (cret != CR_SUCCESS) {
            msg(context,
                ll_error,
                "CM_Get_DevNode_Registry_PropertyA with property "
                "CM_DRP_BUSTYPEGUID returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != REG_BINARY
            || property_size != sizeof(GUID_BUS_TYPE_USB)) {
            msg(context,
                ll_warn,
                "Unexpected type 0x%08lX or size %luo for bus type key.",
                property_type,
                property_size);
            goto fail;
        }

        if (memcmp(property, &GUID_BUS_TYPE_USB, sizeof(GUID_BUS_TYPE_USB))) {
            /* The bus type is not USB, we do not have the
             * correct device. */
            continue;
        }

        /* Get the device address, to check if it corresponds to the
         * address we have previously found. */
        property_size = sizeof(property);
        cret = CM_Get_DevNode_Registry_PropertyA(
            device_instance,
            CM_DRP_ADDRESS,
            &property_type,
            (PBYTE)property,
            &property_size,
            0
        );
        if (cret != CR_SUCCESS) {
            msg(context,
                ll_error,
                "CM_Get_DevNode_Registry_PropertyA with property "
                "CM_DRP_ADDRESS returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != REG_DWORD) {
            msg(context,
                ll_warn,
                "Unexpected type 0x%08lX for device address key.",
                property_type);
            goto fail;
        }

        if (*(DWORD *)property != addr)
            continue;

        /* ---
         * Step 2. If the loaded driver is CESG, get the USB device interface
         *         path corresponding to the USB device.
         * ---
         * We use the "Service" interface property to identify the driver,
         * since CESG502 uses a driver key that is normally forbidden to
         * independent hardware vendors (IHVs). */

        property_size = sizeof(property);
        cret = CM_Get_DevNode_Registry_PropertyA(
            device_instance,
            CM_DRP_SERVICE,
            &property_type,
            (PBYTE)property,
            &property_size,
            0
        );
        if (cret == CR_NO_SUCH_VALUE)
            continue; /* No driver installed. */

        if (cret != CR_SUCCESS) {
            msg(context,
                ll_error,
                "CM_Get_DevNode_Registry_PropertyA returned error "
                "0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type == REG_SZ && property_size == 6
            && !memcmp(property, "PVUSB", 6)) {
            /* The device is handled by the CESG driver. */
            *typep = TYPE_CESG;
            err = find_win32_interface(
                context,
                path,
                path_size,
                usb_device_id,
                (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE
            );

            goto fail;
        }

        *typep = TYPE_UMS;

        /* From here, the expected type is VOLUME.
         * ---
         * Step 3. Get the disk drive device corresponding to the USB device
         *         through Bus Relations.
         * --- */

        cret = CM_Get_Device_ID_ListA(
            usb_device_id,
            disk_drive_id_buf,
            sizeof(disk_drive_id_buf),
            CM_GETIDLIST_FILTER_BUSRELATIONS
        );
        if (cret == CR_BUFFER_SMALL) {
            msg(context,
                ll_error,
                "Sub device id buffer size was not big enough for USB "
                "device bus relations.");
            err = CAHUTE_ERROR_SIZE;
            goto fail;
        }

        if (cret != CR_SUCCESS) {
            msg(context,
                ll_error,
                "CM_Get_Device_ID_ListA (disk drive) returned error "
                "0x%08lX.",
                cret);
            goto fail;
        }

        for (disk_drive_id = disk_drive_id_buf; *disk_drive_id;
             disk_drive_id += strlen(disk_drive_id) + 1) {
            DEVINST disk_drive_device_instance;

            /* Get the device behind the interface. */
            cret = CM_Locate_DevNodeA(
                &disk_drive_device_instance,
                disk_drive_id,
                CM_LOCATE_DEVNODE_NORMAL
            );
            if (cret == CR_NO_SUCH_DEVINST)
                continue;

            if (cret != CR_SUCCESS) {
                msg(context,
                    ll_error,
                    "CM_Locate_DevNodeA (disk drive) returned error 0x%08lX.",
                    cret);
                goto fail;
            }

            /* Check that the class of the device is a disk drive. */
            property_size = sizeof(property);
            cret = CM_Get_DevNode_Registry_PropertyA(
                disk_drive_device_instance,
                CM_DRP_CLASSGUID,
                &property_type,
                (PBYTE)property,
                &property_size,
                0
            );
            if (cret == CR_NO_SUCH_VALUE)
                continue;

            if (cret != CR_SUCCESS) {
                msg(context,
                    ll_error,
                    "CM_Get_DevNode_Registry_PropertyA (disk drive) with "
                    "property CM_DRP_CLASSGUID returned error 0x%08lX.",
                    cret);
                goto fail;
            }

            if (property_type == REG_SZ && property_size == 39) {
                if (decode_guid(context, &guid, (char const *)property))
                    goto fail;
            } else if (property_type != REG_BINARY || property_size != sizeof(GUID_DEVCLASS_DISKDRIVE))
                memcpy(&guid, property, sizeof(GUID));
            else {
                msg(context,
                    ll_warn,
                    "Unexpected type 0x%08lX or size %luo for device "
                    "class key.",
                    property_type,
                    property_size);
                goto fail;
            }

            if (memcmp(
                    &guid,
                    &GUID_DEVCLASS_DISKDRIVE,
                    sizeof(GUID_DEVCLASS_DISKDRIVE)
                )) {
                /* The device is not a disk drive. */
                continue;
            }

            /* ---
             * Step 4. Get the volume device corresponding to the disk drive
             *         device through Bus Relations.
             * --- */

            cret = CM_Get_Device_ID_ListA(
                disk_drive_id,
                volume_id_buf,
                sizeof(volume_id_buf),
                CM_GETIDLIST_FILTER_REMOVALRELATIONS
            );

            if (cret == CR_BUFFER_SMALL) {
                msg(context,
                    ll_error,
                    "Sub device id buffer size was not big enough for disk "
                    "drive bus relations.");
                err = CAHUTE_ERROR_SIZE;
                goto fail;
            }

            if (cret != CR_SUCCESS) {
                msg(context,
                    ll_error,
                    "CM_Get_Device_ID_ListA (volume) returned error 0x%08lX.",
                    cret);
                goto fail;
            }

            for (volume_id = volume_id_buf; *volume_id;
                 volume_id += strlen(volume_id) + 1) {
                DEVINST volume_device_instance;

                /* Get the device behind the interface. */
                cret = CM_Locate_DevNodeA(
                    &volume_device_instance,
                    volume_id,
                    CM_LOCATE_DEVNODE_NORMAL
                );
                if (cret == CR_NO_SUCH_DEVINST)
                    continue;

                if (cret != CR_SUCCESS) {
                    msg(context,
                        ll_error,
                        "CM_Locate_DevNodeA (volume) returned error 0x%08lX.",
                        cret);
                    goto fail;
                }

                /* Check that the class of the device is a volume. */
                property_size = sizeof(property);
                cret = CM_Get_DevNode_Registry_PropertyA(
                    volume_device_instance,
                    CM_DRP_CLASSGUID,
                    &property_type,
                    (PBYTE)property,
                    &property_size,
                    0
                );
                if (cret == CR_NO_SUCH_VALUE)
                    continue;

                if (cret != CR_SUCCESS) {
                    msg(context,
                        ll_error,
                        "CM_Get_DevNode_Registry_PropertyA (volume) with "
                        "property CM_DRP_CLASSGUID returned error 0x%08lX.",
                        cret);
                    goto fail;
                }

                if (property_type == REG_SZ && property_size == 39) {
                    if (decode_guid(context, &guid, (char const *)property))
                        goto fail;
                } else if (property_type != REG_BINARY || property_size != sizeof(GUID))
                    memcpy(&guid, property, sizeof(GUID));
                else {
                    msg(context,
                        ll_warn,
                        "Unexpected type 0x%08lX or size %luo for device "
                        "class key.",
                        property_type,
                        property_size);
                    goto fail;
                }

                if (memcmp(
                        &guid,
                        &GUID_DEVCLASS_VOLUME,
                        sizeof(GUID_DEVCLASS_VOLUME)
                    )) {
                    /* The device is not a volume. */
                    continue;
                }

                /* ---
                 * Step 5. Get the volume device interface path corresponding
                 *         to the volume device.
                 * --- */

                err = find_win32_interface(
                    context,
                    path,
                    path_size,
                    volume_id,
                    (LPGUID)&GUID_DEVINTERFACE_VOLUME
                );
                if (!err)
                    goto end;
            }
        }
    }

    /* No device has been found! */
    msg(context, ll_error, "No device for USB port number %ld was found.", addr
    );
    goto fail;

end:
    err = CAHUTE_OK;

fail:
    if (usb_device_id_list)
        HeapFree(GetProcessHeap(), 0, usb_device_id_list);

    return err;
}

/**
 * Open a USB device using the Win32 interface.
 *
 * @param context
 * @param open_params
 * @param address
 * @return
 */
CAHUTE_EXTERN(int)
cahute_open_win32_usb_device_from_address(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    int address
) {
    char device_interface[300];
    int type, err;

    err = find_win32_usb_device(
        context,
        device_interface,
        sizeof(device_interface),
        &type,
        address
    );
    if (err)
        return err;

    switch (type) {
    case TYPE_UMS:
        return cahute_open_win32_ums_link(
            context,
            open_params,
            device_interface
        );

    case TYPE_CESG:
        return cahute_open_win32_cesg_link(
            context,
            open_params,
            device_interface
        );
    }

    CAHUTE_RETURN_IMPL(context, "Unsupported USB device type.");
}
