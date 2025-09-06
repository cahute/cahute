.. _feature-topic-links:

Links
=====

All communication implementations are centered around resources called links.
Internally, links are mostly constituted of:

* A context; see :ref:`feature-topic-contexts` for more information;
* A platform-specific transport implementation;
* A protocol, which is a set of resources and functions that do not constitute
  an interface, since protocols may obey different logics.

Links are opened using one of the following functions:

* :c:func:`cahute_open_simple_usb_link`;
* :c:func:`cahute_open_usb_link`;
* :c:func:`cahute_open_serial_link`.

.. note::

    The internal behaviour of these methods is documented in
    :ref:`internals-topic-link-open`.

.. _feature-topic-links-usb-device-names:

USB device names
----------------

On detection or opening a link to a USB device, the name or path depends on the
platform:

* On platforms using libusb, including :ref:`feature-topic-system-linux` and
  :ref:`feature-topic-system-macos`, it represents the bus and address
  numbers in decimal separated by ``:``, e.g. ``001:014``.
* On :ref:`feature-topic-system-win32`, it represents the device path,
  such as ``USB\VID_07CF&PID_6101\6&1ff8b909&0&9`` or
  ``USB\VID_07CF&PID_6102\0000uJPLBWa1``.

.. _feature-topic-links-generic:

Generic links
-------------

While Cahute aims at implementing official and widespread community protocols,
the portability of its transports can be appealing to developers implementing
a custom, possibly program-specific protocol between a host and a CASIO
calculator, notably for the host part.

Since Cahute does not expose transports publicly, it allows selecting a
special kind of links called "generic links", which allows the following
functions to be used on such links:

* :c:func:`cahute_receive_on_link`;
* :c:func:`cahute_send_on_link`;
* :c:func:`cahute_set_serial_params_to_link`.

The following methods can be used to open a generic link with Cahute:

* Call :c:func:`cahute_open_serial_link` with the
  :c:macro:`CAHUTE_SERIAL_PROTOCOL_NONE` protocol, for opening a generic
  link over a serial transport;
* Call :c:func:`cahute_open_simple_usb_link` or :c:func:`cahute_open_usb_link`
  with the :c:macro:`CAHUTE_USB_NOPROTO` flag, for opening a generic link
  to a USB device.

The following developer guides demonstrate how to open and use generic links
using the aforementioned methods:

* :ref:`developer-guide-use-generic-serial-link`.
