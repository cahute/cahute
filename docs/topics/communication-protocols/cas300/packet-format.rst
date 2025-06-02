.. _protocol-topic-cas300-packet-format:

CAS300 packet format
====================

All packets in the CAS100 protocol are introduced by a single byte, which
defines the basic purpose of the packet, and defines the kind of payload that
follows it.

See the following sections for more information.

.. _protocol-topic-cas300-packet-00:

``0x00`` -- Serial status packet
--------------------------------

This packet is found over serial links. It has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Serial status (*SS*)
      - Current serial status.
      - 1-byte value, among the following:

        .. list-table::
            :header-rows: 1

            * - Code
              - Description
            * - ``0x00``
              - Communication established; equivalent to
                :ref:`protocol-topic-cas300-packet-13` on USB.
            * - ``0x05``
              - Calculator not (yet) in receive mode, or communication
                not started.
            * - ``0x09``
              - Calculator switching to receive mode automatically.

.. _protocol-topic-cas300-packet-01:

``0x01`` -- Command packet
--------------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      - Identifier of the packet the other party acknowledges.
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value.
    * - 2 (0x02)
      - 4 B
      - Payload size (*PZ*)
      - Size of the payload.
      - 4-char :ref:`protocol-topic-seven-ascii-hex` value.
    * - 6 (0x06)
      - *PZ* B
      - Payload (*P*)
      - Payload of the command.
      - :ref:`0x5C padded <protocol-topic-seven-5c-padding>` content.
    * - 6 + *PZ*
      - 2 B
      - Checksum (*CS*)
      -
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value.

The checksum can be obtained or verified by summing all bytes going from
*PZ* to *P*, and adding 1 to its bitwise complement.

See :ref:`protocol-topic-cas300-commands` for more information about commands.

.. _protocol-topic-cas300-packet-02:

``0x02`` -- Data packet
-----------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      - Identifier of the packet the other party acknowledges.
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value.
    * - 2 (0x02)
      - 4 B
      - Payload size (*PZ*)
      - Size of the payload.
      - 4-char :ref:`protocol-topic-seven-ascii-hex` value.
    * - 6 (0x06)
      - *PZ* B
      - Payload (*P*)
      - Payload of the command.
      - :ref:`0x5C padded <protocol-topic-seven-5c-padding>` content.
    * - 6 + *PZ*
      - 2 B
      - Checksum (*CS*)
      -
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value.

The checksum can be obtained or verified by summing all bytes going from
*PZ* to *P*, and adding 1 to its bitwise complement.

.. _protocol-topic-cas300-packet-06:

``0x06`` -- Acknowledge packet
------------------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      - Identifier of the packet the other party acknowledges.
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value.

.. _protocol-topic-cas300-packet-13:

``0x13`` -- Established packet
------------------------------

This packet is single-byte, and is used by the receiver to acknowledge
link initialization. This is common to all CASIOLINK variants.

.. warning::

    On serial links, instead of sending ``0x13``, the ClassPad 300 / 330 (+)
    may send two ``0x00`` bytes. This nuance is CAS300-specific.

.. _protocol-topic-cas300-packet-15:

``0x15`` -- Out-of-order packet
-------------------------------

This packet is used by either party to signal that the identifier of the packet
sent by the other party is out-of-order. It has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Expected packet identifier (*EID*)
      - Expected packet identifier.
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value.

.. _protocol-topic-cas300-packet-18:

``0x18`` -- Terminate packet
----------------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      -
      - 2-char :ref:`protocol-topic-seven-ascii-hex` value, set to ``0x11``.
    * - 2 (0x02)
      - 4 B
      -
      -
      - 4-char :ref:`protocol-topic-seven-ascii-hex` value, among the
        following:

        * ``0x0000``: terminated from the calculator.
        * ``0x0004``: terminated from the host.
