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

#include "../internals.h"
#define HEXDIGIT(C) \
    ((C) >= 'a' ? (C) - 'a' + 10 : (C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')
#define TOXDIGIT(N) (((N) & 15) > 9 ? 'W' + ((N) & 15) : '0' + ((N) & 15))

/**
 * Serialize a GUID in a string.
 *
 * @param context Context in which the function is run.
 * @param buf Buffer in which to write.
 * @param size Size of the buffer in which to write.
 * @param guid GUID to serialize.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_serialize_win32_guid(
    cahute_context *context,
    char *buf,
    size_t size,
    GUID const *guid
) {
    if (size < 39)
        return CAHUTE_ERROR_SIZE;

    *buf++ = '{';
    *buf++ = TOXDIGIT(guid->Data1 >> 28);
    *buf++ = TOXDIGIT(guid->Data1 >> 24);
    *buf++ = TOXDIGIT(guid->Data1 >> 20);
    *buf++ = TOXDIGIT(guid->Data1 >> 16);
    *buf++ = TOXDIGIT(guid->Data1 >> 12);
    *buf++ = TOXDIGIT(guid->Data1 >> 8);
    *buf++ = TOXDIGIT(guid->Data1 >> 4);
    *buf++ = TOXDIGIT(guid->Data1);
    *buf++ = '-';
    *buf++ = TOXDIGIT(guid->Data2 >> 12);
    *buf++ = TOXDIGIT(guid->Data2 >> 8);
    *buf++ = TOXDIGIT(guid->Data2 >> 4);
    *buf++ = TOXDIGIT(guid->Data2);
    *buf++ = '-';
    *buf++ = TOXDIGIT(guid->Data3 >> 12);
    *buf++ = TOXDIGIT(guid->Data3 >> 8);
    *buf++ = TOXDIGIT(guid->Data3 >> 4);
    *buf++ = TOXDIGIT(guid->Data3);
    *buf++ = '-';
    *buf++ = TOXDIGIT(guid->Data4[0] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[0]);
    *buf++ = TOXDIGIT(guid->Data4[1] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[1]);
    *buf++ = '-';
    *buf++ = TOXDIGIT(guid->Data4[2] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[2]);
    *buf++ = TOXDIGIT(guid->Data4[3] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[3]);
    *buf++ = TOXDIGIT(guid->Data4[4] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[4]);
    *buf++ = TOXDIGIT(guid->Data4[5] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[5]);
    *buf++ = TOXDIGIT(guid->Data4[6] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[6]);
    *buf++ = TOXDIGIT(guid->Data4[7] >> 4);
    *buf++ = TOXDIGIT(guid->Data4[7]);
    *buf++ = '}';
    *buf++ = '\0';

    return CAHUTE_OK;
}


/**
 * Decode a GUID from a string.
 *
 * An example string is "{4d36e967-e325-11ce-bfc1-08002be10318}".
 *
 * @param context Context in which the function is run.
 * @param guid Pointer to the GUID to set.
 * @param raw Raw GUID to parse.
 * @return 1 if parsing has failed, 0 otherwise.
 */
CAHUTE_INTERNAL(int)
cahute_decode_win32_guid(
    cahute_context *context,
    GUID *guid,
    char const *raw
) {
    int enclosed = raw[0] == '{';

    raw += enclosed;
    if (!isxdigit(raw[0]) || !isxdigit(raw[1]) || !isxdigit(raw[2])
        || !isxdigit(raw[3]) || !isxdigit(raw[4]) || !isxdigit(raw[5])
        || !isxdigit(raw[6]) || !isxdigit(raw[7]) || raw[8] != '-'
        || !isxdigit(raw[9]) || !isxdigit(raw[10]) || !isxdigit(raw[11])
        || !isxdigit(raw[12]) || raw[13] != '-' || !isxdigit(raw[14])
        || !isxdigit(raw[15]) || !isxdigit(raw[16]) || !isxdigit(raw[17])
        || raw[18] != '-' || !isxdigit(raw[19]) || !isxdigit(raw[20])
        || !isxdigit(raw[21]) || !isxdigit(raw[22]) || raw[23] != '-'
        || !isxdigit(raw[24]) || !isxdigit(raw[25]) || !isxdigit(raw[26])
        || !isxdigit(raw[27]) || !isxdigit(raw[28]) || !isxdigit(raw[29])
        || !isxdigit(raw[30]) || !isxdigit(raw[31]) || !isxdigit(raw[32])
        || !isxdigit(raw[33]) || !isxdigit(raw[34]) || !isxdigit(raw[35])
        || (enclosed && raw[36] != '}') || raw[36 + enclosed]) {
        msg(context, ll_error, "Unable to decode GUID: %s", raw - enclosed);
        return 1;
    }

    guid->Data1 = cahute_htole32(
        (HEXDIGIT(raw[0]) << 28) | (HEXDIGIT(raw[1]) << 24)
        | (HEXDIGIT(raw[2]) << 20) | (HEXDIGIT(raw[3]) << 16)
        | (HEXDIGIT(raw[4]) << 12) | (HEXDIGIT(raw[5]) << 8)
        | (HEXDIGIT(raw[6]) << 4) | HEXDIGIT(raw[7])
    );
    guid->Data2 = cahute_htole16(
        (HEXDIGIT(raw[9]) << 12) | (HEXDIGIT(raw[10]) << 8)
        | (HEXDIGIT(raw[11]) << 4) | HEXDIGIT(raw[12])
    );
    guid->Data3 = cahute_htole16(
        (HEXDIGIT(raw[14]) << 12) | (HEXDIGIT(raw[15]) << 8)
        | (HEXDIGIT(raw[16]) << 4) | HEXDIGIT(raw[17])
    );
    guid->Data4[0] = (HEXDIGIT(raw[19]) << 4) | HEXDIGIT(raw[20]);
    guid->Data4[1] = (HEXDIGIT(raw[21]) << 4) | HEXDIGIT(raw[22]);

    /* We skip ``raw[24]`` because it's a dash. */
    guid->Data4[2] = (HEXDIGIT(raw[24]) << 4) | HEXDIGIT(raw[25]);
    guid->Data4[3] = (HEXDIGIT(raw[26]) << 4) | HEXDIGIT(raw[27]);
    guid->Data4[4] = (HEXDIGIT(raw[28]) << 4) | HEXDIGIT(raw[29]);
    guid->Data4[5] = (HEXDIGIT(raw[30]) << 4) | HEXDIGIT(raw[31]);
    guid->Data4[6] = (HEXDIGIT(raw[32]) << 4) | HEXDIGIT(raw[33]);
    guid->Data4[7] = (HEXDIGIT(raw[34]) << 4) | HEXDIGIT(raw[35]);
    return 0;
}
