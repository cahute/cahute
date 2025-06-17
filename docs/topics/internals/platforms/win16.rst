.. _internals-topic-win16:

Win16 specific implementation details
=====================================

.. warning::

    These details have not been tested yet; see
    `#100 <https://gitlab.com/cahute/cahute/-/issues/100>`_ for
    more information.

This document details implementation details specific to Cahute's usage
of :ref:`feature-topic-system-win16`.

Serial device management
------------------------

16-bit Windows exposes DOS communication ports (serial), including ``COM1``
to ``COM4``. As such, those are exposed statically on detection.

Communications functions, however, are Win16-specific:

* We use ``OpenComm()`` to open the serial port;
* We use ``GetCommState()`` and ``SetCommState()`` to set the port's serial
  parameters;
* We use ``ReadComm()`` to read data from the port;
* We use ``WriteComm()`` to write data to the port;
* We use ``CloseComm()`` to close the serial port.

On reading, Win16 does not directly support managing timeouts that is not
induced by optional hardware control flows, such as RTS/CTS or DTR/DSR.
Instead, on reading, if a timeout is set, we check the number of bytes
available in the input buffer using ``GetCommError()`` (through the
``COMSTAT`` pointer) every 20 ms until the timeout has passed.

File management
---------------

.. todo::

    Cahute does not currently support opening files with Win16, due to the
    fact that Win16 only defines a method to open a file, then relies on
    MS-DOS system calls to read, write, seek, and close the obtained file
    handle.

    See `#101 <https://gitlab.com/cahute/cahute/-/issues/101>`_ for
    more information.
