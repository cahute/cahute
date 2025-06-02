.. _file-format-topic-eam:

eAct Maker file
===============

`eAct Maker`_, a community-made tool for storing a document, is a tool that
allows converting simple text into other document formats readable by
calculators, including :ref:`G1E and G2E <file-format-topic-g1e>`, FLS, XCP
and :ref:`CAT <file-format-topic-cat>`.

It also allows saving the editor's content before exporting for later loading
and modification, since the editor does not allow importing any of the export
formats. This format is saved using the ``.<ext>.eam`` extension, e.g.
``.g2e.eam``, and is formatted using the following format:

* Destination format, among ``g1e``, ``g2e``, ``g3e``, ``fls``, ``xcp``, or
  ``cat``;
* ``0x1E`` separator;
* Title, e.g. ``example``;
* ``0x1E`` separator;
* Additional data, e.g. password for the program if CAT export is selected;
* ``0x1E`` separator;
* Contents.

The title, additional data and contents are UTF-8 encoded.

.. _eAct Maker: https://tools.planet-casio.com/EactMaker/
