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
 * Close and free a link.
 *
 * @param link Link to close and free.
 */
CAHUTE_EXTERN(void) cahute_close_link(cahute_link *link) {
    if (!link)
        return;

    msg(link->context, ll_info, "Closing the link.");

    if ((link->flags & CAHUTE_LINK_FLAG_TERMINATE)
        && !(
            link->flags
            & (CAHUTE_LINK_FLAG_IRRECOVERABLE | CAHUTE_LINK_FLAG_TERMINATED
               | CAHUTE_LINK_FLAG_RECEIVER | CAHUTE_LINK_FLAG_GONE)
        )) {
        switch (link->protocol) {
        case CAHUTE_LINK_PROTOCOL_SERIAL_NONE:
        case CAHUTE_LINK_PROTOCOL_USB_NONE:
            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_CAS40:
            cahute_cas40_terminate(link);
            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_CAS50:
            cahute_cas50_terminate(link);
            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_CAS100:
            cahute_cas100_terminate(link);
            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_CAS300:
        case CAHUTE_LINK_PROTOCOL_USB_CAS300:
            cahute_cas300_terminate(link);
            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
            cahute_seven_terminate(link);
            break;

        default:
            msg(link->context,
                ll_warn,
                "No method to terminate protocol %s (%d).",
                cahute_get_protocol_name(link->protocol),
                link->protocol);
        }
    }

    if (link->transport_close_func)
        (*link->transport_close_func)(link->context, link->transport_cookie);

    free(link);
}
