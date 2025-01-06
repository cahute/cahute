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
 * Send a file to the calculator's storage.
 *
 * @param link Link to use to send the file.
 * @param flags Usage flags.
 * @param directory Name of the directory to place the file into,
 *        NULL if at root.
 * @param name Name of the file to place the file as.
 * @param storage Storage on which to place the file.
 * @param file File to read from.
 * @param overwrite_func Function to call to confirm overwrite.
 * @param overwrite_cookie Cookie to pass to the overwrite confirmation
 *        function.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_send_file_to_storage(
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
    unsigned long unsupported_flags =
        (flags
         & ~(CAHUTE_SEND_FILE_FLAG_FORCE | CAHUTE_SEND_FILE_FLAG_OPTIMIZE
             | CAHUTE_SEND_FILE_FLAG_DELETE));
    int err;

    if (unsupported_flags) {
        msg(link->context,
            ll_error,
            "Unsupported flags: 0x%08lX",
            unsupported_flags);
        return CAHUTE_ERROR_UNKNOWN;
    }

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    /* Send the file using the protocol. */
    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_send_file_to_storage(
            link,
            flags,
            directory,
            name,
            storage,
            file,
            overwrite_func,
            overwrite_cookie,
            progress_func,
            progress_cookie
        );

    default:
        CAHUTE_RETURN_IMPL(
            link->context,
            "Operation not supported by the link protocol."
        );
    }
}
