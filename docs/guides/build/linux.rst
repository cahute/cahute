.. _build-guide-linux:

|linux| Building Cahute for Linux distributions
===============================================

.. warning::

    In order to install Cahute on Linux, it is recommended to use one of the
    methods described in :ref:`install-guide-linux`. However, if Cahute is not
    available for your system, or if you wish to build it manually, this guide
    is for you.

The following building methods are available.

.. note::

    Since you will not be using a packaged version of Cahute, the project won't
    be automatically updated when updating the rest of the system, which
    means you will need to do it manually, especially if a security update is
    made.

    You can subscribe to releases by creating a Gitlab.com account, and
    following the steps in `Get notified when a release is created`_.
    You can check your notification settings at any time in Notifications_.

.. _build-guide-linux-sh:

Building Cahute natively for Linux
----------------------------------

This guide will assume you have a POSIX or compatible shell, such as
``bash`` or ``zsh``.

Downloading the Cahute source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: _download_source.rst

Installing the dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cahute depends on the following build-only dependencies:

* cmake_ >= 3.21;
* Python_ >= 3.8;
* `toml module for Python <python-toml_>`_, either installed through pip
  or as a native package such as ``python-toml`` or ``python3-toml``;
* `GNU Make`_, `pkg-config`_, and other C compilation and linking utilities.

It also depends on the following build and runtime dependencies:

* SDL_ >= 2.0 (for ``p7screen``);
* libusb_ >= 1.0.23.

In order to install the dependencies, it is recommended you use your native
package manager. A few examples are the following:

* On Debian and derivatives:

  .. code-block:: bash

      sudo apt-get update
      sudo apt-get install cmake python3 python3-toml libusb-1.0-0-dev libsdl2-dev

* On Archlinux and derivatives:

  .. code-block:: bash

      sudo pacman -Sy cmake python python-toml libusb sdl2

* On Voidlinux and derivatives:

  .. code-block:: bash

      xbps-install cmake python3 python3-toml libusb-devel sdl2-devel

Building the project
~~~~~~~~~~~~~~~~~~~~

In the parent directory to the source, you can now create the ``build``
directory aside it, and install from it, by running the following commands:

.. parsed-literal::

    cmake -B build -S cahute-|version| -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
    cmake --build build

.. note::

    While CMake uses ``-O3`` by default for the ``Release`` configuration,
    this is `considered an undesired behaviour
    <CMake Release undesired behaviour_>`_ by Cahute, hence Cahute overrides
    it with ``-O2`` by default.

    See :ref:`CMAKE_BUILD_TYPE <cmake-ref-setting-cmake-build-type>` for more
    information.

Installing the project
~~~~~~~~~~~~~~~~~~~~~~

If you want to install Cahute from the built version on your system directly,
you can use the following command while in the build directory:

.. code-block:: text

    sudo cmake --install build --strip

If, however, you want to install the result into a given directory,
you can use the following command:

.. code-block:: text

    DESTDIR=./dist cmake --install build --strip

.. warning::

    For communicating with calculators over USB and serial, Cahute library
    and command-line utilities require access to such devices.

    For serial devices, this is traditionally represented by being a member
    of the ``uucp`` group, defined as the group owner on ``/dev/ttyS*``
    devices; you can check this by running ``ls -l /dev/ttyS*``.
    However, by default, USB devices don't have such rules.

    CMake automatically installs the udev rules, which means you need to
    do the following:

    * Reload the udev daemon reload to apply the newly installed rules
      on the running system without a reboot, with this command **as root**::

          udevadm control --reload

    * Adding your user to the ``uucp`` group, then restarting your session::

          usermod -a -G uucp <your-username>

.. |linux| image:: ../install/linux.svg

.. _Get notified when a release is created:
    https://docs.gitlab.com/ee/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications

.. _cmake: https://cmake.org/
.. _Python: https://www.python.org/
.. _python-toml: https://pypi.org/project/toml/
.. _GNU Make: https://www.gnu.org/software/make/
.. _pkg-config: https://git.sr.ht/~kaniini/pkgconf
.. _SDL: https://www.libsdl.org/
.. _libusb: https://libusb.info/

.. _CMake Release undesired behaviour:
    https://wiki.archlinux.org/title/CMake_package_guidelines
    #CMake_can_automatically_override_the_default_compiler_optimization_flag
