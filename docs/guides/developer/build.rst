.. _developer-guide-build:

Building with the Cahute library
================================

.. warning::

    This guide assumes that you have already installed the Cahute library.
    If you have not, see :ref:`install-guide`.

The instructions to build your project using the Cahute library depend on
your build system; please read the correct section for yours!

Using |cmake| CMake
-------------------

If your project is built using CMake, you can add the following to your
``CMakeLists.txt`` file, before defining your targets, to include and start
building with Cahute:

.. parsed-literal::

    pkg_check_modules(cahute REQUIRED cahute-|public_version| IMPORTED_TARGET)
    link_libraries(PkgConfig::cahute)

.. note::

    If you prefer to link with the static version of the library, replace
    |shared_pkg| with |static_pkg|.

Using pkg-config
----------------

Cahute defines the ``cahute`` pkg-config configuration. Therefore:

* You can obtain the compilation flags by running the following command:

  .. parsed-literal::

      pkg-config cahute-|public_version| --cflags

* You can obtain the linking flags by running the following command:

  .. parsed-literal::

      pkg-config cahute-|public_version| --libs

For example, if you want to compile a simple project using Cahute, you can
use the following command:

.. parsed-literal::

    cc main.c -o ./my_util \`pkg-config cahute-|public_version| --cflags --libs\`

.. note::

    If you prefer to link with the static version of the library, replace
    |shared_pkg| with |static_pkg|.

.. |cmake| image:: cmake.svg

.. _CMake: https://cmake.org/
