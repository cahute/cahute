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
 * Upload and run a program on the calculator.
 *
 * @param link Link to the device.
 * @param program Program to upload and run.
 * @param program_size Size of the program to upload and run.
 * @param load_address Address at which to load the program.
 * @param start_address Address at which to start the program.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_upload_and_run_program(
    cahute_link *link,
    cahute_u8 const *program,
    size_t program_size,
    unsigned long load_address,
    unsigned long start_address,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 command_payload[24];
    int err;

    if (program_size > 0xFFFFFFFF || load_address > 0xFFFFFFFF
        || start_address > 0xFFFFFFFF) {
        msg(link->context,
            ll_error,
            "One of the addresses or sizes does not fit in 32 bits.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* Prepare the command payload. */
    cahute_set_ascii_hex(&command_payload[0], program_size >> 24);
    cahute_set_ascii_hex(&command_payload[2], (program_size >> 16) & 255);
    cahute_set_ascii_hex(&command_payload[4], (program_size >> 8) & 255);
    cahute_set_ascii_hex(&command_payload[6], program_size & 255);
    cahute_set_ascii_hex(&command_payload[8], load_address >> 24);
    cahute_set_ascii_hex(&command_payload[10], (load_address >> 16) & 255);
    cahute_set_ascii_hex(&command_payload[12], (load_address >> 8) & 255);
    cahute_set_ascii_hex(&command_payload[14], load_address & 255);
    cahute_set_ascii_hex(&command_payload[16], start_address >> 24);
    cahute_set_ascii_hex(&command_payload[18], (start_address >> 16) & 255);
    cahute_set_ascii_hex(&command_payload[20], (start_address >> 8) & 255);
    cahute_set_ascii_hex(&command_payload[22], start_address & 255);

    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_COMMAND,
        0x56,
        command_payload,
        24,
        TIMEOUT_PACKET_TIMEOUT
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    link->protocol_state.seven.last_command = 0x56;
    return cahute_seven_send_bytes(
        link,
        CAHUTE_SEVEN_SEND_BYTES_FLAG_DISABLE_SHIFTING,
        program,
        program_size,
        progress_func,
        progress_cookie
    );
}
