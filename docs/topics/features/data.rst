.. _feature-topic-data:

Main memory data
================

.. todo::

    "The CASIOLINK protocols is about sending and receiving main memory files"
    is actually incorrect, and based on an obsolete and partial view of CAS40
    and CAS50. CAS50 allows other interactions, CAS40 also supports
    screenshots, and CAS100 and CAS300 support interacting with the
    storage memory, along other operations.

Main memory files are the files present on the calculator's main memory, which
is present on all graphic CASIO calculators since the fx-7000G at least.
They can be programs, captures, and so on, and are small enough to fit in
the main memory, usually stored in RAM in a filesystem no bigger than 64 KiB.

Most elements in the CASIO ecosystem are thought around exchanging these
files:

* The CASIOLINK protocols is about sending and receiving main memory files;
* Part of the commands in Protocol 7.00 are about sending and receiving
  main memory files, and interacting with the main memory in general.
  See :ref:`protocol-topic-seven-casio-commands` for more information;
* A lot of the file formats documented here are about containing one or
  more main memory files:

  - CASIOLINK files are a binary collection of main memory files as represented
    using their protocol counterpart. See :ref:`file-format-topic-casiolink`
    for more information;
  - CAT and CTF are textual equivalents of the previous, one being official
    and the other being defined by the community. See
    :ref:`file-format-topic-cat` and :ref:`file-format-topic-ctf` for more
    information;
  - MCS (Main Control Structure) files, with the G1M/G1R/G2M/G2R/G3M/G3R
    extensions, are a binary collection of main memory files for the fx-9860G
    and compatible. See :ref:`file-format-topic-mainmem` for more information.

Main memory files usually have an associated data type and a format.

.. todo:: Catalog known main memory file natures here: program, ...

.. _feature-topic-data-program:

Programs
--------

.. todo:: Write about this.
