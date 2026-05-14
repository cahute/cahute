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
 * Send a file to storage on the calculator.
 *
 * @param link Link on which to send the file.
 * @param flags File sending flags.
 * @param directory Optional name of the directory.
 * @param name Name of the file.
 * @param storage Name of the storage device.
 * @param file File to read from.
 * @param overwrite_func Overwrite confirmation function.
 * @param overwrite_cookie Cookie to pass to the overwrite confirmation
 *        function.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_send_file_to_storage(
    cahute_link *link,
    unsigned long flags,
    char const *directory,
    char const *name,
    char const *storage,
    cahute_file *file,
    cahute_confirm_overwrite_func *overwrite_func,
    void *overwrite_cookie,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    unsigned long file_size;
    int err, should_upload_data = 1;

    err = cahute_get_file_size(file, &file_size);
    if (err)
        return err;

    if (flags & CAHUTE_SEND_FILE_FLAG_DELETE) {
        int file_type = CAHUTE_SEVEN_FILE_TYPE_NONE;

        err = cahute_seven_request_file_type(
            link,
            &file_type,
            directory,
            name,
            storage
        );
        if (err)
            return err;

        /* NOTE: This means that we can actually override directories, by
         * deleting them first then adding a file of the same name! */
        if (file_type != CAHUTE_SEVEN_FILE_TYPE_NONE) {
            err = cahute_seven_delete_file_from_storage(
                link,
                directory,
                name,
                storage
            );
            if (err)
                return err;
        }
    }

    if (flags & CAHUTE_SEND_FILE_FLAG_OPTIMIZE) {
        unsigned long capacity = 0;

        msg(link->context, ll_info, "Requesting storage capacity.");
        err = cahute_seven_request_storage_capacity(link, storage, &capacity);
        if (err)
            return err;

        msg(link->context, ll_info, "Storage capacity is %luB.", capacity);
        if ((size_t)capacity < file_size) {
            msg(link->context,
                ll_info,
                "Storage capacity is insufficient for file!.");
            msg(link->context, ll_info, "Requesting storage optimization.");
            err = cahute_seven_optimize_storage(link, storage);
            if (err)
                return err;
        } else
            msg(link->context, ll_info, "Enough storage is available!");
    }

    err = cahute_seven_send_command(
        link,
        0x45,
        flags & CAHUTE_SEND_FILE_FLAG_FORCE ? 2 : 0, /* Overwrite mode. */
        0,
        file_size & 0xFFFFFFFF,
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

    if (link->protocol_state.seven.last_packet_type == PACKET_TYPE_NAK
        && link->protocol_state.seven.last_packet_subtype
               == PACKET_SUBTYPE_NAK_OVERWRITE) {
        /* The device is currently requesting whether we want to overwrite
          * an existing file, which means we want to check on our side. */
        int should_overwrite = 0;

        if (overwrite_func)
            should_overwrite = (*overwrite_func)(overwrite_cookie);

        if (!should_overwrite) {
            /* We want to reject overwrite. */
            should_upload_data = 0;
            err = cahute_seven_send_basic(
                link,
                0,
                PACKET_TYPE_NAK,
                PACKET_SUBTYPE_NAK_REJECT_OVERWRITE
            );
        } else {
            /* We want to confirm overwrite! */
            err = cahute_seven_send_basic(
                link,
                0,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_CONFIRM_OVERWRITE
            );
        }

        if (err)
            return err;
    }

    /* Whether we've been through an overwrite confirmation flow and confirmed
     * it, rejected it, or if no overwrite confirmation was requested, at this
     * point, we expect an acknowledgement to be the last packet to have
     * been received. */
    EXPECT_BASIC_ACK;

    if (should_upload_data && file_size
        && (err = cahute_seven_send_bytes_from_stream(
                link,
                0,
                file,
                file_size,
                progress_func,
                progress_cookie
            )))
        return err;

    return CAHUTE_OK;
}
