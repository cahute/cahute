.. _install-linux:

|linux| Installing Cahute on Linux distributions
================================================

In order to install Cahute's library and command-line utilities on
your Linux distribution, the following methods are available.

.. _install-linux-aur:

|archlinux| Installing Cahute on Archlinux and derivatives, using the AUR
-------------------------------------------------------------------------

Cahute and its command-line utilities are present on the
`Archlinux User Repository`_, you can pop up your favourite pacman frontend
and install the `cahute <cahute on AUR_>`_ package:

* Using paru_::

    paru -S cahute

* Using pikaur_::

    pikaur -S cahute

Once installed, it is recommended to add your user to the ``uucp`` group,
for access to serial and USB devices, by running the following command
**as root** then restarting your session::

    usermod -a -G uucp <your-username>

.. _install-linux-giteapc:

|lephe| Installing Cahute on any Linux distribution, using GiteaPC
------------------------------------------------------------------

Cahute and its command-line utilities are installable through GiteaPC_,
by running the following command:

.. parsed-literal::

    giteapc install cake/cahute@\ |version|

.. note::

    By default, the logging level is set to ``warning``, and the command-line
    utilities are stripped.

    If you wish to set the default logging level to ``info`` and keep the
    debug symbols, you can set the ``debug`` configuration by using the
    following command instead of the previous one:

    .. parsed-literal::

        giteapc install cake/cahute@\ |version|\ :debug

.. warning::

    If you are using GiteaPC on Linux, it is likely that your system is using
    udev_. If this is the case, you must move the udev rule from the user
    install directory to the system directory, by running the following
    command:

    .. code-block:: text

        sudo mv ~/.local/lib/udev/rules.d/*.rules /etc/udev/rules.d/

    From there, you must reload the rules to make sure they apply by running
    the following command:

    .. code-block:: text

        sudo udevadm control --reload-rules

    If your user isn't already in the ``uucp`` group, you must also make that
    the case by running the following command:

    .. code-block:: text

        sudo usermod -a -G uucp <your_username>

    Then restart the login session or host to ensure that the new group applies
    to your new session.

Installing Cahute on other distributions
----------------------------------------

.. note::

    This guide may not be exhaustive, and a package may exist for your
    distribution. Please check with your distribution's package registry
    and/or wiki before proceeding!

If no package exists for your distribution, or you are to package Cahute for
your distribution, you can build the project yourself.

See :ref:`build-linux` for more information.

.. _Archlinux User Repository: https://aur.archlinux.org/
.. _cahute on AUR: https://aur.archlinux.org/packages/cahute
.. _p7 on AUR: https://aur.archlinux.org/packages/p7
.. _p7screen on AUR: https://aur.archlinux.org/packages/p7screen
.. _paru: https://github.com/morganamilo/paru
.. _pikaur: https://github.com/actionless/pikaur
.. _GiteaPC: https://git.planet-casio.com/Lephenixnoir/giteapc
.. _udev: https://wiki.archlinux.org/title/Udev

.. |linux| image:: linux.svg
.. |archlinux| image:: arch.svg
.. |lephe| image:: lephe.png
