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

#ifndef LINK_SEVEN_INTERNALS_H
#define LINK_SEVEN_INTERNALS_H 1
#include "../internals.h"

/* ---
 * Prototypes.
 * --- */

#define CAHUTE_SEVEN_SEND_FLAG_DISABLE_CHECKSUM \
    0x00000001 /* Disable checksum flow. */
#define CAHUTE_SEVEN_SEND_FLAG_DISABLE_TIMEOUT \
    0x00000002 /* Disable timeout flow. */
#define CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE \
    0x00000004 /* Disable packet reception. */

#define CAHUTE_SEVEN_SEND_BYTES_FLAG_DISABLE_SHIFTING \
    0x00000001 /* Disable shifting. */

#define CAHUTE_SEVEN_RECEIVE_BYTES_FLAG_DISABLE_SHIFTING \
    0x00000001 /* Disable shifting. */

#define CAHUTE_SEVEN_FILE_TYPE_NONE 0 /* Path does not exist. */
#define CAHUTE_SEVEN_FILE_TYPE_FILE 1 /* Path leads to a regular file. */
#define CAHUTE_SEVEN_FILE_TYPE_DIR  2 /* Path leads to a directory. */

CAHUTE_INTERNAL(int)
cahute_seven_receive(cahute_link *link, unsigned long timeout);

CAHUTE_INTERNAL(int)
cahute_seven_send_and_receive(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *raw_packet,
    size_t raw_packet_size,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_seven_send_basic(
    cahute_link *link,
    unsigned long flags,
    int type,
    int subtype
);

CAHUTE_INTERNAL(int)
cahute_seven_send_extended(
    cahute_link *link,
    unsigned long flags,
    int type,
    int subtype,
    cahute_u8 const *data,
    size_t data_size,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_seven_send_command(
    cahute_link *link,
    int code,
    int overwrite,
    int datatype,
    unsigned long filesize,
    char const *param1,
    char const *param2,
    char const *param3,
    char const *param4,
    char const *param5,
    char const *param6,
    unsigned long timeout
);

CAHUTE_INTERNAL(int)
cahute_seven_decode_command(
    cahute_link *link,
    int *overwritep,
    int *datatypep,
    unsigned long *filesizep,
    cahute_u8 const **param1p,
    size_t *param1_sizep,
    cahute_u8 const **param2p,
    size_t *param2_sizep,
    cahute_u8 const **param3p,
    size_t *param3_sizep,
    cahute_u8 const **param4p,
    size_t *param4_sizep,
    cahute_u8 const **param5p,
    size_t *param5_sizep,
    cahute_u8 const **param6p,
    size_t *param6_sizep
);

CAHUTE_INTERNAL(int)
cahute_seven_send_bytes_from_stream(
    cahute_link *link,
    unsigned long flags,
    cahute_file *file,
    unsigned long size,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_INTERNAL(int)
cahute_seven_send_bytes(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *data,
    size_t size,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_INTERNAL(int)
cahute_seven_receive_bytes(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 *buf,
    size_t size,
    int command_code,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_INTERNAL(int)
cahute_seven_receive_bytes_into_stream(
    cahute_link *link,
    cahute_file *file,
    size_t size,
    int command_code,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

/* ---
 * Utilities.
 * --- */

/* TIMEOUT_PACKET_START is the maximum delay before which the first byte of a
 * packet is expected under normal circumstances.
 * TIMEOUT_PACKET_INIT is the maximum delay before which the first byte of
 * the response to an initial check in the context of link initiation is
 * expected.
 * TIMEOUT_PACKET_TIMEOUT is the maximum delay before which the first byte of
 * the response to a presence check in the context of a normal timeout is
 * expected.
 * TIMEOUT_PACKET_CONTENTS is the timeout in between bytes for a packet, after
 * the first byte of a packet has been received, in any circumstances.
 * TIMEOUT_COMMAND_RESPONSE is the default timeout for the response to a
 * command packet.
 * TIMEOUT_OPTIMIZE_RESPONSE is the timeout for the response to an
 * "Optimize filesystem" (51) command. */
#define TIMEOUT_PACKET_START      10000 /* 10 seconds. */
#define TIMEOUT_PACKET_INIT       800   /* 800 ms (.8 second). */
#define TIMEOUT_PACKET_TIMEOUT    10000 /* 10 seconds. */
#define TIMEOUT_PACKET_CONTENTS   2000  /* 2 seconds. */
#define TIMEOUT_COMMAND_RESPONSE  10000 /* 10 seconds. */
#define TIMEOUT_OPTIMIZE_RESPONSE 30000 /* 30 seconds. */

#define CONDITIONAL_ASCII_HEX_DIGIT(C) \
    (cahute_is_ascii_hex(C) ? cahute_ascii_hex_to_nibble(C) : 0)
#define CONDITIONAL_ASCII_DEC_DIGIT(C) (isdigit(C) ? (C) - '0' : 0)

#define PACKET_TYPE_COMMAND  1  /* 0x01 */
#define PACKET_TYPE_DATA     2  /* 0x02 */
#define PACKET_TYPE_ROLESWAP 3  /* 0x03 */
#define PACKET_TYPE_CHECK    5  /* 0x05 */
#define PACKET_TYPE_ACK      6  /* 0x06 */
#define PACKET_TYPE_NAK      21 /* 0x15 */
#define PACKET_TYPE_TERM     24 /* 0x18 */

#define PACKET_SUBTYPE_CHECK_INIT    0 /* '00' */
#define PACKET_SUBTYPE_CHECK_REGULAR 1 /* '01' */

#define PACKET_SUBTYPE_ACK_BASIC             0 /* '00' */
#define PACKET_SUBTYPE_ACK_CONFIRM_OVERWRITE 1 /* '01' */
#define PACKET_SUBTYPE_ACK_EXTENDED          2 /* '02' */
#define PACKET_SUBTYPE_ACK_TERM              3 /* '03' */

#define PACKET_SUBTYPE_NAK_RESEND           1 /* '01' */
#define PACKET_SUBTYPE_NAK_OVERWRITE        2 /* '02' */
#define PACKET_SUBTYPE_NAK_REJECT_OVERWRITE 3 /* '03' */
#define PACKET_SUBTYPE_NAK_OTHER            4 /* '04' */

#define PACKET_SUBTYPE_TERM_BASIC 0 /* '00' */

#define EXPECT_PACKET(TYPE, SUBTYPE) \
    if (link->protocol_state.seven.last_packet_type != (TYPE) \
        || link->protocol_state.seven.last_packet_subtype != (SUBTYPE)) { \
        msg(link->context, \
            ll_debug, \
            "Expected a packet of type %02X and subtype %02X, " \
            "got a packet of type %02X and subtype %02X.", \
            (TYPE), \
            (SUBTYPE), \
            link->protocol_state.seven.last_packet_type, \
            link->protocol_state.seven.last_packet_subtype); \
        return CAHUTE_ERROR_UNKNOWN; \
    }

#define EXPECT_PACKET_OR_FAIL(TYPE, SUBTYPE) \
    if (link->protocol_state.seven.last_packet_type != (TYPE) \
        || link->protocol_state.seven.last_packet_subtype != (SUBTYPE)) { \
        msg(link->context, \
            ll_debug, \
            "Expected a packet of type %02X and subtype %02X, " \
            "got a packet of type %02X and subtype %02X.", \
            (TYPE), \
            (SUBTYPE), \
            link->protocol_state.seven.last_packet_type, \
            link->protocol_state.seven.last_packet_subtype); \
        err = CAHUTE_ERROR_UNKNOWN; \
        goto fail; \
    }

#define EXPECT_BASIC_ACK \
    EXPECT_PACKET(PACKET_TYPE_ACK, PACKET_SUBTYPE_ACK_BASIC)
#define EXPECT_BASIC_ACK_OR_FAIL \
    EXPECT_PACKET_OR_FAIL(PACKET_TYPE_ACK, PACKET_SUBTYPE_ACK_BASIC)

CAHUTE_INTERNAL(int)
cahute_seven_request_file_type(
    cahute_link *link,
    int *typep,
    char const *directory,
    char const *name,
    char const *storage
);

#endif /* LINK_SEVEN_INTERNALS_H */
