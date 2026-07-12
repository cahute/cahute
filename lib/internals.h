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

#ifndef INTERNALS_H
#define INTERNALS_H 1
#include <cahute.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Common to both command-line utilities and the library. */
#include <compat.h>

CAHUTE_DECLARE_TYPE(cahute_context_pointer)
CAHUTE_DECLARE_TYPE(cahute_casiolink_data_description)

CAHUTE_DECLARE_TYPE(cahute_serial_link_interface)
CAHUTE_DECLARE_TYPE(cahute_serial_over_usb_bulk_link_interface)
CAHUTE_DECLARE_TYPE(cahute_ums_link_interface)

/* Defined in link/open/internals.h to ensure it stays opaque to
 * platform-specific utilities. */
CAHUTE_DECLARE_TYPE(cahute_serial_link_open_params)
CAHUTE_DECLARE_TYPE(cahute_usb_link_open_params)

CAHUTE_DECLARE_TYPE(cahute_file_open_params)
CAHUTE_DECLARE_TYPE(cahute_file_open_interface)
CAHUTE_DECLARE_TYPE(cahute_file_create_params)
CAHUTE_DECLARE_TYPE(cahute_file_create_interface)
CAHUTE_DECLARE_TYPE(cahute_stdout_open_params)
CAHUTE_DECLARE_TYPE(cahute_stdout_open_interface)

/* ---
 * Endianess management.
 * --- */

CAHUTE_INTERNAL(cahute_u16) cahute_be16toh(cahute_u16 cahute__x);
CAHUTE_INTERNAL(cahute_u16) cahute_le16toh(cahute_u16 cahute__x);
CAHUTE_INTERNAL(cahute_u32) cahute_be32toh(cahute_u32 cahute__x);
CAHUTE_INTERNAL(cahute_u32) cahute_le32toh(cahute_u32 cahute__x);

CAHUTE_INTERNAL(cahute_u16) cahute_htobe16(cahute_u16 cahute__x);
CAHUTE_INTERNAL(cahute_u16) cahute_htole16(cahute_u16 cahute__x);
CAHUTE_INTERNAL(cahute_u32) cahute_htobe32(cahute_u32 cahute__x);
CAHUTE_INTERNAL(cahute_u32) cahute_htole32(cahute_u32 cahute__x);

/* Try to get native macros. */
#if defined(__APPLE__)
# include <libkern/OSByteOrder.h>
# define cahute_macro_be16toh(CAHUTE__X) OSSwapBigToHostInt16(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) OSSwapLittleToHostInt16(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) OSSwapBigToHostInt32(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) OSSwapLittleToHostInt32(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) OSSwapHostToBigInt16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) OSSwapHostToLittleInt16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) OSSwapHostToBigInt32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) OSSwapHostToLittleInt32(CAHUTE__X)
#elif defined(__OpenBSD__)
# include <sys/endian.h>
# define cahute_macro_be16toh(CAHUTE__X) be16toh(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) le16toh(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) be32toh(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) le32toh(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) htobe16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) htole16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) htobe32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) htole32(CAHUTE__X)
#elif defined(__GLIBC__) && defined(__USE_MISC)
# include <endian.h>
# define cahute_macro_be16toh(CAHUTE__X) be16toh(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) le16toh(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) be32toh(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) le32toh(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) htobe16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) htole16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) htobe32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) htole32(CAHUTE__X)
#endif

/* CAHUTE_NO_ENDIAN may be defined by cdefs.c to be able to define the
 * functions prototyped above. */
#ifndef CAHUTE_NO_ENDIAN
# ifdef cahute_macro_be16toh
#  define cahute_be16toh(CAHUTE__X) cahute_macro_be16toh(CAHUTE__X)
# endif
# ifdef cahute_macro_le16toh
#  define cahute_le16toh(CAHUTE__X) cahute_macro_le16toh(CAHUTE__X)
# endif
# ifdef cahute_macro_be32toh
#  define cahute_be32toh(CAHUTE__X) cahute_macro_be32toh(CAHUTE__X)
# endif
# ifdef cahute_macro_le32toh
#  define cahute_le32toh(CAHUTE__X) cahute_macro_le32toh(CAHUTE__X)
# endif
# ifdef cahute_macro_htobe16
#  define cahute_htobe16(CAHUTE__X) cahute_macro_htobe16(CAHUTE__X)
# endif
# ifdef cahute_macro_htole16
#  define cahute_htole16(CAHUTE__X) cahute_macro_htole16(CAHUTE__X)
# endif
# ifdef cahute_macro_htobe32
#  define cahute_htobe32(CAHUTE__X) cahute_macro_htobe32(CAHUTE__X)
# endif
# ifdef cahute_macro_htole32
#  define cahute_htole32(CAHUTE__X) cahute_macro_htole32(CAHUTE__X)
# endif
#endif

/* ---
 * Context definition.
 * --- */

#define CAHUTE_CONTEXT_POINTER_LIBUSB_CONTEXT 0
#define CAHUTE_CONTEXT_POINTER_AMIGAOS_TIMER  1
#define CAHUTE_CONTEXT_POINTER_WIN32_CFGMGR32 2
#define CAHUTE_CONTEXT_POINTER_WIN32_WINUSB   3

#define CAHUTE_CONTEXT_POINTER_COUNT 4 /* Update with the maximum value + 1. */

typedef void(cahute_context_destroy_func)(cahute_context *, void *);
typedef int(cahute_context_init_func)(cahute_context *, void **, cahute_context_destroy_func **);

#define CAHUTE_CONTEXT_POINTER_FLAG_INIT 0x00000001

struct cahute_context_pointer {
    void *value;
    cahute_context_destroy_func *destroy_func;
    unsigned long flags;
};

/**
 * Internal structure of a context.
 *
 * @property log_callback Current logging callback for a context.
 * @property log_callback_cookie Cookie to pass to the current logging
 *           callback.
 * @property log_prefix Prefix to prepend to messages in the default logging
 *           callback.
 * @property log_level Current logging level, as one of the
 *           ``CAHUTE_LOGLEVEL_*`` constants.
 * @property pointers Context pointers.
 */
struct cahute_context {
    cahute_log_func *log_callback;
    void *log_callback_cookie;
    char const *log_prefix;
    int log_level;
    struct cahute_context_pointer pointers[CAHUTE_CONTEXT_POINTER_COUNT];
};

/* ---
 * Logging internals.
 * --- */

CAHUTE_INTERNAL_VA(void)
cahute_log_message(
    cahute_context *cahute__context,
    int cahute__loglevel,
    char const *cahute__func,
    char const *cahute__format,
    ...
);

CAHUTE_INTERNAL(void)
cahute_log_memory(
    cahute_context *cahute__context,
    int cahute__loglevel,
    char const *cahute__func,
    void const *cahute__memory,
    size_t cahute__size
);

CAHUTE_INTERNAL(void)
cahute_log_external_message(
    cahute_context *context,
    int loglevel,
    char const *source,
    char const *func,
    char const *message,
    size_t len
);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# define CAHUTE_LOGFUNC __func__
#elif !defined(__STRICT_ANSI__) && CAHUTE_GNUC_PREREQ(2, 0)
# define CAHUTE_LOGFUNC __FUNCTION__
#else
# define CAHUTE_LOGFUNC NULL
#endif

#define ll_debug 10, CAHUTE_LOGFUNC
#define ll_info  20, CAHUTE_LOGFUNC
#define ll_warn  30, CAHUTE_LOGFUNC
#define ll_error 40, CAHUTE_LOGFUNC
#define ll_fatal 50, CAHUTE_LOGFUNC

#define msg cahute_log_message
#define mem cahute_log_memory

/* Macro to print a message and return CAHUTE_ERROR_IMPL.
 * This is necessary to avoid having to track down exactly what was not
 * implemented in the chain using the message.
 * Usage of this macro is enforced with pre-commit. */
#define CAHUTE_RETURN_IMPL(CONTEXT, MESSAGE) \
    { \
        msg(CONTEXT, ll_error, MESSAGE); \
        return CAHUTE_ERROR_IMPL /* Comment to prevent match by hook. */; \
    } \
    (void)0 /* Force introducing a semicolon. */

/* ---
 * Link internals.
 * --- */

typedef void(cahute_link_close_func)(cahute_context *context, void *cookie);
typedef int(cahute_link_receive_func)(
    cahute_context *context,
    void *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
);
typedef int(cahute_link_send_func)(
    cahute_context *context,
    void *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
);
typedef int(cahute_link_set_serial_params_func)(
    cahute_context *context,
    void *cookie,
    unsigned long flags,
    unsigned long speed
);
typedef int(cahute_link_scsi_request_to_func)(
    cahute_context *context,
    void *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);
typedef int(cahute_link_scsi_request_from_func)(
    cahute_context *context,
    void *cookie,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

/**
 * Serial link interface.
 *
 * @property name Name of the interface, for logging purposes.
 * @property close_func Function used when closing the link, to process the
 *           cookie. Can be set to NULL.
 * @property receive_func Function used to receive bytes on the serial link.
 * @property send_func Function used to send bytes on the serial link.
 * @property set_serial_params_func Function used to set the serial parameters
 *           on the serial link.
 */
struct cahute_serial_link_interface {
    char const *name;
    cahute_link_close_func *close_func;
    cahute_link_receive_func *receive_func;
    cahute_link_send_func *send_func;
    cahute_link_set_serial_params_func *set_serial_params_func;
};

/**
 * Serial over USB bulk link interface.
 *
 * @property name Name of the interface, for logging purposes.
 * @property close_func Function used when closing the link, to process the
 *           cookie. Can be set to NULL.
 * @property receive_func Function used to receive bytes on the link.
 * @property send_func Function used to send bytes on the link.
 */
struct cahute_serial_over_usb_bulk_link_interface {
    char const *name;
    cahute_link_close_func *close_func;
    cahute_link_receive_func *receive_func;
    cahute_link_send_func *send_func;
};

/**
 * UMS (SCSI) link interface.
 *
 * @property name Name of the interface, for logging purposes.
 * @property close_func Function used when closing the link, to process the
 *           cookie. Can be set to NULL.
 * @property request_to_func Function used to make an SCSI request with
 *           optional outgoing data.
 * @property request_from_func Function used to make an SCSI request with
 *           incoming data.
 */
struct cahute_ums_link_interface {
    char const *name;
    cahute_link_close_func *close_func;
    cahute_link_scsi_request_to_func *request_to_func;
    cahute_link_scsi_request_from_func *request_from_func;
};

/* ---
 * File internals.
 * --- */

#define CAHUTE_FILE_READ_BUFFER_SIZE 4096U

#define CAHUTE_MAX_FILE_OFFSET 2147483647

#define CAHUTE_FILE_FLAG_WRITE    0x00000001 /* Can write to medium. */
#define CAHUTE_FILE_FLAG_READ     0x00000002 /* Can read from medium. */
#define CAHUTE_FILE_FLAG_SEEK     0x00000004 /* Can seek on medium. */
#define CAHUTE_FILE_FLAG_SIZE     0x00000008 /* File size is avail. */
#define CAHUTE_FILE_FLAG_EXAMINED 0x00000010 /* File type was examined. */

typedef void(cahute_file_close_func)(cahute_context *, void *);
typedef int(cahute_file_read_func)(cahute_context *, void *, cahute_u8 *, size_t, size_t *);
typedef int(cahute_file_write_func)(cahute_context *, void *, cahute_u8 const *, size_t, size_t *);
typedef int(cahute_file_seek_func)(cahute_context *, void *, unsigned long, unsigned long *);

/**
 * File related information.
 *
 * @property context Context in which the file is defined and used.
 * @property flags Flags.
 * @property file_size File size computed when the file was opened.
 * @property offset Current offset on the underlying medium.
 * @property read_offset Current offset of the read buffer.
 * @property read_size Number of bytes in the read buffer, starting at the
 *           offset stored in ``read_offset``.
 * @property read_buffer Buffer for reading from the medium in a stream-like
 *           interface. See ``cahute_read_from_file`` definition for more
 *           information. Guaranteed to be 32-byte aligned.
 * @property cookie Cookie to pass to the underlying medium.
 * @property close_func Function to call when closing the file.
 * @property read_func Function to call when reading from the current offset
 *           in the file.
 * @property write_func Function to call when writing from the current offset
 *           in the file.
 * @property seek_func Function to call when changing the current offset in
 *           the file.
 * @property type Found file type.
 *           If flag CAHUTE_FILE_FLAG_EXAMINED is present and this is
 *           set to 0, this means that the file has been examined but no
 *           known type was found.
 * @property extension Extension normalized in ASCII lowercase, for later use
 *           in guessing, if found in the file name.
 */
struct cahute_file {
    unsigned long flags;
    unsigned long file_size;
    unsigned long offset;
    unsigned long read_offset;
    size_t read_size;

    cahute_context *context;
    void *cookie;
    cahute_u8 *read_buffer;
    cahute_file_close_func *close_func;
    cahute_file_read_func *read_func;
    cahute_file_write_func *write_func;
    cahute_file_seek_func *seek_func;

    int type;
    char extension[5];
};

struct cahute_file_open_params {
    cahute_context *context;
    cahute_file **filep;
    void const *path;
    int path_type;
};

struct cahute_file_open_interface {
    cahute_file_close_func *close_func;
    cahute_file_read_func *read_func;
    cahute_file_seek_func *seek_func;
};

struct cahute_file_create_params {
    cahute_context *context;
    cahute_file **filep;
    unsigned long file_size;
};

struct cahute_file_create_interface {
    cahute_file_close_func *close_func;
    cahute_file_write_func *write_func;
    cahute_file_seek_func *seek_func;
};

struct cahute_stdout_open_params {
    cahute_context *context;
    cahute_file **filep;
};

struct cahute_stdout_open_interface {
    cahute_file_close_func *close_func;
    cahute_file_write_func *write_func;
};

/* Internal function to declare a file for a memory buffer, without having
 * to use dynamic memory. */
CAHUTE_INTERNAL(void)
cahute_populate_file_from_memory(
    cahute_file *file,
    cahute_context *context,
    cahute_u8 *buf,
    size_t size
);

CAHUTE_INTERNAL(int)
cahute_create_file_from_interface(
    cahute_file_create_params *create_params,
    cahute_file_create_interface const *interface,
    void *cookie,
    size_t cookie_size
);

CAHUTE_INTERNAL(int)
cahute_open_file_from_interface(
    cahute_file_open_params *open_params,
    cahute_file_open_interface const *interface,
    void *cookie,
    size_t cookie_size,
    unsigned long file_size
);

CAHUTE_INTERNAL(int)
cahute_open_stdout_from_interface(
    cahute_stdout_open_params *open_params,
    cahute_stdout_open_interface const *interface,
    void *cookie,
    size_t cookie_size
);

CAHUTE_INTERNAL(int)
cahute_checksum_from_file(
    cahute_file *file,
    unsigned long offset,
    size_t size,
    unsigned int *checksump
);

/* ---
 * Platform-specific functions.
 * --- */

#if CAHUTE_PLATFORM_AMIGAOS
CAHUTE_INTERNAL(int)
cahute_amigaos_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_amigaos_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);
#endif

#if CAHUTE_PLATFORM_LINUX
CAHUTE_INTERNAL(int)
cahute_linux_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);
#endif

#if CAHUTE_PLATFORM_POSIX
CAHUTE_INTERNAL(size_t) cahute_get_posix_path_max(cahute_context *context);

CAHUTE_INTERNAL(int) cahute_posix_is_stderr_tty(void);

CAHUTE_INTERNAL(int)
cahute_posix_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_posix_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);

CAHUTE_INTERNAL(int)
cahute_create_posix_file(
    cahute_context *context,
    cahute_file_create_params *create_params,
    unsigned long file_size,
    void const *path,
    int path_type
);

CAHUTE_INTERNAL(int)
cahute_open_posix_file(
    cahute_context *context,
    cahute_file_open_params *open_params,
    void const *path,
    int path_type
);

CAHUTE_INTERNAL(int)
cahute_open_posix_stdout(
    cahute_context *context,
    cahute_stdout_open_params *open_params
);
#endif

#if CAHUTE_PLATFORM_WIN32
CAHUTE_INTERNAL(int)
cahute_win32_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_win32_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);

CAHUTE_INTERNAL(int)
cahute_win32_detect_usb(
    cahute_context *context,
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_win32_usb_device(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *name
);

CAHUTE_INTERNAL(int)
cahute_create_win32_file(
    cahute_context *context,
    cahute_file_create_params *create_params,
    unsigned long file_size,
    void const *path,
    int path_type
);

CAHUTE_INTERNAL(int)
cahute_open_win32_file(
    cahute_context *context,
    cahute_file_open_params *create_params,
    void const *path,
    int path_type
);

CAHUTE_INTERNAL(int)
cahute_open_win32_stdout(
    cahute_context *context,
    cahute_stdout_open_params *open_params
);
#endif

#if CAHUTE_PLATFORM_WIN16
CAHUTE_INTERNAL(int)
cahute_win16_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_win16_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);
#endif

#if CAHUTE_PLATFORM_LIBUSB
CAHUTE_INTERNAL(int)
cahute_libusb_detect_usb(
    cahute_context *context,
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_open_libusb_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    char const *path
);
#endif

/* ---
 * Miscellaneous functions.
 * --- */

CAHUTE_INTERNAL(int) cahute_sleep(cahute_context *context, unsigned long ms);

CAHUTE_INTERNAL(int)
cahute_monotonic(cahute_context *context, unsigned long *msp);

CAHUTE_INTERNAL(int)
cahute_pad_data(cahute_u8 *buf, cahute_u8 const *data, size_t data_size);

CAHUTE_INTERNAL(int)
cahute_unpad_data(
    cahute_u8 *buf,
    size_t *buf_sizep,
    cahute_u8 const *data,
    size_t data_size
);

CAHUTE_INTERNAL(void)
cahute_set_ascii_hex(cahute_u8 *buf, unsigned int number);

CAHUTE_INTERNAL(char *)
cahute_copy_ff_string(char *buf, cahute_u8 const *raw, size_t max_size);

CAHUTE_INTERNAL(unsigned long) cahute_get_long_hex(cahute_u8 const *raw);
CAHUTE_INTERNAL(unsigned long) cahute_get_long_dec(cahute_u8 const *raw);

#define cahute_is_ascii_hex(C) \
    (((C) >= '0' && (C) <= '9') || ((C) >= 'A' && (C) <= 'F'))
#define cahute_ascii_hex_to_nibble(C) ((C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')

CAHUTE_INTERNAL(unsigned int)
cahute_checksum(cahute_u8 const *data, size_t size);

#define cahute_checksub(CAHUTE__BUF, CAHUTE__SIZE) \
    ((~cahute_checksum((CAHUTE__BUF), (CAHUTE__SIZE)) + 1) & 255)
#define cahute_checksub_from_checksum(CAHUTE__RESULT) \
    ((~(CAHUTE__RESULT) + 1) & 255)

/* ---
 * Context management functions.
 * --- */

CAHUTE_INTERNAL(int)
cahute_get_context_pointer(
    cahute_context *context,
    void **valuep,
    int key,
    cahute_context_init_func *init_func
);

/* ---
 * Link opening and management functions.
 * --- */

/* From link_open_serial.c */
CAHUTE_INTERNAL(int)
cahute_open_serial_link_from_interface(
    cahute_serial_link_open_params *open_params,
    cahute_serial_link_interface const *interface,
    void *cookie,
    size_t cookie_size
);

/* From link_open_serial_over_usb_bulk.c */
CAHUTE_INTERNAL(int)
cahute_open_serial_over_usb_bulk_link_from_interface(
    cahute_usb_link_open_params *open_params,
    cahute_serial_over_usb_bulk_link_interface const *interface,
    void *cookie,
    size_t cookie_size
);

/* From link_open_ums.c */
CAHUTE_INTERNAL(int)
cahute_open_ums_link_from_interface(
    cahute_usb_link_open_params *open_params,
    cahute_ums_link_interface const *interface,
    void *cookie,
    size_t cookie_size
);

/* From link_init.c */
CAHUTE_INTERNAL(int) cahute_initialize_link(cahute_link *link);

CAHUTE_INTERNAL(char const *) cahute_get_protocol_name(int protocol);

/* ---
 * Link transport functions.
 * --- */

CAHUTE_INTERNAL(int)
cahute_receive_on_link_transport(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    unsigned long first_timeout,
    unsigned long next_timeout
);

CAHUTE_INTERNAL(int)
cahute_send_on_link_transport(
    cahute_link *link,
    cahute_u8 const *buf,
    size_t size
);

CAHUTE_INTERNAL(int)
cahute_set_serial_params_on_link_transport(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_INTERNAL(int)
cahute_scsi_request_to_link_transport(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);

CAHUTE_INTERNAL(int)
cahute_scsi_request_from_link_transport(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

CAHUTE_INTERNAL(int)
cahute_receive_byte_on_link_transport(
    cahute_link *link,
    int *bytep,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_send_byte_on_link_transport(cahute_link *link, int byte);

/* ---
 * Data management, defined in data.c
 * --- */

CAHUTE_INTERNAL(int)
cahute_create_program_from_file(
    cahute_data **datap,
    int encoding,
    void const *name,
    size_t name_size,
    void const *password,
    size_t password_size,
    cahute_file *file,
    unsigned long content_offset,
    size_t content_size
);

/* ---
 * CASIOLINK header and file format management, defined in casiolink.c
 * --- */

#define CAHUTE_CASIOLINK_DATA_FLAG_END    0x00000001 /* Ends communication. */
#define CAHUTE_CASIOLINK_DATA_FLAG_FINAL  0x00000002 /* Final. */
#define CAHUTE_CASIOLINK_DATA_FLAG_AL     0x00000004 /* Starts AL mode. */
#define CAHUTE_CASIOLINK_DATA_FLAG_AL_END 0x00000008 /* Ends AL mode. */
#define CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG 0x00000010 /* Do not log part. */
#define CAHUTE_CASIOLINK_DATA_FLAG_MDL    0x00000020 /* Is CAS100 MDL data. */

/**
 * Data description to be determined from a header.
 *
 * This allows, in the CASIOLINK protocol implementation, to separate reading
 * and acknowledging over the link from the file decoding part.
 * It can be determined from a header and variant using the
 * ``cahute_casiolink_determine_data_description()`` function.
 *
 * A few examples of such structure are the following:
 *
 * ``{part_count=0}``
 *     No data part accompanying the header.
 *
 * ``{part_count=1, last_part_repeat=1, part_sizes=[55]}
 *     One data part of size 55 bytes accompanying the header.
 *
 * ``{part_count=2, last_part_repeat=1, part_sizes=[56, 57]}``
 *     Two data parts of respective sizes 56 and 57 bytes accompanying the
 *     header.
 *
 * ``{part_count=2, last_part_repeat=3, part_sizes=[32, 16]}``
 *     Four data parts, of respective sizes 32, 16, 16 and 16 bytes
 *     accompanying the header.
 *
 * @param flags Data description flags, using ``CAHUTE_CASIOLINK_DATA_FLAG_*``
 *        values.
 * @param packet_type Packet type (first byte of the packet) to be expected
 *        with the data parts.
 * @param part_count Number of part sizes used in the ``part_sizes`` array.
 * @param last_part_repeat How much times the last part is repeated.
 * @param part_sizes Distinct part sizes.
 */
struct cahute_casiolink_data_description {
    unsigned long flags;
    int packet_type;
    size_t part_count;
    size_t last_part_repeat;
    size_t part_sizes[5];
};

CAHUTE_INTERNAL(size_t)
cahute_casiolink_compute_data_description_size(
    struct cahute_casiolink_data_description const *desc
);

CAHUTE_INTERNAL(int)
cahute_casiolink_check_file_data(
    cahute_file *file,
    unsigned long offset,
    struct cahute_casiolink_data_description const *desc
);

CAHUTE_INTERNAL(void)
cahute_casiolink_log_data_description(
    cahute_context *context,
    struct cahute_casiolink_data_description const *desc
);

CAHUTE_INTERNAL(int)
cahute_casiolink_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp
);

CAHUTE_INTERNAL(int)
cahute_casiolink_receive_raw_data(
    cahute_link *link,
    struct cahute_casiolink_data_description const *desc,
    cahute_u8 *buf,
    size_t *buf_sizep
);

/* Make the CASIOLINK handshake only. */
CAHUTE_INTERNAL(int) cahute_casiolink_initiate_as_receiver(cahute_link *link);
CAHUTE_INTERNAL(int) cahute_casiolink_initiate_as_sender(cahute_link *link);

CAHUTE_INTERNAL(int)
cahute_casiolink_receive_first_byte(
    cahute_link *link,
    int *first_bytep,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_casiolink_receive_packet(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    int expected_type,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_casiolink_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
);

/* ---
 * CASIOLINK main memory decoding functions.
 * --- */

CAHUTE_INTERNAL(int)
cahute_cas40_decode_data(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long *offsetp
);

CAHUTE_INTERNAL(int)
cahute_cas50_decode_data(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long *offsetp
);

/* ---
 * MCS encoding and decoding functions, defined in mcs.c
 * --- */

CAHUTE_INTERNAL(int)
cahute_mcs_decode_data(
    cahute_context *context,
    cahute_data **datap,
    cahute_u8 const *group,
    size_t group_size,
    cahute_u8 const *directory,
    size_t directory_size,
    cahute_u8 const *name,
    size_t name_size,
    cahute_file *file,
    unsigned long content_offset,
    size_t content_size,
    int data_type
);

#endif /* INTERNALS_H */
