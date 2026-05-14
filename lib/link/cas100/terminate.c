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
 * Terminate the connection, for the CAS100 protocol.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int) cahute_cas100_terminate(cahute_link *link) {
    cahute_u8 buf[40];
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    memset(buf, 0xFF, 40);
    buf[0] = ':';
    buf[1] = 'E';
    buf[2] = 'N';
    buf[3] = 'D';
    buf[4] = '1';
    buf[39] = cahute_checksub(&buf[1], 38);

    msg(link->context, ll_debug, "Sending the following end packet:");
    mem(link->context, ll_debug, buf, 40);

    err = cahute_send_on_link_transport(link, buf, 40);
    if (err)
        return err;

    link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
    return CAHUTE_OK;
}
