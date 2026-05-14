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
 * Close a POSIX serial port.
 *
 * @param context Context in which the function is called.
 * @param cookie Cookie.
 */
CAHUTE_INTERNAL(void)
cahute_close_posix_serial_link(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie
) {
    close(cookie->fd);
}


/**
 * Receive on a POSIX serial link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer in which to receive.
 * @param capacity Capacity of the buffer.
 * @param receivedp Pointer to the received bytes count to set.
 * @param timeout Timeout; 0 for infinite.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_receive_on_posix_serial_link(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    cahute_ssize ret;

    if (timeout > 0) {
        /* Use select() to wait for input to be present. */
        fd_set read_fds, write_fds, except_fds;
        struct timeval timeout_tv;
        int select_ret;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        FD_SET(cookie->fd, &read_fds);

        timeout_tv.tv_sec = timeout / 1000;
        timeout_tv.tv_usec = (timeout % 1000) * 1000;

        select_ret = select(
            cookie->fd + 1,
            &read_fds,
            &write_fds,
            &except_fds,
            &timeout_tv
        );

        switch (select_ret) {
        case 1:
            /* Input is ready for us to read! */
            break;

        case 0:
            return CAHUTE_ERROR_TIMEOUT;

        default:
            msg(context,
                ll_error,
                "An error occurred while calling select() %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    ret = read(cookie->fd, buf, capacity);

    if (ret < 0)
        switch (errno) {
        case 0:
            *receivedp = 0;
            return CAHUTE_OK;

        case ENODEV:
        case EIO:
            return CAHUTE_ERROR_GONE;

        default:
            msg(context,
                ll_error,
                "An error occurred while calling read(): %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

    *receivedp = (size_t)ret;
    return CAHUTE_OK;
}

/**
 * Send on a POSIX serial link.
 *
 * @param context
 * @param cookie
 * @param buf Buffer to send.
 * @param size Size of the buffer to send.
 * @param sentp Pointer to the sent bytes count to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_send_on_posix_serial_link(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
) {
    cahute_ssize ret;

    ret = write(cookie->fd, buf, size);
    if (ret < 0)
        switch (errno) {
        case ENODEV:
            return CAHUTE_ERROR_GONE;

        default:
            msg(context, ll_fatal, "errno was %d: %s", errno, strerror(errno));
            return CAHUTE_ERROR_UNKNOWN;
        }

    *sentp = (size_t)ret;
    return CAHUTE_OK;
}

/**
 * Set serial params on a POSIX serial link.
 *
 * @param context
 * @param cookie
 * @param flags
 * @param speed
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_set_posix_serial_link_params(
    cahute_context *context,
    cahute_posix_serial_link_cookie *cookie,
    unsigned long flags,
    unsigned long speed
) {
    struct termios term;
    speed_t termios_speed;
    int set_dtr = 0, dtr_value = 0;
    int set_rts = 0, rts_value = 0;

    switch (speed) {
    case 300:
        termios_speed = B300;
        break;
    case 600:
        termios_speed = B600;
        break;
    case 1200:
        termios_speed = B1200;
        break;
    case 2400:
        termios_speed = B2400;
        break;
    case 4800:
        termios_speed = B4800;
        break;
#ifdef B9600
    case 9600:
        termios_speed = B9600;
        break;
#endif
#ifdef B19200
    case 19200:
        termios_speed = B19200;
        break;
#endif
#ifdef B38400
    case 38400:
        termios_speed = B38400;
        break;
#endif
#ifdef B57600
    case 57600:
        termios_speed = B57600;
        break;
#endif
#ifdef B115200
    case 115200:
        termios_speed = B115200;
        break;
#endif
    default:
        msg(context, ll_error, "Speed unsupported by termios: %lu", speed);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (tcdrain(cookie->fd)) {
        msg(context,
            ll_error,
            "Could not wait until data has been written: %s (%d)",
            strerror(errno),
            errno);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (tcgetattr(cookie->fd, &term) < 0) {
        msg(context,
            ll_error,
            "Could not get serial attributes: %s (%d)",
            strerror(errno),
            errno);
        return CAHUTE_ERROR_UNKNOWN;
    }

    cfsetispeed(&term, termios_speed);
    cfsetospeed(&term, termios_speed);

    /* Most of the flags' usage here is taken from previous work on
        * libcasio. */
    term.c_iflag &=
        ~(IGNBRK | IGNCR | BRKINT | PARMRK | ISTRIP | INLCR | ICRNL | IGNPAR
          | IXON | IXOFF);
    if ((flags & CAHUTE_SERIAL_XONXOFF_MASK) == CAHUTE_SERIAL_XONXOFF_ENABLE) {
        term.c_iflag |= IXON | IXOFF;
        term.c_cc[VSTART] = 0x11; /* XON */
        term.c_cc[VSTOP] = 0x13;  /* XOFF */
        term.c_cc[VMIN] = 0;
    }

    term.c_oflag = 0;
    term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    term.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE);
#ifdef CRTSCTS
    if ((flags & CAHUTE_SERIAL_RTS_MASK) == CAHUTE_SERIAL_RTS_HANDSHAKE)
        term.c_cflag |= CRTSCTS;
    else
        term.c_cflag &= ~CRTSCTS;
#else
    if ((flags & CAHUTE_SERIAL_RTS_MASK) == CAHUTE_SERIAL_RTS_HANDSHAKE)
        CAHUTE_RETURN_IMPL(context, "Platform did not define CRTSCTS.");
#endif
    term.c_cflag |= CREAD | CS8;

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        term.c_cflag |= PARENB;
        break;

    case CAHUTE_SERIAL_PARITY_ODD:
        term.c_cflag |= PARENB | PARODD;
        break;
    }

    if ((flags & CAHUTE_SERIAL_STOP_MASK) == CAHUTE_SERIAL_STOP_TWO)
        term.c_cflag |= CSTOPB;

    if (tcsetattr(cookie->fd, TCSANOW, &term)) {
        msg(context,
            ll_error,
            "Could not get serial attributes: %s (%d)",
            strerror(errno),
            errno);
        return CAHUTE_ERROR_UNKNOWN;
    }

    switch (flags & CAHUTE_SERIAL_DTR_MASK) {
    case CAHUTE_SERIAL_DTR_DISABLE:
        dtr_value = 1;
        /* FALLTHRU */
    case CAHUTE_SERIAL_DTR_ENABLE:
        set_dtr = 1;
    }

    switch (flags & CAHUTE_SERIAL_RTS_MASK) {
    case CAHUTE_SERIAL_RTS_DISABLE:
        rts_value = 1;
        /* FALLTHRU */
    case CAHUTE_SERIAL_RTS_ENABLE:
        set_rts = 1;
        break;
    }

#if !defined(TIOCMGET)
    if (set_dtr || set_rts)
        CAHUTE_RETURN_IMPL(
            context,
            "No mechanism available to set control bits."
        );
#else
    if (set_dtr || set_rts) {
        int status;

        if (ioctl(cookie->fd, TIOCMGET, &status)) {
            msg(context,
                ll_error,
                "ioctl(TIOCMGET) failed: %s",
                strerror(errno));
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (set_dtr)
            status = dtr_value ? status & ~TIOCM_DTR : status | TIOCM_DTR;

        if (set_rts)
            status = rts_value ? status & ~TIOCM_RTS : status | TIOCM_RTS;

        if (ioctl(cookie->fd, TIOCMSET, &status)) {
            msg(context,
                ll_error,
                "ioctl(TIOCMSET) failed: %s",
                strerror(errno));
            return CAHUTE_ERROR_UNKNOWN;
        }
    }
#endif

    return CAHUTE_OK;
}

CAHUTE_LOCAL_DATA(cahute_serial_link_interface)
posix_serial_interface = {
    "Serial (POSIX)",
    (cahute_link_close_func *)cahute_close_posix_serial_link,
    (cahute_link_receive_func *)cahute_receive_on_posix_serial_link,
    (cahute_link_send_func *)cahute_send_on_posix_serial_link,
    (cahute_link_set_serial_params_func *)cahute_set_posix_serial_link_params
};

/**
 * Open a POSIX serial link.
 *
 * @param context Context on which to open a serial link.
 * @param open_params Opening parameters to transmit.
 * @param name_or_path Name or path to the serial port to open.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_INTERNAL(int)
cahute_open_posix_serial_link(
    cahute_context *context,
    cahute_serial_link_open_params *open_params,
    char const *name_or_path
) {
    cahute_posix_serial_link_cookie cookie;
    int fd;

    fd = open(name_or_path, O_NOCTTY | O_RDWR);
    if (fd < 0) {
        switch (errno) {
        case ENODEV:
        case ENOENT:
        case ENXIO:
        case EPIPE:
        case ESPIPE:
            msg(context,
                ll_error,
                "Could not open serial device: %s",
                strerror(errno));
            return CAHUTE_ERROR_NOT_FOUND;

        case EACCES:
            return CAHUTE_ERROR_PRIV;

        default:
            msg(context,
                ll_error,
                "Unknown error: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    /* In case there's still unread data, we want to remove it. */
    if (tcflush(fd, TCIOFLUSH)) {
        msg(context,
            ll_error,
            "Could not flush existing input or output: %s (%d)",
            strerror(errno),
            errno);
        close(fd);
        return CAHUTE_ERROR_UNKNOWN;
    }

    cookie.fd = fd;
    return cahute_open_serial_link_from_interface(
        open_params,
        &posix_serial_interface,
        &cookie,
        sizeof(cookie)
    );
}
