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

#include "../internals.h"

CAHUTE_DECLARE_TYPE(cahute_amigaos_serial_link_cookie)

/**
 * AmigaOS serial link cookie definition.
 *
 * @property read_msg_port Message port for the read I/O request.
 * @property write_msg_port Message port for the write I/O request.
 * @property read_io Read I/O request.
 * @property write_io Write I/O request.
 */
struct cahute_amigaos_serial_link_cookie {
    struct MsgPort *read_msg_port;
    struct MsgPort *write_msg_port;
    struct IOExtSer *read_io;
    struct IOExtSer *write_io;
};

/**
 * Close an AmigaOS serial port.
 *
 * @param context Context in which the function is called.
 * @param cookie Cookie.
 */
CAHUTE_LOCAL(void)
close_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie
) {
    CloseDevice((struct IORequest *)cookie->read_io);

    DeleteIORequest(cookie->write_io);
    DeleteIORequest(cookie->read_io);
    DeleteMsgPort(cookie->write_msg_port);
    DeleteMsgPort(cookie->read_msg_port);
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
CAHUTE_LOCAL(int)
receive_on_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *readp,
    unsigned long timeout
) {
    struct timerequest *timer;
    struct IOExtSer *io = cookie->read_io;
    struct MsgPort *timer_msgport, *serial_msgport = cookie->read_msg_port;
    cahute_u32 signals = 0, obtained_signals = 0;
    size_t unread_bytes, read_bytes = 0;
    int err;

    /* We need to query the number of bytes currently unread.
     * If there is at least one, we read the maximum we can (either
     * limited by the buffer size, or the number of unread bytes).
     * Otherwise, we wait for at least one byte to be available. */
    io->IOSer.io_Command = SDCMD_QUERY;
    if (DoIO((struct IORequest *)io)) {
        msg(context,
            ll_error,
            "Error %d occurred while checking device status.",
            io->IOSer.io_Error);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!io->IOSer.io_Actual) {
        /* We want to make a read of 1 byte with timeout, until
         * there the byte is provided. */
        err = cahute_get_amiga_timer(context, &timer_msgport, &timer);
        if (err)
            return err;

        /* Run two operations at once:
         * - Read into the buffer.
         * - Start a timer to the currently requested timeout. */
        io->IOSer.io_Command = CMD_READ;
        io->IOSer.io_Length = 1;
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

        /* We want to wait only if the request has not finished
         * immediately. */
        obtained_signals = Wait(signals);

        if (timeout > 0) {
            if (!CheckIO((struct IORequest *)timer))
                AbortIO((struct IORequest *)timer);

            WaitIO((struct IORequest *)timer);
        }

        /* Wait for either completion and clearing of serial read, or
         * for cancellation of I/O request. */
        if (!CheckIO((struct IORequest *)io))
            AbortIO((struct IORequest *)io);

        WaitIO((struct IORequest *)io);

        /* We must clear signals for the next iteration. */
        SetSignal(0L, signals);

        if (obtained_signals & SIGBREAKF_CTRL_C)
            return CAHUTE_ERROR_ABORT;

        /* From here, the serial signal may or may not have been set.
         * If yes, this means a byte has been received, and more may
         * still be available.
         * Otherwise, this means a byte has not been read, but due to
         * a race condition, it is possible that the bytes could have
         * been read between the initial status check and read, so
         * we still want to check the status here.
         *
         * Note that in the first case, the buffer we provide to the
         * system for read later is unaligned (1 past 32-byte
         * alignment guaranteed in linkopen.c). */
        if (obtained_signals & (1L << serial_msgport->mp_SigBit)) {
            if (io->IOSer.io_Error) {
                msg(context,
                    ll_error,
                    "Error %d occurred while reading from device.",
                    io->IOSer.io_Error);
                return CAHUTE_ERROR_UNKNOWN;
            }

            buf++;
            capacity--;
            read_bytes++;
        }

        io->IOSer.io_Command = SDCMD_QUERY;
        if (DoIO((struct IORequest *)io)) {
            msg(context,
                ll_error,
                "Error %d occurred while checking device status.",
                io->IOSer.io_Error);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!io->IOSer.io_Actual)
            return CAHUTE_ERROR_TIMEOUT;
    }

    /* Make a synchronous read of all available bytes.
     * According to the AmigaOS Wiki, this operation is guaranteed
     * to return without waiting. */
    unread_bytes = io->IOSer.io_Actual;
    if (unread_bytes > capacity)
        unread_bytes = capacity;

    if (unread_bytes) {
        io->IOSer.io_Command = CMD_READ;
        io->IOSer.io_Length = unread_bytes;
        io->IOSer.io_Data = (APTR)buf;

        if (DoIO((struct IORequest *)io)) {
            msg(context,
                ll_error,
                "Error %d occurred while reading available bytes.",
                io->IOSer.io_Error);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    *readp = read_bytes + unread_bytes;
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
CAHUTE_LOCAL(int)
send_on_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *writtenp
) {
    struct IOExtSer *io = cookie->write_io;

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
CAHUTE_LOCAL(int)
set_serial_params_on_link(
    cahute_context *context,
    cahute_amigaos_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
) {
    struct IOExtSer *io = cookie->read_io;

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

    io->IOSer.io_Command = CMD_FLUSH;
    if (DoIO((struct IORequest *)io)) {
        msg(context, ll_error, "Unable to flush!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/* AmigaOS serial link callbacks. */
CAHUTE_LOCAL_DATA(cahute_serial_link_interface)
amigaos_serial_link_interface = {
    "Serial (AmigaOS)",
    (cahute_link_close_func *)close_link,
    (cahute_link_receive_func *)receive_on_link,
    (cahute_link_send_func *)send_on_link,
    (cahute_link_set_serial_params_func *)set_serial_params_on_link
};

/**
 * Open an AmigaOS serial link.
 *
 * @param context Context on which to open a serial link.
 * @param open_params Opening parameters to transmit.
 * @param name_or_path Name or path to the serial port to open.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_open_amigaos_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
) {
    cahute_amigaos_device device;
    cahute_amigaos_serial_link_cookie cookie;
    struct MsgPort *read_msg_port;
    struct MsgPort *write_msg_port;
    struct IOExtSer *read_io;
    struct IOExtSer *write_io;
    int ret;

    ret =
        cahute_get_amigaos_device(context, &device, name_or_path, SERIALNAME);
    if (ret)
        return ret;

    read_msg_port = CreateMsgPort();
    if (!read_msg_port) {
        msg(context, ll_error, "Could not open read message port.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    write_msg_port = CreateMsgPort();
    if (!write_msg_port) {
        msg(context, ll_error, "Could not open write message port.");
        DeleteMsgPort(read_msg_port);
        return CAHUTE_ERROR_UNKNOWN;
    }

    read_io = CreateIORequest(read_msg_port, sizeof(struct IOExtSer));
    if (!read_io) {
        msg(context, ll_error, "Could not create read IORequest.");
        DeleteMsgPort(write_msg_port);
        DeleteMsgPort(read_msg_port);
        return CAHUTE_ERROR_UNKNOWN;
    }

    write_io = CreateIORequest(write_msg_port, sizeof(struct IOExtSer));
    if (!write_io) {
        msg(context, ll_error, "Could not create write IORequest.");
        DeleteIORequest(read_io);
        DeleteMsgPort(write_msg_port);
        DeleteMsgPort(read_msg_port);
        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(context,
        ll_info,
        "Opening DEVICE=%s,UNIT=%lu.",
        device.name,
        device.unit);
    ret = OpenDevice(
        (CONST_STRPTR)device.name,
        device.unit,
        (struct IORequest *)read_io,
        0L
    );
    if (ret) {
        msg(context,
            ll_error,
            "Error %d has occurred while opening DEVICE=%s,UNIT=%lu.",
            ret,
            device.name,
            device.unit);

        if (ret == IOERR_BADADDRESS)
            ret = CAHUTE_ERROR_NOT_FOUND;
        else if (ret == IOERR_UNITBUSY)
            ret = CAHUTE_ERROR_BUSY;
        else
            ret = CAHUTE_ERROR_UNKNOWN;

        DeleteIORequest(write_io);
        DeleteIORequest(read_io);
        DeleteMsgPort(write_msg_port);
        DeleteMsgPort(read_msg_port);
        return ret;
    }

    CopyMem(read_io, write_io, sizeof(struct IOExtSer));
    write_io->IOSer.io_Message.mn_ReplyPort = write_msg_port;

    cookie.read_msg_port = read_msg_port;
    cookie.write_msg_port = write_msg_port;
    cookie.read_io = read_io;
    cookie.write_io = write_io;
    return cahute_open_serial_link_from_interface(
        open_params,
        &amigaos_serial_link_interface,
        &cookie,
        sizeof(cookie)
    );
}
