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

/**
 * Populate an existing file structure with memory related data.
 *
 * NOTE: This is an internal function only.
 *
 * WARNING: cahute_close_file() MUST NOT be called with such a resource.
 *
 * @param file File structure to populate.
 * @param context Context to provide the file with.
 * @param buf Buffer to read or write from.
 * @param size Size of the buffer.
 * @return Error, or 0 if successful.
 */
CAHUTE_INTERNAL(void)
cahute_populate_file_from_memory(
    cahute_file *file,
    cahute_context *context,
    cahute_u8 *buf,
    size_t size
) {
    file->flags = CAHUTE_FILE_FLAG_WRITE | CAHUTE_FILE_FLAG_READ
                  | CAHUTE_FILE_FLAG_SEEK | CAHUTE_FILE_FLAG_SIZE;
    file->file_size = size;
    file->offset = 0;
    file->read_offset = 0;
    file->read_size = size;
    file->context = context;
    file->cookie = NULL;
    file->read_buffer = NULL;
    file->close_func = 0;
    file->read_func = 0;
    file->write_func = 0;
    file->seek_func = 0;
    file->type = 0;
    file->extension[0] = '\0';
}
