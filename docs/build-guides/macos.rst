.. _build-macos:

|apple| Building Cahute for macOS / OS X
========================================

.. warning::

    In order to install Cahute on macOS / OS X, it is recommended to use one of
    the methods in :ref:`install-macos`. However, if you wish to build Cahute
    manually, this guide is for you.

The following building methods are available.

.. note::

    Since you will not be using a packaged version of Cahute, the project won't
    be automatically updated when updating the rest of the system, which
    means you will need to do it manually, especially if a security update is
    made.

    You can subscribe to releases by creating a Gitlab.com account, and
    following the steps in `Get notified when a release is created`_.
    You can check your notification settings at any time in Notifications_.

.. _build-macos-sh:

Building Cahute natively for macOS / OS X
-----------------------------------------

This guide will assume you hae a POSIX or compatible shell, such as
``bash`` or ``zsh``.

Downloading the Cahute source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: _download_source.rst

Installing the dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cahute depends on the following build-only dependencies:

* cmake_ >= 3.16;
* Python_ >= 3.8;
* `toml module for Python <python-toml_>`_, either installed through pip
  or as a native package such as ``python-toml`` or ``python3-toml``;
* `GNU Make`_, `pkg-config`_, and other C compilation and linking utilities.

It also depends on the following build and runtime dependencies:

* SDL_ >= 2.0 (for ``p7screen``);
* libusb_.

In order to install the native dependencies, it is recommended you use
Homebrew_:

.. code-block:: bash

    brew install cmake pkg-config python@3.12 libusb sdl2

You must also install the Python dependencies, by going to the parent directory
to the source, and creating a virtual environment within ``build/venv`` using
the following commands:

.. code-block:: bash

    python3 -m venv build/venv
    source build/venv/bin/activate
    python -m pip install toml

Building the project
~~~~~~~~~~~~~~~~~~~~

You can now initialize the build directory by running the following command:

.. parsed-literal::

    cmake -B build -S cahute-|version| -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release

.. warning::

    The build initialization command **must be run in the same shell as
    the one where the Python venv was initialized**.

    If you have created a new shell, you must run the following command
    beforehand:

    .. code-block:: bash

        source build/venv/bin/activate

Now that the build directory has been initialized, you can build using the
following command:

.. code-block:: bash

    cmake --build build

.. note::

    This can be run in any shell, provided you adapt the path to be absolute
    or relative to the current directory (e.g. ``cmake --build .`` if within
    the build directory), and the Python venv does not need to be
    activated beforehand since CMake remembers it must use it.

.. |apple| image:: ../install-guides/apple.svg
.. |homebrew| image:: ../install-guides/homebrew.svg

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

.. _Homebrew: https://brew.sh/
