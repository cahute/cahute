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

.. _cmake-ref-setting-cahute-appstream:

``CAHUTE_APPSTREAM``
    Include AppStream declarations.

    This is disabled by default.

.. _cmake-ref-setting-cahute-cli:

``CAHUTE_CLI``
    Enable command-line utilities.

    This is enabled by default.

.. _cmake-ref-setting-cahute-cli-experimental:

``CAHUTE_CLI_EXPERIMENTAL``
    Enable experimental / unfinished command-line utilities.

    This is disabled by default.

.. _cmake-ref-setting-cahute-cli-library-type:

``CAHUTE_CLI_LIBRARY_TYPE``
    Which type of the Cahute library should the CLI utilities, if built, depend
    on.

    This can be set to the following values:

    ``auto`` (*by default*)
        The most suitable library type is chosen:

        * If :ref:`CAHUTE_SHARED <cmake-ref-setting-cahute-shared>` is enabled,
          the shared library is used;
        * Otherwise, if :ref:`CAHUTE_CLI_PIE
          <cmake-ref-setting-cahute-cli-pie>` is enabled, the static library
          with position-independent code is used, which means
          :ref:`CAHUTE_STATIC_PIC <cmake-ref-setting-cahute-static-pic>` must
          be enabled;
        * Otherwise, the static library with position-dependent code is used,
          which means :ref:`CAHUTE_STATIC <cmake-ref-setting-cahute-static>`
          must be enabled.

    ``shared``
        The shared library is used, which means :ref:`CAHUTE_SHARED
        <cmake-ref-setting-cahute-shared>` must be enabled.

    ``static``
        The most suitable static library is chosen:

        * If :ref:`CAHUTE_CLI_PIE <cmake-ref-setting-cahute-cli-pie>` is
          enabled, the static library with position-independent code is used,
          which means :ref:`CAHUTE_STATIC_PIC
          <cmake-ref-setting-cahute-static-pic>` must be enabled;
        * Otherwise, the static library with position-dependent code is used,
          which means :ref:`CAHUTE_STATIC <cmake-ref-setting-cahute-static>`
          must be enabled.

.. _cmake-ref-setting-cahute-cli-pie:

``CAHUTE_CLI_PIE``
    Enable building CLI utilities as position-independent executables (PIE).

    By default, this is enabled on :ref:`feature-topic-system-linux` and
    :ref:`feature-topic-system-win32`, and disabled on all others.

.. _cmake-ref-setting-cahute-cli-runtime-deps:

``CAHUTE_CLI_RUNTIME_DEPS``
    Whether to install runtime dependencies (such as shared libraries)
    with the command-line utilities.

    This is disabled by default on all platforms.

.. _cmake-ref-setting-cahute-default-loglevel:

``CAHUTE_DEFAULT_LOGLEVEL``
    Default logging level used when creating a context, among ``debug``
    ``info``, ``warning`` (*by default*), ``error``, ``fatal`` and ``none``.

    See :ref:`feature-topic-logging` for more information.

.. _cmake-ref-setting-cahute-git:

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

.. _cmake-ref-setting-cahute-install-udevdir:

``CAHUTE_INSTALL_UDEVDIR``
    Path to the directory in which the udev configuration is assumed to be
    stored.

    If :ref:`CAHUTE_UDEV <cmake-ref-setting-cahute-udev>` is ``ON``, the
    udev rules will be installed into ``${CAHUTE_INSTALL_UDEVDIR}/rules.d``.

    By default, this is set to ``${CMAKE_INSTALL_LIBDIR}/udev``, e.g.
    ``/usr/lib/udev`` on Linux.

.. _cmake-ref-setting-cahute-libusb:

``CAHUTE_LIBUSB``
    Enable the use of libusb_.

    .. note::

        This will be ignored on Win32, since Cahute needs to support drivers
        libusb doesn't.

    This is enabled by default on `platforms libusb support
    <libusb features_>`_.

.. _cmake-ref-setting-cahute-pkgconf:

``CAHUTE_PKGCONF``
    Enable installing pkgconf_ / `pkg-config`_ files.

    This is supported by default.

.. _cmake-ref-setting-cahute-report-url:

``CAHUTE_REPORT_URL``
    URL to the bug reporting guide included within the library and
    command-line utilities.

.. _cmake-ref-setting-cahute-sdl:

``CAHUTE_SDL``
    Enable the use of SDL_.

    This is enabled by default on `platforms SDL3 support
    <SDL3 platforms_>`_.

.. _cmake-ref-setting-cahute-shared:

``CAHUTE_SHARED``
    Enable building the library as a shared library (``.so``, ``.dylib``
    or ``.dll``, depending on the platform).

    By default, this is enabled on :ref:`feature-topic-system-win32`, and
    disabled on all others.

.. _cmake-ref-setting-cahute-static:

``CAHUTE_STATIC``
    Enable building the library as a static library (``.a``, ``.lib``),
    with position-dependent code.

    By default, this is enabled on all platforms.

.. _cmake-ref-setting-cahute-static-pic:

``CAHUTE_STATIC_PIC``
    Enable building the library as a static library (``.a``, ``.lib``),
    with position-independent code.

    By default, this is disabled on all platforms.

.. _cmake-ref-setting-cahute-udev:

``CAHUTE_UDEV``
    Enable building and installing the udev rules.

    This is enabled by default when building for Linux.

.. _cmake-ref-setting-cahute-udev-group:

``CAHUTE_UDEV_GROUP``
    Name of the group to which the udev rule gives permission to calculators
    plugged in via USB.

    This is set to ``uucp`` by default.

    .. _cmake-ref-setting-cahute-udev-group-choose:

    .. note::

        It is recommended to set the same system group here as for other serial
        devices, as defined by your distribution.

        The `Linux Standard Base groups`_ defines ``uucp`` and the
        `Archlinux user groups`_ use it for serial devices, other distributions
        use other groups such as ``dialout``. See the following for more
        information:

        * `Debian system groups`_;
        * `Void Linux default groups`_.

    .. _cmake-ref-setting-cahute-udev-group-system:

    .. warning::

        Since `systemd v258`_ (released on September 17th, 2025), udev no
        longer allows user groups to be used in udev rules:

            systemd-udevd ignores ``OWNER=``/``GROUP=`` settings with a
            non-system user/group specified in udev rules files, to avoid
            device nodes being owned by a non-system user/group.
            It is recommended to check udev rules files with ``udevadm verify``
            and/or ``udevadm test`` commands if the specified user/group in
            ``OWNER=``/``GROUP=`` are valid.

        If building and installing the udev rules is enabled (i.e.
        :ref:`CAHUTE_UDEV <cmake-ref-setting-cahute-udev>` is set to ``ON``),
        this must be set to the name of a system group on the destination
        system.

        This policy is enforced in at least the following distributions:

        * Debian Forky (14) and above, unreleased as of writing (planned for
          2027);
        * `Ubuntu Resolute Raccoon (26.04)`_ and above, since April 2026;
        * Arch Linux and up-to-date derivatives, since September 2025.

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
.. _SDL3 platforms: https://wiki.libsdl.org/SDL3/README-platforms
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
.. _systemd v258:
    https://github.com/systemd/systemd/releases/tag/v258
.. _`Ubuntu Resolute Raccoon (26.04)`: https://releases.ubuntu.com/resolute/
