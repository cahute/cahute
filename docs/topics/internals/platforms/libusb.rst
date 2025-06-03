.. _internals-topic-libusb:

libusb specific implementation details
======================================

This document details implementation details specific to libusb, used on
all platforms with USB interactions.

libusb context management
-------------------------

All system resources with libusb are represented using a |libusb_context|_
instance, opened using |libusb_init|_. Cahute stores this as a context
pointer; see :ref:`internals-topic-context-pointers` for more information.

.. note::

    libusb can provide logs through a callback, using |libusb_set_log_cb|_.
    However, instead of taking a cookie, this callback takes the libusb
    context, so matching it to a Cahute context would require a global
    mapping.

    See `#80 <https://gitlab.com/cahute/cahute/-/issues/80>`_
    for more information.

USB device detection using libusb
---------------------------------

Cahute can use libusb to detect USB devices, using
``cahute_libusb_detect_usb()``.

This function gets the device list using |libusb_get_device_list|_, then
browses it. If an entry matches one of the entries in
:ref:`protocol-topic-usb-detection`, then it is yielded.

.. note::

    libusb requires opening the device with |libusb_open|_ in order to get
    a string descriptor using control flows. Thus, we cannot use
    ``iManufacturer`` to distinguish between models.

USB links
---------

libusb is used by Cahute to interact with calculators connected as devices
to the current machine, assumed to be a host (such as a PC or smartphone with
OTG).

USB link opening using libusb
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cahute can use libusb to open a device with a given bus number and address,
using ``cahute_open_libusb_link()``.

This function gets the device list using |libusb_get_device_list|_, and finds
one matching the provided bus and address numbers using
|libusb_get_bus_number|_ and |libusb_get_device_address|_ on every entry.

If a matching device is found, the configuration is obtained using
|libusb_get_device_descriptor|_ and |libusb_get_active_config_descriptor|_,
in order to:

* Get the vendor (VID) and product (PID) identifiers, to ensure they match
  one of the known combinations for CASIO calculators.
* Get the interface class (``bInterfaceClass``) to determine the protocol
  and transport.
* In both cases, ensure that the bulk IN and OUT endpoints exist, and
  get their endpoint identifiers.

.. note::

    While historical implementations of CASIO's protocols using libusb
    hardcode 0x82 as Bulk IN and 0x01 as Bulk OUT, this has proven to
    change on other platforms such as OS X; see `#3 (comment 1823215641)
    <https://gitlab.com/cahute/cahute/-/issues/3#note_1823215641>`_
    for more context.

The interface class to transport mapping is the following:

.. list-table::
    :header-rows: 1
    :width: 100%

    * - (in) Intf. class
      - (in) Intf. subclass
      - (in) Intf. protocol
      - (out) Link type
    * - 8
      - 6
      - 80
      - UMS
    * - 255
      - 0
      - 255
      - Serial over bulk

See :ref:`protocol-topic-usb-detection` for more information.

Once all metadata has been gathered, the function opens the device using
|libusb_open|_, and attempt to claim its interface using
|libusb_claim_interface|_ and |libusb_detach_kernel_driver|_.

.. note::

    Access errors, i.e. any of these two functions returning
    ``LIBUSB_ERROR_ACCESS``, are ignored, since libusb is still
    able to communicate with the device on some platforms afterwards.

    See `#3 <https://gitlab.com/cahute/cahute/-/issues/3>`_
    for more context.

If the device opening yields ``LIBUSB_ERROR_NOT_SUPPORTED``,
it means that the device is running a driver that is not supported by
libusb. Then, if the platform supports it, we get the port number
using |libusb_get_port_number|_, and try to open the device through the
system-specific interface.

Serial over bulk USB links using libusb
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to handle serial over bulk USB links using libusb, we have a
|libusb_device_handle|_. The available operations are the following:

* Closing uses |libusb_close|_ on the device handle, and |libusb_exit|_
  on the libusb context;
* Receiving and sending uses |libusb_bulk_transfer|_.

See :ref:`protocol-topic-transport-serial-over-usb-bulk` for more information.

UMS links using libusb
~~~~~~~~~~~~~~~~~~~~~~

In order to handle UMS links using libusb, we have a
|libusb_device_handle|_. The available operations are the following:

* Closing uses |libusb_close|_ on the device handle, and |libusb_exit|_
  on the libusb context;
* Requesting using SCSI uses |libusb_bulk_transfer|_ with manual reading
  and writing of the Command Block Wrapper (CBW) and
  Command Status Wrapper (CSW).

See :ref:`protocol-topic-transport-ums` for more information.

.. |libusb_context| replace:: ``libusb_context``
.. |libusb_set_log_cb| replace:: ``libusb_set_log_cb``
.. |libusb_init| replace:: ``libusb_init``
.. |libusb_exit| replace:: ``libusb_exit``
.. |libusb_device_handle| replace:: ``libusb_device_handle``
.. |libusb_get_device_list| replace:: ``libusb_get_device_list``
.. |libusb_get_bus_number| replace:: ``libusb_get_bus_number``
.. |libusb_get_device_address| replace:: ``libusb_get_device_address``
.. |libusb_get_device_descriptor| replace:: ``libusb_get_device_descriptor``
.. |libusb_get_port_number| replace:: ``libusb_get_port_number``
.. |libusb_get_active_config_descriptor|
   replace:: ``libusb_get_active_config_descriptor``
.. |libusb_detach_kernel_driver| replace:: ``libusb_detach_kernel_driver``
.. |libusb_claim_interface| replace:: ``libusb_claim_interface``
.. |libusb_open| replace:: ``libusb_open``
.. |libusb_close| replace:: ``libusb_close``
.. |libusb_bulk_transfer| replace:: ``libusb_bulk_transfer``

.. _libusb_context:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #ga4ec088aa7b79c4a9599e39bf36a72833
.. _libusb_set_log_cb:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #ga2efb66b8f16ffb0851f3907794c06e20
.. _libusb_init:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #ga7deaef521cfb1a5b3f8d6c01be11a795
.. _libusb_exit:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #gadc174de608932caeb2fc15d94fa0844d
.. _libusb_device_handle:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga7df95821d20d27b5597f1d783749d6a4
.. _libusb_get_device_list:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gac0fe4b65914c5ed036e6cbec61cb0b97
.. _libusb_get_bus_number:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gaf2718609d50c8ded2704e4051b3d2925
.. _libusb_get_device_address:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gab6d4e39ac483ebaeb108f2954715305d
.. _libusb_get_port_number:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga14879a0ea7daccdcddb68852d86c00c4
.. _libusb_get_device_descriptor:
    https://libusb.sourceforge.io/api-1.0/group__libusb__desc.html
    #ga5e9ab08d490a7704cf3a9b0439f16f00
.. _libusb_get_active_config_descriptor:
    https://libusb.sourceforge.io/api-1.0/group__libusb__desc.html
    #ga425885149172b53b3975a07629c8dab3
.. _libusb_detach_kernel_driver:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga5e0cc1d666097e915748593effdc634a
.. _libusb_claim_interface:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gaee5076addf5de77c7962138397fd5b1a
.. _libusb_open:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga3f184a8be4488a767b2e0ae07e76d1b0
.. _libusb_close:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga779bc4f1316bdb0ac383bddbd538620e
.. _libusb_bulk_transfer:
    https://libusb.sourceforge.io/api-1.0/group__libusb__syncio.html
    #ga2f90957ccc1285475ae96ad2ceb1f58c
