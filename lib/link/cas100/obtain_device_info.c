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
 * Obtain raw CAS100 device information.
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
cahute_cas100_get_flash_rom_capacity(
    cahute_link *link,
    unsigned long *valuep
) {
    cahute_u8 const *raw;
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    *valuep = ((unsigned long)raw[20] << 24) | ((unsigned long)raw[19] << 16)
              | ((unsigned long)raw[18] << 8) | raw[17];
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas100_get_ram_capacity(cahute_link *link, unsigned long *valuep) {
    cahute_u8 const *raw;
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    *valuep = ((unsigned long)raw[24] << 24) | ((unsigned long)raw[23] << 16)
              | ((unsigned long)raw[22] << 8) | raw[21];
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas100_get_os_version(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    if (size < 5)
        return CAHUTE_ERROR_SIZE;

    memcpy(buf, &raw[13], 4);
    buf[4] = 0;
    return CAHUTE_OK;
}

CAHUTE_INTERNAL(int)
cahute_cas100_get_hwid(cahute_link *link, char *buf, size_t size) {
    cahute_u8 const *raw;
    int err;

    err = obtain_raw_device_info(link, &raw);
    if (err)
        return err;

    if (size < 7)
        return CAHUTE_ERROR_SIZE;

    cahute_copy_ff_string(buf, raw, 6);
    return CAHUTE_OK;
}
