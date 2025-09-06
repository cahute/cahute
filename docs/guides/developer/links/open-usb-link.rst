Opening a link to a calculator connected by USB
===============================================

In order to open the link to the calculator, the steps are the following:

#. Create a context using :c:func:`cahute_create_context`.
#. Open a link using :c:func:`cahute_open_simple_usb_link`.
#. *Profit!*
#. Call :c:func:`cahute_close_link` to close the link.
#. Call :c:func:`cahute_destroy_context` to destroy the context.

.. note::

    If there are multiple calculators connected by USB to your system,
    you can manage multiple or a specific subset of them by:

    * Detecting available USB devices using :c:func:`cahute_detect_usb`.
      See :ref:`developer-guide-detect-usb` for steps to do so;
    * Opening a link to a specific USB device using
      :c:func:`cahute_open_usb_link`.

An example program to do this is the following:

.. literalinclude:: open-usb-link.c
    :language: c
