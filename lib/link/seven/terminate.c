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
 * Terminate the Protocol 7.00 communication.
 *
 * This must be called while the link is in sender / active mode.
 * For more information on this flow, see :ref:`seven-terminate-link`.
 *
 * @param link Link on which to initiate the Protocol 7.00 communication.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int) cahute_seven_terminate(cahute_link *link) {
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    if (~link->flags & CAHUTE_LINK_FLAG_RECEIVER) {
        err = cahute_seven_send_basic(
            link,
            0,
            PACKET_TYPE_TERM,
            PACKET_SUBTYPE_TERM_BASIC
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    return CAHUTE_OK;
}
