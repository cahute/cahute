Cahute |version|
================

Cahute is a library and set of command-line utilities to handle serial
and USB communication protocols and file formats related to CASIO calculators,
dating from the 1990s to today. It provides the following
features\ [#mutant]_\ :

.. feature-list::

    * - |feat-transfer|
      - File transfer between storages
      - With ``p7``, transfer files from and to storages on
        fx-9860G compatible calculators, over USB and serial links!
    * - |feat-program|
      - Program backup
      - With ``CaS``, extract programs from all CASIO calculators since 1992,
        over USB and serial links!
    * - |feat-text|
      - Text conversions
      - With :c:func:`cahute_convert_text`, convert text between standard and
        CASIO-specific text encodings!
    * - |feat-ohp|
      - Screenstreaming
      - With ``p7screen``, display screen captures and streaming from your
        calculator on your host system!
    * - |feat-flash|
      - ROM flashing
      - With ``p7os``, flash fx-9860G and compatible calculators!

Installing or building Cahute is officially supported on the following systems:

.. include:: guides/_system_guides.rst

The project is being worked on `on Gitlab <Cahute on Gitlab_>`_.
It is maintained by `Thomas Touhey`_. See :ref:`project-topic-forums` for the
topics describing the projects in other communities.

The project's code and documentation contents are licensed under CeCILL_
version 2.1 as distributed by the CEA, CNRS and Inria on
`cecill.info <CeCILL_>`_.

This documentation is organized using `Diátaxis`_' structure.

How-to guides
-------------

These sections provide guides, i.e. recipes, targeted towards various actors.
They guide you through the steps involved in addressing key problems
and use-cases.

.. toctree::
    :caption: How-to guides
    :maxdepth: 3

    guides/install
    guides/build
    guides/contribution
    guides/cli
    guides/developer
    guides/misc

Discussion topics
-----------------

These sections discuss key topics and concepts at a fairly high level,
and provide useful background information and explanation.

.. toctree::
    :caption: Discussion topics
    :maxdepth: 3

    topics/communication
    topics/file-formats
    topics/data-formats
    topics/features
    topics/internals
    topics/project

References
----------

These sections provide technical reference for APIs and other aspects of
Cahute's machinery. They go into detail, and therefore, assume you have a
basic understanding of key concepts.

.. toctree::
    :caption: References
    :maxdepth: 3

    references/cli
    references/headers
    references/cmake

Glossary
--------

The following are definitions for words and expressions used in this
documentation.

.. glossary::

    3-pin
        Serial connector available on all CASIO calculators from 1991
        to today, used for both PC-to-calculator and calculator-to-calculator
        communications.

        See :ref:`protocol-topic-port-3pin` for more information.

    Add-in
        Additional application for calculators.

        While the system applications are embedded in the calculator's
        system, some calculators allow for additional applications to be
        placed in the :term:`storage memory`, and can be executed from the
        main menu; these are named "add-ins".

    Addin
        See :term:`Add-in`.

    CAS40
        Communication protocol used by pre-1996 CASIO calculators.

        See :ref:`protocol-topic-cas40` for more information.

    CAS50
        Communication protocol used by CASIO calculators from 1996 to 2004.

        See :ref:`protocol-topic-cas50` for more information.

    CAS100
        Communication protocol used by CASIO AFX / Graph 100(+) calculators.

        See :ref:`protocol-topic-cas100` for more information.

    CAS300
        Communication protocol used by CASIO ClassPad 300/330(+) calculators.

        See :ref:`protocol-topic-cas300` for more information.

    Connector
        Physical connector present on a host or a calculator, on which a
        corresponding cable can be plugged.

    Context
        Collection of system and library resources used to accomplish tasks
        using Cahute.

        See :ref:`feature-topic-contexts` for more information.

    Generic link
        :term:`Link` with no protocol, providing access to the underlying
        :term:`transport` for sending and receiving data using a custom
        protocol.

        See :ref:`feature-topic-links-generic` for more information.

    Link
        Link established with a calculator or host using a :term:`protocol`
        over a :term:`transport`, within a :term:`context`.

        See :ref:`feature-topic-links` for more information.

    Main memory
        Small filesystem present on all calculators used to store most files
        used by various system applications, such as :term:`programs
        <program>`. Usually 64 KiB or less, and stored in RAM.

        See :ref:`feature-topic-data` for more information.

    Main memory data
        Data represented as a file on the :term:`main memory`, bearing a type
        and type-specific data.

        See :ref:`feature-topic-data` for more information.

    OHP
        Overhead projector; see :term:`Screenstreaming`.

    PXH-A16
        Calculator-side connector available on the Classpad 300.

        See :ref:`protocol-topic-port-pxh-a16` for more information.

    Port
        See :term:`Connector`.

    Program
        :term:`main memory data` representing a CASIO BASIC program which can
        be created, edited, deleted, and executed by a system application on
        any CASIO calculator, usually PRGM.

        See :ref:`feature-topic-data-program` for more information.

    Protocol
        Communication protocol used over a :term:`transport` to fulfill
        one or more purposes, in the context of a :term:`link`.

        See :ref:`protocol-topic-purposes` for more information.

    Protocol 7.00
        Communication protocol used over both serial and USB starting from the
        fx-9860G (2004) calculators.

        See :ref:`protocol-topic-seven` for more information.

    SB-62
        Official 3-pin to 3-pin cable by CASIO, used for calculator to
        calculator communication.

        See :ref:`protocol-topic-cable-sb-62` for more information.

    SB-87
        Official serial (DB-9) to 3-pin cable by CASIO.

        See :ref:`protocol-topic-cable-sb-87` for more information.

    SB-88
        Official USB Type-A to 3-pin cable by CASIO.

        See :ref:`protocol-topic-cable-sb-88` for more information.

    SB-88(A)
        Official variation of the :term:`SB-88` cable by CASIO.

        See :ref:`protocol-topic-cable-sb-88` for more information.

    SB-300
        Official USB Type-A to :term:`PXH-A16` cable by CASIO.

        See :ref:`protocol-topic-cable-sb-300` for more information.

    SB-305
        Official serial (DB-99) to 3-pin cable by CASIO.

        See :ref:`protocol-topic-cable-sb-305` for more information.

    Screenstreaming
        Communications use case where the calculator shares the contents of
        its screen to a host over serial or USB.

        This can be used to project the screen contents onto a bigger screen
        or wall using a projector, or record the screen contents on the
        host.

    Storage memory
        Filesystem larger than the :term:`main memory`, than can contain
        :term:`main memory` archives, :term:`add-ins <add-in>` and more.

        Used for larger files with fewer changes during the calculator's
        lifetime. Pricier calculators use a multi-MiB section on the flash
        memory to provide a storage memory, and some calculators have a
        secondary storage memory in the form of an SD card slot.

    Transport
        Transport underlying a link, composed of a physical transport
        (:term:`connector` and cable set) as well as the system or hardware
        interface (system interface, driver) used to access it.

    UMS
        See :term:`USB Mass Storage`.

    USB Mass Storage
        Used in this documentation to refer to both
        :ref:`UMS as transport <protocol-topic-transport-ums>` and
        :ref:`UMS as protocol <protocol-topic-ums>`.

Acknowledgements
----------------

There have been many projects over the years about reversing and
reimplementing CASIO's shenannigans for using their calculators from
alternative OSes, or simply for fun or out of curiosity. Cahute couldn't
have been made without their research, implementations and in some cases,
documentation. This page is a little tribute to these works.

* Thanks to Tom Wheeley and Tom Lynn for their work on CaS and Caspro_
  and the `Casio Graphical Calculator Encyclopaedia`_.
* Thanks to the (now defunct) Graph100.com wiki, `saved here
  <Graph100.com Wiki_>`_ for historical purposes.
* Thanks to the team behind Casetta_ for their documentation on legacy
  protocols and file formats, which helped me navigate the subtleties more
  easily.
* Thanks to `Simon Lothar`_ and Andreas Bertheussen for their work on
  Protocol 7.00 and derivatives through fxReverse_ and xfer9860, and to
  Teamfx_ for `their additions <Teamfx additions_>`_.
* Thanks to the Cemetech community for their `Prizm Wiki`_, especially
  gbl08ma, BrandonWilson and amazonka.
* Thanks to Nessotrin_ for their work on UsbConnector_, which prompted me
  to work on a better version in the first place.
* Thanks to Massena_ for their work on the Cahute cover image, on the top of
  every page.

There are obviously plenty more people working on other connected aspects
(hardware, low-level system stuff), administering or moderating forums and
websites, maintaining communication with CASIO and other partners.
Quoting you all would take a substantial time, and I'd likely miss quite a lot
of you, but thank you all for your efforts!

.. [#mutant] Icons used here are from the `Mutant Standard`_,
  licensed under `CC BY-NC-SA 4.0 International`_.
  Copyright © 2017 - 2024 \ `Caius Nocturne`_.

.. |feat-transfer| image:: feat-transfer.svg
.. |feat-program| image:: feat-program.svg
.. |feat-text| image:: feat-text.svg
.. |feat-ohp| image:: feat-ohp.svg
.. |feat-flash| image:: feat-flash.svg

.. _Cahute on Gitlab: https://gitlab.com/cahute/cahute
.. _Thomas Touhey: https://thomas.touhey.fr/
.. _CeCILL: http://www.cecill.info/licences.en.html
.. _Diátaxis: https://diataxis.fr/

.. _Simon Lothar:
    https://www.casiopeia.net/forum/memberlist.php?mode=viewprofile&u=10405
.. _Teamfx:
    https://www.casiopeia.net/forum/memberlist.php?mode=viewprofile&u=10504&sid=b1f4fb842b29e6f686d832a7e1117789
.. _Nessotrin:
    https://www.planet-casio.com/Fr/compte/voir_profil.php?membre=nessotrin

.. _Casio Graphical Calculator Encyclopaedia:
    https://serval.mythic-beasts.com/~tom/calcs/calcs/encyc/
.. _Graph100.com Wiki:
    https://bible.planet-casio.com/cakeisalie5/websaves/graph100.com/
.. _fxReverse:
    https://bible.planet-casio.com/simlo/fxreverse/fxReverse2.pdf
.. _Teamfx additions: https://bible.planet-casio.com/teamfx/
.. _Prizm Wiki: https://prizm.cemetech.net/
.. _UsbConnector:
    https://www.planet-casio.com/Fr/forums/topic13656-1-usbconnector
    -remplacement-de-fa124-multi-os.html
.. _Massena: https://pannocat.to/

.. _Casetta: https://casetta.tuxfamily.org/
.. _Caspro:
    https://web.archive.org/web/20160504230033/
    http://www.spiderpixel.co.uk:80/caspro/

.. _Mutant Standard: https://mutant.tech/
.. _Caius Nocturne: https://nocturne.works/
.. _CC BY-NC-SA 4.0 International:
    http://creativecommons.org/licenses/by-nc-sa/4.0/
