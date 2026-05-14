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

/* The MDL1 command for Graph 100 / AFX used for initialization when
 * in sender / control mode. The speed and parity are inserted into the
 * copy of this buffer before the checksum is recomputed and placed into
 * the last byte. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
default_mdl1_payload =
    (cahute_u8 const *)":MDL1GY351\xFF" "000000N1.03\0\0\x01\0\0\0\x04\0\0\0"
    "\x01\0\x03\xFF\xFF\xFF\xFF\0";

/**
 * Send CAS100 model information.
 *
 * @param link Link on which to send CAS100 model information.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int) cahute_cas100_send_model_information(cahute_link *link) {
    cahute_u8 buf[40];
    char serial_params[7];

    /* NOTE: sprintf() adds a terminating zero, but we don't care,
     * since we do not copy serial_params[6] afterwards. */
    sprintf(serial_params, "%06lu", link->transport_serial_speed);
    switch (link->transport_serial_flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        serial_params[6] = 'E';
        break;

    case CAHUTE_SERIAL_PARITY_ODD:
        serial_params[6] = 'O';
        break;

    default:
        serial_params[6] = 'N';
    }

    memcpy(buf, default_mdl1_payload, 40);
    memcpy(&buf[11], serial_params, 7);

    buf[39] = cahute_checksub(&buf[1], 38);

    return cahute_send_on_link_transport(link, buf, 40);
}
