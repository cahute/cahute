System compatibility
====================

Cahute is compatible with multiple host systems and system libraries.
The following sections represents these.

.. warning::

    Some sections are present in this document for reference, and the presence
    of a section in this document does not mean the system is currently
    supported; it does mean however that support for the platform is
    considered.

.. note::

    We consider that Cahute officially supports a platform (system with
    specific components, such as the C library, and so on) when:

    1. Cahute can be built for the platform from Linux, using
       freely-distributed software;
    2. Cahute can run and access at least one system resource relevant to
       its function (at least one serial port, some USB devices, or the
       filesystem);
    3. Cahute build for the platform is integrated within the project's CI;
    4. Cahute can be installed without being manually built on or for the
       platform, using a package manager or an installer distribution.

    Also note that requirement 1 does not imply that all build methods have
    to use freely-distributed software, we only need one to be.

.. _feature-topic-system-linux:

|linux| Linux distributions
---------------------------

Linux_ is only a kernel, which has spawned multiple distributions around it
considered systems on which Cahute can run. Their support is one of the main
reasons why alternative tooling to CASIO's own exist in the first place, which
is only Windows-compatible.

Officially supported targets for Linux are the following:

.. list-table::
    :header-rows: 1

    * - Processor architecture
      - C library
      - Target
    * - x86_
      - `GNU C library`_
      - ``i686-pc-linux-gnu``
    * - x64_
      - `GNU C library`_
      - ``x86_64-pc-linux-gnu``
    * - x86_
      - `musl libc`_
      - ``i686-pc-linux-musl``
    * - x64_
      - `musl libc`_
      - ``x86_64-pc-linux-musl``

In addition to the binary compatibility, details such as the filesystem
hierarchy must be examined. By default, Cahute assumes the
`Filesystem Hierarchy Standard`_ (FHS), which for example defines that devices
are present in ``/dev``.

Build instructions for Linux are distribution-agnostic, and the built project
is selected based on every distribution's configuration.
See :ref:`build-guide-linux` for more information.

.. _feature-topic-system-arch:

|archlinux| Archlinux and derivatives
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Archlinux_ is a Linux distribution based on the Pacman_ package manager.
Many distributions are based on it, with one of the more well-known ones being
Manjaro_. It uses the `GNU C library`_.

Cahute does not provide a package repository for Archlinux_ and derivatives;
it however is available on the `Archlinux User Repository`_ as the
`cahute <cahute on AUR_>`_ package; see :ref:`install-guide-linux-aur` for
instructions on how to install it.

.. _feature-topic-system-debian:

|debian| Debian and derivatives
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. warning::

    Debian is not yet supported as an official target by Cahute.
    See `#8 <https://gitlab.com/cahuteproject/cahute/-/issues/8>`_
    for more information.

Debian_ is a Linux distribution based on APT_ (*Advanced Package Tool*).
Many distributions are based on it, with some of the more well-known ones
being Ubuntu_ and `Linux Mint`_. It uses the `GNU C library`_.

Cahute does not yet provide a package repository for Debian and derivatives,
nor endorses any external package repository; one must build Cahute to use
it on such systems for now.

.. _feature-topic-system-void:

|void| Void Linux
~~~~~~~~~~~~~~~~~

.. warning::

    Void Linux is not yet supported as an official target by Cahute.
    See `#72 <https://gitlab.com/cahuteproject/cahute/-/issues/72>`_
    for more information.

`Void Linux`_ is a Linux distribution based on XBPS_ (*X Binary Package
System*). It uses the `musl libc`_.

Cahute does not yet provide a package repository for Void Linux, nor
endorses any external package repository; one must build Cahute to use
it on such systems for now.

.. _feature-topic-system-macos:

|apple| macOS, OS X
-------------------

`macOS / OS X`_ is, in this context, an alias for Darwin_, a system based on
XNU_ developed by Apple_ for its Mac_ computers, among others.
It is derived from BSD_, among other systems.

Support for this platform is mostly common with other UNIX-like platforms
such as Linux_, and most of the platform-specific code is handled by libusb_.
The following is implemented in Cahute directly:

* macOS does not allow detaching the kernel driver for USB devices, unless
  it is `code signed <Apple code signing_>`_. Cahute ignores access-related
  errors on kernel driver detaching for this reason;
* Like for other BSD_ systems, serial devices are present in ``/dev`` as
  ``cu.*`` and ``cuad.*`` devices, instead of ``ttyUSB*`` for Linux.

Installation on macOS / OS X of Cahute is done via Homebrew_, which requires
macOS Ventura (13) or higher; see :ref:`install-guide-macos` and `Homebrew macOS
requirements`_ for more information.

For now, Cahute is only built natively for this platform; see
:ref:`build-guide-macos` for more information.

.. _feature-topic-system-freebsd:

|freebsd| FreeBSD
-----------------

.. warning::

    MS-DOS is not yet supported as an official target by Cahute.

FreeBSD_ is a BSD-derived system.

.. _feature-topic-system-netbsd:

|netbsd| NetBSD
---------------

.. warning::

    MS-DOS is not yet supported as an official target by Cahute.

NetBSD_ is a BSD-derived system.

.. _feature-topic-system-windows:

|win| Microsoft Windows
-----------------------

.. warning::

    Windows is not yet supported as an official target by Cahute.
    See `#10 <https://gitlab.com/cahuteproject/cahute/-/issues/10>`_
    for more information.

    NT 5.0 (Windows 2000) compatibility is also being discussed in
    `#73 <https://gitlab.com/cahuteproject/cahute/-/issues/73>`_.

.. warning::

    `Microsoft Windows`_ in this context refers to `Windows NT`_ based
    systems by Microsoft_. For other systems by Microsoft_ bearing the name
    "Windows", see :ref:`feature-topic-system-msdos` and
    :ref:`feature-topic-system-win9x`.

`Windows NT`_ is a family of systems made by Microsoft_.
In this context, we only consider systems including the Win32 subsystem, which
was introduced in NT 3.1. Currently, only NT 5.1 (Windows XP) and above
are supported by Cahute.

Cahute only supports x86_ (i686+) and x64_ for Windows.
It aims at supporting both available runtimes for Windows:

* Microsoft Visual C++ Runtime (MSVCRT), available by default on NT 3.1+;
* Universal C Runtime (UCRT), only available by default on NT 10.0
  (Windows 10), and through updates on NT 6.0 (Windows Vista) and above;
  see `Universal CRT deployment`_ for more information.

See `UCRT vs. MSVCRT`_ for more information.

One can make use of `MinGW-w64`_ in order to build from Windows itself without
Microsoft's C libraries exclusively distributed with `Visual Studio`_, or
from other platforms such as Linux. See :ref:`build-guide-windows` for more
details.

Microsoft Windows drivers for CASIO calculators over USB
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As opposed to other platforms, Cahute cannot access USB devices on calculators,
but must make use of a driver specifically installed for the calculator.
Possible drivers are the following:

* CASIO's ``CESG502`` driver, which matches devices using the ``07cf:6101``
  VID/PID pair for devices implementing
  :ref:`protocol-topic-transport-serial-over-usb-bulk` in order to provide a
  stream-like interface.

  It is compatible with NT 5.0 (Windows 2000) onwards.
  32-bit (x86_) drivers can be found `here <32-bit CESG502 driver_>`_, and
  64-bit (x64_) drivers are installed with `FA-124`_;
* WinUSB_, a generic USB driver made by Microsoft and compatible
  with NT 6.0 (Windows Vista) onwards. Cahute uses this through libusb_;
* `libusbK.sys`_, a generic USB driver provided by libusbK_, compatible
  with NT 5.1 (Windows XP) onwards. Cahute uses this through libusb_;
* `libusb0.sys`_, a generic USB driver provided by `libusb-win32`_, compatible
  with NT 5.0 (Windows 2000) onwards. Cahute uses this through libusb_.

This compatibility is limited by the fact that Cahute uses libusb_, which,
while originally compatible NT 5.1 (Windows XP) onwards, has dropped
compatibility with Windows XP in version 1.0.24; see `libusb supported
environments`_ for more information.

.. _feature-topic-system-msdos:

|msdos| MS-DOS
--------------

.. warning::

    MS-DOS is not yet supported as an official target by Cahute.

`MS-DOS`_ is a system made by Microsoft_ in 1981, on which `Windows 3.1`_ runs.

.. _feature-topic-system-win9x:

|win95| Windows 9x
------------------

.. warning::

    Windows 9x is not yet supported as an official target by Cahute.

`Windows 9x`_ is a series of systems made by Microsoft_ in the 1990s, including
`Windows 95`_, `Windows 98`_ and `Windows Me`_ (*Millennium Edition*).
It is distinct from :ref:`feature-topic-system-msdos` as it uses a different
kernel that takes advance of 32-bit *protected* mode on x86_; see
`What was the role of MS-DOS in Windows 95?`_ for more information.

.. _feature-topic-system-amigaos:

|amigaos| AmigaOS
-----------------

.. warning::

    AmigaOS is not yet supported as an official target by Cahute.
    See `#26 <https://gitlab.com/cahuteproject/cahute/-/issues/26>`_
    for more information.

AmigaOS_ is a system originally made by Commodore_ for the Amiga_ family of
computers. Versions 3.2+ of the system were made by `Hyperion Entertainment`_,
which `went bankrupt in March of 2024 <Hyperion Entertainment bankrupcy_>`_.

There were some community-made `CASIO software for AmigaOS`_ in the early
2000s, including Amicas_ and ACas_ which both implemented
:ref:`CAS50 <protocol-topic-cas50>` over serial links.

Cahute can be built for AmigaOS 3.1+ using the Native Development Kit (NDK),
which can be found in the `Hyperion Entertainment Downloads`_. See
:ref:`build-guide-amigaos` for more details.

.. |linux| image:: ../../guides/install/linux.svg
.. |archlinux| image:: ../../guides/install/arch.svg
.. |debian| image:: ../../guides/install/debian.svg
.. |void| image:: ../../guides/install/voidlinux.svg
.. |win| image:: ../../guides/install/win.png
.. |win95| image:: ../../guides/install/win95.svg
.. |freebsd| image:: ../../guides/install/freebsd.png
.. |netbsd| image:: ../../guides/install/netbsd.png
.. |apple| image:: ../../guides/install/apple.svg
.. |msdos| image:: ../../guides/install/msdos.svg
.. |amigaos| image:: ../../guides/install/amigaos.png

.. _x86: https://fr.wikipedia.org/wiki/X86
.. _x64: https://fr.wikipedia.org/wiki/X64

.. _GNU C library: https://www.gnu.org/software/libc/
.. _musl libc: https://musl.libc.org/
.. _libusb: https://libusb.info/

.. _Linux: https://kernel.org/
.. _Filesystem Hierarchy Standard:
    https://refspecs.linuxfoundation.org/FHS_3.0/fhs/index.html
.. _Archlinux: https://archlinux.org/
.. _Pacman: https://wiki.archlinux.org/title/Pacman
.. _Manjaro: https://manjaro.org/
.. _Archlinux User Repository: https://aur.archlinux.org/
.. _cahute on AUR: https://aur.archlinux.org/packages/cahute

.. _Debian: https://www.debian.org/
.. _APT: https://wiki.debian.org/PackageManagement
.. _Ubuntu: https://ubuntu.com/
.. _Linux Mint: https://www.linuxmint.com/

.. _Void Linux: https://voidlinux.org/
.. _XBPS: https://docs.voidlinux.org/xbps/index.html

.. _Microsoft Windows: http://windows.microsoft.com/
.. _Microsoft: https://www.microsoft.com/
.. _Visual Studio: https://visualstudio.microsoft.com/fr/
.. _Windows NT: https://en.wikipedia.org/wiki/Windows_NT
.. _Universal CRT deployment:
    https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment
.. _UCRT vs. MSVCRT:
    https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-doc/
    howto-build/ucrt-vs-msvcrt.txt
.. _MinGW-w64: https://www.mingw-w64.org/

.. _32-bit CESG502 driver:
    https://www.planet-casio.com/Fr/logiciels/voir_un_logiciel_casio.php
    ?showid=75
.. _FA-124:
    https://www.planet-casio.com/Fr/logiciels/voir_un_logiciel_casio.php
    ?showid=16
.. _WinUSB:
    https://learn.microsoft.com/fr-fr/windows-hardware/drivers/usbcon/
    using-winusb-api-to-communicate-with-a-usb-device
.. _libusbK: https://libusbk.sourceforge.net/UsbK3/index.html
.. _`libusbK.sys`:
    https://libusbk.sourceforge.net/UsbK3/usbk_about.html#usbk_about_sys
.. _libusb-win32: https://github.com/mcuee/libusb-win32
.. _`libusb0.sys`:
    https://github.com/mcuee/libusb-win32/wiki#development
.. _libusb supported environments:
    https://github.com/libusb/libusb/wiki/Windows/
    0a6dc490c1766b8fc5a2d14e90efa8957663f0f0#supported-environments

.. _FreeBSD: https://www.freebsd.org/
.. _NetBSD: https://www.netbsd.org/

.. _`macOS / OS X`: https://www.apple.com/macos/
.. _Darwin: https://en.wikipedia.org/wiki/Darwin_(operating_system)
.. _XNU: https://en.wikipedia.org/wiki/XNU
.. _Apple: https://www.apple.com/
.. _Mac: https://www.apple.com/mac/
.. _BSD: https://en.wikipedia.org/wiki/Berkeley_Software_Distribution
.. _Apple code signing:
    https://developer.apple.com/library/archive/documentation/Security/
    Conceptual/CodeSigningGuide/Introduction/Introduction.html
.. _Homebrew: https://brew.sh/
.. _Homebrew macOS requirements:
    https://docs.brew.sh/Installation#macos-requirements

.. _MS-DOS: https://en.wikipedia.org/wiki/MS-DOS
.. _Windows 3.1: https://en.wikipedia.org/wiki/Windows_3.1

.. _Windows 9x: https://en.wikipedia.org/wiki/Windows_9x
.. _Windows 95: https://en.wikipedia.org/wiki/Windows_95
.. _Windows 98: https://en.wikipedia.org/wiki/Windows_98
.. _Windows Me: https://en.wikipedia.org/wiki/Windows_Me
.. _`What was the role of MS-DOS in Windows 95?`:
    https://devblogs.microsoft.com/oldnewthing/20071224-00/?p=24063

.. _AmigaOS: https://www.amigaos.net/
.. _Amiga: https://en.wikipedia.org/wiki/Amiga
.. _Commodore: https://en.wikipedia.org/wiki/Commodore_International
.. _Hyperion Entertainment:
    https://www.hyperion-entertainment.com/
.. _Hyperion Entertainment bankrupcy:
    https://www.amiga-news.de/en/news/AN-2024-04-00029-EN.html
.. _CASIO software for AmigaOS:
    https://aminet.net/search?query=casio
.. _Amicas: https://aminet.net/package/comm/misc/amicas
.. _ACas: https://aminet.net/package/comm/misc/ACas
.. _Hyperion Entertainment Downloads:
    https://www.hyperion-entertainment.com/index.php/downloads
