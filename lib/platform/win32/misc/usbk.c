/* ****************************************************************************
 * Copyright (C) 2026 Thomas Touhey <thomas@touhey.fr>
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
 * Unload the UsbK library.
 *
 * @param context
 * @param lib Library to unload.
 */
CAHUTE_LOCAL(void)
unload_usbk_lib(cahute_context *context, cahute_win32_usbk *lib) {
    FreeLibrary(lib->dll);
    free(lib);
}

/**
 * Unload a UsbK driver.
 *
 * @param context
 * @param driver Driver to unload.
 */
CAHUTE_LOCAL(void)
unload_usbk_driver(cahute_context *context, cahute_win32_usbk_driver *driver) {
    free(driver);
}

/**
 * Load the UsbK library.
 *
 * @param context
 * @param libp
 * @param destroy_funcp
 * @return
 */
CAHUTE_LOCAL(int)
load_usbk_lib(
    cahute_context *context,
    cahute_win32_usbk **libp,
    cahute_context_destroy_func **destroy_funcp
) {
    int err = CAHUTE_ERROR_UNKNOWN;
    HMODULE dll = NULL;
    cahute_win32_usbk *lib = NULL;
    cahute_win32_usbk_get_version_func *get_version_func = NULL;
    cahute_win32_usbk_version usbk_version;

    err = cahute_load_win32_system_library(context, &dll, "libusbK");
    if (err)
        goto fail;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&get_version_func,
        dll,
        "LibK_GetVersion"
    );
    if (err)
        goto fail;

    /* With libusbK 3.0.7.0, we have encountered issues with control transfers
     * with USB devices using the libusb-win32 driver. We want to make sure
     * this version fails for an explicit error here. */
    (*get_version_func)(&usbk_version);
    if (usbk_version.Major < 3
        || (usbk_version.Major == 3 && usbk_version.Minor == 0
            && usbk_version.Micro < 8)) {
        msg(context,
            ll_error,
            "LibUsbK %d.%d.%d.%d unsupported, only 3.0.8.0+ is supported by "
            "Cahute.",
            usbk_version.Major,
            usbk_version.Minor,
            usbk_version.Micro,
            usbk_version.Nano);
        err = CAHUTE_ERROR_UNKNOWN;
        goto fail;
    }

    lib = malloc(sizeof(cahute_win32_usbk));
    if (!lib) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    lib->dll = dll;

    err = cahute_get_win32_library_function(
        context,
        (FARPROC *)&lib->get_proc_address,
        dll,
        "LibK_GetProcAddress"
    );
    if (err)
        goto fail;

    *libp = lib;
    *destroy_funcp = (cahute_context_destroy_func *)&unload_usbk_lib;
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
cahute_get_win32_usbk(cahute_context *context, cahute_win32_usbk **libp) {
    return cahute_get_context_pointer(
        context,
        (void **)libp,
        CAHUTE_CONTEXT_POINTER_WIN32_USBK,
        (cahute_context_init_func *)load_usbk_lib
    );
}

/**
 * Load a function from a UsbK driver.
 *
 * @param context Context in which the function is run.
 * @param funcp Pointer to the function to set.
 * @param usbk USBk library to use.
 * @param did Driver ID.
 * @param fid Function ID.
 * @param name Name of the function to extract from the library.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
get_usbk_driver_function(
    cahute_context *context,
    FARPROC *funcp,
    cahute_win32_usbk *usbk,
    int did,
    int fid,
    char const *name
) {
    FARPROC func;
    int res;

    res = (*usbk->get_proc_address)(&func, did, fid);
    if (!res) {
        msg(context,
            ll_error,
            "Could not load function `%s` for driver %d (fid %d):",
            name,
            fid);
        log_windows_error(context, "LibK_GetProcAddress", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    *funcp = func;
    return CAHUTE_OK;
}

/**
 * Load a UsbK driver.
 *
 * Function IDs are available here (count from 0):
 * https://libusbk.sourceforge.net/UsbK3/group__libk.html
 * #ga8a86fa7b1ae765cc000d9c4c5abc9146
 *
 * @param context
 * @param driverp Pointer to the driver to initialize.
 * @param did ID of the driver to load.
 */
CAHUTE_LOCAL(int)
get_usbk_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp,
    int did
) {
    cahute_win32_usbk_driver *driver = NULL;
    cahute_win32_usbk *usbk;
    int err;

    err = cahute_get_win32_usbk(context, &usbk);
    if (err)
        goto fail;

    driver = malloc(sizeof(cahute_win32_usbk_driver));
    if (!driver) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->initialize,
        usbk,
        did,
        13,
        "Initialize"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->free,
        usbk,
        did,
        1,
        "Free"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->query_interface_settings,
        usbk,
        did,
        17,
        "QueryInterfaceSettings"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->query_pipe,
        usbk,
        did,
        21,
        "QueryPipe"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->control_transfer,
        usbk,
        did,
        7,
        "ControlTransfer"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->read_pipe,
        usbk,
        did,
        24,
        "ReadPipe"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->write_pipe,
        usbk,
        did,
        25,
        "WritePipe"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->abort_pipe,
        usbk,
        did,
        27,
        "AbortPipe"
    );
    if (err)
        goto fail;

    err = get_usbk_driver_function(
        context,
        (FARPROC *)&driver->get_overlapped_result,
        usbk,
        did,
        32,
        "GetOverlappedResult"
    );
    if (err)
        goto fail;

    *driverp = driver;
    return CAHUTE_OK;

fail:
    if (driver)
        free(driver);

    return err;
}

/**
 * Load libusbK UsbK driver.
 *
 * @param context
 * @param driverp
 * @param destroy_funcp
 * @return
 */
CAHUTE_LOCAL(int)
load_usbk_libusbk_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp,
    cahute_context_destroy_func **destroy_funcp
) {
    cahute_win32_usbk_driver *driver;
    int err;

    err = get_usbk_driver(context, &driver, 0);
    if (err)
        return err;

    *driverp = driver;
    *destroy_funcp = (cahute_context_destroy_func *)unload_usbk_driver;
    return CAHUTE_OK;
}

/**
 * Get the UsbK libusbK driver.
 *
 * @param context
 * @param driverp
 */
CAHUTE_INTERNAL(int)
cahute_get_win32_usbk_libusbk_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp
) {
    return cahute_get_context_pointer(
        context,
        (void **)driverp,
        CAHUTE_CONTEXT_POINTER_WIN32_USBK_DRIVER_LIBUSBK,
        (cahute_context_init_func *)load_usbk_libusbk_driver
    );
}

/**
 * Load libusb0 UsbK driver.
 *
 * @param context
 * @param driverp
 * @param destroy_funcp
 * @return
 */
CAHUTE_LOCAL(int)
load_usbk_libusb0_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp,
    cahute_context_destroy_func **destroy_funcp
) {
    cahute_win32_usbk_driver *driver;
    int err;

    err = get_usbk_driver(context, &driver, 1);
    if (err)
        return err;

    *driverp = driver;
    *destroy_funcp = (cahute_context_destroy_func *)unload_usbk_driver;
    return CAHUTE_OK;
}

/**
 * Get the UsbK libusb0 driver.
 *
 * @param context
 * @param driverp
 */
CAHUTE_INTERNAL(int)
cahute_get_win32_usbk_libusb0_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp
) {
    return cahute_get_context_pointer(
        context,
        (void **)driverp,
        CAHUTE_CONTEXT_POINTER_WIN32_USBK_DRIVER_LIBUSB0,
        (cahute_context_init_func *)load_usbk_libusb0_driver
    );
}

/**
 * Load WinUSB UsbK driver.
 *
 * @param context
 * @param driverp
 * @param destroy_funcp
 * @return
 */
CAHUTE_LOCAL(int)
load_usbk_winusb_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp,
    cahute_context_destroy_func **destroy_funcp
) {
    cahute_win32_usbk_driver *driver;
    int err;

    err = get_usbk_driver(context, &driver, 2);
    if (err)
        return err;

    *driverp = driver;
    *destroy_funcp = (cahute_context_destroy_func *)unload_usbk_driver;
    return CAHUTE_OK;
}

/**
 * Get the UsbK WinUSB driver.
 *
 * @param context
 * @param driverp
 */
CAHUTE_INTERNAL(int)
cahute_get_win32_usbk_winusb_driver(
    cahute_context *context,
    cahute_win32_usbk_driver **driverp
) {
    return cahute_get_context_pointer(
        context,
        (void **)driverp,
        CAHUTE_CONTEXT_POINTER_WIN32_USBK_DRIVER_WINUSB,
        (cahute_context_init_func *)load_usbk_winusb_driver
    );
}
