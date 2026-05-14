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
 * Write to the current offset using a Win32 file.
 *
 * @param context
 * @param cookie
 * @param buf
 * @param size
 * @param writtenp
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_write_to_win32_file(
    cahute_context *context,
    cahute_win32_file_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *writtenp
) {
    BOOL ret;
    DWORD written;

    ret = WriteFile(cookie->handle, buf, size, &written, NULL);
    if (!ret) {
        log_windows_error(context, "WriteFile", GetLastError());
        return CAHUTE_ERROR_UNKNOWN;
    }

    *writtenp = (size_t)written;
    return CAHUTE_OK;
}
