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
 * Request for a file to be deleted on the calculator.
 *
 * @param link Link on which to make the request.
 * @param directory Directory name in which to delete the file.
 * @param name Name of the file to delete.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_delete_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage
) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x46,
        0,
        0,
        0,
        directory,
        name,
        NULL,
        NULL,
        storage,
        NULL,
        TIMEOUT_COMMAND_RESPONSE
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}
