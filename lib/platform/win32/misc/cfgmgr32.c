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

/**
 * Unload the Cfgmgr32 library.
 *
 * @param context
 * @param lib Library to unload.
 */
CAHUTE_LOCAL(void)
unload_cfgmgr32_lib(cahute_context *context, cahute_win32_cfgmgr32 *lib) {
    FreeLibrary(lib->dll);
    free(lib);
}

/**
 * Load the Cfgmgr32 library.
 *
 * @param context
 * @param libp
 * @param destroy_funcp
 * @return
 */
CAHUTE_LOCAL(int)
load_cfgmgr32_lib(
    cahute_context *context,
    cahute_win32_cfgmgr32 **libp,
    cahute_context_destroy_func **destroy_funcp
) {
    int err = CAHUTE_ERROR_UNKNOWN;
    HMODULE dll = NULL;
    cahute_win32_cfgmgr32 *lib = NULL;

    err = cahute_load_win32_system_library(context, &dll, "cfgmgr32");
    if (err)
        goto fail;

    lib = malloc(sizeof(cahute_win32_cfgmgr32));
    if (!lib) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    lib->dll = dll;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_device_interface_list_size,
        dll,
        "CM_Get_Device_Interface_List_SizeA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_device_interface_list,
        dll,
        "CM_Get_Device_Interface_ListA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_device_id_list_size,
        dll,
        "CM_Get_Device_ID_List_SizeA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_device_id_list,
        dll,
        "CM_Get_Device_ID_ListA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_device_id_size,
        dll,
        "CM_Get_Device_ID_Size"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_device_id,
        dll,
        "CM_Get_Device_IDA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->locate_devnode,
        dll,
        "CM_Locate_DevNodeA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->open_devnode_key,
        dll,
        "CM_Open_DevNode_Key"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_devnode_registry_property,
        dll,
        "CM_Get_DevNode_Registry_PropertyA"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_parent,
        dll,
        "CM_Get_Parent"
    );
    if (err)
        goto fail;

    *libp = lib;
    *destroy_funcp = (cahute_context_destroy_func *)&unload_cfgmgr32_lib;
    return CAHUTE_OK;

fail:
    if (lib)
        free(lib);
    if (dll)
        FreeLibrary(dll);

    return err;
}

/**
 * Get the loaded Cfgmgr32 library.
 *
 * @param context
 * @param libp
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_get_win32_cfgmgr32(
    cahute_context *context,
    cahute_win32_cfgmgr32 **libp
) {
    return cahute_get_context_pointer(
        context,
        (void **)libp,
        CAHUTE_CONTEXT_POINTER_WIN32_CFGMGR32,
        (cahute_context_init_func *)load_cfgmgr32_lib
    );
}
