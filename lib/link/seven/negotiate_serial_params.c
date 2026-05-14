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
 * Negotiate new serial parameters with the passive side.
 *
 * @param link Link to write to.
 * @param flags Serial flags to negotiate.
 * @param speed Speed to negotiate.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_seven_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    char baudrate_buf[20];
    char const *parity, *stopbits;
    int err;

    sprintf(baudrate_buf, "%lu", speed);

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        parity = "EVEN";
        break;
    case CAHUTE_SERIAL_PARITY_ODD:
        parity = "ODD";
        break;
    default:
        parity = "NONE";
        break;
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case CAHUTE_SERIAL_STOP_TWO:
        stopbits = "2";
        break;
    default:
        stopbits = "1";
        break;
    }

    err = cahute_seven_send_command(
        link,
        0x02,
        0,
        0,
        0,
        baudrate_buf,
        parity,
        stopbits,
        NULL,
        NULL,
        NULL,
        TIMEOUT_COMMAND_RESPONSE
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}
