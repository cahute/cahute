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
 * Request a file from storage on the calculator.
 *
 * @param link Link to the device.
 * @param directory Optional name of the directory.
 * @param name Name of the file.
 * @param storage Name of the storage device.
 * @param path Path to which to write the result.
 * @param path_type Type of the path.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_request_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage,
    void const *path,
    int path_type,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    int err = CAHUTE_OK;
    unsigned long filesize;
    cahute_file *file = NULL;

    /* Active sends 0x44 command.
     * Active receives ACK. */
    err = cahute_seven_send_command(
        link,
        0x44,
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
        goto fail;

    EXPECT_BASIC_ACK;

    /* Active sends roleswap.
     * Passive sends command 0x45 with file size. */
    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    EXPECT_PACKET(PACKET_TYPE_COMMAND, 0x45);

    err = cahute_seven_decode_command(
        link,
        NULL,
        NULL,
        &filesize,
        NULL,
        NULL, /* D1 */
        NULL,
        NULL, /* D2 */
        NULL,
        NULL, /* D3 */
        NULL,
        NULL, /* D4 */
        NULL,
        NULL, /* D5 */
        NULL,
        NULL /* D6 */
    );
    if (err)
        goto fail;

    if (path)
        err = cahute_create_file(
            link->context,
            &file,
            filesize,
            path,
            path_type
        );
    else
        err = cahute_open_stdout(link->context, &file);

    if (err)
        goto fail;

    /* Active sends ACK.
     * Data flow occurs from passive to active.
     * Last ACK is not yet sent. */
    err = cahute_seven_receive_bytes_into_stream(
        link,
        file,
        filesize,
        0x45,
        progress_func,
        progress_cookie
    );
    if (err)
        goto fail;

    /* Active sends ACK for last data packet.
     * Passive sends ROLESWAP.
     * We are back to initial situation. */
    err = cahute_seven_send_basic(
        link,
        0,
        PACKET_TYPE_ACK,
        PACKET_SUBTYPE_ACK_BASIC
    );
    if (err)
        goto fail;

    EXPECT_PACKET(PACKET_TYPE_ROLESWAP, 0);

fail:
    if (file)
        cahute_close_file(file);

    return err;
}
