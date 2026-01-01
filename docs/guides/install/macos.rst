.. _install-guide-macos:

|apple| Installing Cahute on macOS / OS X
=========================================

In order to install Cahute's library and command-line utilities on
:ref:`macOS / OS X <feature-topic-system-macos>`, the following methods
are available.

.. _install-guide-macos-homebrew:

|homebrew| Installing Cahute on macOS / OS X for local use and development, using Homebrew
------------------------------------------------------------------------------------------

Cahute and its command-line utilities can be installed for local use and
development, using Homebrew_.

Once Homebrew is installed, **disconnect all calculators from your computer**
and install the `cahute formula <cahute homebrew formula_>`_ with the
following command::

    brew install cahute

.. warning::

    The installation requires that no calculator is currently connected
    to your computer through USB; having one currently in receive mode may
    result in the installation failing.

.. _Homebrew: https://brew.sh/
.. _cahute homebrew formula: https://formulae.brew.sh/formula/cahute

.. |apple| image:: apple.svg
.. |homebrew| image:: homebrew.svg
