.. _protocol-topic-usb-detection:

USB detection for CASIO calculators
===================================

When looking for CASIO calculators, the following USB metadata can be
found in the wild:

.. list-table::
    :header-rows: 1

    * - ``idVendor``
      - ``idProduct``
      - ``iManufacturer``
      - ``bInterfaceClass``
      - ``bInterfaceSubclass``
      - ``bInterfaceProtocol``
      - Models
    * - ``07cf``
      - ``6101``
      - ``CASIO COMPUTER CO., LTD.``
      - ``255`` (Vendor-Specific)
      - ``0``
      - ``255``
      - Classpad 300 / 330 (+), fx-9860G Slim
    * - ``07cf``
      - ``6101``
      - ``CESG502``
      - ``255`` (Vendor-Specific)
      - ``0``
      - ``255``
      - Graph 35+/75/85/95
    * - ``07cf``
      - ``6102``
      - ``CASIO MassStorage Device``
      - ``8`` (Mass Storage)
      - ``6`` (SCSI)
      - ``80`` (Bulk-Only)
      - Classpad 330+, fx-CG20, fx-CP400, fx-CP400+E
    * - ``07cf``
      - ``6103``
      - ``CASIO MassStorage Device``
      - ``8`` (Mass Storage)
      - ``6`` (SCSI)
      - ``80`` (Bulk-Only)
      - fx-CG50, Graph 90+E, fx-9750GIII (>= OS 3.8)

.. warning::

    The actual string referenced by ``iManufacturer``, which is only an
    integer, can only be accessed after opening the device on some platforms.
    Therefore, it cannot be used for communication protocol detection in
    a portable fashion.

In order to be compatible with all calculators, Cahute looks for recognized
VID/PID pairs (VID ``07cf`` with PID ``6101``, ``6102`` or ``6103``), then
only assumes the transport based on the interface details:

.. list-table::
    :header-rows: 1

    * - ``bInterfaceClass``
      - ``bInterfaceSubclass``
      - ``bInterfaceProtocol``
      - Transport protocol
    * - ``8`` (Mass Storage)
      - ``6`` (SCSI)
      - ``80`` (Bulk-Only)
      - :ref:`protocol-topic-transport-ums`
    * - ``255`` (Vendor-Specific)
      - ``0``
      - ``255``
      - :ref:`protocol-topic-transport-serial-over-usb-bulk`

.. warning::

    While more modern devices speaking Protocol 7.00 / Protocol 7.00
    Screenstreaming over bulk transfers use a specific ``iManufacturer``
    and a greater ``bcdUSB``, ancient Protocol 7.00 / Protocol 7.00
    Screenstreaming devices, such as the fx-9860G Slim, are indistinguishable
    from Classpad 300 / 330 (+) devices on USB alone.

    Because of this, in the :c:func:`cahute_open_usb_link` and
    :c:func:`cahute_open_simple_usb_link` interfaces, the protocol can either
    be forced by the caller using the :c:macro:`CAHUTE_USB_CAS300` or
    :c:macro:`CAHUTE_USB_SEVEN` flag, or determined once the link is made
    using automatic protocol detection.

.. warning::

    Some older fx-9860G derivatives using OS 1.x require a specific USB control
    transfer to be run before Protocol 7.00 can be used; see
    :ref:`protocol-topic-seven-init-link` for more information.

.. note::

    For reference, the following USB serial cables have also be encountered
    in the wild:

    .. list-table::
        :header-rows: 1

        * - ``idVendor``
          - ``idProduct``
          - Description
        * - ``0711``
          - ``0230``
          - SB-88 serial cable (official CASIO cable).
        * - ``0bda``
          - ``5606``
          - Util-Pocket (defunct alternative vendor) serial cable.
            Uses an USB serial converter from FTDI_.

    Note however that these should be used through the system's serial
    bus interface rather than directly.

.. _FTDI: https://ftdichip.com/
