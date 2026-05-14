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
 * Get the type of a file with a provided path on the calculator.
 *
 * There is no command specific to that use case, so this function actually
 * lists all files present on the storage device, optionally in a subdirectory,
 * and checks if one of the returned entries correspond to the file path.
 *
 * @param link Link to the calculator.
 * @param typep Pointer to the integer to set to the file type.
 * @param directory Optional name of the directory.
 * @param name Name of the file.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_request_file_type(
    cahute_link *link,
    int *typep,
    char const *directory,
    char const *name,
    char const *storage
) {
    cahute_u8 const *raw_directory_name, *raw_file_name;
    size_t raw_directory_name_size, raw_file_name_size;
    size_t directory_size = directory ? strlen(directory) : 0;
    size_t name_size = strlen(name);
    int err;

    *typep = CAHUTE_SEVEN_FILE_TYPE_NONE;

    err = cahute_seven_send_command(
        link,
        0x4D,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        storage,
        NULL,
        TIMEOUT_COMMAND_RESPONSE
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    if (err)
        return err;

    while (link->protocol_state.seven.last_packet_type == PACKET_TYPE_COMMAND
    ) {
        if (link->protocol_state.seven.last_packet_subtype != 0x4E) {
            /* The command is not "Transfer file information".
             * We just try to ACK and skip it here. */
            msg(link->context,
                ll_error,
                "Unhandled command %02X for file listing.",
                link->protocol_state.seven.last_packet_subtype);
            continue;
        }

        if (*typep != CAHUTE_SEVEN_FILE_TYPE_NONE)
            goto next_entry;

        err = cahute_seven_decode_command(
            link,
            NULL,
            NULL,
            NULL,
            &raw_directory_name,
            &raw_directory_name_size,
            &raw_file_name,
            &raw_file_name_size,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL, /* &raw_storage, although we aren't interested. */
            NULL, /* &raw_storage_size. */
            NULL,
            NULL
        );
        if (err)
            return err;

        if (!raw_file_name_size) {
            /* Entry is a directory. */
            if (!directory_size && name_size == raw_directory_name_size
                && !memcmp(name, raw_directory_name, name_size))
                *typep = CAHUTE_SEVEN_FILE_TYPE_DIR;

            goto next_entry;
        }

        if (raw_directory_name_size == directory_size
            && raw_file_name_size == name_size
            && (!directory_size
                || !memcmp(directory, raw_directory_name, directory_size))
            && !memcmp(name, raw_file_name, name_size))
            *typep = CAHUTE_SEVEN_FILE_TYPE_FILE;

next_entry:
        err = cahute_seven_send_basic(
            link,
            0,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;
    }

    EXPECT_PACKET(PACKET_TYPE_ROLESWAP, 0);
    return CAHUTE_OK;
}
