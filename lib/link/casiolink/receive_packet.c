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
 * Receive a CASIOLINK packet, whatever the variant is.
 *
 * This is useful for CAS40, CAS50 and CAS100.
 *
 * WARNING: The buffer is expected to have a minimum capacity
 *          of ``size + 2`` bytes.
 *
 * @param link Link to receive the CASIOLINK packet on.
 * @param buf Buffer in which to place the packet.
 * @param size Expected packet size.
 * @param expected_type Expected type of the packet.
 * @param timeout Timeout for the first byte of the packet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_casiolink_receive_packet(
    cahute_link *link,
    cahute_u8 *buf,
    size_t size,
    int expected_type,
    unsigned long timeout
) {
    unsigned int checksum, checksum_alt;
    int err, first_byte;

    err = cahute_casiolink_receive_first_byte(link, &first_byte, timeout);
    if (err == CAHUTE_ERROR_TIMEOUT_START) {
        msg(link->context,
            ll_error,
            "Timeout received while reading the packet type.");
        return CAHUTE_ERROR_TIMEOUT;
    } else if (err)
        return err;

    if (first_byte != expected_type) {
        msg(link->context,
            ll_error,
            "Expected 0x%02X packet type, got 0x%02X.",
            expected_type,
            first_byte);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *buf = first_byte;
    if (size) {
        size_t size_left = size + 1;
        cahute_u8 *p = &buf[1];

        while (size_left) {
            size_t to_read = size_left > 512 ? 512 : size_left;

            err = cahute_receive_on_link_transport(
                link,
                p,
                to_read,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            else if (err)
                return err;

            size_left -= to_read;
            p += to_read;
        }

        /* For color screenshots, sometimes the first byte is not
         * taken into account in the checksum calculation, as it's
         * metadata for the sheet and not the "actual data" of the
         * sheet. But sometimes it also gets the checksum right!
         * In any case, we want to compute and check both checksums
         * to see if at least one matches. */
        checksum = cahute_checksub(&buf[1], size);
        checksum_alt = cahute_checksub(&buf[2], size - 1);
    } else {
        checksum = 0;
        checksum_alt = 0;
    }

    /* Check the checksum. */
    if (checksum != buf[1 + size] && checksum_alt != buf[1 + size]) {
        msg(link->context,
            ll_warn,
            "Received a packet with an invalid checksum.");
        msg(link->context,
            ll_debug,
            "  Obtained: 0x%02X, computed: 0x%02X",
            buf[1 + size],
            checksum);
        mem(link->context, ll_debug, buf, size);

        return CAHUTE_ERROR_CORRUPT;
    }

    return CAHUTE_OK;
}
