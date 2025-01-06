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

/**
 * Update the serial parameters for the current link.
 *
 * @param link Link for which to update the serial parameters.
 * @param flags Serial flags to set to the current link.
 * @param speed Serial speed to set to the current link.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    unsigned long unsupported_flags = 0;
    unsigned long new_serial_flags = 0;
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

    /* We want to check if there are unsupported flags, that is:
     *
     * - An invalid value for the stop bits flags.
     * - An invalid value for the parity flags.
     * - Any other unassigned flag we don't recognize. */
    unsupported_flags =
        flags & ~(CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK);

    if ((flags & CAHUTE_SERIAL_STOP_MASK) == 0) {
        /* For ease of use of the flags by the protocol-specific function,
         * we actually want to take the current parameters. */
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_STOP_MASK;
    } else if ((flags & CAHUTE_SERIAL_STOP_MASK) == CAHUTE_SERIAL_STOP_ONE) {
    } else if ((flags & CAHUTE_SERIAL_STOP_MASK) != CAHUTE_SERIAL_STOP_TWO)
        unsupported_flags |= flags & CAHUTE_SERIAL_STOP_MASK;

    if ((flags & CAHUTE_SERIAL_PARITY_MASK) == 0) {
        /* For ease of use of the flags by the protocol-specific function,
         * we actually want to take the current parameters. */
        flags |= link->transport_serial_flags & CAHUTE_SERIAL_PARITY_MASK;
    } /* No possible invalid value, 3+1 value in 2 bits. */

    if (unsupported_flags)
        CAHUTE_RETURN_IMPL(
            link->context,
            "At least one unsupported flag was present."
        );

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    /* We want the complete serial flags here. */
    new_serial_flags = link->transport_serial_flags;
    new_serial_flags &= ~(CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK);
    new_serial_flags |= flags;

    /* Now that our flags and speed has been validated, we can call our
     * protocol-specific function. */
    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        err = cahute_seven_negotiate_serial_params(
            link,
            new_serial_flags,
            speed
        );
        if (err)
            return err;

        break;

    default:
        CAHUTE_RETURN_IMPL(
            link->context,
            "Operation not supported by the link protocol."
        );
    }

    err = cahute_set_serial_params_on_link_transport(
        link,
        new_serial_flags,
        speed
    );
    if (err) {
        /* We have successfully negociated with the device to switch
         * serial settings but have not managed to change settings
         * ourselves. We can no longer communicate with the device,
         * hence can no longer negotiate the serial settings back.
         * Therefore, we consider the link to be irrecoverable. */
        msg(link->context,
            ll_error,
            "Could not set the serial params; that makes our connection "
            "irrecoverable!");
        link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
        return err;
    }

    /* Wait until the new serial parameters have been applied by the device. */
    err = cahute_sleep(link->context, 50);
    if (err)
        return err;

    return CAHUTE_OK;
}
