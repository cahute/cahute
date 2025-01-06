/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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

#include "internals.h"

/**
 * Close an AmigaOS serial port.
 *
 * @param context Context in which the function is called.
 * @param cookie Cookie.
 */
CAHUTE_EXTERN(void)
cahute_close_amigaos_serial_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie
) {
    AbortIO((struct IORequest *)cookie->io);
    WaitIO((struct IORequest *)cookie->io);
    CloseDevice((struct IORequest *)cookie->io);
    DeleteIORequest(cookie->io);
    DeleteMsgPort(cookie->msg_port);
}

/**
 * Receive data on an AmigaOS serial link.
 *
 * @param context Context on which the function is called.
 * @param cookie Cookie.
 * @param buf Buffer in which to read.
 * @param capacity Capacity of the buffer.
 * @param readp Pointer to the read bytes count to set.
 * @param timeout Timeout; 0 for infinite.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_receive_on_amigaos_serial_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *readp,
    unsigned long timeout
) {
    struct timerequest *timer;
    struct IOExtSer *io = cookie->io;
    struct MsgPort *timer_msgport, *serial_msgport = cookie->msg_port;
    cahute_u32 signals = 0;
    int err, has_serial = 0;

    err = cahute_get_amiga_timer(context, &timer_msgport, &timer);
    if (err)
        return err;

    /* Run two operations at once:
     * - Read into the buffer.
     * - Start a timer to the currently requested timeout. */
    io->IOSer.io_Command = CMD_READ;
    io->IOSer.io_Length = capacity;
    io->IOSer.io_Data = (APTR)buf;

    SendIO((struct IORequest *)io);
    signals = (1L << serial_msgport->mp_SigBit) | SIGBREAKF_CTRL_C;

    if (timeout > 0) {
        timer->tr_time.tv_secs = timeout / 1000;
        timer->tr_time.tv_micro = timeout % 1000 * 1000;
        timer->tr_node.io_Command = TR_ADDREQUEST;

        SendIO((struct IORequest *)timer);
        signals |= (1L << timer_msgport->mp_SigBit);
    }

    if (CheckIO((struct IORequest *)io)) {
        /* Request has terminated immediately.
         * Note that we may have started a timer request for nothing
         * here, but we want to have this CheckIO() call as close as
         * possible to the Wait() to avoid race conditions as much
         * as possible. */
        signals = 0;
        has_serial = 1;
    } else {
        /* We want to wait only if the request has not finished
         * immediately. */
        signals = Wait(signals);
        has_serial = CheckIO((struct IORequest *)io) ? 1 : 0;
    }

    if (timeout > 0) {
        if (!CheckIO((struct IORequest *)timer))
            AbortIO((struct IORequest *)timer);

        WaitIO((struct IORequest *)timer);
    }

    /* Wait for either completion and clearing of serial read, or
     * for cancellation of I/O request.
     * This is required for refreshing the buffer and 'io_Actual'. */
    if (!has_serial)
        AbortIO((struct IORequest *)io);

    WaitIO((struct IORequest *)io);

    if (signals & SIGBREAKF_CTRL_C)
        return CAHUTE_ERROR_ABORT;
    else if (!has_serial)
        return CAHUTE_ERROR_TIMEOUT;

    if (io->IOSer.io_Error) {
        msg(context,
            ll_error,
            "Error %d occurred while reading from device.",
            io->IOSer.io_Error);
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* I/O request was completed, we want to read the contents. */
    if (io->IOSer.io_Error) {
        msg(context,
            ll_error,
            "Error %d occurred while reading from device.",
            io->IOSer.io_Error);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *readp = io->IOSer.io_Actual;
    return CAHUTE_OK;
}

/**
 * Send on an AmigaOS serial link.
 *
 * @param context Context in which the function is called.
 * @param cookie Cookie.
 * @param buf Buffer to send.
 * @param size Size of the buffer to send.
 * @param writtenp Pointer to the written bytes count to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_send_on_amigaos_serial_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *writtenp
) {
    struct IOExtSer *io = cookie->io;

    io->IOSer.io_Length = size;
    io->IOSer.io_Data = (cahute_u8 *)buf; /* Explicit non-const. */
    io->IOSer.io_Command = CMD_WRITE;
    if (DoIO((struct IORequest *)io)) {
        msg(context, ll_error, "Unable to set the serial parameters!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    *writtenp = size;
    return CAHUTE_OK;
}

/**
 * Set serial params on an AmigaOS serial link.
 *
 * @param context Context in which the function is called.
 * @param cookie Cookie.
 * @param flags Flags to set.
 * @param speed Speed / baud rate to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_set_amigaos_serial_link_params(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
) {
    struct IOExtSer *io = cookie->io;

    io->io_CtlChar = 0x00001311;
    io->io_RBufLen = 1024;
    io->io_ExtFlags = 0;
    io->io_Baud = speed;
    io->io_BrkTime = 250000;
    io->io_TermArray.TermArray0 = 0;
    io->io_TermArray.TermArray1 = 0;
    io->io_ReadLen = 8;
    io->io_WriteLen = 8;
    io->io_SerFlags = SERF_SHARED;
    io->io_Status = 0;

    switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
    case CAHUTE_SERIAL_XONXOFF_DISABLE:
        io->io_SerFlags |= SERF_XDISABLED;
        break;
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case CAHUTE_SERIAL_STOP_ONE:
        io->io_StopBits = 1;
        break;

    case CAHUTE_SERIAL_STOP_TWO:
        io->io_StopBits = 2;
        break;
    }

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        io->io_SerFlags |= SERF_PARTY_ON;
        break;

    case CAHUTE_SERIAL_PARITY_ODD:
        io->io_SerFlags |= SERF_PARTY_ON | SERF_PARTY_ODD;
        break;
    }

    if ((flags & CAHUTE_SERIAL_DTR_MASK) != CAHUTE_SERIAL_DTR_IGNORE) {
        /* TODO */
        CAHUTE_RETURN_IMPL(context, "DTR line control not implemented yet.");
    }

    if ((flags & CAHUTE_SERIAL_RTS_MASK) != CAHUTE_SERIAL_RTS_IGNORE) {
        /* TODO */
        CAHUTE_RETURN_IMPL(context, "RTS line control not implemented yet.");
    }

    io->IOSer.io_Command = SDCMD_SETPARAMS;
    if (DoIO((struct IORequest *)io)) {
        msg(context, ll_error, "Unable to set the serial parameters!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/* AmigaOS serial link callbacks. */
CAHUTE_LOCAL_DATA(cahute_serial_link_interface)
amigaos_serial_link_interface = {
    "Serial (AmigaOS)",
    (cahute_link_close_func *)cahute_close_amigaos_serial_link,
    (cahute_link_receive_func *)cahute_receive_on_amigaos_serial_link,
    (cahute_link_send_func *)cahute_send_on_amigaos_serial_link,
    (cahute_link_set_serial_params_func *)cahute_set_amigaos_serial_link_params
};

/**
 * Get the AmigaOS serial port from the raw device name.
 *
 * This function expects a format such as "U=<unit>" or "UNIT=<unit>",
 * case-insensitive;
 *
 * @param raw Raw device name.
 * @param unitp Pointer to the unit number to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
get_amigaos_serial_port(char const *raw, unsigned long *unitp) {
    unsigned long unit;

    if (tolower(raw[0]) == 'u' && raw[1] == '=')
        raw += 2;
    else if (tolower(raw[0]) == 'u' && tolower(raw[1]) == 'n' && tolower(raw[2]) == 'i' && tolower(raw[3]) == 't' && raw[4] == '=')
        raw += 5;
    else {
        /* Unknown port format. */
        return CAHUTE_ERROR_NOT_FOUND;
    }

    if (!isdigit(raw[0]))
        return CAHUTE_ERROR_NOT_FOUND;

    unit = raw[0] - '0';
    while (*++raw) {
        if (!isdigit(*raw))
            return CAHUTE_ERROR_NOT_FOUND;

        unit = unit * 10 + (raw[0] - '0');
    }

    *unitp = unit;
    return CAHUTE_OK;
}

/**
 * Open an AmigaOS serial link.
 *
 * @param context Context on which to open a serial link.
 * @param open_params Opening parameters to transmit.
 * @param name_or_path Name or path to the serial port to open.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_amigaos_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
) {
    cahute_amigaos_serial_link_cookie cookie;
    struct MsgPort *msg_port;
    struct IOExtSer *io;
    unsigned long unit;
    int ret;

    /* The serial device name is the unit number for the serial device. */
    ret = get_amigaos_serial_port(name_or_path, &unit);
    if (ret)
        return ret;

    msg_port = CreateMsgPort();
    if (!msg_port) {
        msg(context, ll_error, "Could not open message port.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    io = CreateIORequest(msg_port, sizeof(struct IOExtSer));
    if (!io) {
        msg(context, ll_error, "Could not create IORequest.");
        DeleteMsgPort(msg_port);
        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(context, ll_info, "Opening DEVICE=%s,UNIT=%lu.", SERIALNAME, unit);
    ret =
        OpenDevice((CONST_STRPTR)SERIALNAME, unit, (struct IORequest *)io, 0L);
    if (ret) {
        msg(context,
            ll_error,
            "Error %d has occurred while opening DEVICE=%s,UNIT=%lu.",
            ret,
            SERIALNAME,
            unit);

        if (ret == IOERR_BADADDRESS)
            ret = CAHUTE_ERROR_NOT_FOUND;
        else if (ret == IOERR_UNITBUSY)
            ret = CAHUTE_ERROR_BUSY;
        else
            ret = CAHUTE_ERROR_UNKNOWN;

        DeleteIORequest(io);
        DeleteMsgPort(msg_port);
        return ret;
    }

    cookie.msg_port = msg_port;
    cookie.io = io;
    return cahute_open_serial_link_from_interface(
        open_params,
        &amigaos_serial_link_interface,
        &cookie,
        sizeof(cookie)
    );
}
