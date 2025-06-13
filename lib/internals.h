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

CAHUTE_EXTERN(cahute_u16) cahute_be16toh(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u16) cahute_le16toh(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_be32toh(cahute_u32 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_le32toh(cahute_u32 cahute__x);

CAHUTE_EXTERN(cahute_u16) cahute_htobe16(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u16) cahute_htole16(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_htobe32(cahute_u32 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_htole32(cahute_u32 cahute__x);

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

#define CAHUTE_CONTEXT_POINTER_LIBUSB_CONTEXT   0
#define CAHUTE_CONTEXT_POINTER_AMIGAOS_TIMER    1
#define CAHUTE_CONTEXT_POINTER_WIN32_CFGMGR32   2
#define CAHUTE_CONTEXT_POINTER_WIN32_WINUSB     3
#define CAHUTE_CONTEXT_POINTER_WIN32_HUB_REF_ID 4

#define CAHUTE_CONTEXT_POINTER_COUNT 5 /* Update with the maximum value + 1. */

typedef void(cahute_context_destroy_func)(cahute_context *, void *);
typedef int(cahute_context_init_func)(cahute_context *, void **, cahute_context_destroy_func **);

#define CAHUTE_CONTEXT_POINTER_FLAG_INIT 0x00000001

struct cahute_context_pointer {
    void *value;
    cahute_context_destroy_func *destroy_func;
    unsigned long flags;
};

struct cahute_context {
    cahute_log_func *log_callback;
    void *log_callback_cookie;
    int log_level;
    struct cahute_context_pointer pointers[CAHUTE_CONTEXT_POINTER_COUNT];
};

/* ---
 * Logging internals.
 * --- */

CAHUTE_EXTERN(void)
cahute_log_message(
    cahute_context *cahute__context,
    int cahute__loglevel,
    char const *cahute__func,
    char const *cahute__format,
    ...
);

CAHUTE_EXTERN(void)
cahute_log_memory(
    cahute_context *cahute__context,
    int cahute__loglevel,
    char const *cahute__func,
    void const *cahute__memory,
    size_t cahute__size
);

CAHUTE_EXTERN(void)
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

#define CAHUTE_LINK_RECEIVE_BUFFER_SIZE 32768U

/* Flags that can be present on a link at runtime. */
#define CAHUTE_LINK_FLAG_TERMINATE 0x00000002UL /* Should terminate. */
#define CAHUTE_LINK_FLAG_RECEIVER  0x00000004UL /* Act as a receiver. */

#define CAHUTE_LINK_FLAG_GONE          0x00000100UL /* No longer available. */
#define CAHUTE_LINK_FLAG_TERMINATED    0x00000200UL /* Was terminated! */
#define CAHUTE_LINK_FLAG_IRRECOVERABLE 0x00000400UL /* Cannot recover. */
#define CAHUTE_LINK_FLAG_ALMODE        0x00000800UL /* CAS40 AL received. */

/* Transport type stored in ``link->transport``. */
#define CAHUTE_LINK_TRANSPORT_SERIAL               1
#define CAHUTE_LINK_TRANSPORT_SERIAL_OVER_USB_BULK 2
#define CAHUTE_LINK_TRANSPORT_UMS                  3

/* Protocol stored in ``link->protocol``. */
#define CAHUTE_LINK_PROTOCOL_SERIAL_NONE      1
#define CAHUTE_LINK_PROTOCOL_SERIAL_CAS       2 /* Generic. */
#define CAHUTE_LINK_PROTOCOL_SERIAL_CAS40     3
#define CAHUTE_LINK_PROTOCOL_SERIAL_CAS50     4
#define CAHUTE_LINK_PROTOCOL_SERIAL_CAS100    5
#define CAHUTE_LINK_PROTOCOL_SERIAL_CAS300    6
#define CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN     7
#define CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP 8

#define CAHUTE_LINK_PROTOCOL_USB_NONE         11
#define CAHUTE_LINK_PROTOCOL_USB_CAS300       12
#define CAHUTE_LINK_PROTOCOL_USB_SEVEN        13
#define CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP    14
#define CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE 15

/* Absolute minimum buffer size for CASIOLINK. */
#define CASIOLINK_MINIMUM_BUFFER_SIZE 50

/* Raw device information size for CAS100. */
#define CAS100_RAW_DEVICE_INFO_SIZE 33

/* Raw device information size for CASIOLINK.
 * CAS100 device information is 33 bytes long.
 * CAS300 device information is 49 bytes long. */
#define CASIOLINK_RAW_DEVICE_INFO_BUFFER_SIZE 49

/* Flag to describe whether device information was obtained or not. */
#define CASIOLINK_FLAG_DEVICE_INFO_OBTAINED 0x00000001UL

/* Flag to describe whether the obtained device info was of CAS300 type
 * (49 bytes long), or CAS100 type (33 bytes long). */
#define CASIOLINK_FLAG_DEVICE_INFO_CAS300 0x00000002UL

/* Flag to describe whether the calculator has provided an AL packet, to
 * determine whether an END ends the communication or not. */
#define CASIOLINK_FLAG_DEVICE_INFO_CAS40_AL 0x00000004UL

/* Maximum size of raw data that can come from a CAS100 command or data packet.
 * Calculators have an obligatory 9 bytes of metadata (1 byte packet type,
 * 2 byte packet identifier, 4 byte payload size, and 2 byte checksum),
 * and seem to support payloads to up to 512 raw bytes (1024 encoded bytes).
 *
 * Payloads corresponding to commands have a minimum of 4 bytes, and occupies
 * the first 4 bytes with the command identifier.
 *
 * NOTE: It is unsure if the 4 bytes of the command are actually counted in the
 *       512 bytes or not. By security (mostly on reception), we consider that
 *       it does not, and therefore, the maximum packet size is 9 + 4 + 1024,
 *       resulting in 1037 bytes. */
#define CAS300_MAX_PAYLOAD_SIZE         512U
#define CAS300_MAX_ENCODED_PAYLOAD_SIZE 1024U
#define CAS300_MAX_PACKET_SIZE          1037U

/* Timeouts common to all CASIOLINK variants. */
#define CASIOLINK_TIMEOUT_PACKET_CONTENTS 2000

/**
 * Peer state for CAS300.
 *
 * @property next_id Next identifier to use on sent packets.
 * @property packet_type Type of the last packet.
 * @property packet_subtype Command number in the last received packet.
 * @property packet_id Identifier of the last received packet.
 * @property packet_payload Payload of the last received command or
 *           data packet.
 * @property packet_payload_size Payload size of the last received command or
 *           data packet.
 */
struct cahute_cas300_state {
    int next_id;
    int packet_type;
    unsigned int packet_subtype;
    cahute_u8 packet_id[2];
    size_t packet_payload_size;
    cahute_u8 packet_payload[CAS300_MAX_PAYLOAD_SIZE];
};

/**
 * Peer state for all CASIOLINK protocols.
 *
 * @property flags Flags for the CASIOLINK peer state.
 * @property cas300 CAS300 peer state.
 * @property raw_device_info Raw device information buffer, so that data
 *           can be extracted later if actual device information is requested.
 */
struct cahute_casiolink_state {
    unsigned long flags;
    struct cahute_cas300_state cas300;
    cahute_u8 raw_device_info[CASIOLINK_RAW_DEVICE_INFO_BUFFER_SIZE];
};

/* Maximum size of raw data that can come from an extended packet.
 * Calculators support data packets with up to 256 raw bytes (512 encoded
 * bytes), but fxRemote uses payloads that go up to 1028 raw bytes
 * (2056 encoded bytes). */
#define SEVEN_MAX_PACKET_DATA_SIZE         1028
#define SEVEN_MAX_ENCODED_PACKET_DATA_SIZE 2056 /* Max data size x 2. */
#define SEVEN_MAX_PACKET_SIZE              2066 /* Enc. data size + 10. */

/* Size of the raw device information buffer for Protocol 7.00.
 * This actually varies between devices: the fx-9860G use 164 bytes,
 * the fx-CG use 188 bytes. */
#define SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE 200

/* Flag to describe whether device information has been requested. */
#define SEVEN_FLAG_DEVICE_INFO_REQUESTED 0x00000001UL

/**
 * Protocol 7.00 peer state.
 *
 * @property flags Flags for the Protocol 7.00 peer state.
 * @property last_command Code of the last executed command. Protocol 7.00
 *           requires the code of the corresponding command to be placed as
 *           the subtype of subsequent data packets.
 * @property last_packet_type Type of the last received packet, or -1 if not
 *           available.
 * @property last_packet_subtype Subtype of the last received packet, or -1
 *           if not available.
 * @property last_packet_data Buffer to the last packet data.
 * @property last_packet_data_size Size of the last packet data.
 * @property raw_device_info Raw device information buffer, so that data can
 *           be extracted later if actual device information is requested.
 * @property raw_device_info_size Raw device information size (not capacity).
 */
struct cahute_seven_state {
    unsigned long flags;

    int last_command;

    int last_packet_type;
    int last_packet_subtype;

    size_t last_packet_data_size;
    size_t raw_device_info_size;

    cahute_u8 last_packet_data[SEVEN_MAX_PACKET_DATA_SIZE];
    cahute_u8 raw_device_info[SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE];
};

/**
 * Protocol 7.00 screenstreaming receiver state.
 *
 * If reception of a frame packet has been successful, the data buffer
 * will contain the frame data.
 *
 * @property last_packet_type Type of the last received packet, or -1 if not
 *           available.
 * @property last_packet_subtype Subtype of the last received packet, if
 *           relevant.
 * @property picture_format Type of the last received picture, as a
 *           ``CAHUTE_PICTURE_FORMAT_*`` constant.
 * @property picture_width Width of the last picture in pixels, -1
 *           if not relevant.
 * @property picture_height Height of the last picture in pixels, -1
 *           if not relevant.
 */
struct cahute_seven_ohp_state {
    int last_packet_type;
    int picture_format;
    int picture_width;
    int picture_height;

    cahute_u8 last_packet_subtype[5];
};

/**
 * Link protocol client state, to be used depending on the protocol selected
 * in the link flags.
 *
 * @property casiolink CASIOLINK peer state.
 * @property seven Protocol 7.00 peer state.
 * @property seven_ohp Protocol 7.00 screenstreaming receiver state.
 */
union cahute_link_protocol_state {
    struct cahute_casiolink_state casiolink;
    struct cahute_seven_state seven;
    struct cahute_seven_ohp_state seven_ohp;
};

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

/**
 * Internal base link representation.
 *
 * @property context Context in which the link is defined.
 * @property flags Link flags, as OR'd ``CAHUTE_LINK_FLAG_*`` constants.
 * @property transport Transport type and protocol, as any
 *           ``CAHUTE_LINK_TRANSPORT_*`` constant.
 * @property transport_cookie Cookie used by the transport.
 * @property transport_stream_cookie Cookie used by the send and receive functions
 *           for the transport specifically, which may be different from the cookie
 *           used by the rest of the functions.
 * @property transport_serial_flags Current serial flags, as or'd
 *           ``CAHUTE_SERIAL_FLAG_*`` constants.
 * @property transport_serial_speed Current serial speed.
 * @property transport_receive_buffer Buffer for receiving from the transport in
 *           a stream-like interface. See ``cahute_receive_on_link_transport``
 *           definition for more information. Guaranteed to be 32-byte aligned.
 * @property transport_receive_start Offset at which the unread data starts
 *           within the receive buffer for the transport.
 * @property transport_receive_size Number of unread bytes in the receive buffer
 *           for the transport, starting at the offset stored in
 *           ``transport_receive_start``.
 * @property protocol Protocol type, as any ``CAHUTE_LINK_PROTOCOL_*`` constant
 *           representing the protocol state to use.
 * @property protocol_state State of the specific protocol to use, e.g.
 *           current role in the protocol and details regarding the last
 *           received packet.
 *           The protocol data buffer is not included within this property.
 * @property cached_device_info Device information, if it has been requested
 *           at least once, so it can be free'd when the link is closed.
 * @property data_buffer General-purpose buffer for the protocol
 *           implementation to use. This can contain payloads, frame data,
 *           etc.
 * @property data_buffer_size Size of the data currently present within
 *           the data buffer, in bytes.
 * @property data_buffer_capacity Total amount of data the data buffer
 *           can contain, in bytes.
 */
struct cahute_link {
    cahute_context *context;
    unsigned long flags;
    int protocol, transport;

    void *transport_cookie;
    void *transport_stream_cookie;
    char const *transport_name; /* TODO: add description */
    unsigned long transport_serial_flags;
    unsigned long transport_serial_speed;

    cahute_u8 *transport_receive_buffer;
    size_t transport_receive_start;
    size_t transport_receive_size;

    /* TODO: Add description for these. */
    cahute_link_close_func *transport_close_func;
    cahute_link_receive_func *transport_receive_func;
    cahute_link_send_func *transport_send_func;
    cahute_link_set_serial_params_func *transport_set_serial_params_func;
    cahute_link_scsi_request_to_func *transport_scsi_request_to_func;
    cahute_link_scsi_request_from_func *transport_scsi_request_from_func;

    union cahute_link_protocol_state protocol_state;

    cahute_device_info *cached_device_info;

    /* Raw data buffer, used by the protocol implementation to store raw data.
     * This can be of varying length depending on the protocol in use.
     * The buffer is allocated in the same block as the link. */
    cahute_u8 *data_buffer;
    size_t data_buffer_size, data_buffer_capacity;

    /* Stored frame, so that screen reception does not use dynamic
     * memory allocation for every frame. */
    cahute_frame stored_frame;
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
CAHUTE_EXTERN(void)
cahute_populate_file_from_memory(
    cahute_file *file,
    cahute_context *context,
    cahute_u8 *buf,
    size_t size
);

CAHUTE_EXTERN(int)
cahute_create_file_from_interface(
    cahute_file_create_params *create_params,
    cahute_file_create_interface const *interface,
    void *cookie,
    size_t cookie_size
);

CAHUTE_EXTERN(int)
cahute_open_file_from_interface(
    cahute_file_open_params *open_params,
    cahute_file_open_interface const *interface,
    void *cookie,
    size_t cookie_size,
    unsigned long file_size
);

CAHUTE_EXTERN(int)
cahute_open_stdout_from_interface(
    cahute_stdout_open_params *open_params,
    cahute_stdout_open_interface const *interface,
    void *cookie,
    size_t cookie_size
);

CAHUTE_EXTERN(int)
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
CAHUTE_EXTERN(int)
cahute_amigaos_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_open_amigaos_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);
#endif

#if CAHUTE_PLATFORM_LINUX
CAHUTE_EXTERN(int)
cahute_linux_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);
#endif

#if CAHUTE_PLATFORM_POSIX
CAHUTE_EXTERN(size_t) cahute_get_posix_path_max(cahute_context *context);

CAHUTE_EXTERN(int)
cahute_posix_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_open_posix_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);

CAHUTE_EXTERN(int)
cahute_create_posix_file(
    cahute_context *context,
    cahute_file_create_params *create_params,
    unsigned long file_size,
    void const *path,
    int path_type
);

CAHUTE_EXTERN(int)
cahute_open_posix_file(
    cahute_context *context,
    cahute_file_open_params *open_params,
    void const *path,
    int path_type
);

CAHUTE_EXTERN(int)
cahute_open_posix_stdout(
    cahute_context *context,
    cahute_stdout_open_params *open_params
);
#endif

#if CAHUTE_PLATFORM_WIN32
CAHUTE_EXTERN(int)
cahute_win32_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_open_win32_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
);

CAHUTE_EXTERN(int)
cahute_win32_detect_usb(
    cahute_context *context,
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_open_win32_usb_device_from_address(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    int bus,
    int address
);

CAHUTE_EXTERN(int)
cahute_create_win32_file(
    cahute_context *context,
    cahute_file_create_params *create_params,
    unsigned long file_size,
    void const *path,
    int path_type
);

CAHUTE_EXTERN(int)
cahute_open_win32_file(
    cahute_context *context,
    cahute_file_open_params *create_params,
    void const *path,
    int path_type
);

CAHUTE_EXTERN(int)
cahute_open_win32_stdout(
    cahute_context *context,
    cahute_stdout_open_params *open_params
);
#endif

#if CAHUTE_PLATFORM_LIBUSB
CAHUTE_EXTERN(int)
cahute_libusb_detect_usb(
    cahute_context *context,
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_open_libusb_link(
    cahute_context *context,
    cahute_usb_link_open_params *open_params,
    int bus,
    int address
);
#endif

/* ---
 * Miscellaneous functions, defined in misc.c
 * --- */

CAHUTE_EXTERN(int) cahute_sleep(cahute_context *context, unsigned long ms);

CAHUTE_EXTERN(int)
cahute_monotonic(cahute_context *context, unsigned long *msp);

CAHUTE_EXTERN(int)
cahute_pad_data(cahute_u8 *buf, cahute_u8 const *data, size_t data_size);

CAHUTE_EXTERN(int)
cahute_unpad_data(
    cahute_u8 *buf,
    size_t *buf_sizep,
    cahute_u8 const *data,
    size_t data_size
);

/**
 * Compute an 2-byte ASCII-HEX number representation on a given buffer.
 *
 * @param buf Buffer on which to represent the number.
 * @param number Number to represent.
 */
CAHUTE_INLINE(void)
cahute_set_ascii_hex(cahute_u8 *buf, unsigned int number) {
    buf[0] = "0123456789ABCDEF"[(number >> 4) & 15];
    buf[1] = "0123456789ABCDEF"[number & 15];
}

/**
 * Copy a string from a payload to a buffer, while null-terminating it
 * and detecting 0xFF characters as end of strings.
 *
 * SECURITY: The destination buffer is expected to be at least
 * ``max_size + 1`` long.
 *
 * @param bufp Pointer to the buffer pointer for where to copy the data.
 *        This method will increment the pointer to after the end of the
 *        copied string with the null terminator, so that other strings or
 *        pieces of data can be copied after.
 * @param raw Raw data from which to get the string.
 * @param max_size Maximum size to read from raw data.
 * @return Pointer to the obtained string.
 */
CAHUTE_INLINE(char *)
cahute_copy_ff_string(char **bufp, cahute_u8 const *raw, size_t max_size) {
    char *buf = *bufp, *result = buf;

    for (; max_size--; raw++) {
        int byte = *raw;

        if (!byte || byte >= 128)
            break;

        *(unsigned char *)buf++ = byte;
    }

    *buf++ = '\0';
    *bufp = buf;
    return result;
}

#define cahute_is_ascii_hex(C) \
    (((C) >= '0' && (C) <= '9') || ((C) >= 'A' && (C) <= 'F'))
#define cahute_ascii_hex_to_nibble(C) ((C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')

/**
 * Compute a checksub.
 *
 * @param data Buffer to read from.
 * @param size Size of the buffer to read from.
 * @return Computed checksum.
 */
CAHUTE_INLINE(unsigned int)
cahute_checksum(cahute_u8 const *data, size_t size) {
    unsigned int checksum = 0;

    for (; size; size--)
        checksum += *data++;

    return checksum;
}

#define cahute_checksub(CAHUTE__BUF, CAHUTE__SIZE) \
    ((~cahute_checksum((CAHUTE__BUF), (CAHUTE__SIZE)) + 1) & 255)
#define cahute_checksub_from_checksum(CAHUTE__RESULT) \
    ((~(CAHUTE__RESULT) + 1) & 255)

/* ---
 * Context management functions.
 * --- */

CAHUTE_EXTERN(int)
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
CAHUTE_EXTERN(int)
cahute_open_serial_link_from_interface(
    cahute_serial_link_open_params *open_params,
    cahute_serial_link_interface const *interface,
    void *cookie,
    size_t cookie_size
);

/* From link_open_serial_over_usb_bulk.c */
CAHUTE_EXTERN(int)
cahute_open_serial_over_usb_bulk_link_from_interface(
    cahute_usb_link_open_params *open_params,
    cahute_serial_over_usb_bulk_link_interface const *interface,
    void *cookie,
    size_t cookie_size
);

/* From link_open_ums.c */
CAHUTE_EXTERN(int)
cahute_open_ums_link_from_interface(
    cahute_usb_link_open_params *open_params,
    cahute_ums_link_interface const *interface,
    void *cookie,
    size_t cookie_size
);

/* From link_init.c */
CAHUTE_EXTERN(int) cahute_initialize_link(cahute_link *link);

CAHUTE_EXTERN(char const *) cahute_get_protocol_name(int protocol);

/* ---
 * Link transport functions.
 * --- */

CAHUTE_EXTERN(int)
cahute_receive_on_link_transport(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    unsigned long first_timeout,
    unsigned long next_timeout
);

CAHUTE_EXTERN(int)
cahute_send_on_link_transport(
    cahute_link *link,
    cahute_u8 const *buf,
    size_t size
);

CAHUTE_EXTERN(int)
cahute_set_serial_params_on_link_transport(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_EXTERN(int)
cahute_scsi_request_to_link_transport(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);

CAHUTE_EXTERN(int)
cahute_scsi_request_from_link_transport(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

/**
 * Receive a byte on a link's transport.
 *
 * NOTE: If an error occurs, *bytep is NOT set and keeps whatever value it
 * had before the function call.
 *
 * @param link Link on the transport of which to receive the byte.
 * @param bytep Pointer to the byte to receive.
 * @param timeout Timeout to receive the byte.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INLINE(int)
cahute_receive_byte_on_link_transport(
    cahute_link *link,
    int *bytep,
    unsigned long timeout
) {
    cahute_u8 buf[8];
    int err;

    err = cahute_receive_on_link_transport(link, buf, 1, timeout, timeout);
    if (!err && bytep)
        *bytep = buf[0];

    return err;
}

/**
 * Send a byte on a link's transport.
 *
 * @param link Link on the transport of which to send the byte.
 * @param byte Byte to send.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INLINE(int)
cahute_send_byte_on_link_transport(cahute_link *link, int byte) {
    cahute_u8 buf[8];

    buf[0] = byte;
    return cahute_send_on_link_transport(link, buf, 1);
}

/* ---
 * Data management, defined in data.c
 * --- */

CAHUTE_EXTERN(int)
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

/**
 * Compute the total size of a data description.
 *
 * @param desc Description of the data to receive.
 * @return Computed size of the data description.
 */
CAHUTE_INLINE(size_t)
cahute_casiolink_compute_data_description_size(
    struct cahute_casiolink_data_description const *desc
) {
    size_t total_size = 0, part_i;

    if (!desc->part_count)
        return 0;

    for (part_i = desc->part_count - 1; part_i > 0; part_i--)
        total_size += desc->part_sizes[part_i - 1] + 2;

    total_size +=
        (desc->part_sizes[desc->part_count - 1] + 2) * desc->last_part_repeat;
    return total_size;
}

CAHUTE_EXTERN(int)
cahute_casiolink_check_file_data(
    cahute_file *file,
    unsigned long offset,
    struct cahute_casiolink_data_description const *desc
);

CAHUTE_EXTERN(void)
cahute_casiolink_log_data_description(
    cahute_context *context,
    struct cahute_casiolink_data_description const *desc
);

CAHUTE_EXTERN(int)
cahute_casiolink_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp
);

CAHUTE_EXTERN(int)
cahute_casiolink_receive_raw_data(
    cahute_link *link,
    struct cahute_casiolink_data_description const *desc,
    cahute_u8 *buf,
    size_t *buf_sizep
);

/* Make the CASIOLINK handshake only. */
CAHUTE_EXTERN(int) cahute_casiolink_initiate_as_receiver(cahute_link *link);
CAHUTE_EXTERN(int) cahute_casiolink_initiate_as_sender(cahute_link *link);

CAHUTE_EXTERN(int)
cahute_casiolink_receive_first_byte(
    cahute_link *link,
    int *first_bytep,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_casiolink_receive_packet(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    int expected_type,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_casiolink_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_casiolink_make_device_info(
    cahute_link *link,
    cahute_device_info **infop
);

/* ---
 * CAS40 protocol functions, defined in cas40.c
 * --- */

CAHUTE_EXTERN(int)
cahute_cas40_decode_data(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long *offsetp
);

CAHUTE_EXTERN(int)
cahute_cas40_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_cas40_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_EXTERN(int) cahute_cas40_terminate(cahute_link *link);

/* ---
 * CAS50 protocol functions, defined in cas50.c
 * --- */

CAHUTE_EXTERN(int)
cahute_cas50_decode_data(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long *offsetp
);

CAHUTE_EXTERN(int)
cahute_cas50_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_EXTERN(int) cahute_cas50_terminate(cahute_link *link);

/* ---
 * CAS100 protocol functions, defined in cas100.c
 * --- */

CAHUTE_EXTERN(int)
cahute_cas100_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_cas100_make_device_info(
    cahute_device_info **infop,
    cahute_u8 const *raw_info
);

CAHUTE_EXTERN(int) cahute_cas100_exchange_model_information(cahute_link *link);
CAHUTE_EXTERN(int)
cahute_cas100_handle_mdl1(cahute_link *link, cahute_u8 const *header);

CAHUTE_EXTERN(int) cahute_cas100_initiate(cahute_link *link);
CAHUTE_EXTERN(int) cahute_cas100_terminate(cahute_link *link);

/* ---
 * CAS300 protocol functions, defined in cas300.c
 * --- */

CAHUTE_EXTERN(int)
cahute_cas300_receive_packet(
    cahute_link *link,
    int first_byte,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_cas300_receive_data(
    cahute_link *link,
    cahute_data **datap,
    int first_byte,
    unsigned long timeout
);

CAHUTE_EXTERN(int) cahute_cas300_initiate_as_sender(cahute_link *link);
CAHUTE_EXTERN(int) cahute_cas300_initiate_as_receiver(cahute_link *link);
CAHUTE_EXTERN(int) cahute_cas300_discover(cahute_link *link);
CAHUTE_EXTERN(int) cahute_cas300_terminate(cahute_link *link);

CAHUTE_EXTERN(int)
cahute_cas300_make_device_info(
    cahute_context *context,
    cahute_device_info **infop,
    cahute_u8 const *raw_info
);

/* ---
 * Protocol 7.00 functions, defined in seven.c
 * --- */

CAHUTE_EXTERN(int) cahute_seven_initiate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_seven_terminate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_seven_discover(cahute_link *link);

CAHUTE_EXTERN(int)
cahute_seven_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_seven_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_EXTERN(int)
cahute_seven_make_device_info(cahute_link *link, cahute_device_info **infop);

CAHUTE_EXTERN(int)
cahute_seven_request_storage_capacity(
    cahute_link *link,
    char const *storage,
    unsigned long *capacityp
);

CAHUTE_EXTERN(int)
cahute_seven_send_file_to_storage(
    cahute_link *link,
    unsigned long flags,
    char const *directory,
    char const *name,
    char const *storage,
    cahute_file *file,
    cahute_confirm_overwrite_func *overwrite_func,
    void *overwrite_cookie,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_request_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage,
    void const *path,
    int path_type,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_copy_file_on_storage(
    cahute_link *link,
    char const *source_directory,
    char const *source_name,
    char const *target_directory,
    char const *target_name,
    char const *storage
);

CAHUTE_EXTERN(int)
cahute_seven_delete_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage
);

CAHUTE_EXTERN(int)
cahute_seven_list_storage_entries(
    cahute_link *link,
    char const *storage,
    cahute_list_storage_entry_func *callback,
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_seven_reset_storage(cahute_link *link, char const *storage);

CAHUTE_EXTERN(int)
cahute_seven_optimize_storage(cahute_link *link, char const *storage);

CAHUTE_EXTERN(int)
cahute_seven_backup_rom(
    cahute_link *link,
    cahute_u8 **romp,
    size_t *sizep,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_upload_and_run_program(
    cahute_link *link,
    cahute_u8 const *program,
    size_t program_size,
    unsigned long load_address,
    unsigned long start_address,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_flash_system_using_fxremote_method(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *system,
    size_t system_size
);

/* ---
 * Protocol 7.00 Screenstreaming functions, defined in seven_ohp.c
 * --- */

CAHUTE_EXTERN(int)
cahute_seven_ohp_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
);

/* ---
 * MCS encoding and decoding functions, defined in mcs.c
 * --- */

CAHUTE_EXTERN(int)
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
