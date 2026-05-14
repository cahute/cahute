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
 * Unload the WinUSB library.
 *
 * @param context
 * @param lib Library to unload.
 */
CAHUTE_LOCAL(void)
unload_winusb_lib(cahute_context *context, cahute_win32_winusb *lib) {
    FreeLibrary(lib->dll);
    free(lib);
}

/**
 * Load the WinUSB library.
 *
 * @param context
 * @param libp
 * @param destroy_funcp
 * @return
 */
CAHUTE_LOCAL(int)
load_winusb_lib(
    cahute_context *context,
    cahute_win32_winusb **libp,
    cahute_context_destroy_func **destroy_funcp
) {
    int err = CAHUTE_ERROR_UNKNOWN;
    HMODULE dll = NULL;
    cahute_win32_winusb *lib = NULL;

    err = cahute_load_win32_system_library(context, &dll, "winusb");
    if (err)
        goto fail;

    lib = malloc(sizeof(cahute_win32_winusb));
    if (!lib) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    lib->dll = dll;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->initialize,
        dll,
        "WinUsb_Initialize"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->free,
        dll,
        "WinUsb_Free"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->query_interface_settings,
        dll,
        "WinUsb_QueryInterfaceSettings"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->query_pipe,
        dll,
        "WinUsb_QueryPipe"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->control_transfer,
        dll,
        "WinUsb_ControlTransfer"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->read_pipe,
        dll,
        "WinUsb_ReadPipe"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->write_pipe,
        dll,
        "WinUsb_WritePipe"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->abort_pipe,
        dll,
        "WinUsb_AbortPipe"
    );
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_overlapped_result,
        dll,
        "WinUsb_GetOverlappedResult"
    );
    if (err)
        goto fail;

    *libp = lib;
    *destroy_funcp = (cahute_context_destroy_func *)&unload_winusb_lib;
    return CAHUTE_OK;

fail:
    if (lib)
        free(lib);
    if (dll)
        FreeLibrary(dll);

    return err;
}

/**
 * Get the loaded WinUSB library.
 *
 * @param context
 * @param libp
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_get_win32_winusb(cahute_context *context, cahute_win32_winusb **libp) {
    return cahute_get_context_pointer(
        context,
        (void **)libp,
        CAHUTE_CONTEXT_POINTER_WIN32_WINUSB,
        (cahute_context_init_func *)load_winusb_lib
    );
}
