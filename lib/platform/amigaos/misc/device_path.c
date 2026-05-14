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

/**
 * Get an AmigaOS device name and unit number the raw device name.
 *
 * This function expects ``<KW>=<VALUE>`` parameters separated by commas,
 * with the following supported case-insensitive keywords:
 *
 * ``DEVICE``
 *     Device name.
 *
 * ``U``, ``UNIT``
 *     Unit number.
 *
 * @param context Context in which the function is run.
 * @param dev Pointer to the unit number to set.
 * @param raw Raw device name.
 * @param default_device Default device name.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_get_amigaos_device(
    cahute_context *context,
    cahute_amigaos_device *dev,
    char const *raw,
    char const *default_device
) {
    if (!default_device) {
        msg(context, ll_error, "Default device has not been defined.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (strlen(default_device) + 1 > sizeof(dev->name)) {
        msg(context,
            ll_error,
            "Default device '%s' is too long (max supported: %" CAHUTE_PRIuSIZE
            ").",
            default_device,
            sizeof(dev->name) - 1);
        return CAHUTE_ERROR_UNKNOWN;
    }

    strcpy(dev->name, default_device);
    dev->unit = 0;

    while (1) {
        int isdevice = 0;
        size_t len = 0;

        for (; *raw == ','; raw++)
            ;
        if (!*raw)
            break;

        if (tolower(raw[0]) == 'u' && raw[1] == '=')
            raw += 2;
        else if (tolower(raw[0]) == 'u' && tolower(raw[1]) == 'n' && tolower(raw[2]) == 'i' && tolower(raw[3]) == 't' && raw[4] == '=')
            raw += 5;
        else if (tolower(raw[0]) == 'd' && tolower(raw[1]) == 'e' && tolower(raw[2]) == 'v' && tolower(raw[3]) == 'i' && tolower(raw[4]) == 'c' && tolower(raw[5]) == 'e' && raw[6] == '=') {
            raw += 7;
            isdevice = 1;
        } else {
            /* Unknown keyword. */
            return CAHUTE_ERROR_NOT_FOUND;
        }

        /* Find the length of the property. */
        for (len = 0; raw[len] && raw[len] != ','; len++)
            ;

        if (isdevice) {
            if (!len || len + 1 > sizeof(dev->name))
                return CAHUTE_ERROR_NOT_FOUND;

            memcpy(dev->name, raw, len);
            dev->name[len] = '\0';
            raw += len;
        } else {
            unsigned long unit;

            if (!isdigit(raw[0]))
                return CAHUTE_ERROR_NOT_FOUND;

            unit = raw[0] - '0';
            for (raw++, len--; len; raw++, len--) {
                if (!isdigit(*raw))
                    return CAHUTE_ERROR_NOT_FOUND;

                unit = unit * 10 + (raw[0] - '0');
            }

            dev->unit = unit;
        }
    }

    return CAHUTE_OK;
}
