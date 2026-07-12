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
 * Obtain raw Protocol 7.00 device information.
 *
 * @param link Link from which to obtain the device information.
 * @param rawp Pointer to set to the raw data.
 * @param sizep Pointer to set to the raw data size.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
obtain_raw_device_info(
    cahute_link *link,
    cahute_u8 const **rawp,
    size_t *sizep
) {
    if (~link->protocol_state.seven.flags & SEVEN_FLAG_DEVICE_INFO_REQUESTED) {
        /* We don't have a 'generic device information' if discovery has
         * been disabled. */
        CAHUTE_RETURN_IMPL(
            link->context,
            "No generic device with Protocol 7.00."
        );
    }

    *rawp = link->protocol_state.seven.raw_device_info;
    *sizep = link->protocol_state.seven.raw_device_info_size;
    return CAHUTE_OK;
}

/**
 * Copy a string from a payload to a buffer, while null-terminating it
 * and detecting 0xFF characters as end of strings.
 *
 * @param buf Destination buffer.
 * @param size Size of the destination buffer.
 * @param raw Raw data from which to get the string.
 * @param max_size Maximum size to read from raw data.
 * @return Pointer to the obtained string.
 */
CAHUTE_LOCAL(int)
extract_info_string(
    char *buf,
    size_t size,
    cahute_u8 const *raw,
    size_t max_size
) {
    for (; max_size--; raw++, size--) {
        int byte = *raw;

        if (!size)
            return CAHUTE_ERROR_SIZE;

        if (!byte || byte >= 128)
            break;

        *(unsigned char *)buf++ = byte;
    }

    if (!size)
        return CAHUTE_ERROR_SIZE;

    *buf++ = '\0';
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_product_id(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    return extract_info_string(buf, size, &raw[132], 16);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_username(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    return extract_info_string(
        buf,
        size,
        &raw[148],
        raw_size == 164 ? 16 : 20
    );
}

CAHUTE_INTERNAL(int)
cahute_seven_get_organisation(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw_size == 164)
        return CAHUTE_ERROR_INCOMPAT;

    return extract_info_string(buf, size, &raw[168], 20);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_hwid(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    return extract_info_string(buf, size, raw, 8);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_cpuid(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    return extract_info_string(buf, size, &raw[8], 16);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_rom_capacity(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[50] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    *valuep = cahute_get_long_dec(&raw[24]) * 1024;
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_flash_rom_capacity(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    *valuep = cahute_get_long_dec(&raw[32]) * 1024;
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_ram_capacity(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    *valuep = cahute_get_long_dec(&raw[40]) * 1024;
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_rom_version(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[50] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    return extract_info_string(buf, size, &raw[48], 16);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_bootcode_version(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[66] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    return extract_info_string(buf, size, &raw[64], 16);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_bootcode_offset(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[66] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    *valuep = cahute_get_long_hex(&raw[80]);
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_bootcode_size(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[66] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    *valuep = cahute_get_long_dec(&raw[88]) * 1024;
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_os_version(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[98] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    return extract_info_string(buf, size, &raw[96], 16);
}

CAHUTE_INTERNAL(int)
cahute_seven_get_os_offset(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[98] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    *valuep = cahute_get_long_hex(&raw[112]);
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_seven_get_os_size(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    size_t raw_size;
    int err;

    err = obtain_raw_device_info(link, &raw, &raw_size);
    if (err)
        return err;

    if (raw[98] != '.')
        return CAHUTE_ERROR_UNAVAIL;

    *valuep = cahute_get_long_dec(&raw[120]) * 1024;
    return CAHUTE_OK;
}
