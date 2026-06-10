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
#define WINVER 0x0500 /* Windows 2000 */

#include "../../internals.h"
#include <windows.h>
#include <usbioctl.h>

/* OpenWatcom may not have the definition for this error. */
#ifndef ERROR_NO_SUCH_DEVICE
# define ERROR_NO_SUCH_DEVICE 433
#endif

CAHUTE_DECLARE_TYPE(cahute_win32_cfgmgr32)
CAHUTE_DECLARE_TYPE(cahute_win32_winusb)

CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devinterface_usb_hub;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devinterface_usb_device;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devinterface_volume;

CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devclass_usb;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devclass_usb_device;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devclass_libusb_win32_device;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devclass_libusbk_device;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devclass_diskdrive;
CAHUTE_INTERNAL_DATA_DECL(GUID) cahute_guid_devclass_volume;

CAHUTE_INTERNAL(void)
cahute_win32_log_error(
    cahute_context *context,
    char const *func_name,
    char const *win_func,
    DWORD code
);

#define log_windows_error(CTX, FUNC, CODE) \
    cahute_win32_log_error(CTX, CAHUTE_LOGFUNC, FUNC, CODE)

CAHUTE_INTERNAL(int)
cahute_decode_win32_guid(cahute_context *context, GUID *guid, char const *raw);

CAHUTE_INTERNAL(int)
cahute_serialize_win32_guid(
    cahute_context *context,
    char *buf,
    size_t size,
    GUID const *guid
);

CAHUTE_INTERNAL(int)
cahute_load_win32_system_library(
    cahute_context *context,
    HMODULE *dllp,
    char const *name
);

CAHUTE_INTERNAL(int)
cahute_get_win32_library_function(
    cahute_context *context,
    FARPROC *funcp,
    HMODULE dll,
    char const *name
);

CAHUTE_INTERNAL(int) cahute_check_win32_version(unsigned int version);

#define cahute_is_win_vista() cahute_check_win32_version(0x0600)
#define cahute_is_win_7()     cahute_check_win32_version(0x0601)

/* ---
 * Cfgmgr32 management.
 * --- */

/* CM_Get_Device_Interface_List_SizeA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_interface_list_size_func)(
    PULONG,
    GUID const *,
    PCSTR,
    ULONG
);

/* CM_Get_Device_Interface_ListA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_interface_list_func)(
    GUID const *,
    PCSTR,
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

/* CM_Get_Device_ID_Size() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_id_size_func)(
    PULONG,
    DWORD,
    ULONG
);

/* CM_Get_Device_IDA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_device_id_func)(
    DWORD,
    PSTR,
    ULONG,
    ULONG
);

/* CM_Locate_DevNodeA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_locate_devnode_func)(
    DWORD *,
    CHAR const *,
    ULONG
);

/* CM_Open_DevNode_Key() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_open_devnode_key_func)(
    DWORD,
    REGSAM,
    ULONG,
    ULONG,
    PHKEY,
    ULONG
);

/* CM_Get_DevNode_Registry_PropertyA() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_devnode_registry_property_func)(
    DWORD,
    ULONG,
    PULONG,
    PVOID,
    PULONG,
    ULONG
);

/* CM_Get_Parent() function type. */
typedef DWORD(WINAPI cahute_cfgmgr32_get_parent_func)(DWORD *, DWORD, ULONG);

struct cahute_win32_cfgmgr32 {
    HMODULE dll;

    cahute_cfgmgr32_get_device_interface_list_size_func
        *get_device_interface_list_size;
    cahute_cfgmgr32_get_device_interface_list_func *get_device_interface_list;
    cahute_cfgmgr32_get_device_id_list_size_func *get_device_id_list_size;
    cahute_cfgmgr32_get_device_id_list_func *get_device_id_list;
    cahute_cfgmgr32_get_device_id_size_func *get_device_id_size;
    cahute_cfgmgr32_get_device_id_func *get_device_id;
    cahute_cfgmgr32_locate_devnode_func *locate_devnode;
    cahute_cfgmgr32_open_devnode_key_func *open_devnode_key;
    cahute_cfgmgr32_get_devnode_registry_property_func
        *get_devnode_registry_property;
    cahute_cfgmgr32_get_parent_func *get_parent;
};

CAHUTE_INTERNAL(int)
cahute_get_win32_cfgmgr32(
    cahute_context *context,
    cahute_win32_cfgmgr32 **libp
);

/* ---
 * WinUSB management.
 * --- */

struct cahute_winusb_pipe_information {
    DWORD PipeType;
    UCHAR PipeId;
    USHORT MaximumPacketSize;
    UCHAR Interval;
};

/* Setup packet definition, from `WINUSB_SETUP_PACKET`. */
struct cahute_winusb_setup_packet {
    UCHAR bmRequestType;
    UCHAR bRequest;
    USHORT wValue;
    USHORT wIndex;
    USHORT wLength;
};

/* WinUsb_Initialize() function type. */
typedef BOOL(WINAPI cahute_winusb_initialize_func)(HANDLE, void **);

/* WinUsb_Free() function type. */
typedef BOOL(WINAPI cahute_winusb_free_func)(void *);

/* WinUsb_QueryInterfaceSettings() function type. */
typedef BOOL(WINAPI
                 cahute_winusb_query_interface_settings_func)(void *, UCHAR, USB_INTERFACE_DESCRIPTOR *);

/* WinUsb_QueryPipe() function type. */
typedef BOOL(WINAPI
                 cahute_winusb_query_pipe_func)(void *, UCHAR, UCHAR, struct cahute_winusb_pipe_information *);

/* WinUsb_ControlTransfer() function type. */
typedef BOOL(WINAPI cahute_winusb_control_transfer)(
    void *,
    struct cahute_winusb_setup_packet,
    UCHAR *,
    ULONG,
    ULONG *,
    LPOVERLAPPED
);

/* WinUsb_ReadPipe() function type. */
typedef BOOL(WINAPI cahute_winusb_read_pipe_func)(
    void *,
    UCHAR,
    UCHAR *,
    ULONG,
    ULONG *,
    LPOVERLAPPED
);

/* WinUsb_WritePipe() function type. */
typedef BOOL(WINAPI cahute_winusb_write_pipe_func)(
    void *,
    UCHAR,
    UCHAR const *,
    ULONG,
    ULONG *,
    LPOVERLAPPED
);

/* WinUsb_AbortPipe() function type. */
typedef BOOL(WINAPI cahute_winusb_abort_pipe_func)(void *, UCHAR);

/* WinUsb_GetOverlappedResult() function type. */
typedef BOOL(WINAPI cahute_winusb_get_overlapped_result_func)(
    void *,
    LPOVERLAPPED,
    LPDWORD,
    BOOL
);

struct cahute_win32_winusb {
    HMODULE dll;

    cahute_winusb_initialize_func *initialize;
    cahute_winusb_free_func *free;
    cahute_winusb_query_interface_settings_func *query_interface_settings;
    cahute_winusb_query_pipe_func *query_pipe;
    cahute_winusb_control_transfer *control_transfer;
    cahute_winusb_read_pipe_func *read_pipe;
    cahute_winusb_write_pipe_func *write_pipe;
    cahute_winusb_abort_pipe_func *abort_pipe;
    cahute_winusb_get_overlapped_result_func *get_overlapped_result;
};

CAHUTE_INTERNAL(int)
cahute_get_win32_winusb(cahute_context *context, cahute_win32_winusb **libp);

/* ---
 * Device detection.
 * --- */

CAHUTE_DECLARE_TYPE(cahute_win32_device_filter)
CAHUTE_DECLARE_TYPE(cahute_win32_device)

/**
 * Device filter for enumeration.
 *
 * @property related_to_device_id ID of the device in the bus relations of
 *           which to enumerate devices. Can be NULL.
 * @property in_removal_relations_of_device_id ID of the device in the removal
 *           relations of which to enumerate devices. Can be NULL.
 * @property device_class Device class to match. Can be NULL.
 * @property interface_class Interface class to match. Can be NULL.
 * @property path Device or device interface path to find. Can be NULL.
 */
struct cahute_win32_device_filter {
    char const *related_to_device_id;
    char const *in_removal_relations_of_device_id;
    GUID const *device_class;
    GUID const *interface_class;
    char const *path;
};

/**
 * Device enumeration result.
 *
 * @property device_id Device identifier.
 * @property service Service name, if available.
 * @property driver_name Driver name, if available.
 * @property driver_version Driver version, if available.
 * @property device_instance Device instance.
 * @property address Device address.
 */
struct cahute_win32_device {
    char const *device_id;
    char const *service;
    char const *driver_name;
    char const *driver_version;
    DWORD device_instance;
    DWORD address;
};

typedef int(cahute_enumerate_win32_device_func)(void *, cahute_win32_device const *);

CAHUTE_INTERNAL(int)
cahute_enumerate_win32_devices(
    cahute_context *context,
    cahute_win32_device_filter const *filter,
    cahute_enumerate_win32_device_func *func,
    void *cookie
);

#endif /* PLATFORM_WIN32_INTERNALS_H */
