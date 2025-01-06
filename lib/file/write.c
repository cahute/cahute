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
#define WRITE_CHUNK_SIZE 4096

/* NUL bytes to write when skipping a stream that does not support seeking. */
CAHUTE_LOCAL_DATA(cahute_u8) null_buffer[1024] = {0};

/**
 * Write in the current offset of the file.
 *
 * This writes ``data`` with the provided ``size`` into the file, updates
 * the current read buffer if need be, and moves ``file->offset`` to the
 * offset right after the current write, even in the case of cursor-less
 * files.
 *
 * @param file File in which to write.
 * @param data Data to write.
 * @param sizep Pointer to the size of the data to write.
 *        This is set to the number of bytes actually written afterwards.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
write_to_current_offset(cahute_file *file, void const *data, size_t *sizep) {
    cahute_u8 const *write_buffer = data;
    size_t write_size = *sizep;
    size_t bytes_written = 0;

    if (write_size > WRITE_CHUNK_SIZE)
        write_size = WRITE_CHUNK_SIZE;

    /* The implementation must write data from ``write_buffer``,
     * for up to ``write_size``. It must set ``bytes_written`` to the
     * actual number of bytes written this pass.
     *
     * A ``bytes_written`` value of 0 is interpreted as an unknown
     * error. */
    if (file->write_func) {
        int err;

        err = (*file->write_func)(
            file->context,
            file->cookie,
            data,
            write_size,
            &bytes_written
        );
        if (err)
            return err;
    }

    if (!bytes_written || bytes_written > write_size) {
        /* This should not have occurred, it is considered a bug. */
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* If the read buffer overlaps with the data actually written, we
     * also want to update our read buffer with the written data if there
     * is an overlap.
     *
     * We first need to find the overlap between both positions.
     * Suppose the following diagram:
     *
     *              off1  off2  off3  off4
     *    (written)  |-----------|
     *   (read_buf)        |-----------|
     *
     * In this case, we need to copy the data from off2 to off3
     * in both buffers. Here:
     *
     * - off2 is the maximum value of both off1 and off2.
     * - off3 is the minimum value of both off3 and off4.
     * - Boundaries of the written buffer is done by doing
     *   ``[max(off1, off2) - off1, min(off3, off4) - off1]``.
     * - Boundaries of the read buffer is done by doing
     *   ``[max(off1, off2) - off2, min(off3, off4) - off2]``. */
    {
        size_t off1 = file->offset;
        size_t off3 = file->offset + bytes_written;
        size_t off2 = file->read_offset;
        size_t off4 = file->read_offset + file->read_size;
        size_t loff = off1 > off2 ? off1 : off2;
        size_t roff = off3 < off4 ? off3 : off4;

        if (loff <= roff)
            memcpy(
                &file->read_buffer[loff - off2],
                &write_buffer[loff - off1],
                roff - loff
            );
    }

    /* If we have arrived here, we consider the read offset to have been
     * modified, so we update it here. */
    file->offset += bytes_written;

    *sizep = bytes_written;
    return CAHUTE_OK;
}

/**
 * Move to a given offset, and ensure that we can write a given size.
 *
 * @param file File object.
 * @param off Offset at which to move.
 * @param size Size to ensure that we can write.
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
            "Cannot write %" CAHUTE_PRIuSIZE
            " bytes at offset %lu, since it would "
            "cause the file offset to reach undefined values.",
            off);
        return CAHUTE_ERROR_SIZE;
    }

    if ((file->flags & CAHUTE_FILE_FLAG_SIZE)
        && (size > file->file_size || off > file->file_size - size)) {
        /* Our file interface requires setting the file size explicitely
         * if writing further than the current file size. */
        msg(file->context,
            ll_error,
            "Cannot write %" CAHUTE_PRIuSIZE
            " bytes at offset %lu, since it would "
            "cause the file offset to go %lu bytes past the file size"
            " of %lu bytes.",
            size,
            off,
            off + size - file->file_size,
            file->file_size);
        return CAHUTE_ERROR_SIZE;
    }

    /* If we're already at the right offset, there is no need to explicitely
     * move here. */
    if (off == file->offset)
        return CAHUTE_OK;

    if (~file->flags & CAHUTE_FILE_FLAG_SEEK) {
        /* If the offset we're trying to read is actually further in the
         * file, we can read or write up until the offset.
         *
         * NOTE: Cursorless files are expected to have this flag set,
         * and have their type be a no-op later in this function. */
        if (off < file->offset) {
            msg(file->context, ll_error, "File does not support seeking.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        while (file->offset < off) {
            size_t write_size = off - file->offset;

            if (write_size > sizeof(null_buffer))
                write_size = sizeof(null_buffer);

            err = write_to_current_offset(file, null_buffer, &write_size);
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
 * Write to a file.
 *
 * @param file File to which to write to.
 * @param offset Offset at which to write data.
 * @param data Data to write.
 * @param size Size of the data to write.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_write_to_file(
    cahute_file *file,
    unsigned long offset,
    void const *data,
    size_t size
) {
    int err;

    if (~file->flags & CAHUTE_FILE_FLAG_WRITE) {
        msg(file->context, ll_error, "File is not writable.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!size)
        return CAHUTE_OK;

    err = move_to_offset(file, offset, size);
    if (err)
        return err;

    while (size) {
        size_t write_size = size;

        err = write_to_current_offset(file, data, &write_size);
        if (err)
            return err;

        data = (cahute_u8 const *)data + write_size;
        size -= write_size;
    }

    return CAHUTE_OK;
}
