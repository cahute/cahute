.. _internals-topic-posix:

POSIX specific implementation details
=====================================

This document details implementation details specific to fully POSIX compliant
platforms, including :ref:`feature-topic-system-linux` and
:ref:`feature-topic-system-macos`.

.. note::

    POSIX compliant platforms are identified using the presence of the
    ``__unix`` macro, except Apple's OS X platform, which is identified
    separately since it doesn't define the aforementioned macro.

Serial device management
------------------------

Serial device management is mostly standardized on POSIX compliant platforms,
except for serial device paths and advanced operations such as DTR/RTS
management.

Serial device detection on POSIX compliant platforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As described in `SUSv4TC2 section 10`_, devices should be present in
``/dev``. Therefore, we attempt at finding devices with the following
patterns in the filesystem, expressed using regular expressions:

``/dev/ttyUSB[0-9]+``
    USB serial devices on :ref:`feature-topic-system-linux`;
    see `USB serial on Linux`_ for more information.

``/dev/cua[dn][0-9]+``
    Call-out ports on FreeBSD; see
    `Serial communications on FreeBSD`_ for more information.

``/dev/cu\..+``
    Call-out ports on :ref:`feature-topic-system-macos`.

``/dev/dty[0-9]+``
    Call-out ports on NetBSD; see `tty(4) on NetBSD`_ for more information.

Serial link handling on POSIX compliant platforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Opening a serial link on POSIX compliant platforms is accomplished by
``cahute_open_posix_serial_link()``, interpreting the given name or path as
a file path, using |open|_.

This will obtain us a file descriptor (*fd*), which can then use for the
following operations:

* Closing using |close|_;
* Receiving uses |select|_ and |read|_;
* Sending uses |write|_;
* Serial params setting uses |termios|_, including |tcdrain|_, and
  |tty_ioctl|_ if available, especially ``TIOCMGET`` and ``TIOCMSET``.

File implementation
-------------------

When creating or opening a file on POSIX compliant systems,
``cahute_create_posix_file()`` and ``cahute_open_posix_file()``
use |open|_. Then, depending on the situation:

* On creation, we want to set the file size to the provided one.

  In order to do this, we call |ftruncate|_.
* On reading, we want to get the current file size.

  In order to do this, we call |lseek|_ to seek 0 bytes from ``SEEK_END``,
  which returns the current file size, then use the same function to seek
  0 bytes from ``SEEK_SET``.

In order to open the standard output, we just use file descriptor ``1``.

Once a file or stdout is opened, we have a file descriptor we can then use
for the following operations:

* Closing using |close|_ (except for the standard output, which we must
  not close);
* Reading uses |read|_;
* Writing uses |write|_;
* Seeking uses |lseek|_.

.. |close| replace:: ``close(2)``
.. |ftruncate| replace:: ``ftruncate(2)``
.. |lseek| replace:: ``lseek(2)``
.. |open| replace:: ``open(2)``
.. |read| replace:: ``read(2)``
.. |select| replace:: ``select(2)``
.. |tcdrain| replace:: ``tcdrain(3)``
.. |termios| replace:: ``termios(3)``
.. |tty_ioctl| replace:: ``tty_ioctl(4)``
.. |write| replace:: ``write(2)``

.. _close: https://linux.die.net/man/2/close
.. _ftruncate: https://linux.die.net/man/2/ftruncate
.. _lseek: https://linux.die.net/man/2/lseek
.. _open: https://linux.die.net/man/2/open
.. _read: https://linux.die.net/man/2/read
.. _select: https://linux.die.net/man/2/select
.. _tcdrain: https://linux.die.net/man/3/tcdrain
.. _termios: https://linux.die.net/man/3/termios
.. _tty_ioctl: https://linux.die.net/man/4/tty_ioctl
.. _write: https://linux.die.net/man/2/write

.. _SUSv4TC2 section 10:
    https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap10.html
.. _USB serial on Linux:
    https://docs.kernel.org/usb/usb-serial.html
.. _Serial communications on FreeBSD:
    https://docs.freebsd.org/en/books/handbook/serialcomms/
.. _tty(4) on NetBSD:
    https://man.bsd.lv/NetBSD-9.2/tty.4
