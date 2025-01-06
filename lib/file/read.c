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
 * Read from the current offset in the file, using the file specific
 * function.
 *
 * This reads at most ``CAHUTE_FILE_READ_BUFFER_SIZE`` into the
 * read buffer, sets ``file->read_offset`` and ``file->read_size``
 * accordingly, and moves ``file->offset`` to the offset right after
 * the current read, even in the case of cursor-less mediums.
 *
 * @param file File from which to read.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
read_from_current_offset(cahute_file *file) {
    cahute_u8 *read_buffer = file->read_buffer;
    size_t read_size = CAHUTE_FILE_READ_BUFFER_SIZE;
    size_t bytes_read = 0;

    /* The implementation must read data in ``read_buffer``,
     * for up to ``read_size`` (while the caller only requires ``size``,
     * although ``size`` may be larger than ``read_size``).
     * It must set ``bytes_read`` to the actual number of bytes
     * read this pass.
     *
     * A ``bytes_read`` value of 0 is interpreted later as an EOF. */
    if (file->read_func) {
        int err;

        err = (*file->read_func)(
            file->context,
            file->cookie,
            read_buffer,
            read_size,
            &bytes_read
        );
        if (err)
            return err;
    }

    /* If we have arrived here, we consider the read offset to have been
     * modified, so we update it here. */
    file->offset += bytes_read;
    file->read_offset += file->read_size;
    file->read_size = bytes_read;

    if (!bytes_read) {
        /* An EOF was signalled, but should not have occurred! */
        msg(file->context, ll_error, "EOF signalled too early!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}


/**
 * Move to a given offset, and ensure that we can read a given size.
 *
 * @param file File object.
 * @param off Offset at which to move.
 * @param size Size to ensure that we can read.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
move_to_offset(cahute_file *file, unsigned long off, size_t size) {
    int err;

    if (off > CAHUTE_MAX_FILE_OFFSET - size) {
        /* Offsets above CAHUTE_MAX_FILE_OFFSET are not supported, therefore
         * we prefer to fail explicitely here. */
        msg(file->context,
            ll_error,
            "Cannot read %" CAHUTE_PRIuSIZE
            " bytes from offset %lu, since it would "
            "cause the file offset to reach undefined values.",
            off);
        return CAHUTE_ERROR_TRUNC;
    }

    if ((file->flags & CAHUTE_FILE_FLAG_SIZE)
        && (size > file->file_size || off > file->file_size - size)) {
        /* Our file interface requires setting the file size explicitely
         * if writing further than the current file size. */
        msg(file->context,
            ll_error,
            "Cannot read %" CAHUTE_PRIuSIZE
            " bytes from offset %lu, since it would "
            "cause the file offset to go %lu bytes past the file size"
            " of %lu bytes.",
            size,
            off,
            off + size - file->file_size,
            file->file_size);
        return CAHUTE_ERROR_TRUNC;
    }

    /* If we're already at the right offset, there is no need to explicitely
     * move here. */
    if (off == file->offset)
        return CAHUTE_OK;

    if (~file->flags & CAHUTE_FILE_FLAG_SEEK) {
        /* If the offset we're trying to read is actually further in the
         * file, we can read or write up until the offset.
         *
         * NOTE: Cursorless mediums are expected to have this flag set,
         * and have their type be a no-op later in this function. */
        if (off < file->offset) {
            msg(file->context, ll_error, "File does not support seeking.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        while (file->offset < off) {
            /* NOTE: Since we don't want to control the size here
             * to avoid making two read() syscalls instead of one,
             * it means that we may need to compensate in
             * ``cahute_read_from_file()`` to actually check
             * the read buffer again after moving the offset. */
            err = read_from_current_offset(file);
            if (err)
                return err;
        }

        return CAHUTE_OK;
    }

    /* The implementation must ask for relocation to ``off``, and
     * if the new offset is obtained, set it to ``new_off``. */
    if (!file->seek_func)
        CAHUTE_RETURN_IMPL(
            file->context,
            "No method available for seeking in the file."
        );

    return (*file->seek_func)(
        file->context,
        file->cookie,
        file->offset,
        &file->offset
    );
}

/**
 * Read data from a file.
 *
 * @param file File object.
 * @param off Offset at which to read data.
 * @param vbuf Buffer in which to write the result.
 * @param size Size of the data to read.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_read_from_file(
    cahute_file *file,
    unsigned long off,
    void *vbuf,
    size_t size
) {
    cahute_u8 *buf = (cahute_u8 *)vbuf;
    int err;

    if (~file->flags & CAHUTE_FILE_FLAG_READ) {
        msg(file->context, ll_error, "File is not readable.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!size)
        return CAHUTE_OK;

    if (!buf) {
        /* As opposed to cahute_receive_on_link_transport(), this function does
         * not support "skipping", as it takes an "off" parameter to do
         * exactly that. This may however cause some confusion, so we want
         * to catch this explicitely. */
        msg(file->context,
            ll_error,
            "cahute_read_from_file() requires a non-NULL buffer!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* Check if we can determine at least part of the data using our current
     * read buffer. */
    if (off < file->read_offset + file->read_size
        && off >= file->read_offset) {
        size_t start_offset = off - file->read_offset;
        size_t to_copy = file->read_size;

        /* We want to copy what exists from the current read buffer. */
        if (to_copy >= size) {
            memcpy(buf, &file->read_buffer[start_offset], size);
            return CAHUTE_OK;
        }

        memcpy(buf, &file->read_buffer[start_offset], to_copy);
        off += to_copy;
        buf += to_copy;
        size -= to_copy;
    }

    /* There is still content to be read, so we want to move the cursor to
     * the offset we want to read now. */
    err = move_to_offset(file, off, size);
    if (err)
        return err;

    /* We can discard the current read buffer in any case. */
    file->read_offset = file->offset;
    file->read_size = 0;

    /* We need to complete the buffer here, by doing multiple passes on the
     * file implementation specific read code until the caller's need
     * is fully satisfied. */
    while (size) {
        err = read_from_current_offset(file);
        if (err)
            return err;

        if (file->read_size >= size) {
            memcpy(buf, file->read_buffer, size);
            break;
        }

        memcpy(buf, file->read_buffer, file->read_size);
        buf += file->read_size;
        size -= file->read_size;
    }

    return CAHUTE_OK;
}
