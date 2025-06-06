.. _internals-topic-win32:

Win32 specific implementation details
=====================================

This document details implementation details specific to
:ref:`feature-topic-system-windows`.

Serial device management
------------------------

Windows NT can handle serial devices, and provide specific interfaces to
communicate with them.

For more information, see `Serial Communications in Win32`_.

Serial device detection using the Win32 API
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to list available serial devices using the Win32, the
``HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM`` registry key
contents is read, therefore making the same operations as |GetCommPorts|_
with better compatibility.

.. _internals-topic-win32-serial-link:

Serial link handling using the Win32 API
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cahute can open a serial link using the Win32 API with the
``cahute_open_win32_serial_link()`` function.

This function interprets the provided name or path as a path,
and attempts at opening the device using |CreateFile|_.
If it succeeds, it calls |SetCommTimeouts|_ with ``ReadTimeoutInterval`` set
to ``MAXDWORD`` in order to only read what is directly available, and create
the event for the overlapped object using |CreateEvent|_.

If it succeeds, the link can then be opened, with a |HANDLE|_ with
`Overlapped I/O`_ and an internal buffer.

The available operations use :

* Closing uses |CloseHandle|_ and, optionally, |CancelIo|_;
* Receiving uses |ReadFile|_, |WaitForSingleObject|_, and
  |GetOverlappedResult|_;
* Sending uses |WriteFile|_ and |WaitForSingleObject|_, and depending
  on whether the second function succeeded or not, either
  |GetOverlappedResult|_ or |CancelIo|_, to ensure we don't have any
  buffer reads post-freeing the link;
* Serial params setting uses |SetCommState|_.

.. note::

    If |WaitForSingleObject|_ on receiving ends with ``WAIT_TIMEOUT``, i.e.
    if a timeout has occurred, rather than cancelling the call, the
    respective function lets the call continue in the background.

    This is because the function is also used by the CESG driver usage
    implementation in Cahute, which may crash under some circumstances
    when trying to cancel an overlapped read call; see
    `#17 <https://gitlab.com/cahute/cahute/-/issues/17>`_
    for more information.

    The same overlapped event is used between calls to the receive
    implementation, until it completes or the link is closed.

    Rather than reading directly in the provided buffer, the implementation
    reads into an internal buffer first, then copies the contents read
    in the internal buffer in the provided buffer. This is because, while
    the provided buffer may change from one call to the other, the internal
    buffer doesn't, and the operation can safely continue asynchronously
    in between calls.

USB device management
---------------------

Win32 implements USB devices through drivers, either generic or specific.

For now, most of the USB device management is done using libusb; see
:ref:`internals-topic-libusb` for more information.

.. note::

    In order to support older versions of the platform not supported by libusb,
    such as Windows 2000, we are to implement USB device management on Win32
    without libusb; see `#73
    <https://gitlab.com/cahute/cahute/-/issues/73>`_ for more
    information.

.. _internals-topic-win32-usb-drivers:

USB driver support
~~~~~~~~~~~~~~~~~~

Cahute can communicate with USB calculators through the following drivers:

.. list-table::
    :header-rows: 1

    * - Name
      - Description
      - Compatibility
      - Supported
    * - Generic volume driver
      - The generic volume driver, used when the device presents an UMS
        interface through its descriptors.
      - All versions
      - Yes
    * - CESG502_
      - CASIO's official driver provided with `FA-124`_.

        See :ref:`internals-topic-win32-cesg` for more information.
      - Windows 2000 (NT 5.0)+
      - Yes
    * - WinUSB_
      - Official generic USB device driver by Microsoft.

        Can be selected automatically if the calculator presents WCID_
        attributes.
      - Windows Vista (NT 6.0)+
      - Through libusb_
    * - libusbK_
      - KMF-based driver that comes with the eponym library, with the same
        interface as WinUSB_.
      - Windows XP (NT 5.1)+
      - Through libusb_
    * - `libusb-win32`_
      - Driver that comes with the eponym library.
      - Windows 2000 (NT 5.0)+
      - Through libusb_
    * - UsbDk_
      - Driver that comes with the eponym library.

        libusb_ loads a dedicated library when using it.
      - Windows XP (NT 5.1)+
      - Through libusb_

See `libusb-compatible kernel drivers`_ for more information.

.. _internals-topic-win32-cesg:

Device communication using CESG502
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CESG502 is distributed with `FA-124`_, and is necessary for CASIO's software
to successfully detect and communicate calculators connected using USB.
This means it is necessary to support it for any user program that co-exists
with it to work with CASIO's driver.

CESG502 abstracts both :ref:`protocol-topic-transport-serial-over-usb-bulk` and
:ref:`protocol-topic-transport-ums` behind a stream-oriented device.
It can be detected using libusb, but cannot be opened using the same tool;
one must use detection with SetupAPI_ or cfgmgr32_, check that the device
driver is CESG502, and if it's the case, open and use the device using
fileapi_ (``CreateFile``, ``ReadFile``, ``WriteFile``, ``CloseFile``).

.. note::

    It is possible to access device instance properties on Windows OSes
    before Vista, e.g. Windows XP; see `Accessing Device Instance Properties
    (Prior to Windows Vista)`_ for more information.

It uses ``{36fc9e60-c465-11cf-8056-444553540000}``, the same GUID as
generic USB devices, which is normally forbidden for Independent
Hardware Vendors (IHV) such as CASIO, so **this key cannot be used to
uniquely identify the driver**.

Cahute currently matches the service (``CM_DRP_SERVICE``) to ``PVUSB``,
since this is the value encountered in the wild.

.. _internals-topic-win32-device-address:

Device opening using a device address
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We can attempt to find a device through the Win32 API using CfgMgr32_:

* If the underlying driver to the device is identified as CESG502,
  we open a serial over USB bulk device with the CESG implementation;
* Otherwise, we look for disk drive then volume devices via bus
  relations, and use the volume device interface as UMS.

Serial over bulk link implementation, using the CESG driver
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Devices using the CESG502 driver, once opened, behaves mostly like native
serial devices; see :ref:`internals-topic-win32-serial-link` for more information.
It however has the following differences:

* Serial specific parameters are not available (as this is USB);
* It does the :ref:`device enabling control flow
  <protocol-topic-transport-serial-over-usb-bulk-enable-control-flow>`
  automatically when the device is connected;
* On reading, the provided buffer must be large enough to get all of the
  available data at once.

  If using :ref:`Protocol 7.00 <protocol-topic-seven>`, 4096 bytes is enough,
  however in other contexts such as :ref:`Protocol 7.00 Screenstreaming
  <protocol-topic-seven-ohp>`, 32768 bytes is safer.

UMS link implementation
~~~~~~~~~~~~~~~~~~~~~~~

Devices using the native SCSI driver (abstracting UMS away), usually through
the volume manager (``VOLMGR``), can be used to implement calculators
presenting UMS.

Once opened using a |HANDLE|_, operations use the following mechanisms:

* Closing uses |CloseHandle|_;
* Requesting using SCSI uses |DeviceIoControl|_ with
  |IOCTL_SCSI_PASS_THROUGH_DIRECT|_.

File handling using the Win32 API
---------------------------------

When creating or opening a file on Win32, |CreateFile|_ is called with the
appropriate options. Then, depending on the situation:

* On creation, we want to set the file size to the provided one.

  In order to do this, we call |SetFilePointer|_ to seek the provided file
  size from ``FILE_BEGIN``, |SetEndOfFile|_ to set the file size
  explicitely, and finally, |SetFilePointer|_ again to seek to ``FILE_BEGIN``.
* On reading, we want to get the current file size.

  In order to do this, we call |SetFilePointer|_ to seek 0 bytes from
  ``FILE_END``, which returns the current file size, then the
  same function to seek 0 bytes from ``FILE_BEGIN``.

.. note::

    Files are opened **without** exclusivity, meaning another program may
    modify the file while it is being read or written.

When opening standard output on Win32, we call |GetStdHandle|_ with
``STD_OUTPUT_HANDLE``.

Once a file or stdout is opened, we have a |HANDLE|_ we can then use with
the following operations:

* Closing uses |CloseHandle|_ (except for the standard output, which we
  must not close);
* Reading uses |ReadFile|_;
* Writing uses |WriteFile|_;
* Seeking uses |SetFilePointer|_.

.. |CancelIo| replace:: ``CancelIo``
.. |CloseHandle| replace:: ``CloseHandle``
.. |CreateEvent| replace:: ``CreateEvent``
.. |CreateFile| replace:: ``CreateFile``
.. |DeviceIoControl| replace:: ``DeviceIoControl``
.. |GetCommPorts| replace:: ``GetCommPorts``
.. |GetOverlappedResult| replace:: ``GetOverlappedResult``
.. |GetStdHandle| replace:: ``GetStdHandle``
.. |HANDLE| replace:: ``HANDLE``
.. |IOCTL_SCSI_PASS_THROUGH_DIRECT| replace:: ``IOCTL_SCSI_PASS_THROUGH_DIRECT``
.. |ReadFile| replace:: ``ReadFile``
.. |SetCommState| replace:: ``SetCommState``
.. |SetCommTimeouts| replace:: ``SetCommTimeouts``
.. |SetEndOfFile| replace:: ``SetEndOfFile``
.. |SetFilePointer| replace:: ``SetFilePointer``
.. |WaitForSingleObject| replace:: ``WaitForSingleObject``
.. |WriteFile| replace:: ``WriteFile``

.. _CESG502:
    https://www.planet-casio.com/Fr/logiciels/voir_un_logiciel_casio.php
    ?showid=75&page=10
.. _WinUSB:
    https://learn.microsoft.com/fr-fr/windows-hardware/drivers/usbcon/
    using-winusb-api-to-communicate-with-a-usb-device
.. _libusbK: https://libusbk.sourceforge.net/UsbK3/index.html
.. _libusb-win32: https://github.com/mcuee/libusb-win32/wiki#development
.. _UsbDk: https://github.com/daynix/UsbDk
.. _WCID: https://github.com/pbatard/libwdi/wiki/WCID-Devices
.. _libusb: https://libusb.info/

.. _fileapi: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/
.. _SetupAPI:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/setupapi
.. _cfgmgr32:
    https://learn.microsoft.com/en-us/windows/win32/api/cfgmgr32/
.. _Overlapped I/O:
    https://learn.microsoft.com/en-us/windows/win32/sync/
    synchronization-and-overlapped-input-and-output
.. _Serial Communications in Win32:
    https://learn.microsoft.com/en-us/previous-versions/ms810467(v=msdn.10)
.. _libusb-compatible kernel drivers:
    https://github.com/libusb/libusb/wiki/
    Windows#user-content-Driver_Installation
.. _FA-124:
    https://www.planet-casio.com/Fr/logiciels/voir_un_logiciel_casio.php
    ?showid=16
.. _Accessing Device Instance Properties (Prior to Windows Vista):
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/
    accessing-device-instance-spdrp-xxx-properties

.. _CancelIo:
    https://learn.microsoft.com/en-us/windows/win32/fileio/cancelio
.. _CloseHandle:
    https://learn.microsoft.com/en-us/windows/win32/api/
    handleapi/nf-handleapi-closehandle
.. _CreateEvent:
    https://learn.microsoft.com/en-us/windows/win32/api/
    synchapi/nf-synchapi-createeventa
.. _CreateFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-createfilea
.. _DeviceIoControl:
    https://learn.microsoft.com/en-us/windows/win32/api/
    ioapiset/nf-ioapiset-deviceiocontrol
.. _GetCommPorts:
    https://learn.microsoft.com/vi-vn/windows/win32/api/winbase/
    nf-winbase-getcommports
.. _GetOverlappedResult:
    https://learn.microsoft.com/en-us/windows/win32/api/
    ioapiset/nf-ioapiset-getoverlappedresult
.. _GetStdHandle:
    https://learn.microsoft.com/en-us/windows/console/getstdhandle
.. _HANDLE:
    https://learn.microsoft.com/en-us/windows/win32/sysinfo/handles-and-objects
.. _IOCTL_SCSI_PASS_THROUGH_DIRECT:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddscsi/
    ni-ntddscsi-ioctl_scsi_pass_through_direct
.. _ReadFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-readfile
.. _SetCommState:
    https://learn.microsoft.com/en-us/windows/win32/api/
    winbase/nf-winbase-setcommstate
.. _SetCommTimeouts:
    https://learn.microsoft.com/en-us/windows/win32/api/
    winbase/nf-winbase-setcommtimeouts
.. _SetEndOfFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-setendoffile
.. _SetFilePointer:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-setfilepointer
.. _WaitForSingleObject:
    https://learn.microsoft.com/en-us/windows/win32/api/
    synchapi/nf-synchapi-waitforsingleobject
.. _WriteFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-writefile
