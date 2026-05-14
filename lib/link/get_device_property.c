/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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
#include <stdarg.h>
#define CAS100 CAHUTE_LINK_PROTOCOL_SERIAL_CAS100
#define CAS300 \
CAHUTE_LINK_PROTOCOL_SERIAL_CAS300: \
    case CAHUTE_LINK_PROTOCOL_USB_CAS300
#define SEVEN \
CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN: \
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN

/**
 * Get the device property regarding a given link.
 *
 * @param link Link for which to get the device property.
 * @param name Name of the device property to extract.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN_VA(int)
cahute_get_device_property(cahute_link *link, char const *name, ...) {
    va_list ap;
    char *buf;
    size_t size;
    unsigned long *ulp;
    int err = CAHUTE_ERROR_INCOMPAT;

    va_start(ap, name);

    if (!strcmp(name, "product_id")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_product_id(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "username")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_username(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "organisation")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_organisation(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "hwid")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case CAS100:
            err = cahute_cas100_get_hwid(link, buf, size);
            break;

        case CAS300:
            err = cahute_cas300_get_hwid(link, buf, size);
            break;

        case SEVEN:
            err = cahute_seven_get_hwid(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "cpuid")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_cpuid(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "os_version")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case CAS100:
            err = cahute_cas100_get_os_version(link, buf, size);
            break;

        case CAS300:
            err = cahute_cas300_get_os_version(link, buf, size);
            break;

        case SEVEN:
            err = cahute_seven_get_os_version(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "os_offset")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_os_offset(link, ulp);
            break;
        }
    } else if (!strcmp(name, "os_size")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_os_size(link, ulp);
            break;
        }
    } else if (!strcmp(name, "rom_capacity")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_rom_capacity(link, ulp);
            break;
        }
    } else if (!strcmp(name, "rom_version")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_rom_version(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "flash_rom_capacity")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case CAS100:
            err = cahute_cas100_get_flash_rom_capacity(link, ulp);
            break;

        case CAS300:
            err = cahute_cas300_get_flash_rom_capacity(link, ulp);
            break;

        case SEVEN:
            err = cahute_seven_get_flash_rom_capacity(link, ulp);
            break;
        }
    } else if (!strcmp(name, "ram_capacity")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case CAS100:
            err = cahute_cas100_get_ram_capacity(link, ulp);
            break;

        case SEVEN:
            err = cahute_seven_get_ram_capacity(link, ulp);
            break;
        }
    } else if (!strcmp(name, "bootcode_version")) {
        buf = va_arg(ap, char *);
        size = va_arg(ap, size_t);

        switch (link->protocol) {
        case CAS300:
            err = cahute_cas300_get_bootcode_version(link, buf, size);
            break;

        case SEVEN:
            err = cahute_seven_get_bootcode_version(link, buf, size);
            break;
        }
    } else if (!strcmp(name, "bootcode_offset")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_bootcode_offset(link, ulp);
            break;
        }
    } else if (!strcmp(name, "bootcode_size")) {
        ulp = va_arg(ap, unsigned long *);

        switch (link->protocol) {
        case SEVEN:
            err = cahute_seven_get_bootcode_size(link, ulp);
            break;
        }
    } else {
        msg(link->context, ll_error, "Unknown device property \"%s\".", name);
        err = CAHUTE_ERROR_INVALID;
    }

    if (!err)
        msg(link->context,
            ll_debug,
            "Obtained property \"%s\" successfully.",
            name);
    else
        msg(link->context,
            ll_debug,
            "Obtained error %s while obtaining property \"%s\".",
            cahute_get_error_name(err),
            name);

    va_end(ap);
    return err;
}
