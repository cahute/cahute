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
