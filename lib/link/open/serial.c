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
 * Open a serial link from an interface.
 *
 * @param open_params Opaque parameter transmitted by ``cahute_open_serial_link()``,
 *        to transmit to the function without modification.
 * @param interface Interface defined by the platform.
 * @param cookie Cookie to copy on the link, and transmit to the platform-specific
 *        functions set in the interface.
 * @param cookie_size
 * @return
 */
CAHUTE_INTERNAL(int)
cahute_open_serial_link_from_interface(
    cahute_serial_link_open_params *open_params,
    cahute_serial_link_interface const *interface,
    void *cookie,
    size_t cookie_size
) {
    cahute_link *link = NULL;
    int err;

    err = cahute_alloc_link(open_params->context, &link, cookie, cookie_size);
    if (err)
        goto fail;

    link->transport = CAHUTE_LINK_TRANSPORT_SERIAL;
    link->transport_name = interface->name;
    link->transport_close_func = interface->close_func;
    link->transport_receive_func = interface->receive_func;
    link->transport_send_func = interface->send_func;
    link->transport_set_serial_params_func = interface->set_serial_params_func;

    err = cahute_set_serial_params_on_link_transport(
        link,
        open_params->serial_flags,
        open_params->serial_speed
    );
    if (err)
        goto fail;

    *open_params->linkp = link;
    err = cahute_initialize_link_protocol(
        link,
        open_params->protocol,
        open_params->init_flags
    );
    if (!err)
        return CAHUTE_OK;

fail:
    if (link)
        free(link);
    if (interface->close_func)
        (*interface->close_func)(open_params->context, cookie);
    return err;
}

/**
 * Open a link over a serial transport.
 *
 * @param context Context in which the link is opened.
 * @param linkp Pointer to the link to set with the opened link.
 * @param flags Flags to open the link and underlying transport with.
 * @param name_or_path Name or path of the serial port.
 * @param speed Speed in bauds with which to open the underlying transport.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_serial_link(
    cahute_context *context,
    cahute_link **linkp,
    unsigned long flags,
    char const *name_or_path,
    unsigned long speed
) {
    cahute_serial_link_open_params params;
    unsigned long init_flags = 0;
    unsigned long unsupported_flags;
    int protocol, err = CAHUTE_OK;

    unsupported_flags =
        flags
        & ~(CAHUTE_SERIAL_PROTOCOL_MASK | CAHUTE_SERIAL_STOP_MASK
            | CAHUTE_SERIAL_PARITY_MASK | CAHUTE_SERIAL_XONXOFF_MASK
            | CAHUTE_SERIAL_DTR_MASK | CAHUTE_SERIAL_RTS_MASK
            | CAHUTE_SERIAL_RECEIVER | CAHUTE_SERIAL_NOCHECK
            | CAHUTE_SERIAL_NODISC | CAHUTE_SERIAL_NOTERM);

    if (unsupported_flags)
        CAHUTE_RETURN_IMPL(
            context,
            "At least one unsupported flag was present."
        );

    if (!(flags & CAHUTE_SERIAL_PROTOCOL_MASK)) {
        /* Default value depends on the presence of the
         * CAHUTE_SERIAL_RECEIVER flag. */
        if (flags & CAHUTE_SERIAL_RECEIVER)
            flags |= CAHUTE_SERIAL_PROTOCOL_AUTO;
        else
            flags |= CAHUTE_SERIAL_PROTOCOL_AUTO_CAS50;
    }

    switch (flags & CAHUTE_SERIAL_PROTOCOL_MASK) {
    case CAHUTE_SERIAL_PROTOCOL_NONE:
        /* The generic protocol is being selected.
         * We don't want to have any protocol opened and managed on the link,
         * and instead open the direct device functions. */
        unsupported_flags = flags
                            & (CAHUTE_SERIAL_RECEIVER | CAHUTE_SERIAL_NOCHECK
                               | CAHUTE_SERIAL_NODISC | CAHUTE_SERIAL_NOTERM);
        if (unsupported_flags) {
            msg(context,
                ll_error,
                "The following flags are not supported by the generic "
                "protocol: 0x%08lX",
                unsupported_flags);
            return CAHUTE_ERROR_UNKNOWN;
        }

        protocol = PROTOCOL_SERIAL_NONE;
        break;

    case CAHUTE_SERIAL_PROTOCOL_CAS40:
        protocol = PROTOCOL_SERIAL_CAS40;
        break;

    case CAHUTE_SERIAL_PROTOCOL_CAS50:
        protocol = PROTOCOL_SERIAL_CAS50;
        break;

    case CAHUTE_SERIAL_PROTOCOL_CAS100:
        protocol = PROTOCOL_SERIAL_CAS100;
        break;

    case CAHUTE_SERIAL_PROTOCOL_CAS300:
        protocol = PROTOCOL_SERIAL_CAS300;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN:
        protocol = PROTOCOL_SERIAL_SEVEN;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN_OHP:
        /* TODO */
        if (~flags & CAHUTE_SERIAL_RECEIVER)
            CAHUTE_RETURN_IMPL(
                context,
                "Only receiver is supported for screenstreaming."
            );

        protocol = PROTOCOL_SERIAL_SEVEN_OHP;
        break;

    case CAHUTE_SERIAL_PROTOCOL_AUTO:
        /* In sender mode, we need to know which CASIOLINK variant to use. */
        if (~flags & CAHUTE_SERIAL_RECEIVER) {
            msg(context,
                ll_error,
                "Fully automatic protocol detection can only be selected when "
                "receiver mode is enabled.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        protocol = PROTOCOL_SERIAL_AUTO;
        ;
        break;

    case CAHUTE_SERIAL_PROTOCOL_AUTO_CAS40:
        protocol = PROTOCOL_SERIAL_AUTO_CAS40;
        break;

    case CAHUTE_SERIAL_PROTOCOL_AUTO_CAS50:
        protocol = PROTOCOL_SERIAL_AUTO_CAS50;
        break;

    case CAHUTE_SERIAL_PROTOCOL_AUTO_CAS100:
        protocol = PROTOCOL_SERIAL_AUTO_CAS100;
        break;

    case CAHUTE_SERIAL_PROTOCOL_AUTO_CAS300:
        protocol = PROTOCOL_SERIAL_AUTO_CAS300;
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported serial protocol.");
    }

    /* If we are not allowed to initiate the connection, we cannot test
     * different things, therefore this cannot be used with
     * ``CAHUTE_SERIAL_NOCHECK``. */
    switch (protocol) {
    case PROTOCOL_SERIAL_AUTO:
    case PROTOCOL_SERIAL_AUTO_CAS40:
    case PROTOCOL_SERIAL_AUTO_CAS50:
    case PROTOCOL_SERIAL_AUTO_CAS100:
    case PROTOCOL_SERIAL_AUTO_CAS300:
        if (flags & CAHUTE_SERIAL_NOCHECK) {
            msg(context,
                ll_error,
                "We need the check flow to determine the protocol.");
            return CAHUTE_ERROR_UNKNOWN;
        }
        break;
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case 0:
        /* We use a default value depending on the protocol. */
        flags |= CAHUTE_SERIAL_STOP_TWO;
        break;

    case CAHUTE_SERIAL_STOP_ONE:
    case CAHUTE_SERIAL_STOP_TWO:
        /* Valid values! */
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported value for stop bits.");
    }

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case 0:
        switch (protocol) {
        case PROTOCOL_SERIAL_CAS40:
        case PROTOCOL_SERIAL_AUTO_CAS40:
            flags |= CAHUTE_SERIAL_PARITY_EVEN;
            break;

        default:
            flags |= CAHUTE_SERIAL_PARITY_OFF;
        }
        break;
    }

    switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
    case 0:
        switch (protocol) {
        case PROTOCOL_SERIAL_CAS300:
        case PROTOCOL_SERIAL_AUTO_CAS300:
            flags |= CAHUTE_SERIAL_XONXOFF_ENABLE;
            break;

        default:
            flags |= CAHUTE_SERIAL_XONXOFF_DISABLE;
        }
        break;

    case CAHUTE_SERIAL_XONXOFF_DISABLE:
    case CAHUTE_SERIAL_XONXOFF_ENABLE:
        /* Valid values! */
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported XON/XOFF mode.");
    }

    switch (flags & CAHUTE_SERIAL_DTR_MASK) {
    case 0:
        flags |= CAHUTE_SERIAL_DTR_IGNORE;
        break;

    case CAHUTE_SERIAL_DTR_IGNORE:
    case CAHUTE_SERIAL_DTR_DISABLE:
    case CAHUTE_SERIAL_DTR_ENABLE:
        /* Valid values! */
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported DTR mode.");
    }

    switch (flags & CAHUTE_SERIAL_RTS_MASK) {
    case 0:
        flags |= CAHUTE_SERIAL_RTS_IGNORE;
        break;

    case CAHUTE_SERIAL_RTS_IGNORE:
    case CAHUTE_SERIAL_RTS_DISABLE:
    case CAHUTE_SERIAL_RTS_ENABLE:
    case CAHUTE_SERIAL_RTS_HANDSHAKE:
        /* Valid values! */
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported RTS mode.");
    }

    switch (speed) {
    case 0:
        /* We use a default value depending on the protocol. */
        switch (protocol) {
        case PROTOCOL_SERIAL_CAS40:
        case PROTOCOL_SERIAL_AUTO_CAS40:
            speed = 4800;
            break;

        case PROTOCOL_SERIAL_CAS100:
        case PROTOCOL_SERIAL_AUTO_CAS100:
        case PROTOCOL_SERIAL_CAS300:
        case PROTOCOL_SERIAL_AUTO_CAS300:
            speed = 38400;
            break;

        default:
            speed = 9600;
        }
        break;

    case 300:
    case 600:
    case 1200:
    case 2400:
    case 4800:
    case 9600:
    case 19200:
    case 38400:
    case 57600:
    case 115200:
        /* Valid values! */
        break;

    default:
        CAHUTE_RETURN_IMPL(context, "Unsupported serial speed.");
    }

    if (flags & CAHUTE_SERIAL_NOCHECK)
        init_flags |= PROTOCOL_FLAG_NOCHECK;
    if (flags & CAHUTE_SERIAL_NODISC)
        init_flags |= PROTOCOL_FLAG_NODISC;
    if (flags & CAHUTE_SERIAL_NOTERM)
        init_flags |= PROTOCOL_FLAG_NOTERM;
    if (flags & CAHUTE_SERIAL_RECEIVER)
        init_flags |= PROTOCOL_FLAG_RECEIVER;

    params.context = context;
    params.linkp = linkp;
    params.serial_flags = flags;
    params.serial_speed = speed;
    params.protocol = protocol;
    params.init_flags = init_flags;

#if CAHUTE_PLATFORM_WIN32
    err = cahute_open_win32_serial_link(context, &params, name_or_path);
#elif CAHUTE_PLATFORM_WIN16
    err = cahute_open_win16_serial_link(context, &params, name_or_path);
#elif CAHUTE_PLATFORM_AMIGAOS
    err = cahute_open_amigaos_serial_link(context, &params, name_or_path);
#elif CAHUTE_PLATFORM_POSIX
    err = cahute_open_posix_serial_link(context, &params, name_or_path);
#else
    (void)params;
    msg(context, ll_error, "No serial device opening method available.");
    err = CAHUTE_ERROR_IMPL;
#endif

    return err;
}
