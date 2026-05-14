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
 * Backup the ROM from the calculator using Protocol 7.00.
 *
 * @param link Link to the calculator.
 * @param romp Pointer to the ROM to allocate.
 * @param sizep Pointer to the ROM size to define.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_backup_rom(
    cahute_link *link,
    cahute_u8 **romp,
    size_t *sizep,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    unsigned long filesize;
    cahute_u8 *rom = NULL;
    size_t rom_size;
    int err = CAHUTE_OK;

    *romp = NULL;
    *sizep = 0;

    err = cahute_seven_send_command(
        link,
        0x4F,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        TIMEOUT_COMMAND_RESPONSE
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_COMMAND, 0x50);

    err = cahute_seven_decode_command(
        link,
        NULL,
        NULL,
        &filesize,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
    if (err)
        return err;

    rom_size = (size_t)filesize;
    if (rom_size) {
        rom = malloc(rom_size);
        if (!rom) {
            link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            return CAHUTE_ERROR_ALLOC;
        }

        err = cahute_seven_receive_bytes(
            link,
            CAHUTE_SEVEN_RECEIVE_BYTES_FLAG_DISABLE_SHIFTING,
            rom,
            rom_size,
            0x50,
            progress_func,
            progress_cookie
        );
        if (err)
            goto fail;
    }

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

    EXPECT_PACKET_OR_FAIL(PACKET_TYPE_ROLESWAP, 0);

    *romp = rom;
    *sizep = rom_size;
    return CAHUTE_OK;

fail:
    if (rom)
        free(rom);

    return err;
}
