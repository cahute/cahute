.. _cmake-ref:

CMake setting reference
=======================

This section references all settings that can be used with Cahute's
CMake settings.

.. _cmake-ref-builtin-settings:

CMake built-in settings
-----------------------

The following variables can be used with every CMake project, and described
amongst others in `cmake-variables(7)`_, but may be described with information
more relevant to Cahute here.

.. _cmake-ref-setting-cmake-build-type:

|CMAKE_BUILD_TYPE|_
    Build type. Commonly found values with Cahute are the following:

    ``Debug``
        Non-optimized output with debug symbols and no binary stripping.

    ``Release``
        Optimized output.

    .. note::

        As described `in the CMake package guidelines for Archlinux
        <CMake Release undesired behaviour_>`_, CMake automatically
        forces ``-O3`` when ``Release`` is selected.
        Cahute overrides this with ``-O2`` instead.

|CMAKE_INSTALL_PREFIX|_
    Install prefix.

    In the :ref:`build from source guides <build-guide>`, it is recommended
    to set this to ``/usr`` rather than the default ``/usr/local`` value.

|CMAKE_VERBOSE_MAKEFILE|_
    Optional switch to set to ``ON`` to see the commands executed when
    building the project.

.. _cmake-ref-general-settings:

Cahute-specific general settings
--------------------------------

The following variables are specific to Cahute.

.. _cmake-ref-setting-cahute-cli:

``CAHUTE_CLI``
    Enable command-line utilities.

    This is enabled by default.

.. _cmake-ref-setting-cahute-cli-experimental:

``CAHUTE_CLI_EXPERIMENTAL``
    Enable experimental / unfinished command-line utilities.

    This is disabled by default.

``CAHUTE_DEFAULT_LOGLEVEL``
    Default logging level used when creating a context, among ``debug``
    ``info``, ``warning`` (*by default*), ``error``, ``fatal`` and ``none``.

    See :ref:`feature-topic-logging` for more information.

``CAHUTE_GIT``
    Include git_\ -related information to the built targets, i.e. if the
    source directory is a git repository with at least one commit on the
    current branch, the following macros will be defined and included in
    the headers:

    * :c:macro:`CAHUTE_GIT_COMMIT`;
    * :c:macro:`CAHUTE_GIT_BRANCH`;
    * :c:macro:`CAHUTE_GIT_TAGGED`;
    * :c:macro:`CAHUTE_GIT_DIRTY`.

    This is enabled by default.

``CAHUTE_LIBUSB``
    Enable the use of libusb_.

    .. note::

        This will be ignored on Win32, since Cahute needs to support drivers
        libusb doesn't.

    This is enabled by default on `platforms libusb support
    <libusb features_>`_.

``CAHUTE_PKGCONF``
    Enable installing pkgconf_ / `pkg-config`_ files.

    This is supported by default.

``CAHUTE_REPORT_URL``
    URL to the bug reporting guide included within the library and
    command-line utilities.

``CAHUTE_SDL``
    Enable the use of SDL_.

    This is enabled by default on `platforms SDL2 support
    <SDL2 platforms_>`_.

``CAHUTE_UDEV``
    Enable building and installing the udev rule.

    This is enabled by default when building for Linux.

``CAHUTE_UDEV_GROUP``
    Name of the group to which the udev rule gives permission to calculators
    plugged in via USB.

    It is recommended to set the same group here as for normal serial devices,
    as defined by your distribution.

    The `Linux Standard Base groups`_ defines ``uucp`` and the
    `Archlinux user groups`_ use it for serial devices, other distributions
    use other groups such as ``dialout``; see the following for more
    information:

    * `Debian system groups`_;
    * `Void Linux default groups`_.

    This is set to ``uucp`` by default.

.. _cmake-ref-cli:

Cahute-specific command-line utility switches
---------------------------------------------

Most command-line utilities in Cahute can be enabled using the :ref:`CAHUTE_CLI
<cmake-ref-setting-cahute-cli>` setting.

However, they can also be individually enabled using the following switches:

``CAHUTE_CLI_P7``
    Enable building :ref:`p7 <cli-ref-p7>`.

``CAHUTE_CLI_P7OS``
    Enable building :ref:`p7os <cli-ref-p7os>`.

``CAHUTE_CLI_P7SCREEN``
    Enable building :ref:`p7screen <cli-ref-p7screen>`.

``CAHUTE_CLI_XFER9860``
    Enable building :ref:`xfer9860 <cli-ref-xfer9860>`.

Cahute-specific experimental command-line utility switches
----------------------------------------------------------

Some command-line utilities in Cahute are considered experimental (not fully
implemented), and can be enabled by enabling both the :ref:`CAHUTE_CLI
<cmake-ref-setting-cahute-cli>` and :ref:`CAHUTE_CLI_EXPERIMENTAL
<cmake-ref-setting-cahute-cli-experimental>` settings.

However, they can also be individually enabled using the following
switches:

``CAHUTE_CLI_CAS``
    Enable building :ref:`CaS <cli-ref-cas>`.

.. |CMAKE_BUILD_TYPE| replace:: ``CMAKE_BUILD_TYPE``
.. |CMAKE_INSTALL_PREFIX| replace:: ``CMAKE_INSTALL_PREFIX``
.. |CMAKE_VERBOSE_MAKEFILE| replace:: ``CMAKE_VERBOSE_MAKEFILE``

.. _`cmake-variables(7)`:
    https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html
.. _`CMAKE_BUILD_TYPE`:
    https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
.. _`CMAKE_INSTALL_PREFIX`:
    https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html
.. _`CMAKE_VERBOSE_MAKEFILE`:
    https://cmake.org/cmake/help/latest/variable/CMAKE_VERBOSE_MAKEFILE.html
.. _CMake Release undesired behaviour:
    https://wiki.archlinux.org/title/CMake_package_guidelines
    #CMake_can_automatically_override_the_default_compiler_optimization_flag

.. _git: https://git-scm.com/
.. _libusb: https://libusb.info/
.. _libusb features: https://github.com/libusb/libusb/wiki#features
.. _SDL: https://libsdl.org/
.. _SDL2 platforms:
    https://wiki.libsdl.org/SDL2/Introduction#what_platforms_does_sdl_run_on
.. _pkgconf: https://github.com/pkgconf/pkgconf
.. _pkg-config: https://www.freedesktop.org/wiki/Software/pkg-config/

.. _Linux Standard Base groups:
    https://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/usernames.html
.. _Archlinux user groups:
    https://wiki.archlinux.org/title/Users_and_groups#User_groups
.. _Debian system groups:
    https://wiki.debian.org/SystemGroups
.. _Void Linux default groups:
    https://docs.voidlinux.org/config/users-and-groups.html#default-groups
