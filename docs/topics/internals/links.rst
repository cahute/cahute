Link internals
==============

This document describes the internals behind links; see
:ref:`feature-topic-links` for more information.

A link only requires one memory allocation (except for system resources that
are allocated / opened using different functions), and the transport
and the protocol are initialized together using link opening functions.

Transports / system interface
-----------------------------

Platform (system) specific code is isolated in conditionally included source
code within ``lib/platform``, with "public" functions (accessible to the
platform-independent portion of the library, but inaccessible to external code)
declared in ``lib/internals.h`` (search for ``Platform-specific functions.``).

Public entry points for links, being :c:func:`cahute_open_usb_link`,
:c:func:`cahute_open_simple_usb_link` and :c:func:`cahute_open_serial_link`,
call system-specific link opening functions depending on platform detection,
which in turn call the following platform-independent utilities:

.. c:function:: int cahute_open_serial_link_from_interface( \
    cahute_serial_link_open_params *open_params, \
    cahute_serial_link_interface const *interface, \
    void *cookie, size_t cookie_size)

    Open a serial link (internal platform-independent interface);
    see :ref:`protocol-topic-transport-serial` for more information.

    ``open_params`` is computed by :c:func:`cahute_open_serial_link` and
    **must be treated as opaque** (as it is defined by
    :c:func:`cahute_open_serial_link` for its underlying platform-independent
    utility).

    ``interface`` must be defined by the platform-specific code, can be defined
    as static / constant, and its type is defined as follows:

    .. c:struct:: cahute_serial_link_interface

        .. c:member:: char const *name

            Name of the interface for logging purposes.

        .. c:member:: cahute_link_close_func *close_func

            Function to use to close the link.

            Can be set to ``NULL``; if this is the case, no platform-specific
            code will be called to close the link.

        .. c:member:: cahute_link_receive_func *receive_func

            Function to use to receive data on the link.

        .. c:member:: cahute_link_send_func *send_func

            Function to use to send data on the link.

        .. c:member:: cahute_link_set_serial_params_func \
            *set_serial_params_func

            Function to use to set the serial parameters on the serial link.

    ``cookie`` is the cookie that will be passed to the platform-specific
    functions defined in the interface.

    .. warning::

        ``cookie`` will not be used directly, but will be **copied** on
        a new memory area reserved with all other requirements for the link.

        As such, if you need to allocate memory that is shared with your
        link's transport for communication, you must allocate the shared memory
        separately, and include a pointer to it in the cookie, that will
        be copied to the new memory area.

    .. warning::

        Either ``cookie`` is defined and ``cookie_size`` is greater than 0,
        or ``cookie`` is ``NULL`` and ``cookie_size`` is equal to 0.
        Any other case is considered a bug in the platform-specific
        implementation by the library, and the initialization will fail.

    :param open_params: Opaque parameter transmitted by
        :c:func:`cahute_open_serial_link`, to consider opaque and send
        to the function.
    :param interface: Interface defined on a platform basis.
    :param cookie: Cookie to copy on the link, and transmit to the
        platform-specific functions set in the interface.
    :param cookie_size: Size of the cookie to copy.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_open_serial_over_usb_bulk_link_from_interface( \
    cahute_usb_link_open_params *open_params, \
    cahute_serial_over_usb_bulk_link_interface const *interface, \
    void *cookie, size_t cookie_size)

    Open a USB bulk link (internal platform-independent interface);
    see :ref:`protocol-topic-transport-serial-over-usb-bulk` for more
    information.

    ``open_params`` is computed by :c:func:`cahute_open_usb_link` and
    **must be treated as opaque** (as it is defined by
    :c:func:`cahute_open_usb_link` for its underlying platform-independent
    utilities).

    ``interface`` must be defined by the platform-specific code, can be defined
    as static / constant, and its type is defined as follows:

    .. c:struct:: cahute_serial_over_usb_bulk_link_interface

        .. c:member:: char const *name

            Name of the interface for logging purposes.

        .. c:member:: cahute_link_close_func *close_func

            Function to use to close the link.

            Can be set to ``NULL``; if this is the case, no platform-specific
            code will be called to close the link.

        .. c:member:: cahute_link_receive_func *receive_func

            Function to use to receive data on the link.

        .. c:member:: cahute_link_send_func *send_func

            Function to use to send data on the link.

    ``cookie`` is the cookie that will be passed to the platform-specific
    functions defined in the interface.

    .. warning::

        ``cookie`` will not be used directly, but will be **copied** on
        a new memory area reserved with all other requirements for the link.

        As such, if you need to allocate memory that is shared with your
        link's transport for communication, you must allocate the shared memory
        separately, and include a pointer to it in the cookie, that will
        be copied to the new memory area.

    .. warning::

        Either ``cookie`` is defined and ``cookie_size`` is greater than 0,
        or ``cookie`` is ``NULL`` and ``cookie_size`` is equal to 0.
        Any other case is considered a bug in the platform-specific
        implementation by the library, and the initialization will fail.

    :param open_params: Opaque parameter transmitted by
        :c:func:`cahute_open_serial_link`, to consider opaque and send
        to the function.
    :param interface: Interface defined on a platform basis.
    :param cookie: Cookie to copy on the link, and transmit to the
        platform-specific functions set in the interface.
    :param cookie_size: Size of the cookie to copy.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_open_ums_link_from_interface( \
    cahute_usb_link_open_params *open_params, \
    cahute_ums_link_interface const *interface, \
    void *cookie, size_t cookie_size)

    Open a USB Mass Storage (UMS) link (internal platform-independent
    interface); see :ref:`protocol-topic-transport-ums` for more information.

    ``open_params`` is computed by :c:func:`cahute_open_usb_link` and
    **must be treated as opaque** (as it is defined by
    :c:func:`cahute_open_usb_link` for its underlying platform-independent
    utilities).

    ``interface`` must be defined by the platform-specific code, can be defined
    as static / constant, and its type is defined as follows:

    .. c:struct:: cahute_ums_link_interface

        .. c:member:: char const *name

            Name of the interface for logging purposes.

        .. c:member:: cahute_link_close_func *close_func

            Function to use to close the link.

            Can be set to ``NULL``; if this is the case, no platform-specific
            code will be called to close the link.

        .. c:member:: cahute_link_scsi_request_to_func *request_to_func

            Function to use to send a command, accompanied with optional data.

        .. c:member:: cahute_link_scsi_request_from_func *request_from_func

            Function to use to receive a command, and receive data.

    .. note::

        From these functions, the "receive" and "send" callbacks implement
        receiving and sending through :ref:`CASIO's custom SCSI commands
        <protocol-topic-ums-custom-commands>`.

    ``cookie`` is the cookie that will be passed to the platform-specific
    functions defined in the interface.

    .. warning::

        ``cookie`` will not be used directly, but will be **copied** on
        a new memory area reserved with all other requirements for the link.

        As such, if you need to allocate memory that is shared with your
        link's transport for communication, you must allocate the shared memory
        separately, and include a pointer to it in the cookie, that will
        be copied to the new memory area.

    .. warning::

        Either ``cookie`` is defined and ``cookie_size`` is greater than 0,
        or ``cookie`` is ``NULL`` and ``cookie_size`` is equal to 0.
        Any other case is considered a bug in the platform-specific
        implementation by the library, and the initialization will fail.

    :param open_params: Opaque parameter transmitted by
        :c:func:`cahute_open_serial_link`, to consider opaque and send
        to the function.
    :param interface: Interface defined on a platform basis.
    :param cookie: Cookie to copy on the link, and transmit to the
        platform-specific functions set in the interface.
    :param cookie_size: Size of the cookie to copy.
    :return: The error, or 0 if the operation was successful.

Transport functions
~~~~~~~~~~~~~~~~~~~

The transport interface functions are defined as the following:

.. c:type:: void (cahute_link_close_func)(cahute_context *context, \
    void *cookie)

    Function that, if defined, is called to close link-specific resources.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: Link-specific cookie.

.. c:type:: int (cahute_link_receive_func)(cahute_context *context, \
    void *cookie, cahute_u8 *buf, size_t capacity, size_t *receivedp, \
    unsigned long timeout)

    Function called to receive bytes on the link's underlying transport.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: Link-specific cookie.
    :param buf: Buffer in which to receive data.
    :param capacity: Buffer capacity, in bytes.
    :param receivedp: Pointer to the number of received bytes to set.
    :param timeout: Maximum amount of time to wait input for, in
        milliseconds.
    :return: The error, or 0 if the operation was successful.

.. c:type:: int (cahute_link_send_func)(cahute_context *context, \
    void *cookie, cahute_u8 const *buf, size_t size, size_t *sentp)

    Function called to send bytes on the link's underlying transport.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: Link-specific cookie.
    :param buf: Buffer in which the data to send is defined.
    :param size: Number of bytes to send in the provided buffer, in bytes.
    :param sentp: Pointer to the number of sent bytes to set.
    :return: The error, or 0 if the operation was successful.

.. c:type:: int (cahute_link_set_serial_params_func)(cahute_context *context, \
    void *cookie, unsigned long flags, unsigned long speed)

    Function called to set serial parameters on the link's underlying
    transport.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: Link-specific cookie.
    :param flags: Flags containing the serial parameters; see
        :c:func:`cahute_set_serial_params_to_link` for available flags.
    :param speed: Speed to set to the serial transport, in bauds (e.g.
        ``9600``).
    :return: The error, or 0 if the operation was successful.

.. c:type:: int (cahute_link_scsi_request_to_func)(cahute_context *context, \
    void *cookie, cahute_u8 const *command, size_t command_size, \
    cahute_u8 const *data, size_t data_size, int *statusp)

    Function called to send an SCSI request to the link's underlying
    transport, and optionally send data.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: Link-specific cookie.
    :param command: Buffer to the command to send.
    :param command_size: Command size (6, 10 or 12), in bytes.
    :param data: Pointer to the data to send following the command.
        If no data is to be sent, this is set to ``NULL``.
    :param data_size: Size of the data to send, in bytes.
        If no data is to be sent, this is set to ``0``.
    :param statusp: Pointer to the command's status code to set.
    :return: The error, or 0 if the operation was successful.

.. c:type:: int (cahute_link_scsi_request_from_func)(cahute_context *context, \
    void *cookie, cahute_u8 const *command, size_t command_size, \
    cahute_u8 *buf, size_t buf_size, int *statusp)

    Function called to send an SCSI request to the link's underlying
    transport, and receive data.

    :param context: Context in which the function is loaded.
        Can be used for logging purposes, or to get context-specific resources.
    :param cookie: Link-specific cookie.
    :param command: Buffer to the command to send.
    :param command_size: Command size (6, 10 or 12), in bytes.
    :param buf: Buffer to fill with the received data.
        This is always defined.
    :param buf_size: Buffer capacity, in bytes.
        This is always greater than ``0``, and guaranteed to be aligned
        at the 32-byte mark.
        If this is insufficient compared to the actual received data,
        the function should return :c:macro:`CAHUTE_ERROR_SIZE`.
    :param statusp: Pointer to the command's status code to set.
    :return: The error, or 0 if the operation was successful.

Protocols
---------

Protocols define what operations and logics are available, and how to
implement these operations and logics.

All protocols may use the **data buffer**, which is in the link directly,
which serves at storing raw data or screen data received using the protocol.

Available protocols are:

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_NONE

    No protocol on a serial transport.

    This can be selected by the user in order to use the transport functions
    more directly, through the ones referenced in
    :ref:`header-ref-cahute-link-transport`.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_CAS

    Generic CASIOLINK on a serial transport.

    This can only be used when in receiver mode, i.e.
    :c:macro:`CAHUTE_LINK_FLAG_RECEIVER` must be present for this protocol
    to actually be useful.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_CAS40

    CAS40 on a serial transport.

    See :ref:`protocol-topic-cas40` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_CAS50

    CAS50 on a serial transport.

    See :ref:`protocol-topic-cas50` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_CAS100

    CAS100 on a serial transport.

    See :ref:`protocol-topic-cas100` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_CAS300

    CAS300 on a serial transport.

    See :ref:`protocol-topic-cas300` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN

    Protocol 7.00 over a serial transport.

    See :ref:`protocol-topic-seven` for more information.

    This differs from :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN` by the
    availability of command :ref:`protocol-topic-seven-command-02`.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP

    Protocol 7.00 Screenstreaming over a serial transport.

    See :ref:`protocol-topic-seven-ohp` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_NONE

    No protocol on a USB transport.

    This can be selected by the user in order to use the transport functions
    more directly, through the ones referenced in
    :ref:`header-ref-cahute-link-transport`.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_CAS300

    CAS300 over USB bulk transport.

    See :ref:`protocol-topic-cas300` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_SEVEN

    Protocol 7.00 over USB bulk transport or USB Mass Storage or
    USB Mass Storage commands.

    See :ref:`protocol-topic-seven` and :ref:`protocol-topic-ums` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP

    Protocol 7.00 Screenstreaming over USB bulk transport or USB Mass Storage
    extended commands.

    See :ref:`protocol-topic-seven-ohp` and :ref:`protocol-topic-ums` for more
    information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE

    USB Mass Storage without extensions.

.. _internals-topic-link-open:

Opening behaviours
------------------

In this section, we will describe the behaviour of link opening functions.

:c:func:`cahute_open_serial_link`
    This function first validates all params to ensure compatibility, e.g.
    throws an error in case of unsupported flag, speed, or combination.

    .. note::

        The protocol is selected, depending on the flags, to one of the
        following:

        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_NONE`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CAS`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CAS40`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CAS50`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CAS100`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CAS300`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP`.

    It will then use platform-specific functions to open the serial link.

    If the underlying transport has successfully been opened, it will allocate
    the link and call :c:func:`cahute_set_serial_params_to_link` to set
    the initial serial parameters to it.

    It will then initialize the protocol using the common protocol
    initialization procedure; see
    :ref:`internals-topic-link-protocol-initialization`.

:c:func:`cahute_open_usb_link`
    This function first validates all params to ensure compatibility, e.g.
    throws an error in case of unsupported flag or combination.

    Then, the platform-specific USB device opening method is used.

    Once all is done, the link is created with the selected transport and
    protocol. The function will then initialize the protocol using the
    common protocol initialization procedure; see
    :ref:`internals-topic-link-protocol-initialization`.

:c:func:`cahute_open_simple_usb_link`
    This function is a convenience function, using mostly public functions
    to work:

    * It detects available USB devices using :c:func:`cahute_detect_usb`.
      It only picks USB devices matching the provided filter(s) which,
      if non-zero, act as an accepted device type mask, i.e.:

      * If :c:macro:`CAHUTE_USB_FILTER_SERIAL` is set, devices identifying
        as a CASIOLINK, Protocol 7.00 or Protocol 7.00 Screenstreaming
        device are accepted;
      * If :c:macro:`CAHUTE_USB_FILTER_UMS` is set, devices identifying
        as an USB Mass Storage speaking calculator are accepted.

      If this function finds no matching devices, it sleeps and retries until
      it has no attempts left. If it finds multiple, it fails with error
      :c:macro:`CAHUTE_ERROR_TOO_MANY`.
    * It opens the found USB device using :c:func:`cahute_open_usb_link`.

    It used to be to the program or library to define by itself, and was in
    the guides, but this behaviour is found in most simple scripts that
    use the Cahute library, so it was decided to include it within the library.

.. _internals-topic-link-protocol-initialization:

Protocol initialization
~~~~~~~~~~~~~~~~~~~~~~~

The common protocol initialization procedure is defined by a function named
``cahute_initialize_link_protocol`` in ``link/open/init.c``.

First of all, if the selected protocol is automatic detection,
the communication initialization is used to determine the protocol in which
both devices should communicate.

.. note::

    Since the initialization step is necessary for automatic protocol
    discovery to take place:

    * The :c:macro:`CAHUTE_SERIAL_NOCHECK` flag is forbidden with
      :c:macro:`CAHUTE_SERIAL_PROTOCOL_AUTO`.
    * The :c:macro:`CAHUTE_USB_NOCHECK` flag is forbidden if neither
      :c:macro:`CAHUTE_USB_SEVEN` nor :c:macro:`CAHUTE_USB_CAS300` is provided.

Then, the initialization sequence is run depending on the protocol and role
(sender or receiver, depending on the presence of the
:c:macro:`CAHUTE_SERIAL_RECEIVER` :c:macro:`CAHUTE_USB_RECEIVER` in the flags
of the original function).
