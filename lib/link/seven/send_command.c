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
 * Send a command using Protocol 7.00 and get the response.
 *
 * @param link Link on which to send the command.
 * @param code Command code.
 * @param overwrite The overwrite mode, amongst the 'SEVEN_OVERWRITE_*'
 *        constants. Setting this to 0 is fine for most commands.
 * @param datatype Data type for main memory related commands.
 *        Setting this to 0 for other commands is fine.
 * @param filesize Announced file size when sending data.
 *        Setting this to 0 is fine for other commands.
 * @param param1 First parameter, NULL if not relevant.
 * @param param2 Second parameter, NULL if not relevant.
 * @param param3 Third parameter, NULL if not relevant.
 * @param param4 Fourth parameter, NULL if not relevant.
 * @param param5 Fifth parameter, NULL if not relevant.
 * @param param6 Sixth parameter, NULL if not relevant.
 * @param timeout Timeout for the command.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_seven_send_command(
    cahute_link *link,
    int code,
    int overwrite,
    int datatype,
    unsigned long filesize,
    char const *param1,
    char const *param2,
    char const *param3,
    char const *param4,
    char const *param5,
    char const *param6,
    unsigned long timeout
) {
    cahute_u8 buf[256], *p = buf;
    size_t length1 = param1 ? strlen(param1) : 0;
    size_t length2 = param2 ? strlen(param2) : 0;
    size_t length3 = param3 ? strlen(param3) : 0;
    size_t length4 = param4 ? strlen(param4) : 0;
    size_t length5 = param5 ? strlen(param5) : 0;
    size_t length6 = param6 ? strlen(param6) : 0;

    if (!overwrite && !datatype && !filesize && !param1 && !param2 && !param3
        && !param4 && !param5 && !param6) {
        /* Since we don't actually use the payload, we can just send a
         * basic packet here! */
        return cahute_seven_send_basic(link, 0, PACKET_TYPE_COMMAND, code);
    }

    if (length1 + length2 + length3 + length4 + length5 + length6 > 232) {
        msg(link->context,
            ll_error,
            "Combined lengths of the parameters cannot exceed 232 bytes!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We need to keep the last command code in case the command is followed
     * by a data transfer, in which the command code must be set as the
     * data packets' subtype. */
    link->protocol_state.seven.last_command = code;

    cahute_set_ascii_hex(p, overwrite & 255);
    cahute_set_ascii_hex(&p[2], datatype & 255);
    cahute_set_ascii_hex(&p[4], (filesize >> 24) & 255);
    cahute_set_ascii_hex(&p[6], (filesize >> 16) & 255);
    cahute_set_ascii_hex(&p[8], (filesize >> 8) & 255);
    cahute_set_ascii_hex(&p[10], filesize & 255);
    cahute_set_ascii_hex(&p[12], length1 & 255);
    cahute_set_ascii_hex(&p[14], length2 & 255);
    cahute_set_ascii_hex(&p[16], length3 & 255);
    cahute_set_ascii_hex(&p[18], length4 & 255);
    cahute_set_ascii_hex(&p[20], length5 & 255);
    cahute_set_ascii_hex(&p[22], length6 & 255);

    p += 24;

    if (length1) {
        memcpy(p, param1, length1);
        p += length1;
    }
    if (length2) {
        memcpy(p, param2, length2);
        p += length2;
    }
    if (length3) {
        memcpy(p, param3, length3);
        p += length3;
    }
    if (length4) {
        memcpy(p, param4, length4);
        p += length4;
    }
    if (length5) {
        memcpy(p, param5, length5);
        p += length5;
    }
    if (length6) {
        memcpy(p, param6, length6);
        p += length6;
    }

    return cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_COMMAND,
        code,
        buf,
        (size_t)(p - buf),
        timeout
    );
}
