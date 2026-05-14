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
 * Obtain raw CAS300 device information.
 *
 * @param link Link from which to obtain the device information.
 * @param rawp Pointer to set to the raw data.
 * @param sizep Pointer to set to the raw data size.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
obtain_raw_device_info(cahute_link *link, cahute_u8 const **rawp) {
    *rawp = link->protocol_state.casiolink.raw_device_info;
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas300_get_flash_rom_capacity(
    cahute_link *link,
    unsigned long *valuep
) {
    cahute_u8 const *raw;
    char raw_buf[10];
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    /* Flash ROM capacity is presented in a human-readable format.
     * We want to try to determine the machine-readable format here. */
    cahute_copy_ff_string(raw_buf, &raw[32], 8);
    if (!strcmp(raw_buf, "16M"))
        *valuep = 16777216;
    else {
        msg(link->context, ll_error, "Unknown RAM capacity: %s", raw_buf);
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas300_get_bootcode_version(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    char raw_buf[10];
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    cahute_copy_ff_string(raw_buf, &raw[24], 8);
    if (size < strlen(raw_buf) + 1)
        return CAHUTE_ERROR_SIZE;

    strcpy(buf, raw_buf);
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas300_get_os_version(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    char raw_buf[20];
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    /* OS version seems to be presented in a strange format, being
     * "00.00.0(03050000" for OS 03.05.0000. We want to try to extract
     * the OS version from that. */
    cahute_copy_ff_string(raw_buf, &raw[8], 16);
    if (strlen(raw_buf) != 16) {
        msg(link->context,
            ll_error,
            "Unable to extract OS version from: %s",
            raw_buf);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (size < 11)
        return CAHUTE_ERROR_SIZE;

    *buf++ = raw_buf[8];
    *buf++ = raw_buf[9];
    *buf++ = '.';
    *buf++ = raw_buf[10];
    *buf++ = raw_buf[11];
    *buf++ = '.';
    *buf++ = raw_buf[12];
    *buf++ = raw_buf[13];
    *buf++ = raw_buf[14];
    *buf++ = raw_buf[15];
    *buf = 0;

    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas300_get_hwid(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    char raw_buf[10];
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    cahute_copy_ff_string(raw_buf, raw, 8);
    if (size < strlen(raw_buf) + 1)
        return CAHUTE_ERROR_SIZE;

    strcpy(buf, raw_buf);
    return CAHUTE_OK;
}
