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

CAHUTE_DECLARE_TYPE(cahute_win32_cfgmgr32)

CAHUTE_EXTERN(void)
cahute_win32_log_error(
    cahute_context *context,
    char const *func_name,
    char const *win_func,
    DWORD code
);

#define log_windows_error(CTX, FUNC, CODE) \
    cahute_win32_log_error(CTX, CAHUTE_LOGFUNC, FUNC, CODE)

CAHUTE_EXTERN(int)
cahute_load_win32_system_library(
    cahute_context *context,
    HMODULE *dllp,
    char const *name
);
CAHUTE_EXTERN(int)
cahute_get_win32_library_function(
    cahute_context *context,
    FARPROC *funcp,
    HMODULE dll,
    char const *name
);

/* ---
 * Cfgmgr32 management.
 * --- */

/* CM_Get_Device_Interface_List_SizeA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_interface_list_size_func)(
    PULONG,
    LPGUID,
    CHAR *,
    ULONG
);
/* CM_Get_Device_Interface_ListA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_interface_list_func)(
    LPGUID,
    CHAR *,
    PCHAR,
    ULONG,
    ULONG
);
/* CM_Get_Device_ID_List_SizeA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_id_list_size_func)(
    PULONG,
    PCSTR,
    ULONG
);
/* CM_Get_Device_ID_ListA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_id_list_func)(
    PCSTR,
    PCHAR,
    ULONG,
    ULONG
);
/* CM_Locate_DevNodeA() function type. */
typedef DWORD(WINAPI
                  cahute_cfgmgr32_locate_devnode_func)(DWORD *, CHAR *, ULONG);
/* CM_Get_DevNode_Registry_PropertyA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_devnode_registry_property_func)(
    DWORD,
    ULONG,
    PULONG,
    PVOID,
    PULONG,
    ULONG
);

struct cahute_win32_cfgmgr32 {
    HMODULE dll;

    cahute_cfgmgr32_get_device_interface_list_size_func
        *get_device_interface_list_size;
    cahute_cfgmgr32_get_device_interface_list_func *get_device_interface_list;
    cahute_cfgmgr32_get_device_id_list_size_func *get_device_id_list_size;
    cahute_cfgmgr32_get_device_id_list_func *get_device_id_list;
    cahute_cfgmgr32_locate_devnode_func *locate_devnode;
    cahute_cfgmgr32_get_devnode_registry_property_func
        *get_devnode_registry_property;
};

CAHUTE_EXTERN(int)
cahute_get_win32_cfgmgr32(
    cahute_context *context,
    cahute_win32_cfgmgr32 **libp
);

#endif /* PLATFORM_WIN32_INTERNALS_H */
