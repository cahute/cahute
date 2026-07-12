/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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

#ifndef LINK_OPEN_INTERNALS_H
#define LINK_OPEN_INTERNALS_H 1
#include "../internals.h"

/* The maximum data buffer size is thought for Protocol 7.00 Screenstreaming,
 * where we could have up to 528x320 pixels in the R5G6B5 format (2Bpp),
 * 2 times (one for the reference image, one for the received TYPB1 diff
 * to apply), plus the maximum packet information + checksum size (32 bytes),
 * and a bit more for safety.
 *
 * This is allocated no matter what, because we could arrive at Protocol 7.00
 * Screenstreaming when set explicitely, or determined automatically after
 * link allocation by finding a 0B packet.
 *
 * This number is therefore determined using the following formula:
 *
 *   2 * (528 * 320 * 2 + 64)
 *
 * This number also has the advantage to be aligned to 64 bytes at its
 * half-position. */
#define DEFAULT_DATA_BUFFER_SIZE 675968 /* ~660 KiB, ~.64 MiB */

/* This represents the default buffer size to shave off the end of the
 * data buffer when initializing the link for Protocol 7.00 Screenstreaming. */
#define DEFAULT_PICTURE_BUFFER_SIZE 337984 /* ~330 KiB, ~.32 MiB */

/* Protocol constant that may be short-lived at
 * ``cahute_init_link_protocol()``. */
#define PROTOCOL_SERIAL_FLAG      128
#define PROTOCOL_USB_FLAG         64
#define PROTOCOL_AUTO_FLAG        32
#define PROTOCOL_SERIAL_NONE      (PROTOCOL_SERIAL_FLAG | 0)
#define PROTOCOL_SERIAL_CAS       (PROTOCOL_SERIAL_FLAG | 1)
#define PROTOCOL_SERIAL_CAS40     (PROTOCOL_SERIAL_FLAG | 2)
#define PROTOCOL_SERIAL_CAS50     (PROTOCOL_SERIAL_FLAG | 3)
#define PROTOCOL_SERIAL_CAS100    (PROTOCOL_SERIAL_FLAG | 4)
#define PROTOCOL_SERIAL_CAS300    (PROTOCOL_SERIAL_FLAG | 5)
#define PROTOCOL_SERIAL_SEVEN     (PROTOCOL_SERIAL_FLAG | 6)
#define PROTOCOL_SERIAL_SEVEN_OHP (PROTOCOL_SERIAL_FLAG | 7)
#define PROTOCOL_SERIAL_AUTO      (PROTOCOL_SERIAL_FLAG | PROTOCOL_AUTO_FLAG | 1)
#define PROTOCOL_SERIAL_AUTO_CAS40 \
    (PROTOCOL_SERIAL_FLAG | PROTOCOL_AUTO_FLAG | 2)
#define PROTOCOL_SERIAL_AUTO_CAS50 \
    (PROTOCOL_SERIAL_FLAG | PROTOCOL_AUTO_FLAG | 3)
#define PROTOCOL_SERIAL_AUTO_CAS100 \
    (PROTOCOL_SERIAL_FLAG | PROTOCOL_AUTO_FLAG | 4)
#define PROTOCOL_SERIAL_AUTO_CAS300 \
    (PROTOCOL_SERIAL_FLAG | PROTOCOL_AUTO_FLAG | 5)
#define PROTOCOL_USB_NONE         (PROTOCOL_USB_FLAG | 0)
#define PROTOCOL_USB_CAS300       (PROTOCOL_USB_FLAG | 1)
#define PROTOCOL_USB_SEVEN        (PROTOCOL_USB_FLAG | 2)
#define PROTOCOL_USB_SEVEN_OHP    (PROTOCOL_USB_FLAG | 3)
#define PROTOCOL_USB_MASS_STORAGE (PROTOCOL_USB_FLAG | 4)
#define PROTOCOL_USB_AUTO         (PROTOCOL_USB_FLAG | PROTOCOL_AUTO_FLAG)

/* Other protocol flags for ``cahute_init_link_protocol()``. */
#define PROTOCOL_FLAG_NOCHECK  0x00000100 /* Should not send initial check. */
#define PROTOCOL_FLAG_NOTERM   0x00000200 /* Should not send termination. */
#define PROTOCOL_FLAG_NODISC   0x00000400 /* Should not run discovery. */
#define PROTOCOL_FLAG_RECEIVER 0x00000800 /* Act as a receiver. */

/* Open parameters are defined here rather than in the global internals, in
 * order to make them opaque to platform-specific link opening functions to
 * avoid mistakes. */

/**
 * Open parameters for serial links.
 *
 * @param context Context in which to open the link.
 * @param linkp Pointer to the link to set.
 * @param serial_flags Serial flags to set on opening the link.
 * @param serial_speed Serial speed to set on opening the link.
 * @param init_flags Flags to pass to the protocol initialization function.
 * @param protocol Protocol to initialize the link with.
 */
struct cahute_serial_link_open_params {
    cahute_context *context;
    cahute_link **linkp;
    unsigned long serial_flags;
    unsigned long serial_speed;
    unsigned long init_flags;
    int protocol;
};

/**
 * Open parameters for USB links.
 *
 * @param context Context in which to open the link.
 * @param linkp Pointer to the link to set.
 * @param serial_protocol Protocol to initialize the link with, if found to be
 *        using serial over USB bulk transport.
 * @param ums_protocol Protocol to initialize the link with, if found to be
 *        using UMS transport.
 * @param init_flags Flags to pass to the protocol initialization function.
 */
struct cahute_usb_link_open_params {
    cahute_context *context;
    cahute_link **linkp;
    int serial_protocol;
    int ums_protocol;
    unsigned long init_flags;
};

CAHUTE_INTERNAL(int)
cahute_initialize_link_protocol(
    cahute_link *link,
    int protocol,
    unsigned long flags
);

CAHUTE_INTERNAL(int)
cahute_alloc_link(
    cahute_context *context,
    cahute_link **linkp,
    void *cookie,
    size_t cookie_size
);

#endif /* LINK_INTERNALS_H */
