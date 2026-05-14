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
 * List files and directories from the calculator.
 *
 * @param link Link to the device.
 * @param storage Name of the storage device on which to list files and
 *        directories.
 * @param callback Callback function for every entry.
 * @param cookie Cookie to pass to the callback.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_list_storage_entries(
    cahute_link *link,
    char const *storage,
    cahute_list_storage_entry_func *callback,
    void *cookie
) {
    cahute_storage_entry entry;
    cahute_u8 const *raw_directory_name, *raw_file_name;
    size_t raw_directory_name_size, raw_file_name_size;
    char directory_name_buf[24], file_name_buf[24];
    int err, should_skip = 0;

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
            goto skip_entry;
        }

        if (should_skip)
            continue;

        err = cahute_seven_decode_command(
            link,
            NULL,
            NULL,
            &entry.cahute_storage_entry_size,
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

        /* We need to check if the directory and file name are human-readable,
         * if they are present. */
        if (!raw_directory_name || !raw_directory_name_size)
            entry.cahute_storage_entry_directory = NULL;
        else if (raw_directory_name_size >= sizeof(directory_name_buf) - 1)
            goto skip_entry; /* We cannot yield this entry. */
        else {
            cahute_u8 const *p = raw_directory_name;
            size_t size;

            for (size = raw_directory_name_size; size; size--, p++)
                if (*p > 0x7F || (!isgraph(*p) && !isblank(*p)))
                    goto skip_entry;

            entry.cahute_storage_entry_directory = directory_name_buf;
            memcpy(
                directory_name_buf,
                raw_directory_name,
                raw_directory_name_size
            );
            directory_name_buf[raw_directory_name_size] = '\0';
        }

        if (!raw_file_name || !raw_file_name_size)
            entry.cahute_storage_entry_name = NULL;
        else if (raw_file_name_size >= sizeof(file_name_buf) - 1)
            goto skip_entry; /* We cannot yield this entry. */
        else {
            cahute_u8 const *p = raw_file_name;
            size_t size;

            for (size = raw_file_name_size; size; size--, p++)
                if (*p > 0x7F || (!isgraph(*p) && !isblank(*p)))
                    goto skip_entry;

            entry.cahute_storage_entry_name = file_name_buf;
            memcpy(file_name_buf, raw_file_name, raw_file_name_size);
            file_name_buf[raw_file_name_size] = '\0';
        }

        if (!entry.cahute_storage_entry_directory
            && !entry.cahute_storage_entry_name)
            goto skip_entry;

        if ((*callback)(cookie, &entry)) {
            /* The callback has requested that we interrupt the file listing.
             * However, we can't really do that, because Protocol 7.00 has
             * no option to interrupt and rebecome passive as far as we know.
             * So we just set a flag to not process the entry. */
            should_skip = 1;
        }

skip_entry:
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
    return should_skip ? CAHUTE_ERROR_INT : CAHUTE_OK;
}
