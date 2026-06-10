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

#ifndef PLATFORM_WIN32_USB_INTERNALS_H
#define PLATFORM_WIN32_USB_INTERNALS_H 1
#include "../internals.h"

CAHUTE_DECLARE_TYPE(cahute_win32_usb_device)

#define CAHUTE_WIN32_USB_DRIVER_UNKNOWN      0
#define CAHUTE_WIN32_USB_DRIVER_VOLMGR       1
#define CAHUTE_WIN32_USB_DRIVER_CESG_0       2 /* CESG == 1.0.0.0 */
#define CAHUTE_WIN32_USB_DRIVER_CESG_1       3 /* CESG >= 1.0.0.1 */
#define CAHUTE_WIN32_USB_DRIVER_WINUSB       4
#define CAHUTE_WIN32_USB_DRIVER_LIBUSB_WIN32 5
#define CAHUTE_WIN32_USB_DRIVER_LIBUSBK      6

/**
 * USB device enumeration result.
 *
 * @property driver Driver identifier, among ``CAHUTE_WIN32_USB_DRIVER_*``
 *           constants.
 * @property entry_type USB detection entry type, among
 *           ``CAHUTE_USB_DETECTION_ENTRY_TYPE_*`` constants.
 * @property device_id Device identifier, for interactions with Cfgmgr32.
 */
struct cahute_win32_usb_device {
    int driver;
    int entry_type;
    char const *device_id;
};

typedef int(cahute_enumerate_win32_usb_device_func)(void *, cahute_win32_usb_device const *);

CAHUTE_INTERNAL(int)
cahute_enumerate_win32_usb_devices(
    cahute_context *context,
    char const *device_path,
    cahute_enumerate_win32_usb_device_func *func,
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_win32_cesg_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path,
    size_t max_read_capacity
);

CAHUTE_INTERNAL(int)
cahute_open_win32_ums_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path
);

CAHUTE_INTERNAL(int)
cahute_open_win32_winusb_bulk_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path
);

#endif /* PLATFORM_WIN32_USB_INTERNALS_H */
