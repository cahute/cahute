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
 * Set serial parameters.
 *
 * @param link Link which to set the serial parameters of the transport of.
 * @param flags Serial parameters to set.
 * @param speed Speed to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_set_serial_params_on_link_transport(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    int err = 0;

    if (link->transport != CAHUTE_LINK_TRANSPORT_SERIAL)
        CAHUTE_RETURN_IMPL(
            link->context,
            "Cannot set serial parameters on a non-serial transport."
        );

    if (link->transport_serial_flags == flags
        && link->transport_serial_speed == speed)
        return CAHUTE_OK;

    msg(link->context,
        ll_info,
        "Setting serial parameters to %lu%c%d.",
        speed,
        (flags & CAHUTE_SERIAL_PARITY_MASK) == CAHUTE_SERIAL_PARITY_ODD ? 'O'
        : (flags & CAHUTE_SERIAL_PARITY_MASK) == CAHUTE_SERIAL_PARITY_EVEN
            ? 'E'
            : 'N',
        (flags & CAHUTE_SERIAL_STOP_MASK) == CAHUTE_SERIAL_STOP_TWO ? 2 : 1);

    err = (link->transport_set_serial_params_func)(
        link->context,
        link->transport_cookie,
        flags,
        speed
    );
    switch (err) {
    case CAHUTE_OK:
        break;

    case CAHUTE_ERROR_GONE:
        link->flags |= CAHUTE_LINK_FLAG_GONE;
        /* FALLTHRU */

    default:
        return err;
    }

    link->transport_serial_flags = flags;
    link->transport_serial_speed = speed;
    return CAHUTE_OK;
}
