.. _header-ref-cahute-info:

``<cahute/info.h>`` -- Cahute library information
=================================================

Type definitions
----------------

.. c:struct:: cahute_info

    Information regarding the dynamic library.

    .. c:member:: unsigned long cahute_info_flags

        Flags, among the following:

        .. c:macro:: CAHUTE_INFO_FLAG_GIT

            Whether the source directory used for building the library was a
            Git repository.

        .. c:macro:: CAHUTE_INFO_FLAG_GIT_TAGGED

            If the source directory used for building the library was a Git
            repository, whether a tag was positioned on the current git commit
            with the current version.

            For example, if the version is set to ``0.5``, and the ``0.5`` tag
            is currently present and positioned on the current git commit,
            this is set.

        .. c:macro:: CAHUTE_INFO_FLAG_GIT_DIRTY

            If the source directory used for building the library was a Git
            repository, whether uncommitted changes were present on top of the
            current git commit.

    .. c:member:: char const *cahute_info_version_name

        Name of the current version.

    .. c:member:: char const *cahute_info_homepage_url

        Homepage URL for the Cahute project.

    .. c:member:: char const *cahute_info_issues_url

        Bug report URL for the Cahute project.

    .. c:member:: char const *cahute_info_git_commit

        Current git commit for the repository, as a string,
        e.g. ``f1e3623ee60687c59400e2fa876dfc667d69b592``.

        If :c:macro:`CAHUTE_INFO_FLAG_GIT` is unset on
        :c:member:`cahute_info.cahute_info_flags`, this member is set
        to ``NULL``.

    .. c:member:: char const *cahute_info_git_branch

        Current git branch for the repository, as a string,
        e.g. ``develop`` or ``feat/hello-world``.

        If :c:macro:`CAHUTE_INFO_FLAG_GIT` is unset on
        :c:member:`cahute_info.cahute_info_flags`, this member is set
        to ``NULL``.

Function declarations
---------------------

.. c:function:: cahute_info const *cahute_get_info(void)

    Get a pointer to the current library information.

    :return: Pointer to the current library information.
