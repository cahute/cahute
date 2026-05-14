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
 * Send a basic Protocol 7.00 packet and receive its response.
 *
 * This function also takes care of receiving the associated response packet.
 *
 * @param link Link to use to send the Protocol 7.00 packet.
 * @param flags Flags, as or'd `CAHUTE_SEVEN_SEND_FLAG_*` constants.
 * @param type Numeric type (*T*) of the packet to send.
 * @param subtype Numeric subtype (*ST*) of the packet to send.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_seven_send_basic(
    cahute_link *link,
    unsigned long flags,
    int type,
    int subtype
) {
    cahute_u8 packet[6];

    packet[0] = type & 255;
    cahute_set_ascii_hex(&packet[1], subtype);
    packet[3] = '0';
    cahute_set_ascii_hex(&packet[4], cahute_checksub(&packet[1], 3));

    return cahute_seven_send_and_receive(
        link,
        flags,
        packet,
        6,
        TIMEOUT_PACKET_START
    );
}
