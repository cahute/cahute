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

#include "internals.h"

CAHUTE_EXTERN(int)
cahute_set_serial_params_to_link(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    unsigned long unsupported_flags = 0;
    int err;

    if (!speed)
        speed = link->transport_serial_speed;

    switch (speed) {
    case 300:
    case 600:
    case 1200:
    case 2400:
    case 4800:
    case 9600:
    case 19200:
    case 38400:
    case 57600:
    case 115200:
        break;

    default:
        msg(link->context, ll_info, "Provided speed is %lu bauds.", speed);
        CAHUTE_RETURN_IMPL(
            link->context,
            "Unsupported baud rate for the serial link."
        );
    }

    unsupported_flags = flags
                        & ~(CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK
                            | CAHUTE_SERIAL_XONXOFF_MASK
                            | CAHUTE_SERIAL_DTR_MASK | CAHUTE_SERIAL_RTS_MASK);

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case 0:
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_STOP_MASK;
        break;

    case CAHUTE_SERIAL_STOP_ONE:
    case CAHUTE_SERIAL_STOP_TWO:
        break;

    default:
        unsupported_flags |= flags & CAHUTE_SERIAL_STOP_MASK;
    }

    if (!(flags & CAHUTE_SERIAL_PARITY_MASK)) {
        /* For ease of use of the flags by the protocol-specific function,
         * we actually want to take the current parameters. */
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_PARITY_MASK;
    } /* No possible invalid value, 3+1 value in 2 bits. */

    switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
    case 0:
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_XONXOFF_MASK;
        break;

    case CAHUTE_SERIAL_XONXOFF_DISABLE:
    case CAHUTE_SERIAL_XONXOFF_ENABLE:
        break;

    default:
        unsupported_flags |= flags & CAHUTE_SERIAL_XONXOFF_MASK;
    }

    switch (flags & CAHUTE_SERIAL_DTR_MASK) {
    case 0:
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_DTR_MASK;
        break;

    case CAHUTE_SERIAL_DTR_IGNORE:
    case CAHUTE_SERIAL_DTR_DISABLE:
    case CAHUTE_SERIAL_DTR_ENABLE:
        /* Valid values! */
        break;

    default:
        unsupported_flags |= flags & CAHUTE_SERIAL_DTR_MASK;
    }

    switch (flags & CAHUTE_SERIAL_RTS_MASK) {
    case 0:
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_RTS_MASK;
        break;

    case CAHUTE_SERIAL_RTS_IGNORE:
    case CAHUTE_SERIAL_RTS_DISABLE:
    case CAHUTE_SERIAL_RTS_ENABLE:
    case CAHUTE_SERIAL_RTS_HANDSHAKE:
        /* Valid values! */
        break;

    default:
        unsupported_flags |= flags & CAHUTE_SERIAL_RTS_MASK;
    }

    if (unsupported_flags)
        CAHUTE_RETURN_IMPL(
            link->context,
            "At least one unsupported flag was present."
        );

    err = cahute_check_link(link, 0);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_NONE:
        return cahute_set_serial_params_on_link_transport(link, flags, speed);

    default:
        CAHUTE_RETURN_IMPL(
            link->context,
            "Protocol does not support generic serial transport access."
        );
    }
}
