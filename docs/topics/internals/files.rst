File internals
==============

This document describes the internals behind files; see
:ref:`feature-topic-files` for more information about the user-facing
abstractions.

A file only requires one memory allocation (except for system resources that
are allocated / opened using different functions), and the medium is
initialized with the file opening functions.

Mediums / system interface
--------------------------

Platform (system) specific code is isolated in conditionally included source
code within ``lib/platform``, with "public" functions (accessible to the
platform-independent portion of the library, but inaccessible to external code)
declared in ``lib/internals.h`` (search for ``Platform-specific functions.``).

Public entry points for files, being :c:func:`cahute_create_file`,
:c:func:`cahute_open_file` and :c:func:`cahute_open_stdout`,
call system-specific file opening functions depending on platform detection,
which in turn call the following platform-independent utilities:

.. c:function:: int cahute_create_file_from_interface( \
    cahute_file_create_params *create_params, \
    cahute_file_create_interface const *interface, \
    void *cookie, size_t cookie_size)

    Create a file (internal platform-independent interface).

    ``create_params`` is computed by :c:func:`cahute_create_file` and
    **must be treated as opaque** (as it is defined by
    :c:func:`cahute_create_file` for its underlying platform-independent
    utility).

    ``interface`` must be defined by the platform-specific code, can be defined
    as static / constant, and its type is defined as follows:

    .. c:struct:: cahute_file_create_interface

        .. c:member:: cahute_file_close_func *close_func

            Function to use to close the file.

            Can be set to ``NULL``; if this is the case, no platform-specific
            code will be called to close the file.

        .. c:member:: cahute_file_write_func *write_func

            Function to use to write data on the current position of the file.

        .. c:member:: cahute_file_seek_func *seek_func

            Function to use to set the cursor's position on the file.

    ``cookie`` is the cookie that will be passed to the platform-specific
    functions defined in the interface.

    .. warning::

        ``cookie`` will not be used directly, but will be **copied** on
        a new memory area reserved with all other requirements for the file.

        As such, if you need to allocate memory that is shared with your
        file's medium for communication, you must allocate the shared memory
        separately, and include a pointer to it in the cookie, that will
        be copied to the new memory area.

    .. warning::

        Either ``cookie`` is defined and ``cookie_size`` is greater than 0,
        or ``cookie`` is ``NULL`` and ``cookie_size`` is equal to 0.
        Any other case is considered a bug in the platform-specific
        implementation by the library, and the initialization will fail.

    :param create_params: Opaque parameter transmitted by
        :c:func:`cahute_create_file`, to consider opaque and send
        to the function.
    :param interface: Interface defined on a platform basis.
    :param cookie: Cookie to copy on the file, and transmit to the
        platform-specific functions set in the interface.
    :param cookie_size: Size of the cookie to copy.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_open_file_from_interface( \
    cahute_file_open_params *create_params, \
    cahute_file_open_interface const *interface, \
    void *cookie, size_t cookie_size, \
    unsigned long file_size)

    Open a file (internal platform-independent interface).

    ``open_params`` is computed by :c:func:`cahute_open_file` and
    **must be treated as opaque** (as it is defined by
    :c:func:`cahute_open_file` for its underlying platform-independent
    utility).

    ``interface`` must be defined by the platform-specific code, can be defined
    as static / constant, and its type is defined as follows:

    .. c:struct:: cahute_file_open_interface

        .. c:member:: cahute_file_close_func *close_func

            Function to use to close the file.

            Can be set to ``NULL``; if this is the case, no platform-specific
            code will be called to close the file.

        .. c:member:: cahute_file_read_func *read_func

            Function to use to read data from the current position of the file.

        .. c:member:: cahute_file_seek_func *seek_func

            Function to use to set the cursor's position on the file.

    ``cookie`` is the cookie that will be passed to the platform-specific
    functions defined in the interface.

    .. warning::

        ``cookie`` will not be used directly, but will be **copied** on
        a new memory area reserved with all other requirements for the file.

        As such, if you need to allocate memory that is shared with your
        file's medium for communication, you must allocate the shared memory
        separately, and include a pointer to it in the cookie, that will
        be copied to the new memory area.

    .. warning::

        Either ``cookie`` is defined and ``cookie_size`` is greater than 0,
        or ``cookie`` is ``NULL`` and ``cookie_size`` is equal to 0.
        Any other case is considered a bug in the platform-specific
        implementation by the library, and the initialization will fail.

    :param open_params: Opaque parameter transmitted by
        :c:func:`cahute_open_file`, to consider opaque and send
        to the function.
    :param interface: Interface defined on a platform basis.
    :param cookie: Cookie to copy on the file, and transmit to the
        platform-specific functions set in the interface.
    :param cookie_size: Size of the cookie to copy.
    :param file_size: Size of the file, i.e. first invalid offset.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_open_stdout_from_interface( \
    cahute_stdout_open_params *create_params, \
    cahute_stdout_open_interface const *interface, \
    void *cookie, size_t cookie_size)

    Open standard output (internal platform-independent interface).

    ``open_params`` is computed by :c:func:`cahute_open_stdout` and
    **must be treated as opaque** (as it is defined by
    :c:func:`cahute_open_stdout` for its underlying platform-independent
    utility).

    ``interface`` must be defined by the platform-specific code, can be defined
    as static / constant, and its type is defined as follows:

    .. c:struct:: cahute_file_open_interface

        .. c:member:: cahute_file_close_func *close_func

            Function to use to close the file.

            Can be set to ``NULL``; if this is the case, no platform-specific
            code will be called to close the file.

        .. c:member:: cahute_file_write_func *write_func

            Function to use to write data on the output.

    ``cookie`` is the cookie that will be passed to the platform-specific
    functions defined in the interface.

    .. warning::

        ``cookie`` will not be used directly, but will be **copied** on
        a new memory area reserved with all other requirements for the file.

        As such, if you need to allocate memory that is shared with your
        file's medium for communication, you must allocate the shared memory
        separately, and include a pointer to it in the cookie, that will
        be copied to the new memory area.

    .. warning::

        Either ``cookie`` is defined and ``cookie_size`` is greater than 0,
        or ``cookie`` is ``NULL`` and ``cookie_size`` is equal to 0.
        Any other case is considered a bug in the platform-specific
        implementation by the library, and the initialization will fail.

    :param open_params: Opaque parameter transmitted by
        :c:func:`cahute_open_stdout`, to consider opaque and send
        to the function.
    :param interface: Interface defined on a platform basis.
    :param cookie: Cookie to copy on the file, and transmit to the
        platform-specific functions set in the interface.
    :param cookie_size: Size of the cookie to copy.
    :return: The error, or 0 if the operation was successful.

Medium functions
~~~~~~~~~~~~~~~~

The medium interface functions are defined as the following:

.. c:type:: void (cahute_file_close_func)(cahute_context *context, \
    void *cookie)

    Function that, if defined, is called to close file-specific resources.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: File-specific cookie.

.. c:type:: int (cahute_file_read_func)(cahute_context *context, \
    void *cookie, cahute_u8 *buf, size_t capacity, size_t *readp)

    Function called to read data from the current position on the file's
    medium.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: File-specific cookie.
    :param buf: Buffer in which to read data.
    :param capacity: Buffer capacity, in bytes.
    :param readp: Pointer to the number of read bytes to set.
    :return: The error, or 0 if the operation was successful.

.. c:type:: int (cahute_file_write_func)(cahute_context *context, \
    void *cookie, cahute_u8 const *buf, size_t size, size_t *writtenp)

    Function called to write data at the current position on the file's
    underlying medium.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: File-specific cookie.
    :param buf: Buffer in which the data to write is defined.
    :param size: Length of the data to write in the provided buffer, in bytes.
    :param sentp: Pointer to the number of written bytes to set.
    :return: The error, or 0 if the operation was successful.

.. c:type:: int (cahute_file_seek_func)(cahute_context *context, \
    void *cookie, unsigned long pos, unsigned long *new_posp)

    Function called to set the position on the file's underlying medium.

    :param context: Context in which the function is loaded.
    :param cookie: File-specific cookie.
    :param pos: Position to set on the file.
    :param new_posp: Pointer to the current position to set, as an offset.
    :return: The error, or 0 if the operation was successful.

.. _internals-topic-file-inmem:

Internal in-memory file medium
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to work for both files and protocols, some functions such as
:c:func:`cahute_casiolink_decode_data` or :c:func:`cahute_mcs_decode_data`
take a :c:type:`cahute_file` instance. For abstracting memory buffers coming
from protocols as in-memory files, the following internal function is available
within the library:

.. c:function:: void cahute_populate_file_from_memory(cahute_file *file, \
    cahute_u8 *buf, size_t size)

    Populate a file handle from a buffer and a size.

    :param file: File object to populate.
    :param buf: Buffer to abstract as a file.
    :param size: Size of the buffer to abstract.

In order to avoid having too many memory allocations and since
:c:type:`cahute_file` is not opaque within the library, this utility can be
used to populate a statically defined file object which can then be transmitted
to other functions using files to decode data. It is also not necessary to
call :c:func:`cahute_close_file` in such a case.

For example, with :c:func:`cahute_casiolink_decode_data`:

.. code-block:: c

    cahute_file file;
    unsigned long offset = 0;

    cahute_populate_file_from_memory(&file, my_buf, my_buf_size);
    err = cahute_casiolink_decode_data(datap, &file, &offset, my_variant, 1);
    ...

In order to work, this abuses the read buffer to not be
:c:macro:`CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE` bytes long, but sized to
the whole "file", representing the buffer, with the read buffer actually being
the provided buffer **directly** with the :c:macro:`CAHUTE_FILE_MEDIUM_NONE`.

It abuses existing manipulations of the read buffer to read directly from the
read buffer, mirror written data in the read buffer to just write into the
buffer at the provided offset, and not have any side effects, i.e. the
operations become the following:

:c:func:`cahute_read_from_file_medium`
    There is **always** an intersection between our current read buffer
    and the requested data on the left boundary, so we read from the "read"
    buffer to copy data to the user-provided buffer.

:c:func:`cahute_write_to_file_medium`
    We do not move any underlying cursor or have any side-effect.

    There is **always** an intersection between the user-provided boundaries
    and the read buffer boundaries, so we write the user-provided data to the
    correct offset in the read buffer to ensure reads from the same offsets
    will return the updated data, and not the data before the write.

File metadata retrieval
-----------------------

The :c:type:`cahute_file` contains caching for the file metadata retrieval,
namely:

* The retrieved type, using one of the ``CAHUTE_FILE_TYPE_*`` macros defined
  in :ref:`header-ref-cahute-file`;
* The extension extracted from the provided path.

For any of the file reading functions that requires file type and metadata,
if the :c:macro:`CAHUTE_FILE_FLAG_EXAMINED` flag is not present in the
file flags yet, the :c:func:`cahute_examine_file` function is called to
determine it and set the flag.
