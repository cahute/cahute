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
 * Decode a command payload.
 *
 * @param link Link to use to decode the payload.
 * @param overwritep Pointer to the overwrite to define.
 * @param datatypep Pointer to the data type to define.
 * @param param1p Pointer to the first parameter pointer to define.
 * @param param1_sizep Pointer to the first parameter size to define.
 * @param param2p Pointer to the second parameter pointer to define.
 * @param param2_sizep Pointer to the second parameter size to define.
 * @param param3p Pointer to the third parameter pointer to define.
 * @param param3_sizep Pointer to the third parameter size to define.
 * @param param4p Pointer to the fourth parameter pointer to define.
 * @param param4_sizep Pointer to the fourth parameter size to define.
 * @param param5p Pointer to the fifth parameter pointer to define.
 * @param param5_sizep Pointer to the fifth parameter size to define.
 * @param param6p Pointer to the sixth parameter pointer to define.
 * @param param6_sizep Pointer to the sixth parameter size to define.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_decode_command(
    cahute_link *link,
    int *overwritep,
    int *datatypep,
    unsigned long *filesizep,
    cahute_u8 const **param1p,
    size_t *param1_sizep,
    cahute_u8 const **param2p,
    size_t *param2_sizep,
    cahute_u8 const **param3p,
    size_t *param3_sizep,
    cahute_u8 const **param4p,
    size_t *param4_sizep,
    cahute_u8 const **param5p,
    size_t *param5_sizep,
    cahute_u8 const **param6p,
    size_t *param6_sizep
) {
    cahute_u8 const *buf = link->protocol_state.seven.last_packet_data;
    cahute_u8 const *param1 = NULL, *param2 = NULL, *param3 = NULL,
                    *param4 = NULL, *param5 = NULL, *param6 = NULL;
    size_t param1_size = 0, param2_size = 0, param3_size = 0, param4_size = 0,
           param5_size = 0, param6_size = 0;
    unsigned long filesize = 0;
    int overwrite = 0, datatype = 0;

    if (!link->protocol_state.seven.last_packet_data_size)
        goto end;

    if (link->protocol_state.seven.last_packet_data_size < 24) {
        msg(link->context,
            ll_error,
            "Data buffer too small (%" CAHUTE_PRIuSIZE " < 24).",
            link->protocol_state.seven.last_packet_data_size);
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* All 24 first bytes must be ASCII-HEX. */
    if (!cahute_is_ascii_hex(buf[0]) || !cahute_is_ascii_hex(buf[1])
        || !cahute_is_ascii_hex(buf[2]) || !cahute_is_ascii_hex(buf[3])
        || !cahute_is_ascii_hex(buf[4]) || !cahute_is_ascii_hex(buf[5])
        || !cahute_is_ascii_hex(buf[6]) || !cahute_is_ascii_hex(buf[7])
        || !cahute_is_ascii_hex(buf[8]) || !cahute_is_ascii_hex(buf[9])
        || !cahute_is_ascii_hex(buf[10]) || !cahute_is_ascii_hex(buf[11])
        || !cahute_is_ascii_hex(buf[12]) || !cahute_is_ascii_hex(buf[13])
        || !cahute_is_ascii_hex(buf[14]) || !cahute_is_ascii_hex(buf[15])
        || !cahute_is_ascii_hex(buf[16]) || !cahute_is_ascii_hex(buf[17])
        || !cahute_is_ascii_hex(buf[18]) || !cahute_is_ascii_hex(buf[19])
        || !cahute_is_ascii_hex(buf[20]) || !cahute_is_ascii_hex(buf[21])
        || !cahute_is_ascii_hex(buf[22]) || !cahute_is_ascii_hex(buf[23]))
        return CAHUTE_ERROR_UNKNOWN;

    param1_size = (cahute_ascii_hex_to_nibble(buf[12]) << 4)
                  | cahute_ascii_hex_to_nibble(buf[13]);
    param2_size = (cahute_ascii_hex_to_nibble(buf[14]) << 4)
                  | cahute_ascii_hex_to_nibble(buf[15]);
    param3_size = (cahute_ascii_hex_to_nibble(buf[16]) << 4)
                  | cahute_ascii_hex_to_nibble(buf[17]);
    param4_size = (cahute_ascii_hex_to_nibble(buf[18]) << 4)
                  | cahute_ascii_hex_to_nibble(buf[19]);
    param5_size = (cahute_ascii_hex_to_nibble(buf[20]) << 4)
                  | cahute_ascii_hex_to_nibble(buf[21]);
    param6_size = (cahute_ascii_hex_to_nibble(buf[22]) << 4)
                  | cahute_ascii_hex_to_nibble(buf[23]);

    if (link->protocol_state.seven.last_packet_data_size
        != 24 + param1_size + param2_size + param3_size + param4_size
               + param5_size + param6_size)
        return CAHUTE_ERROR_UNKNOWN;

    overwrite = (cahute_ascii_hex_to_nibble(buf[0]) << 4)
                | cahute_ascii_hex_to_nibble(buf[1]);
    datatype = (cahute_ascii_hex_to_nibble(buf[2]) << 4)
               | cahute_ascii_hex_to_nibble(buf[3]);
    filesize =
        ((cahute_ascii_hex_to_nibble(buf[4]) << 28)
         | (cahute_ascii_hex_to_nibble(buf[5]) << 24)
         | (cahute_ascii_hex_to_nibble(buf[6]) << 20)
         | (cahute_ascii_hex_to_nibble(buf[7]) << 16)
         | (cahute_ascii_hex_to_nibble(buf[8]) << 12)
         | (cahute_ascii_hex_to_nibble(buf[9]) << 8)
         | (cahute_ascii_hex_to_nibble(buf[10]) << 4)
         | cahute_ascii_hex_to_nibble(buf[11]));

    param1 = &buf[24];
    param2 = param1 + param1_size;
    param3 = param2 + param2_size;
    param4 = param3 + param3_size;
    param5 = param4 + param4_size;
    param6 = param5 + param5_size;

end:
    if (overwritep)
        *overwritep = overwrite;
    if (datatypep)
        *datatypep = datatype;
    if (filesizep)
        *filesizep = filesize;
    if (param1p)
        *param1p = param1;
    if (param1_sizep)
        *param1_sizep = param1_size;
    if (param2p)
        *param2p = param2;
    if (param2_sizep)
        *param2_sizep = param2_size;
    if (param3p)
        *param3p = param3;
    if (param3_sizep)
        *param3_sizep = param3_size;
    if (param4p)
        *param4p = param4;
    if (param4_sizep)
        *param4_sizep = param4_size;
    if (param5p)
        *param5p = param5;
    if (param5_sizep)
        *param5_sizep = param5_size;
    if (param6p)
        *param6p = param6;
    if (param6_sizep)
        *param6_sizep = param6_size;
    return CAHUTE_OK;
}
