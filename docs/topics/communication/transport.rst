.. _protocol-topic-transport:

Communication transports and protocols
======================================

All communication protocols used by CASIO calculators are present over
mediums and transport protocols, that presents a stream-like interface as
well as other transport-specific operations that may be necessary.

.. _protocol-topic-transport-serial:

Serial transport
----------------

The serial transport is the only transport present from the oldest calculators
supported by Cahute, to the newest ones. It is used for
calculator-to-calculator communication, as well as, for older models,
host-to-calculator communication.

It presents a byte-oriented stream, with settings that must be set on both
sides for the communication to be established. Said settings include:

* The baud speed;
* The number of stop bits to use (1 or 2);
* The parity (even, odd, none/ignored);
* Additional hardware and software flow control settings, such as
  DTR/RTS or XON/XOFF.

These settings are defined differently depending on the communication
protocol: while some are to be defined manually on both end, like
:ref:`protocol-topic-cas40`, some use rendez-vous settings, like
:ref:`protocol-topic-seven`, and may allow these settings to be renegotiated
during the communication.

.. note::

    Some calculators advertised as USB-compatible, such as the Graph 100+,
    did not actually have a USB port. However, the box in which the calculator
    was sold also included an SB-88 USB-serial cable, which allowed PCs
    to communicate with the calculator's serial port through one of its
    USB ports. Therefore, the logics used in this case are actually those of
    the serial transport.

The following communication protocols can be found over serial transport:

* :ref:`protocol-topic-cas40`;
* :ref:`protocol-topic-cas50`;
* :ref:`protocol-topic-cas100`;
* :ref:`protocol-topic-cas300`;
* :ref:`protocol-topic-seven`;
* :ref:`protocol-topic-seven-ohp`.

.. _protocol-topic-transport-usb:

USB transports
--------------

The following transports can be presented when the device presents itself as
a USB device.

Note that some calculators, depending on the mode selected when a USB cable
is plugged, can choose to pick a different transport. See
:ref:`protocol-topic-usb-detection` to see how this choice affects the
presented USB device descriptor.

.. _protocol-topic-transport-serial-over-usb-bulk:

Serial transport over USB bulk
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The unveiling of the Classpad 300 and fx-9860G, in 2003 and 2005 respectively,
introduced `Mini-B`_ USB ports on the calculator directly for
host-to-calculator communication, using any compatible cable.

.. note::

    In practice, not all cables seem to work with these calculators,
    as manifested in the comments of
    `Améliore ta Graph 35+ USB/E en Graph 75(+E) !`_ or
    `Fa 124 pour Graph USB`_.

    Some cables also seem to work, but not support some operations required
    for some speed hacks to work such as :ref:`Protocol 7.00 packet shifting
    <protocol-topic-seven-packet-shifting>`, as manifested in `#31`_ for
    example.

Once the link is established, and the
:ref:`protocol-topic-transport-serial-over-usb-bulk-enable-control-flow`
is executed, data is transferred using
:ref:`protocol-topic-transport-serial-over-usb-bulk-data-transfer`.

The following communication protocols can be found over serial transport over
USB bulks:

* :ref:`protocol-topic-cas300`;
* :ref:`protocol-topic-seven`;
* :ref:`protocol-topic-seven-ohp`.

.. _protocol-topic-transport-serial-over-usb-bulk-enable-control-flow:

Device enabling control flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Some older fx-9860G derivatives, such as the fx-9860G Slim running OS 1.x,
require a special USB control flow to be executed before they can send or
receive any data. This can manifest differently depending on the
application protocol:

* With :ref:`protocol-topic-seven`, the calculator will not answer the initial
  check;
* With :ref:`protocol-topic-seven-ohp`, the calculator will freeze until the
  control flow is made, since it is attempting to send screen data.

This control flow has the following properties:

* ``bmRequestType`` set to ``0x41``, to designate a vendor-specific
  interface request with no incoming data transfer;
* ``bRequest`` set to ``0x01``, as it is the command that enables
  Protocol 7.00 data transfers.
* Both ``wValue`` and ``wIndex`` set to ``0x0000``.
* No data transfer.

Using libusb_, this can be done using the following excerpt:

.. code-block:: c

    libusb_control_transfer(
        device_handle,
        0x41,  /* bmRequestType */
        0x01,  /* bRequest */
        0x0000,  /* wValue */
        0x0000,  /* wIndex */
        NULL,
        0,
        300
    );

Ideally, this flow is run by a driver that can be used as soon as the
calculator is connected to the host. Otherwise, it means that the calculator
may freeze until a transfer utility is used, such as one of Cahute's
command-line utilities.

.. _protocol-topic-transport-serial-over-usb-bulk-data-transfer:

Data transfer
^^^^^^^^^^^^^

In order to transfer data from the host to the device, or from the device to
the host, USB bulk transfers are used.

.. warning::

    While USB bulk endpoints are frequently hardcoded as ``0x82`` for Bulk IN
    and ``0x01`` for Bulk OUT in community-made communication tools for CASIO
    calculators, these have been proven to change on certain platforms,
    including MacOS / OS X; see `#3`_ for more information.

    It is therefore recommended to detect the Bulk IN and OUT endpoints
    automatically before opening the device. When using libusb_ to detect
    calculators, Cahute does this with a more advanced version of the
    following code:

    .. code-block:: c

        struct libusb_config_descriptor *config_descriptor;
        struct libusb_interface_descriptor const *interface_descriptor;
        struct libusb_endpoint_descriptor const *endpoint_descriptor;
        int j, bulk_in = -1, bulk_out = -1;

        libusb_get_active_config_descriptor(calc_device, &config_descriptor);
        interface_descriptor = config_descriptor->interface[0].altsetting;

        for (j = 0; j < interface_descriptor->bNumEndpoints; j++) {
            endpoint_descriptor = interface_descriptor->endpoint[j];
            if ((endpoint_descriptor->bmAttributes & 3) != LIBUSB_TRANSFER_TYPE_BULK)
                continue;

            switch (endpoint_descriptor->bEndpointAddress & 128) {
            case LIBUSB_ENDPOINT_OUT:
                bulk_out = endpoint_descriptor->bEndpointAddress;
                break;

            case LIBUSB_ENDPOINT_IN:
                bulk_in = endpoint_descriptor->bEndpointAddress;
                break;
            }
        }

.. _protocol-topic-transport-ums:

USB Mass Storage (UMS) transport
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Starting from the fx-CG20 (Prizm) and fx-CP400+E, which came out in
2011 and 2016 respectively, calculators started supporting a "USB Key" mode,
that present both their main and storage memory over `USB Mass Storage`_ (UMS)
with `Bulk-Only Transport`_. This had the advantage of not requiring any
specific driver or software to interact with such filesystems.

However, with the introduction of this mode, calculators now used UMS for all
use cases, including screenstreaming which still uses
:ref:`protocol-topic-seven-ohp`.

The following communication protocols can be found over UMS:

* :ref:`protocol-topic-seven-ohp`;
* :ref:`protocol-topic-ums`.

.. _protocol-topic-ums-custom-commands:

Custom SCSI commands
^^^^^^^^^^^^^^^^^^^^

.. todo::

    Add a section using the knowledge from this section to actually describe
    how to send and receive using these commands, what delay to apply, and
    so on.

CASIO makes use of the ``C0h`` to ``FFh`` SCSI vendor-specific command range to
implement its own SCSI commands.

``0xC0``
    This command is a 16-byte command that is run to poll the device's status.
    The command format is the following:

    .. list-table::

        * - Offset
          - Size
          - Description
          - Value
        * - 0 (0x00)
          - 1 B
          - Command code.
          - ``0xC0``
        * - 1 (0x01)
          - 15 B
          - Reserved.
          - Must be set to ``0x00``.

    The command prompts the device to answer with a 16-byte device status,
    with the following format:

    .. list-table::
        :header-rows: 1

        * - Offset
          - Size
          - Description
          - Value
        * - 0 (0x00)
          - 1 B
          - Unknown.
          - Set to ``0xD0``.
        * - 1 (0x01)
          - 5 B
          - Reserved.
          - Set to ``0x00``.
        * - 6 (0x06)
          - 2 B
          - Amount of available bytes to be requested.
          - Big endian 16-bit integer.
        * - 8 (0x08)
          - 2 B
          - Reserved.
          - Set to ``0x00``.
        * - 10 (0x0A)
          - 2 B
          - Activity status.
          - Big endian 16-bit integer.
        * - 12 (0x0C)
          - 4 B
          - Reserved.
          - Set to ``0x00``.

``0xC1``
    This command is a 16-byte long command that is run to read available data.
    The command format is the following:

    .. list-table::

        * - Offset
          - Size
          - Description
          - Value
        * - 0 (0x00)
          - 1 B
          - Command code.
          - ``0xC1``
        * - 1 (0x01)
          - 5 B
          - Reserved.
          - Set to ``0x00``.
        * - 6 (0x06)
          - 2 B
          - Requested bytes count.
          - Big endian 16-bit integer.
        * - 8 (0x08)
          - 8 B
          - Reserved.
          - Set to ``0x00``.

    The command prompts the device to answer with the requested data.

``0xC2``
    This command is a 16-byte long command that is run to write data to the
    calculator. The command format is the following:

    .. list-table::

        * - Offset
          - Size
          - Description
          - Value
        * - 0 (0x00)
          - 1 B
          - Command code.
          - ``0xC2``
        * - 1 (0x01)
          - 5 B
          - Reserved.
          - Set to ``0x00``.
        * - 6 (0x06)
          - 2 B
          - Bytes count.
          - Big endian 16-bit integer.
        * - 8 (0x08)
          - 8 B
          - Reserved.
          - Set to ``0x00``.

    The command should be accompanied with the data to send.

.. |DEVPKEY_Device_Driver| replace:: ``DEVPKEY_Device_Driver``
.. _Mini-B: https://fr.wikipedia.org/wiki/USB#Mini-B
.. _libusb: https://libusb.info/
.. _DEVPKEY_Device_Driver:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/
    devpkey-device-driver
.. _USB Mass Storage:
    https://en.wikipedia.org/wiki/USB_mass_storage_device_class
.. _Bulk-Only Transport:
    https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
.. _`Améliore ta Graph 35+ USB/E en Graph 75(+E) !`:
    https://www.planet-casio.com/Fr/forums/
    topic13930-1-ameliore-ta-graph-35-usbe-en-graph-75e.html
.. _`Fa 124 pour Graph USB`:
    https://www.planet-casio.com/Fr/logiciels/
    voir_un_logiciel_casio.php?showid=16
.. _`#3`: https://gitlab.com/cahute/cahute/-/issues/3
.. _`#31`: https://gitlab.com/cahute/cahute/-/issues/31
