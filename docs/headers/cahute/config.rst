.. _header-cahute-config:

``<cahute/config.h>`` -- Cahute configuration details
=====================================================

This header declares configuration values for Cahute.

Macro definitions
-----------------

.. c:macro:: CAHUTE_VERSION

    Cahute version, as a string, e.g. ``"28.13"``.

.. c:macro:: CAHUTE_VERNUM

    Cahute version, as a hexadecimal number with the following mask::

        0xMMmm0000

    Where:

    * ``MM`` is the major version in hexadecimal, e.g. ``1c`` for major
      version 28;
    * ``mm`` is the minor version in hexadecimal, e.g. ``0d`` for minor
      version 13.

.. c:macro:: CAHUTE_MAJOR

    Cahute major version.

.. c:macro:: CAHUTE_MINOR

    Cahute minor version.

.. c:macro:: CAHUTE_URL

    Homepage URL for the Cahute project.

.. c:macro:: CAHUTE_ISSUES_URL

    Bug report URL for the Cahute project.

.. c:macro:: CAHUTE_GIT_COMMIT

    *(Optional)* Current git commit for the repository, as a string,
    e.g. ``f1e3623ee60687c59400e2fa876dfc667d69b592``.

.. c:macro:: CAHUTE_GIT_BRANCH

    *(Optional)* Current git branch for the repository, as a string,
    e.g. ``develop`` or ``feat/hello-world``.

.. c:macro:: CAHUTE_GIT_TAGGED

    *(Optional)* Whether a tag is positioned on the current git commit
    with the current version, as an integer set to ``0`` or ``1``.

    For example, if :c:macro:`CAHUTE_VERSION` is set to ``"0.5"`` and the
    ``0.5`` tag is currently present and positioned on the current git commit,
    this is set to ``1``.

.. c:macro:: CAHUTE_GIT_DIRTY

    *(Optional)* Whether uncommitted changes are present on top of the
    current git commit for the repository.
