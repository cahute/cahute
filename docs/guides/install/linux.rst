.. _install-guide-linux:

|linux| Installing Cahute on Linux distributions
================================================

In order to install Cahute's library and/or command-line utilities on
your :ref:`Linux distribution <feature-topic-system-linux>`, the following
methods are available.

.. _install-guide-linux-aur:

|archlinux| Installing Cahute on Archlinux and derivatives for local use and development, using the AUR
-------------------------------------------------------------------------------------------------------

The Cahute library and command-line utilities can be installed through
the following packages on the `Archlinux User Repository`_:

`cahute <cahute on AUR_>`_ (**recommended**)
    Main package based on the latest release (|version| at time of writing).

`cahute-git <cahute-git on AUR_>`_
    Alternative package based on the latest development version, i.e.
    latest commit on the ``develop`` branch of the Git repository.

In order to install any of them, you can pop up your favourite pacman frontend
and install the package:

* Using paru_::

    paru -S cahute

* Using pikaur_::

    pikaur -S cahute

Once installed, it is recommended to add your user to the ``uucp`` group,
for access to serial and USB devices, by running the following command
**as root**::

    usermod -a -G uucp <your-username>

Then, you must restart your session, either by logging off and on again, or
by rebooting. You should then have access to the device!

.. _install-guide-linux-aur-mingw:

|archlinux| Installing Cahute on Archlinux and derivatives for MinGW development, using the AUR
-----------------------------------------------------------------------------------------------

The Cahute library can be installed for MinGW development, i.e.
cross-development for :ref:`Win32/Win64 <feature-topic-system-win32>`
using the `MinGW-w64`_ project, through the following packages
on the `Archlinux User Repository`_:

`mingw-w64-cahute <mingw-w64-cahute on AUR_>`_ (**recommended**)
    Main package based on the latest release (|version| at time of writing).

`mingw-w64-cahute-git <mingw-w64-cahute-git on AUR_>`_
    Alternative package based on the latest development version, i.e.
    latest commit on the ``develop`` branch of the Git repository.

In order to install any of them, you can pop up your favourite pacman frontend
and install the package:

* Using paru_::

    paru -S mingw-w64-cahute

* Using pikaur_::

    pikaur -S mingw-w64-cahute

.. _install-guide-linux-deb:

|debian| Installing Cahute on Debian and derivatives for local use and development, using APT
---------------------------------------------------------------------------------------------

.. warning::

    This method is not yet available; see `#8
    <https://gitlab.com/cahute/cahute/-/issues/8>`_ for more
    information.

    In the mean time, you can use one of the following methods as an
    alternative:

    * :ref:`install-guide-linux-giteapc`
    * :ref:`install-guide-linux-other`

.. _install-guide-linux-redhat:

|redhat| Installing Cahute on RHEL and derivatives for local use and development, using RPM
-------------------------------------------------------------------------------------------

.. warning::

    This method is not yet available; see `#85
    <https://gitlab.com/cahute/cahute/-/issues/85>`_ for more
    information.

    In the mean time, you can use one of the following methods as an
    alternative:

    * :ref:`install-guide-linux-giteapc`
    * :ref:`install-guide-linux-other`

.. _install-guide-linux-void:

|void| Installing Cahute on Voidlinux and derivatives for local use and development, using XBPS
-----------------------------------------------------------------------------------------------

.. warning::

    This method is not yet available; see `#72
    <https://gitlab.com/cahute/cahute/-/issues/72>`_ for more
    information.

    In the mean time, you can use one of the following methods as an
    alternative:

    * :ref:`install-guide-linux-giteapc`
    * :ref:`install-guide-linux-other`

.. _install-guide-linux-giteapc:

|lephe| Installing Cahute on any Linux distribution for local use and development, using GiteaPC
------------------------------------------------------------------------------------------------

Cahute and its command-line utilities are installable through GiteaPC_,
by running the following command:

.. parsed-literal::

    giteapc install cahute/cahute@\ |version|

.. warning::

    **Do not close the terminal window once the command has finished.**
    You may need information displayed in the log to continue following
    this guide.

.. note::

    By default, the logging level is set to ``warning``, and the command-line
    utilities are stripped.

    If you wish to set the default logging level to ``info`` and keep the
    debug symbols, you can set the ``debug`` configuration by using the
    following command instead of the previous one:

    .. parsed-literal::

        giteapc install cahute/cahute@\ |version|\ :debug

Within the installation guides, you may see a warning such as the following:

.. code-block:: text

    ********************************************************************

    The udev rules will be installed in the following directory:

        /home/your_user/.local/lib/udev/rules.d/

    You will need to copy them using the following command:

        sudo cp /home/your_user/.local/lib/udev/rules.d/*.rules /etc/udev/rules.d/

    Once installed, you will need to either reboot your computer,
    or run the following command for the rules to be taken into
    account:

        sudo udevadm control --reload-rules

    Then, add your user to the 'uucp' group, by running
    the following command:

        sudo usermod -a -G uucp your_user

    Finally, either reboot your computer, or log off then onto the
    computer for the new group to be taken into account.

    ********************************************************************

If this is the case, that means udev_ has been detected and support for it
in Cahute has been enabled, thus, you must follow the instructions given in
this block.

.. warning::

    Depending on your Linux distribution, the group might not be ``uucp``,
    but another value such as ``dialout``. The safest option is to copy the
    command directly!

.. _install-guide-linux-giteapc-gint:

|lephe| Installing Cahute on any Linux distribution for gint development, using GiteaPC
---------------------------------------------------------------------------------------

.. warning::

    This method is not yet available; see `#113
    <https://gitlab.com/cahute/cahute/-/issues/113>`_ for more
    information.

.. _install-guide-linux-other:

|linux| Installing Cahute on other distributions
------------------------------------------------

.. note::

    This guide may not be exhaustive, and a package may exist for your
    distribution. Please check with your distribution's package registry
    and/or wiki before proceeding!

If no package exists for your distribution and/or use case, or you are to
package Cahute for your distribution, you can build the project yourself.

See :ref:`build-guide-linux` for more information.

.. _Archlinux User Repository: https://aur.archlinux.org/
.. _MinGW-w64: https://www.mingw-w64.org/
.. _cahute on AUR: https://aur.archlinux.org/packages/cahute
.. _cahute-git on AUR: https://aur.archlinux.org/packages/cahute-git
.. _mingw-w64-cahute on AUR:
    https://aur.archlinux.org/packages/mingw-w64-cahute
.. _mingw-w64-cahute-git on AUR:
    https://aur.archlinux.org/packages/mingw-w64-cahute-git
.. _p7 on AUR: https://aur.archlinux.org/packages/p7
.. _p7screen on AUR: https://aur.archlinux.org/packages/p7screen
.. _paru: https://github.com/morganamilo/paru
.. _pikaur: https://github.com/actionless/pikaur
.. _GiteaPC: https://git.planet-casio.com/Lephenixnoir/giteapc
.. _udev: https://wiki.archlinux.org/title/Udev

.. |linux| image:: linux.svg
.. |debian| image:: debian.svg
.. |redhat| image:: redhat.svg
.. |archlinux| image:: arch.svg
.. |void| image:: voidlinux.svg
.. |lephe| image:: lephe.png
