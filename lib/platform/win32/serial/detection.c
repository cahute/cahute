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
#define SERIAL_COMM_HKEY "HARDWARE\\DEVICEMAP\\SERIALCOMM"

/**
 * Detect serial devices using the Win32 API.
 *
 * @param context Context.
 * @param func User function to call back with every serial entry.
 * @param cookie Cookie to pass to the user function.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_win32_detect_serial(
    cahute_context *context,
    cahute_detect_serial_entry_func *func,
    void *cookie
) {
    cahute_serial_detection_entry entry;
    char value[1024];
    BYTE data[1024];
    DWORD werr, i = 0;
    HKEY hkey;
    int err = CAHUTE_OK;

    werr = RegOpenKey(HKEY_LOCAL_MACHINE, SERIAL_COMM_HKEY, &hkey);
    if (werr == ERROR_FILE_NOT_FOUND) {
        /* No serial devices available. */
        goto end;
    } else if (werr) {
        log_windows_error(context, "RegOpenKey", werr);
        return CAHUTE_ERROR_UNKNOWN;
    }

    while (1) {
        DWORD data_size = 1023;
        DWORD value_size = 1023;
        DWORD entry_type;

        switch (werr = RegEnumValue(
                    hkey,
                    i++,
                    value,
                    &value_size,
                    NULL,
                    &entry_type,
                    data,
                    &data_size
                )) {
        case 0:
            break;

        case ERROR_NO_MORE_ITEMS:
            goto end;

        case ERROR_MORE_DATA:
            /* Our buffers weren't big enough. Tough luck!
             * Let's go to the next entry and just ignore this one. */
            continue;

        default:
            log_windows_error(context, "RegEnumValue", werr);
            RegCloseKey(hkey);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* We only want REG_SZ entries. */
        if (entry_type != REG_SZ)
            continue;

        /* The 'data' field contains what we expect! */
        data[data_size] = '\0';

        entry.cahute_serial_detection_entry_name = (char *)data;
        if (func(cookie, &entry)) {
            err = CAHUTE_ERROR_INT;
            break;
        }
    }

end:
    RegCloseKey(hkey);

    return err;
}
