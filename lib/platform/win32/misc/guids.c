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
#undef DEFINE_GUID
#define DEFINE_GUID(NAME, L, W1, W2, B1, B2, B3, B4, B5, B6, B7, B8) \
    CAHUTE_INTERNAL_DATA(GUID) \
    NAME = {L, W1, W2, {B1, B2, B3, B4, B5, B6, B7, B8}}

DEFINE_GUID(
    cahute_guid_devinterface_usb_hub,
    0xf18a0e88,
    0xc30c,
    0x11d0,
    0x88,
    0x15,
    0x00,
    0xa0,
    0xc9,
    0x06,
    0xbe,
    0xd8
);
DEFINE_GUID(
    cahute_guid_devinterface_usb_device,
    0xa5dcbf10,
    0x6530,
    0x11d2,
    0x90,
    0x1f,
    0x00,
    0xc0,
    0x4f,
    0xb9,
    0x51,
    0xed
);
DEFINE_GUID(
    cahute_guid_devinterface_volume,
    0x53f5630d,
    0xb6bf,
    0x11d0,
    0x94,
    0xf2,
    0x00,
    0xa0,
    0xc9,
    0x1e,
    0xfb,
    0x8b
);

DEFINE_GUID(
    cahute_guid_devclass_usb,
    0x36fc9e60,
    0xc465,
    0x11cf,
    0x80,
    0x56,
    0x44,
    0x45,
    0x53,
    0x54,
    0x00,
    0x00
);
DEFINE_GUID(
    cahute_guid_devclass_usb_device,
    0x88bae032,
    0x5a81,
    0x49f0,
    0xbc,
    0x3d,
    0xa4,
    0xff,
    0x13,
    0x82,
    0x16,
    0xd6
);
DEFINE_GUID(
    cahute_guid_devclass_libusb_win32_device,
    0xeb781aaf,
    0x9c70,
    0x4523,
    0xa5,
    0xdf,
    0x64,
    0x2a,
    0x87,
    0xec,
    0xa5,
    0x67
);
DEFINE_GUID(
    cahute_guid_devclass_libusbk_device,
    0xecfb0cfd,
    0x74c4,
    0x4f52,
    0xbb,
    0xf7,
    0x34,
    0x34,
    0x61,
    0xcd,
    0x72,
    0xac
);
DEFINE_GUID(
    cahute_guid_devclass_diskdrive,
    0x4d36e967,
    0xe325,
    0x11ce,
    0xbf,
    0xc1,
    0x08,
    0x00,
    0x2b,
    0xe1,
    0x03,
    0x18
);
DEFINE_GUID(
    cahute_guid_devclass_volume,
    0x71a27cdd,
    0x812a,
    0x11d0,
    0xbe,
    0xc7,
    0x08,
    0x00,
    0x2b,
    0xe2,
    0x09,
    0x2f
);
