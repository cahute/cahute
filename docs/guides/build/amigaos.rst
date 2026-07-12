.. _build-guide-amigaos:

|amigaos| Building Cahute for AmigaOS
=====================================

.. warning::

    In order to install Cahute on AmigaOS, it is recommended to use one of
    the methods in :ref:`install-guide-amigaos`. However, if you wish to build
    Cahute manually, this guide is for you.

The following building methods are available.

.. note::

    Since you will not be using a packaged version of Cahute, the project won't
    be automatically updated when updating the rest of the system, which
    means you will need to do it manually, especially if a security update is
    made.

    You can subscribe to releases by creating a Gitlab.com account, and
    following the steps in `Get notified when a release is created`_.
    You can check your notification settings at any time in Notifications_.

.. _build-guide-amigaos-linux:

Building Cahute for AmigaOS 3.2 and above, using Linux
------------------------------------------------------

.. warning::

    Both AmigaOS 3.2 and above as a target and this build method are not
    officially supported yet.

    See :ref:`feature-topic-system-amigaos` for more information.

Building Cahute for AmigaOS 3.2 and above from Linux distributions
using `m68k-amigaos-gcc`_, including the Native Development Kit (NDK),
as described in `Cross Compiling With CMake`_.

This guide will assume you have a POSIX or compatible shell, such as
``bash`` or ``zsh``.

Downloading the Cahute source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: _download_source.rst

Installing the dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cahute depends on the following build-only dependencies:

* cmake_ >= 3.21;
* Python_ >= 3.11;
* `GNU Make`_, `pkg-config`_, and other C compilation and linking utilities.

.. note::

    As opposed to other platforms, Cahute for AmigaOS does not require libusb
    nor SDL3.

In order to install the build-only dependencies, it is recommended you use your
native package manager. A few examples are the following:

* On Debian and derivatives:

  .. code-block:: bash

      sudo apt-get update
      sudo apt-get install cmake python3

* On Archlinux and derivatives:

  .. code-block:: bash

      sudo pacman -Sy cmake python

* On Voidlinux and derivatives:

  .. code-block:: bash

      xbps-install cmake python3

Then, you need to install ``m68k-amigaos-gcc`` and all available tools,
using instructions present in the `m68k-amigaos-gcc README`_.

.. warning::

    Unfortunately this may not come easy, and there is no issue tracker
    with the project. Good luck!

This build method assumes the toolchain is installed in ``/opt/amiga``, as set
by default. You must also download `m68k-amigaos.cmake`_ on your system, and
prepare the absolute path to it.

Building the project
~~~~~~~~~~~~~~~~~~~~

You can now create the ``build`` directory aside the source directory,
by running the following command:

.. parsed-literal::

    cmake -B build -S cahute-|version| \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/m68k-amigaos.cmake \
    -DTOOLCHAIN_PREFIX=m68k-amigaos \
    -DTOOLCHAIN_PATH=/opt/amiga

You can now build the project using the following command::

    cmake --build build

.. warning::

    ``p7screen`` will not be included, since it requires SDL3 which is not
    available with the AmigaOS 3.2 toolchain.

.. |amigaos| image:: ../install/amigaos.png

.. _Get notified when a release is created:
    https://docs.gitlab.com/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications

.. _cmake: https://cmake.org/
.. _Python: https://www.python.org/
.. _GNU Make: https://www.gnu.org/software/make/
.. _pkg-config: https://git.sr.ht/~kaniini/pkgconf

.. _Cross Compiling With CMake:
    https://cmake.org/cmake/help/book/mastering-cmake/chapter/
    Cross%20Compiling%20With%20CMake.html?highlight=mingw

.. _m68k-amigaos-gcc: https://github.com/AmigaPorts/m68k-amigaos-gcc
.. _m68k-amigaos-gcc README:
    https://github.com/AmigaPorts/m68k-amigaos-gcc/blob/master/README.md
.. _m68k-amigaos.cmake:
    https://github.com/AmigaPorts/AmigaCMakeCrossToolchains/blob/master/
    m68k-amigaos.cmake
