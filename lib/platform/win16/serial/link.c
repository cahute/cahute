/* ****************************************************************************
 * Copyright (C) 2025 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#include "../internals.h"
#define READ_CHECK_DELAY_MS 20

CAHUTE_DECLARE_TYPE(cahute_win16_serial_link_cookie)

/**
 * Win16 serial cookie.
 *
 * @property cid Communication port identifier.
 */
struct cahute_win16_serial_link_cookie {
    int cid;
};

/**
 * Close a Win16 serial or CESG link.
 *
 * @param context
 * @param cookie
 */
CAHUTE_LOCAL(void)
close_link(cahute_context *context, cahute_win16_serial_link_cookie *cookie) {
    CloseComm(cookie->cid);
}

/**
 * Receive bytes on a Win16 serial link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer in which to receive.
 * @param capacity Capacity of the buffer.
 * @param receivedp Pointer to the received bytes count to set.
 * @param timeout Timeout; 0 for infinite.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
receive_on_link(
    cahute_context *context,
    cahute_win16_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    int received, err;

    /* The Win16 API does not provide timeout management other than those
     * handled with DTR/DSR and RTS/CTS flows, and Windows 1.x doesn't seem
     * to have WM_COMMNOTIFY.
     *
     * So, we want to peek periodically at the internal buffer size using
     * the COMSTAT filled by GetCommError(), to check if there is data
     * in the input buffer. */
    if (timeout) {
        COMSTAT stat;
        long end = GetCurrentTime() + timeout, tm;

        while (1) {
            GetCommError(cookie->cid, &stat);
            if (stat.cbInQue) {
                if (stat.cbInQue < capacity)
                    capacity = stat.cbInQue;

                break;
            }

            tm = GetCurrentTime();
            if (tm + READ_CHECK_DELAY_MS >= end)
                return CAHUTE_ERROR_TIMEOUT;

            err = cahute_sleep(context, READ_CHECK_DELAY_MS);
            if (err)
                return err;
        }
    }

    received = ReadComm(cookie->cid, buf, capacity);
    if (received < 0) {
        err = GetCommError(cookie->cid, NULL);
        msg(context, ll_error, "ReadComm() returned error %d", err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *receivedp = (size_t)received;
    return CAHUTE_OK;
}

/**
 * Send bytes on a Win16 serial link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer to send.
 * @param size Size of the buffer to send.
 * @param sentp Pointer to the written bytes count to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
send_on_link(
    cahute_context *context,
    cahute_win16_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
) {
    int sent, err;

    sent = WriteComm(cookie->cid, buf, size);
    if (sent < 0) {
        err = GetCommError(cookie->cid, NULL);
        msg(context, ll_error, "WriteComm() returned error %d", err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *sentp = (size_t)sent;
    return CAHUTE_OK;
}

/**
 * Set serial params on a Win16 serial link.
 *
 * @param context
 * @param cookie
 * @param flags Flags on which to set
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
set_serial_params_on_link(
    cahute_context *context,
    cahute_win16_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
) {
    DCB dcb;
    int err;
    DWORD baudrate;

#if defined(CBR_110)
    switch (speed) {
    case 110:
        baudrate = CBR_110;
        break;

    case 300:
        baudrate = CBR_300;
        break;

    case 600:
        baudrate = CBR_600;
        break;

    case 1200:
        baudrate = CBR_1200;
        break;

    case 2400:
        baudrate = CBR_2400;
        break;

    case 4800:
        baudrate = CBR_4800;
        break;

    case 9600:
        baudrate = CBR_9600;
        break;

    case 14400:
        baudrate = CBR_14400;
        break;

    case 19200:
        baudrate = CBR_19200;
        break;

    case 38400:
        baudrate = CBR_38400;
        break;

    case 56000:
        baudrate = CBR_56000;
        break;

    case 128000:
        baudrate = CBR_128000;
        break;

    case 256000:
        baudrate = CBR_256000;
        break;

    default:
        msg(context, ll_error, "Unsupported speed: %lu", speed);
        return CAHUTE_ERROR_UNKNOWN;
    }
#else
    baudrate = (DWORD)speed;
#endif

    err = GetCommState(cookie->cid, &dcb);
    if (err) {
        msg(context, ll_error, "GetCommState() returned error %d", err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.RlsTimeout = INFINITE;
    dcb.CtsTimeout = INFINITE;
    dcb.DsrTimeout = INFINITE;
    dcb.fBinary = TRUE;
    dcb.fParity = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fPeChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fChEvt = FALSE;
    dcb.XonChar = 0x13;
    dcb.XoffChar = 0x11;
    dcb.XonLim = 0;
    dcb.XoffLim = 0;
    dcb.PeChar = 0;
    dcb.EvtChar = 0;
    dcb.EofChar = 0;
    dcb.TxDelay = INFINITE;

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        dcb.Parity = EVENPARITY;
        break;

    case CAHUTE_SERIAL_PARITY_ODD:
        dcb.Parity = ODDPARITY;
        break;

    default:
        dcb.Parity = NOPARITY;
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case CAHUTE_SERIAL_STOP_ONE:
        dcb.StopBits = ONESTOPBIT;
        break;

    default:
        dcb.StopBits = TWOSTOPBITS;
    }

    switch (flags & CAHUTE_SERIAL_RTS_MASK) {
    case CAHUTE_SERIAL_RTS_DISABLE:
        dcb.fRtsDisable = TRUE;
        break;

    case CAHUTE_SERIAL_RTS_ENABLE:
        dcb.fRtsDisable = FALSE;
        break;

    case CAHUTE_SERIAL_RTS_HANDSHAKE:
        CAHUTE_RETURN_IMPL(
            context,
            "RTS handshake is not supported on Win16."
        );
    }

    switch (flags & CAHUTE_SERIAL_DTR_MASK) {
    case CAHUTE_SERIAL_DTR_DISABLE:
        dcb.fDtrDisable = TRUE;
        break;

    case CAHUTE_SERIAL_DTR_ENABLE:
        dcb.fDtrDisable = FALSE;
    }

    switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
    case CAHUTE_SERIAL_XONXOFF_DISABLE:
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
        break;

    case CAHUTE_SERIAL_XONXOFF_ENABLE:
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
    }

    err = SetCommState(&dcb);
    if (err) {
        msg(context, ll_error, "SetCommState() returned error %d", err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    err = FlushComm(cookie->cid, 0);
    if (err) {
        msg(context, ll_error, "FlushComm(0) returned error %d", err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    err = FlushComm(cookie->cid, 1);
    if (err) {
        msg(context, ll_error, "FlushComm(1) returned error %d", err);
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/* Win16 serial link callbacks. */
CAHUTE_LOCAL_DATA(cahute_serial_link_interface)
win16_serial_link_interface = {
    "Serial (Win16)",
    (cahute_link_close_func *)&close_link,
    (cahute_link_receive_func *)&receive_on_link,
    (cahute_link_send_func *)&send_on_link,
    (cahute_link_set_serial_params_func *)&set_serial_params_on_link
};

/**
 * Open a Win16 serial link.
 *
 * @param context
 * @param open_params
 * @param name_or_path
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_open_win16_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
) {
    cahute_win16_serial_link_cookie cookie;
    int cid;

    cid = OpenComm(name_or_path, 4096, 4096);
    if (cid < 0) {
        msg(context, ll_error, "OpenComm() returned error %d", cid);
        return CAHUTE_ERROR_UNKNOWN;
    }

    cookie.cid = cid;
    return cahute_open_serial_link_from_interface(
        open_params,
        &win16_serial_link_interface,
        &cookie,
        sizeof(cookie)
    );
}
