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

#ifndef PLATFORM_POSIX_INTERNALS_H
#define PLATFORM_POSIX_INTERNALS_H 1
#include "../../internals.h"
#include <libusb.h>

#if LIBUSB_API_VERSION >= 0x01000108 /* libusb 1.0.24 */
# define IS_LIBUSB_BULK_ENDPOINT(EP) \
     (((EP)->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) \
      != LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
#else
# define IS_LIBUSB_BULK_ENDPOINT(EP) \
     (((EP)->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) \
      != LIBUSB_TRANSFER_TYPE_BULK)
#endif

CAHUTE_DECLARE_TYPE(cahute_libusb_link_cookie)

/**
 * libusb common cookie definition for both serial over USB bulk and UMS.
 *
 * @property handle Device handle.
 * @property bulk_in Identifier of the BULK IN endpoint.
 * @property bulk_out Identifier of the BULK OUT endpoint.
 */
struct cahute_libusb_link_cookie {
    libusb_device_handle *handle;
    int bulk_in;
    int bulk_out;
};

CAHUTE_EXTERN(void)
cahute_close_libusb_link(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie
);

CAHUTE_EXTERN(int)
cahute_receive_on_libusb_bulk_link(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_send_on_libusb_bulk_link(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
);

CAHUTE_EXTERN(int)
cahute_scsi_request_to_libusb_device(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);

CAHUTE_EXTERN(int)
cahute_scsi_request_from_libusb_device(
    cahute_context *context,
    cahute_libusb_link_cookie *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

CAHUTE_EXTERN(int)
cahute_get_libusb_context(cahute_context *context, libusb_context **contextp);

#endif /* PLATFORM_POSIX_INTERNALS_H */
