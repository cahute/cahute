.. _feature-topic-contexts:

Contexts
========

Contexts represent execution contexts for Cahute. They are composed of:

* Logging configuration, including the current logging level, callback and
  options.
* System and external library contexts and resources used by the current
  execution context, such as shared objects and libraries (DLL, so).

This resource allows multiple components in the same process to use Cahute
simultaneously without interfering with each other, and allows the library
not to use any read-write data section.
They are analogous to `libusb contexts`_.

A context is created using :c:func:`cahute_create_context`, and destroyed
using :c:func:`cahute_destroy_context`. It is required for at least the
following:

* Logging management functions described in :ref:`header-ref-cahute-logging`;
* :ref:`feature-topic-links`;
* :ref:`feature-topic-files`;
* And many more resources and operations!

.. _libusb contexts: https://libusb.sourceforge.io/api-1.0/libusb_contexts.html
