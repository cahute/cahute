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
#include <dirent.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/select.h>

#if CMAKE_PLATFORM_LINUX && HAVE_LINUX_SERIAL_H
# include <linux/serial.h>
#endif

CAHUTE_DECLARE_TYPE(cahute_posix_serial_link_cookie)
CAHUTE_DECLARE_TYPE(cahute_posix_file_cookie)

/* ---
 * Link internals.
 * --- */

/**
 * POSIX link state.
 *
 * @property fd File descriptor on the opened link.
 */
struct cahute_posix_serial_link_cookie {
    int fd;
};

CAHUTE_INTERNAL(void)
cahute_close_posix_serial_link(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie
);

CAHUTE_INTERNAL(int)
cahute_receive_on_posix_serial_link(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_send_on_posix_serial_link(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
);

CAHUTE_INTERNAL(int)
cahute_set_posix_serial_link_params(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
);

/* ---
 * File internals.
 * --- */

/**
 * POSIX file state.
 *
 * @property close Whether to close the file descriptor.
 * @property fd File descriptor on the opened medium.
 */
struct cahute_posix_file_cookie {
    int close;
    int fd;
};

CAHUTE_INTERNAL(void)
cahute_close_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie
);

CAHUTE_INTERNAL(int)
cahute_read_from_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie,
    cahute_u8 *buf,
    size_t size,
    size_t *readp
);

CAHUTE_INTERNAL(int)
cahute_write_to_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *writtenp
);

CAHUTE_INTERNAL(int)
cahute_move_in_posix_file(
    cahute_context *context,
    cahute_posix_file_cookie *cookie,
    unsigned long offset,
    unsigned long *offsetp
);

#endif /* PLATFORM_POSIX_INTERNALS_H */
