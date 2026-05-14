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

#ifndef LINK_INTERNALS_H
#define LINK_INTERNALS_H 1
#include "../internals.h"

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
 * @property picture_capacity Maximum capacity to the picture buffer.
 * @property picture_size Picture size.
 * @property picture_buf Picture buffer.
 */
struct cahute_seven_ohp_state {
    int last_packet_type;
    int picture_format;
    int picture_width;
    int picture_height;
    size_t picture_capacity;
    size_t picture_size;

    cahute_u8 *picture_buf;
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
 * General link internal functions.
 * --- */

#define CHECK_SENDER   0x00000001 /* Check that a link is not a receiver. */
#define CHECK_RECEIVER 0x00000002 /* Check that a link is a receiver. */

CAHUTE_INTERNAL(int) cahute_check_link(cahute_link *link, unsigned long flags);

/* ---
 * CAS40 protocol functions.
 * --- */

CAHUTE_INTERNAL(int)
cahute_cas40_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_cas40_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_INTERNAL(int) cahute_cas40_terminate(cahute_link *link);

/* ---
 * CAS50 protocol functions, defined in cas50.c
 * --- */

CAHUTE_INTERNAL(int)
cahute_cas50_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_INTERNAL(int) cahute_cas50_terminate(cahute_link *link);

/* ---
 * CAS100 protocol functions, defined in cas100.c
 * --- */

CAHUTE_INTERNAL(int)
cahute_cas100_receive_data(
    cahute_link *link,
    cahute_data **datap,
    cahute_u8 const *header,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_cas100_get_flash_rom_capacity(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_cas100_get_ram_capacity(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_cas100_get_os_version(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_cas100_get_hwid(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_cas100_exchange_model_information(cahute_link *link);
CAHUTE_INTERNAL(int)
cahute_cas100_handle_mdl1(cahute_link *link, cahute_u8 const *header);

CAHUTE_INTERNAL(int) cahute_cas100_initiate(cahute_link *link);
CAHUTE_INTERNAL(int) cahute_cas100_terminate(cahute_link *link);

/* ---
 * CAS300 protocol functions, defined in cas300.c
 * --- */

CAHUTE_INTERNAL(int)
cahute_cas300_receive_packet(
    cahute_link *link,
    int first_byte,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_cas300_receive_data(
    cahute_link *link,
    cahute_data **datap,
    int first_byte,
    unsigned long timeout
);

CAHUTE_INTERNAL(int) cahute_cas300_initiate_as_sender(cahute_link *link);
CAHUTE_INTERNAL(int) cahute_cas300_initiate_as_receiver(cahute_link *link);
CAHUTE_INTERNAL(int) cahute_cas300_discover(cahute_link *link);
CAHUTE_INTERNAL(int) cahute_cas300_terminate(cahute_link *link);

CAHUTE_INTERNAL(int)
cahute_cas300_get_flash_rom_capacity(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_cas300_get_bootcode_version(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_cas300_get_os_version(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_cas300_get_hwid(cahute_link *link, char *buf, size_t size);

/* ---
 * Protocol 7.00 functions, defined in seven.c
 * --- */

CAHUTE_INTERNAL(int) cahute_seven_initiate(cahute_link *link);

CAHUTE_INTERNAL(int) cahute_seven_terminate(cahute_link *link);

CAHUTE_INTERNAL(int) cahute_seven_discover(cahute_link *link);

CAHUTE_INTERNAL(int)
cahute_seven_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_seven_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_INTERNAL(int)
cahute_seven_get_product_id(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_username(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_organisation(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_hwid(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_cpuid(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_rom_capacity(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_get_flash_rom_capacity(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_get_ram_capacity(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_get_rom_version(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_bootcode_version(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_bootcode_offset(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_get_bootcode_size(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_get_os_version(cahute_link *link, char *buf, size_t size);

CAHUTE_INTERNAL(int)
cahute_seven_get_os_offset(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_get_os_size(cahute_link *link, unsigned long *valuep);

CAHUTE_INTERNAL(int)
cahute_seven_request_storage_capacity(
    cahute_link *link,
    char const *storage,
    unsigned long *capacityp
);

CAHUTE_INTERNAL(int)
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

CAHUTE_INTERNAL(int)
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

CAHUTE_INTERNAL(int)
cahute_seven_copy_file_on_storage(
    cahute_link *link,
    char const *source_directory,
    char const *source_name,
    char const *target_directory,
    char const *target_name,
    char const *storage
);

CAHUTE_INTERNAL(int)
cahute_seven_delete_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage
);

CAHUTE_INTERNAL(int)
cahute_seven_list_storage_entries(
    cahute_link *link,
    char const *storage,
    cahute_list_storage_entry_func *callback,
    void *cookie
);

CAHUTE_INTERNAL(int)
cahute_seven_reset_storage(cahute_link *link, char const *storage);

CAHUTE_INTERNAL(int)
cahute_seven_optimize_storage(cahute_link *link, char const *storage);

CAHUTE_INTERNAL(int)
cahute_seven_backup_rom(
    cahute_link *link,
    cahute_u8 **romp,
    size_t *sizep,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_INTERNAL(int)
cahute_seven_upload_and_run_program(
    cahute_link *link,
    cahute_u8 const *program,
    size_t program_size,
    unsigned long load_address,
    unsigned long start_address,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_INTERNAL(int)
cahute_seven_flash_system_using_fxremote_method(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *system,
    size_t system_size
);

/* ---
 * Protocol 7.00 Screenstreaming functions, defined in seven_ohp.c
 * --- */

CAHUTE_INTERNAL(int)
cahute_seven_ohp_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
);

#endif /* LINK_INTERNALS_H */
