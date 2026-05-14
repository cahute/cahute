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

#include "internals.h"

/**
 * Send an extended Protocol 7.00 packet and receive its response.
 *
 * Note that this function only supports sending up to 1028 bytes (maximum
 * data packet size), and handles the 0x5C padding.
 *
 * This function also takes care of receiving the associated response packet.
 *
 * @param link Link to use to send the Protocol 7.00 packet.
 * @param flags Flags, as or'd `CAHUTE_SEVEN_SEND_FLAG_*` constants.
 * @param type Numeric type (*T*) of the packet to send.
 * @param subtype Numeric subtype (*ST*) of the packet to send.
 * @param data Data to send.
 * @param data_size Size of the data to send.
 * @param timeout Timeout in which to expect the response to the packet.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_seven_send_extended(
    cahute_link *link,
    unsigned long flags,
    int type,
    int subtype,
    cahute_u8 const *data,
    size_t data_size,
    unsigned long timeout
) {
    cahute_u8 packet[SEVEN_MAX_PACKET_SIZE];

    if (data_size > SEVEN_MAX_PACKET_DATA_SIZE) {
        msg(link->context,
            ll_error,
            "Tried to send an extended Protocol 7.00 packet with more "
            "than " CAHUTE_PRIuSIZE "o: %" CAHUTE_PRIuSIZE "o!",
            SEVEN_MAX_PACKET_DATA_SIZE,
            data_size);
        return CAHUTE_ERROR_UNKNOWN;
    }

    data_size = cahute_pad_data(&packet[8], data, data_size);

    packet[0] = type & 255;
    cahute_set_ascii_hex(&packet[1], subtype);
    packet[3] = '1';
    cahute_set_ascii_hex(&packet[4], (data_size >> 8) & 255);
    cahute_set_ascii_hex(&packet[6], data_size & 255);

    cahute_set_ascii_hex(
        &packet[8 + data_size],
        cahute_checksub(&packet[1], 7 + data_size)
    );

    return cahute_seven_send_and_receive(
        link,
        flags,
        packet,
        10 + data_size,
        timeout
    );
}
