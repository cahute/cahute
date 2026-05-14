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

/* Raw device info to present for command '01' on receiver mode. */
CAHUTE_LOCAL_DATA(cahute_u8)
fake_device_info[164] = {
    'G', 'y', '3', '6', '3', '0', '0', 'F', 'R', 'E', 'N', 'E', 'S', 'A', 'S',
    ' ', 'S', 'H', '7', '3', '5', '5', '0', '1', '0', '0', '0', '0', '0', '0',
    '0', '0', '0', '0', '0', '0', '4', '0', '9', '6', '0', '0', '0', '0', '0',
    '5', '1', '2', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, '0', '2', '.', '0', '9', '.', '2', '2', '0',
    '1', 255, 255, 255, 255, 255, 255, '0', '0', '0', '1', '0', '0', '0', '0',
    '0', '0', '0', '0', '2', '4', '3', '2', '7', '.', '0', '0', 'A', 'A', 'A',
    'A', 'A', 'A', 'A', 'A', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

/**
 * Receive data using Protocol 7.00.
 *
 * This function must only be run while the device is in passive mode.
 *
 * @param link Link to the device.
 * @param datap Pointer to the data to create.
 * @param timeout Timeout in milliseconds.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_seven_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
) {
    int err, data_type, invalid_command;
    unsigned long data_size;
    cahute_u8 const *param1, *param2, *param3;
    size_t param1_size, param2_size, param3_size;
    unsigned long new_serial_flags, new_serial_speed = 0;

    do {
        err = cahute_seven_receive(link, timeout);
        if (err)
            return err;

        switch (link->protocol_state.seven.last_packet_type) {
        case PACKET_TYPE_COMMAND:
            /* Commands are the main packet type we want to support. */
            break;

        case PACKET_TYPE_CHECK:
            /* Checks can either be the initial check or a regular check;
             * anyway, we want to answer with an ack. */
            err = cahute_seven_send_basic(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_BASIC
            );
            if (err)
                return err;

            continue;

        case PACKET_TYPE_TERM:
            /* The sender wants to terminate the link. */
            err = cahute_seven_send_basic(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_BASIC
            );
            if (err)
                return err;

            link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
            return CAHUTE_ERROR_TERMINATED;

        default: /* Unknown types of packets. */
            err = cahute_seven_send_basic(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_NAK,
                PACKET_SUBTYPE_NAK_OTHER
            );
            if (err)
                return err;

            continue;
        }

        err = cahute_seven_decode_command(
            link,
            NULL,
            &data_type,
            &data_size,
            &param1,
            &param1_size,
            &param2,
            &param2_size,
            &param3,
            &param3_size,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        );
        if (err)
            return err;

        switch (link->protocol_state.seven.last_packet_subtype) {
        case 0x01: /* Command 01 "Get device information" */
            /* We are not sure what section the device reads, so we
             * present a full fake data inspired from the Graph 75+E. */
            err = cahute_seven_send_extended(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_EXTENDED,
                fake_device_info,
                sizeof(fake_device_info),
                TIMEOUT_PACKET_TIMEOUT
            );
            if (err)
                return err;

            continue;

        case 0x02: /* Command 02 "Set link settings" */
            /* Experimentally, the fx-9860G sets the serial link parameters
             * to 115200E1 when the TRANSMIT mode is selected. */
            new_serial_flags = 0;
            invalid_command = 0;

            if (param1_size == 3 && !memcmp(param1, "300", 3))
                new_serial_speed = 300;
            else if (param1_size == 3 && !memcmp(param1, "600", 3))
                new_serial_speed = 600;
            else if (param1_size == 4 && !memcmp(param1, "1200", 4))
                new_serial_speed = 1200;
            else if (param1_size == 4 && !memcmp(param1, "2400", 4))
                new_serial_speed = 2400;
            else if (param1_size == 4 && !memcmp(param1, "4800", 4))
                new_serial_speed = 4800;
            else if (param1_size == 4 && !memcmp(param1, "9600", 4))
                new_serial_speed = 9600;
            else if (param1_size == 5 && !memcmp(param1, "19200", 5))
                new_serial_speed = 19200;
            else if (param1_size == 5 && !memcmp(param1, "38400", 5))
                new_serial_speed = 38400;
            else if (param1_size == 5 && !memcmp(param1, "57600", 5))
                new_serial_speed = 57600;
            else if (param1_size == 6 && !memcmp(param1, "115200", 6))
                new_serial_speed = 115200;
            else {
                msg(link->context,
                    ll_warn,
                    "Unknown setting \"%.*s\" for speed.",
                    param1_size,
                    param1);
                invalid_command = 1;
            }

            if (param2_size == 4 && !memcmp(param2, "EVEN", 4))
                new_serial_flags |= CAHUTE_SERIAL_PARITY_EVEN;
            else if (param2_size == 3 && !memcmp(param2, "ODD", 3))
                new_serial_flags |= CAHUTE_SERIAL_PARITY_ODD;
            else if (param2_size == 4 && !memcmp(param2, "NONE", 4))
                new_serial_flags |= CAHUTE_SERIAL_PARITY_OFF;
            else {
                msg(link->context,
                    ll_warn,
                    "Unknown setting \"%.*s\" for parity.",
                    param2_size,
                    param2);
                invalid_command = 1;
            }

            if (param3_size == 1 && param3[0] == '1')
                new_serial_flags |= CAHUTE_SERIAL_STOP_ONE;
            else if (param3_size == 1 && param3[0] == '2')
                new_serial_flags |= CAHUTE_SERIAL_STOP_TWO;
            else {
                msg(link->context,
                    ll_warn,
                    "Unknown setting \"%.*s\" for stop bits.",
                    param3_size,
                    param3);
                invalid_command = 1;
            }

            if (invalid_command) {
                err = cahute_seven_send_basic(
                    link,
                    CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                    PACKET_TYPE_NAK,
                    PACKET_SUBTYPE_NAK_OTHER
                );
                if (err)
                    return err;

                continue;
            }

            err = cahute_seven_send_basic(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_BASIC
            );
            if (err)
                return err;

            /* We introduce an artificial sleep to make the device believe
             * that we may be slow. Otherwise, the transfer may crash right
             * after we have changed the properties of our link. */
            err = cahute_sleep(link->context, 50);
            if (err)
                return err;

            err = cahute_set_serial_params_on_link_transport(
                link,
                new_serial_flags,
                new_serial_speed
            );
            if (err) {
                /* We have successfully negociated with the device to switch
                 * serial settings but have not managed to change settings
                 * ourselves. We can no longer communicate with the device,
                 * hence can no longer negotiate the serial settings back.
                 * Therefore, we consider the link to be irrecoverable. */
                msg(link->context,
                    ll_error,
                    "Could not set the serial params; that makes our "
                    "connection irrecoverable!");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
                return err;
            }

            msg(link->context, ll_info, "New serial settings have been set!");
            continue;

        case 0x09: /* Command 09 "OS Verification 3" */
            /* We want to imitate the Graph 75+E on this command,
             * and only return an ACK. */
            err = cahute_seven_send_basic(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_BASIC
            );
            if (err)
                return err;

            continue;

        case 0x25: /* Command 25 "Transfer file" (main memory) */
            if (data_size > link->data_buffer_capacity) {
                msg(link->context,
                    ll_error,
                    "File too big for our data buffer capacity "
                    "(%" CAHUTE_PRIuSIZE "o/%" CAHUTE_PRIuSIZE "o).",
                    data_size,
                    link->data_buffer_capacity);

                err = cahute_seven_send_basic(
                    link,
                    CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                    PACKET_TYPE_NAK,
                    PACKET_SUBTYPE_NAK_OTHER
                );
                if (err)
                    return err;

                continue;
            }

            {
                cahute_file file;
                cahute_u8 parambuf[40];

                if (sizeof(parambuf)
                    < param1_size + param2_size + param3_size) {
                    msg(link->context,
                        ll_error,
                        "Parameters are bigger than expected for file "
                        "transfer (%" CAHUTE_PRIuSIZE "o > %" CAHUTE_PRIuSIZE
                        "o)!",
                        param1_size + param2_size + param3_size,
                        sizeof(parambuf));

                    err = cahute_seven_send_basic(
                        link,
                        CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                        PACKET_TYPE_NAK,
                        PACKET_SUBTYPE_NAK_OTHER
                    );
                    if (err)
                        return err;

                    continue;
                }

                /* NOTE: The last_packet_data will be replaced by the
                 * following, so we actually need to copy our parameters
                 * now! */
                {
                    cahute_u8 *parambufp = parambuf;

                    if (param1_size)
                        memcpy(parambufp, param1, param1_size);

                    param1 = parambufp;
                    parambufp += param1_size;

                    if (param2_size)
                        memcpy(parambufp, param2, param2_size);

                    param2 = parambufp;
                    parambufp += param2_size;

                    if (param3_size)
                        memcpy(parambufp, param3, param3_size);

                    param3 = parambufp;
                }

                /* Now the parameters are copied, we can now receive our
                 * raw data! */

                link->data_buffer_size = 0;
                err = cahute_seven_receive_bytes(
                    link,
                    CAHUTE_SEVEN_RECEIVE_BYTES_FLAG_DISABLE_SHIFTING,
                    link->data_buffer,
                    data_size,
                    0x25,
                    NULL,
                    NULL
                );
                if (err)
                    return err;

                link->data_buffer_size = data_size;
                err = cahute_seven_send_basic(
                    link,
                    CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                    PACKET_TYPE_ACK,
                    PACKET_SUBTYPE_ACK_BASIC
                );
                if (err)
                    return err;

                cahute_populate_file_from_memory(
                    &file,
                    link->context,
                    link->data_buffer,
                    link->data_buffer_size
                );

                err = cahute_mcs_decode_data(
                    link->context,
                    datap,
                    param1,
                    param1_size,
                    param3,
                    param3_size,
                    param2,
                    param2_size,
                    &file,
                    0,
                    link->data_buffer_size,
                    data_type
                );
                if (!err)
                    goto end;
                if (err != CAHUTE_ERROR_IMPL)
                    return err;
            }

            /* By default, we want to continue reading data until we find
             * one that we can read. */
            continue;

        default: /* Unsupported command. */
            err = cahute_seven_send_basic(
                link,
                CAHUTE_SEVEN_SEND_FLAG_DISABLE_RECEIVE,
                PACKET_TYPE_NAK,
                PACKET_SUBTYPE_NAK_OTHER
            );
            if (err)
                return err;
        }
    } while (1);

end:
    return CAHUTE_OK;
}
