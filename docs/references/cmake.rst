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

        As described `in the CMake package guidelines
        <CMake Release undesired behaviour_>`_, CMake automatically
        forces ``-O3`` when ``Release`` is selected.
        Cahute overrides this with ``-O2`` instead.

|CMAKE_INSTALL_PREFIX|_
    Install prefix.

    In the :ref:`build-guide` guide, it is recommended to set this to
    ``/usr`` rather than the default ``/usr/local`` value.

|CMAKE_VERBOSE_MAKEFILE|_
    Optional switch to set to ``ON`` to see the commands executed when
    building the project.

.. _cmake-ref-general-settings:

Cahute-specific general settings
--------------------------------

The following variables are specific to Cahute.

``CAHUTE_DEFAULT_LOGLEVEL``
    Default logging level used when creating a context, among
    ``info``, ``warning`` (*by default*), ``error``, ``fatal`` and ``none``.

    See :ref:`feature-topic-logging` for more information.

``CAHUTE_CLI``
    Enable command-line utilities.

.. _cmake-ref-setting-cahute-cli-experimental:

``CAHUTE_CLI_EXPERIMENTAL``
    Enable experimental / unfinished command-line utilities.

``CAHUTE_GIT``
    Include git_\ -related information to the built targets, i.e. if the
    source directory is a git repository with at least one commit on the
    current branch, the following macros will be defined and included in
    the headers:

    * :c:macro:`CAHUTE_GIT_COMMIT`;
    * :c:macro:`CAHUTE_GIT_BRANCH`;
    * :c:macro:`CAHUTE_GIT_TAGGED`;
    * :c:macro:`CAHUTE_GIT_DIRTY`.

``CAHUTE_LIBUSB``
    Enable the use of libusb_.

``CAHUTE_PKGCONF``
    Enable installing pkgconf_ / `pkg-config`_ files.

``CAHUTE_REPORT_URL``
    URL to the bug reporting guide included within the library and
    command-line utilities.

``CAHUTE_SDL``
    Enable the use of SDL_.

``CAHUTE_UDEV``
    Enable building and installing the udev rule.

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

.. _cmake-ref-feature-switches:

Cahute feature switches (advanced)
----------------------------------

The following variables can be used to toggle features within the CMake
configuration on or off directly.

.. warning::

    These switches are for advanced users, and should be indirectly defined
    through settings defined in :ref:`cmake-ref-general-settings`.

``CAHUTE_FEATURE_CLI_CAS``
    Enable and include the :ref:`cli-ref-cas` target.

    By default, this is enabled if both ``CAHUTE_CLI`` and
    ``CAHUTE_CLI_EXPERIMENTAL`` are enabled.

``CAHUTE_FEATURE_CLI_P7``
    Enable and include the :ref:`cli-ref-p7` target.

    By default, this is enabled if ``CAHUTE_CLI`` is enabled.

``CAHUTE_FEATURE_CLI_P7OS``
    Enable and include the :ref:`cli-ref-p7os` target.

    By default, this is enabled if ``CAHUTE_CLI`` is enabled.

``CAHUTE_FEATURE_CLI_P7SCREEN``
    Enable and include the :ref:`cli-ref-p7screen` target.

    By default, this is enabled if both ``CAHUTE_CLI`` and ``CAHUTE_SDL``
    are enabled.

``CAHUTE_FEATURE_CLI_XFER9860``
    Enable and include the :ref:`cli-ref-xfer9860` target.

    By default, this is enabled if ``CAHUTE_CLI`` is enabled.

``CAHUTE_FEATURE_LIB_HEADERS``
    Enable and include the library headers.

    By default, this is enabled.

``CAHUTE_FEATURE_LIB_PKGCONF``
    Enble and include the pkgconf_ / `pkg-config`_ files for the library.

    By default, this is enabled if ``CAHUTE_PKGCONF`` is enabled.

``CAHUTE_FEATURE_LIB_STATIC``
    Enable and include the target to build the static library.

    By default, this is enabled.

``CAHUTE_FEATURE_UDEV``
    Enable and include the udev rules.

    By default, this is enabled if ``CAHUTE_UDEV`` is enabled.

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
.. _SDL: https://libsdl.org/
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
