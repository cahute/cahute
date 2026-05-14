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
#include <regstr.h>

/**
 * Enumerate devices.
 *
 * @param context Context in which to enumerate devices.
 * @param filter Filter to apply.
 * @param func Function to call with the device result.
 * @param cookie Cookie to call the function with.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_enumerate_win32_devices(
    cahute_context *context,
    cahute_win32_device_filter const *filter,
    cahute_enumerate_win32_device_func *func,
    void *cookie
) {
    cahute_win32_cfgmgr32 *cfgmgr32;
    cahute_win32_device result;
    char *allocated_device_id_list = NULL;
    char const *device_id_list;
    char const *device_id;
    size_t device_id_size = 0;
    GUID const *device_class = NULL, *interface_class = NULL;
    PCSTR device_id_list_filter = NULL;
    char device_id_list_filter_buf[40];
    ULONG device_id_list_filter_flags = 0;
    ULONG device_id_list_size = 0;
    DWORD cret;
    int err = CAHUTE_ERROR_UNKNOWN;

    msg(context, ll_debug, "Looking for devices.");

    /* Get dynamic access to the Cfgmgr32 library. */
    err = cahute_get_win32_cfgmgr32(context, &cfgmgr32);
    if (err)
        return err;

    err = CAHUTE_ERROR_UNKNOWN;

    if (filter && filter->path) {
        msg(context, ll_debug, "  Looking for path: %s", filter->path);

        device_id_list = filter->path;
        device_id_list_size = strlen(filter->path) + 1;
    } else {
        if (filter && filter->device_class) {
            char buf[40];

            device_class = filter->device_class;
            cahute_serialize_win32_guid(
                context,
                buf,
                sizeof(buf),
                device_class
            );

            msg(context, ll_debug, "  With device class: %s", buf);
        }

        if (filter && filter->interface_class) {
            char buf[40];

            interface_class = filter->interface_class;
            cahute_serialize_win32_guid(
                context,
                buf,
                sizeof(buf),
                interface_class
            );

            msg(context, ll_debug, "  With interface class: %s", buf);
        }

        if (filter && filter->related_to_device_id) {
            msg(context,
                ll_debug,
                "  With bus relations to: %s",
                filter->related_to_device_id);

            device_id_list_filter = filter->related_to_device_id;
            device_id_list_filter_flags =
                0x20 /* CM_GETIDLIST_FILTER_BUSRELATIONS */;
        } else if (filter && filter->in_removal_relations_of_device_id) {
            msg(context,
                ll_debug,
                "  With removal relations to: %s",
                filter->in_removal_relations_of_device_id);

            /* Removal relations are available starting on Windows Vista.
             * Before that, we just want to use bus relations. */
            device_id_list_filter = filter->in_removal_relations_of_device_id;
            if (cahute_is_win_vista())
                device_id_list_filter_flags =
                    0x08 /* CM_GETIDLIST_FILTER_REMOVALRELATIONS */;
            else {
                msg(context,
                    ll_warn,
                    "Removal relations are not available, falling back on "
                    "bus relations.");
                device_id_list_filter_flags =
                    0x20 /* CM_GETIDLIST_FILTER_BUSRELATIONS */;
            }
        } else if (device_class && cahute_is_win_7()) {
            cahute_serialize_win32_guid(
                context,
                device_id_list_filter_buf,
                sizeof(device_id_list_filter_buf),
                device_class
            );

            device_id_list_filter = device_id_list_filter_buf;
            device_id_list_filter_flags =
                0x200 /* CM_GETIDLIST_FILTER_CLASS */;

            /* No need to filter on the device class below. */
            device_class = NULL;
        }

        /* Get all devices, without distinction. */
        cret = (*cfgmgr32->get_device_id_list_size)(
            &device_id_list_size,
            device_id_list_filter,
            device_id_list_filter_flags
        );
        if (cret) {
            if (cret == 0x25 /* CR_NO_SUCH_VALUE */) {
                /* We just consider that there's no device in the device list
                * in such a case. */
                err = CAHUTE_OK;
                goto fail;
            }

            msg(context,
                ll_error,
                "CM_Get_Device_ID_List_SizeA returned error 0x%08lX.",
                cret);
            goto fail;
        }

        allocated_device_id_list = (char *)
            HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, device_id_list_size);
        if (!allocated_device_id_list) {
            log_windows_error(context, "HeapAlloc", GetLastError());
            err = CAHUTE_ERROR_ALLOC;
            goto fail;
        }

        device_id_list = allocated_device_id_list;

        cret = (*cfgmgr32->get_device_id_list)(
            device_id_list_filter,
            allocated_device_id_list,
            device_id_list_size,
            device_id_list_filter_flags
        );
        if (cret) {
            if (cret == 0x25 /* CR_NO_SUCH_VALUE */) {
                /* We just consider that there's no device in the device list
                * in such a case. */
                err = CAHUTE_OK;
                goto fail;
            }

            msg(context,
                ll_error,
                "CM_Get_Device_ID_ListA returned error 0x%08lX.",
                cret);
            goto fail;
        }
    }

    while (device_id_list_size > 0 && *device_id_list) {
        DWORD devinst, obtained_address;
        GUID obtained_device_class;
        BYTE obtained_raw_guid[50];
        char guid_buf[50], service_buf[50];
        char driver_name_buf[50], driver_version_buf[50];
        char const *service = NULL;
        char const *driver_name = NULL, *driver_version = NULL;
        char const *device_id_end = NULL;
        ULONG property_type = 0, property_size = 0;
        int is_invalid = 0, is_guid_valid = 0;
        HKEY key;

        device_id = device_id_list;
        device_id_end = memchr(device_id, 0, device_id_list_size);

        if (!device_id_end) {
            /* No NUL terminator present in the device ID; to avoid problems,
             * we don't process that entry. */
            msg(context,
                ll_error,
                "Last list entry did not have a NUL terminator.");
            err = CAHUTE_ERROR_UNKNOWN;
            goto fail;
        }

        device_id_size = (size_t)(device_id_end - device_id);

        device_id_list += device_id_size + 1;
        device_id_list_size -= device_id_size + 1;

        /* Get the device behind the interface. */
        cret = (*cfgmgr32->locate_devnode)(&devinst, device_id, 0);
        if (cret == 0x0D /* CR_NO_SUCH_DEVINST */
            || cret == 0x1E /* CR_INVALID_DEVICE_ID */)
            continue;

        if (cret) {
            msg(context,
                ll_error,
                "CM_Locate_DevNodeA returned error 0x%08lX.",
                cret);
            goto fail;
        }

        msg(context, ll_debug, "New device!");
        msg(context, ll_debug, "  Device ID: %s", device_id);
        msg(context, ll_debug, "  Device instance: %d", devinst);

        /* Obtain the device address / port number. */
        property_size = sizeof(obtained_address);
        cret = (*cfgmgr32->get_devnode_registry_property)(
            devinst,
            0x1D /* CM_DRP_ADDRESS */,
            &property_type,
            (PBYTE)&obtained_address,
            &property_size,
            0
        );
        if (cret) {
            msg(context, ll_debug, "  Address: <ERR 0x%08lX>", cret);
            is_invalid = 1;
        } else if (property_type != REG_DWORD) {
            msg(context,
                ll_debug,
                "  Address: <INVALID TYPE=%08lX SIZE=%lu>",
                property_type,
                property_size);
            is_invalid = 1;
        } else
            msg(context, ll_debug, "  Address: %lu", obtained_address);

        /* Obtain the device class. */
        property_size = sizeof(obtained_raw_guid);
        is_guid_valid = 0;
        cret = (*cfgmgr32->get_devnode_registry_property)(
            devinst,
            0x09 /* CM_DRP_CLASSGUID */,
            &property_type,
            (PBYTE)obtained_raw_guid,
            &property_size,
            0
        );
        if (cret) {
            msg(context, ll_debug, "  Device class: <ERR 0x%08lX>", cret);
            is_invalid = 1;
        } else if (property_type == REG_BINARY && property_size == sizeof(obtained_device_class)) {
            memcpy(
                &obtained_device_class,
                obtained_raw_guid,
                sizeof(obtained_device_class)
            );
            is_guid_valid = 1;
        } else if (property_type == REG_SZ && property_size == 39 && !cahute_decode_win32_guid(context, &obtained_device_class, (char const *)obtained_raw_guid)) {
            is_guid_valid = 1;
        } else {
            msg(context,
                ll_debug,
                "  Device class: <INVALID TYPE=%08lX SIZE=%lu>",
                property_type,
                property_size);
            is_invalid = 1;
        }

        if (is_guid_valid) {
            cahute_serialize_win32_guid(
                context,
                guid_buf,
                sizeof(guid_buf),
                &obtained_device_class
            );
            msg(context, ll_debug, "  Device class: %s", guid_buf);
        }

        /* Obtain the service. */
        property_size = sizeof(service_buf) - 1;
        cret = (*cfgmgr32->get_devnode_registry_property)(
            devinst,
            0x05 /* CM_DRP_SERVICE */,
            &property_type,
            (PBYTE)service_buf,
            &property_size,
            0
        );
        if (cret)
            msg(context, ll_debug, "  Service: <ERR 0x%08lX>", cret);
        else if (property_type != REG_SZ)
            msg(context,
                ll_debug,
                "  Service: <INVALID TYPE=%08lX SIZE=%lu>",
                property_type,
                property_size);
        else {
            service_buf[property_size] = '\0';
            service = service_buf;
            msg(context, ll_debug, "  Service: %s", service);
        }

        /* Obtain the driver name. */
        cret = (*cfgmgr32->open_devnode_key)(
            devinst,
            KEY_QUERY_VALUE,
            0,
            1 /* RegDisposition_OpenExisting */,
            &key,
            1 /* CM_REGISTRY_SOFTWARE */
        );
        if (cret) {
            msg(context, ll_debug, "  Driver name: <ERR 0x%08lX>", cret);
            msg(context, ll_debug, "  Driver version: <ERR 0x%08lX>", cret);
        } else {
            LSTATUS lstatus_name, lstatus_version;
            DWORD name_type, version_type;
            DWORD name_size = sizeof(driver_name_buf) - 1;
            DWORD version_size = sizeof(driver_version_buf) - 1;

            lstatus_name = RegQueryValueExA(
                key,
                REGSTR_VAL_DRVDESC,
                NULL,
                &name_type,
                (LPBYTE)driver_name_buf,
                &name_size
            );
            lstatus_version = RegQueryValueExA(
                key,
                REGSTR_VAL_DRIVERVERSION,
                NULL,
                &version_type,
                (LPBYTE)driver_version_buf,
                &version_size
            );
            RegCloseKey(key);

            if (lstatus_name == ERROR_SUCCESS && name_type == REG_SZ) {
                driver_name_buf[name_size] = '\0';
                driver_name = driver_name_buf;

                msg(context, ll_debug, "  Driver name: %s", driver_name);
            } else
                msg(context,
                    ll_debug,
                    "  Driver name: <WINERR 0x%08lX>",
                    lstatus_name);

            if (lstatus_version == ERROR_SUCCESS && version_type == REG_SZ) {
                driver_version_buf[version_size] = '\0';
                driver_version = driver_version_buf;

                msg(context, ll_debug, "  Driver version: %s", driver_version);
            } else
                msg(context,
                    ll_debug,
                    "  Driver version: <WINERR 0x%08lX>",
                    lstatus_version);
        }

        /* Check the device. */
        if (is_invalid) {
            msg(context,
                ll_debug,
                "One or more of the properties could not be obtained, "
                "ignoring the device.");
            continue;
        }

        /* Optionally check the device. */
        if (device_class
            && memcmp(
                &obtained_device_class,
                device_class,
                sizeof(*device_class)
            )) {
            msg(context, ll_debug, "Skipped: incorrect device class.");
            continue;
        }

        /* Check if there is at least one device interface with the
         * interface filter, if need be.
         * An empty list will have a size of 1 (NUL byte only). */
        if (interface_class) {
            ULONG interface_list_size = 0;

            cret = (*cfgmgr32->get_device_interface_list_size)(
                &interface_list_size,
                interface_class,
                device_id,
                0
            );
            if (cret) {
                msg(context,
                    ll_warn,
                    "CM_Get_Device_Interface_List_SizeA returned error "
                    "0x%08lX.",
                    cret);
                continue;
            }

            if (interface_list_size <= 1) {
                msg(context,
                    ll_debug,
                    "Skipped: no device interface with the correct class.");
                continue;
            }
        }

        /* The device matches! */
        result.device_id = device_id;
        result.device_instance = devinst;
        result.service = service_buf;
        result.driver_name = driver_name;
        result.driver_version = driver_version;
        result.address = obtained_address;

        err = (*func)(cookie, &result);
        if (err)
            goto fail;
    }

    err = CAHUTE_OK;
fail:
    msg(context, ll_info, "End of device lookup.");
    if (allocated_device_id_list)
        HeapFree(GetProcessHeap(), 0, allocated_device_id_list);

    return err;
}
