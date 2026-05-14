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
 * Initiate the connection as a receiver, for CAS300.
 *
 * For some reason there seems to be an asymetry between host to device
 * communication initialization, and device to host communication
 * initialization.
 *
 * @param link Link for which to initiate the connection, as a receiver.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int) cahute_cas300_initiate_as_receiver(cahute_link *link) {
    int byte = -1, err;

    while (byte != 0x16) {
        err = cahute_receive_byte_on_link_transport(link, &byte, 0);
        if (err)
            return err;
    }

    err = cahute_send_byte_on_link_transport(link, 0x13);
    if (err)
        return err;

    return CAHUTE_OK;
}
