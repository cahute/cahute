.. _build-guide-linux:

|linux| Building Cahute for Linux distributions
===============================================

.. warning::

    In order to install Cahute on Linux, it is recommended to use one of the
    methods described in :ref:`install-guide-linux`. However, if Cahute is not
    available for your distribution, or if you wish to build it manually, this
    guide is for you.

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
* Python_ >= 3.11;
* `GNU Make`_, `pkg-config`_, and other C compilation and linking utilities.

It also depends on the following build and runtime dependencies:

* SDL_ >= 3.0 (for ``p7screen``);
* libusb_ >= 1.0.23.

In order to install the dependencies, it is recommended you use your native
package manager. A few examples are the following:

* On Debian and derivatives:

  .. code-block:: bash

      sudo apt-get update
      sudo apt-get install cmake python3 libusb-1.0-0-dev libsdl3-dev

* On Archlinux and derivatives:

  .. code-block:: bash

      sudo pacman -Sy cmake python libusb sdl3

* On Voidlinux and derivatives:

  .. code-block:: bash

      xbps-install cmake python3 libusb-devel sdl3-devel

.. _build-guide-linux-sh-group:

Finding out the device group
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. warning::

    This section assumes you have udev_, which is provided along with systemd.
    If your distribution does not use systemd, such as `Artix Linux`_ or
    Devuan_, you must skip this section.

.. note::

    If you do not wish to make USB and serial devices accessible to your user,
    you can skip this section.

On Linux distributions, serial devices such as ``/dev/ttyUSB*`` are usually
assigned a group by udev_. Said group varies depending on your distribution,
even though it usually is either ``dialout`` or ``uucp``.

You can usually find out which group to use by checking the owner of any
``/dev/ttyS*`` device, by running the following command:

.. code-block:: bash

    ls -l /dev/ttyS*

On Arch Linux, the output of this command resembles the following:

.. parsed-literal::

    crw-rw---- 1 root **uucp** 4, 64 20 janv. 15:49 /dev/ttyS0
    crw-rw---- 1 root **uucp** 4, 65 20 janv. 15:49 /dev/ttyS1
    ...

Which means that, in this case, the name of the device group is ``uucp``.

If this method fails, you need to check what your distribution uses.
Check the documentation and/or community of your distribution for something
along "System groups" or "tty".

Building the project
~~~~~~~~~~~~~~~~~~~~

In the parent directory to the source, you can now create the ``build``
directory aside it, and install from it, by running the following commands:

.. parsed-literal::

    cmake -B build -S cahute-|version| -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCAHUTE_UDEV_GROUP=\ *<your_device_group>*
    cmake --build build

.. warning::

    If you have skipped :ref:`build-guide-linux-sh-group`, replace
    ``-DCAHUTE_UDEV_GROUP=...`` with ``-DCAHUTE_UDEV=OFF``.

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

Loading the new udev rules
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. warning::

    If you have skipped :ref:`build-guide-linux-sh-group`, you must skip
    this section.

In order to load the new udev rules, you must either run the following command,
or reboot your system:

.. code-block:: bash

    sudo udevadm control --reload

Providing access to USB and serial devices to your user
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. warning::

    If you have skipped :ref:`build-guide-linux-sh-group`, you must skip
    this section.

In order to obtain access to both USB and serial devices as your current user,
you must add the device group as a supplementary group to your user, by
running the following command:

.. parsed-literal::

    sudo usermod -a -G *<your_device_group>* *<your_username>*

For example, if your device group is ``dialout`` and your username is
``john.cahute``, the command will be the following:

.. code-block:: bash

    sudo usermod -a -G dialout john.cahute

.. note::

    If you don't know your current username, you can find it out by running
    the following command:

    .. code-block:: bash

        id -nu

You must then either **log off completely then log in again**, or reboot your
computer, for the new supplementary group to take effect.

.. warning::

    If you're using a desktop environment, locking the screen or putting your
    computer in standby mode is **NOT** equivalent to logging off.
    If in doubt, rebooting is a straightforward option in comparison.

.. |linux| image:: ../install/linux.svg

.. _Get notified when a release is created:
    https://docs.gitlab.com/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications

.. _cmake: https://cmake.org/
.. _Python: https://www.python.org/
.. _GNU Make: https://www.gnu.org/software/make/
.. _pkg-config: https://git.sr.ht/~kaniini/pkgconf
.. _SDL: https://www.libsdl.org/
.. _libusb: https://libusb.info/
.. _udev: https://wiki.archlinux.org/title/Udev
.. _Artix Linux: https://artixlinux.org/
.. _Devuan: https://www.devuan.org/

.. _CMake Release undesired behaviour:
    https://wiki.archlinux.org/title/CMake_package_guidelines
    #CMake_can_automatically_override_the_default_compiler_optimization_flag
