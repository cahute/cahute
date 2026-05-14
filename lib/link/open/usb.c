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

/* The actual delay used by Screen Receiver is actually 200 microseconds, but
 * the closest delay we can use in Cahute while staying compatible with all
 * of our platforms is 1 millisecond. */
#define UMSSTATUSDELAY 1

/**
 * Receive on an UMS link.
 *
 * @param context Context.
 * @param link Link, passed as a cookie.
 * @param buf
 * @param capacity
 * @param receivedp
 * @param timeout
 * @return
 */
CAHUTE_LOCAL(int)
cahute_receive_on_ums_link(
    cahute_context *context,
    cahute_link *link,
    cahute_u8 *buf,
    size_t capacity,
    size_t *receivedp,
    unsigned long timeout
) {
    cahute_u8 status_buf[16];
    cahute_u8 payload[16];
    size_t avail;
    int err;
    unsigned long start;

    *receivedp = 0;

    err = cahute_monotonic(context, &start);
    if (err)
        return err;

    do {
        unsigned long cur;

        /* We use custom command C0 to poll status and get avail. bytes.
         * See :ref:`ums-command-c0` for more information.
         *
         * Note that it may take time for the calculator to "recharge"
         * the buffer, so we want to try several times in a row before
         * declaring there is no data available yet. */
        err = cahute_scsi_request_from_link_transport(
            link,
            (cahute_u8 *)"\xC0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
            16,
            status_buf,
            16,
            NULL
        );
        if (err)
            return err;

        avail = (status_buf[6] << 8) | status_buf[7];
        if (avail)
            break;

        err = cahute_monotonic(context, &cur);
        if (err)
            return err;

        if (cur - start + UMSSTATUSDELAY >= timeout)
            return CAHUTE_ERROR_TIMEOUT;

        err = cahute_sleep(context, UMSSTATUSDELAY);
        if (err)
            return err;
    } while (1);

    /* NOTE: The target size here should always at least have 4 MiB,
     * which means this condition should actually never evaluate
     * to true. */
    if (avail > capacity)
        avail = capacity;

    /* We now use custom command C1 to request avail. bytes.
     * See :ref:`ums-command-c1` for more information. */
    memcpy(
        payload,
        (cahute_u8 const *)"\xC1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        16
    );
    payload[6] = (avail >> 8) & 255;
    payload[7] = avail & 255;

    err = cahute_scsi_request_from_link_transport(
        link,
        payload,
        16,
        buf,
        avail,
        NULL
    );
    if (err)
        return err;

    *receivedp = avail;
    return CAHUTE_OK;
}

/**
 * Send on an UMS link.
 *
 * @param context Context.
 * @param link Link, passed as a cookie.
 * @param buf
 * @param size
 * @param sentp
 * @return
 */
CAHUTE_LOCAL(int)
cahute_send_on_ums_link(
    cahute_context *context,
    cahute_link *link,
    cahute_u8 const *buf,
    size_t size,
    size_t *sentp
) {
    size_t to_send = size > 0xFFFF ? 0xFFFF : size;
    cahute_u8 payload[16], status_buf[16];
    int err;

    /* We use custom command C0 to poll status and get avail. bytes.
     * See :ref:`ums-command-c0` for more information. */
    err = cahute_scsi_request_from_link_transport(
        link,
        (cahute_u8 *)"\xC0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        16,
        status_buf,
        16,
        NULL
    );
    if (err)
        return err;

    /* We use custom command C2 to send data.
     * See :ref:`ums-command-c2` for more information. */
    memcpy(
        payload,
        (cahute_u8 const *)"\xC2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        16
    );
    payload[6] = (to_send >> 8) & 255;
    payload[7] = to_send & 255;

    err = cahute_scsi_request_to_link_transport(
        link,
        payload,
        16,
        buf,
        to_send,
        NULL
    );
    if (err)
        return err;

    *sentp = to_send;
    return CAHUTE_OK;
}

/**
 * Open a serial over USB bulk link from an interface.
 *
 * @param open_params Parameters passed by ``cahute_open_usb_link()``.
 * @param interface Interface of the underlying transport.
 * @param cookie Cookie.
 * @param cookie_size Cookie size.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_open_serial_over_usb_bulk_link_from_interface(
    cahute_usb_link_open_params *open_params,
    cahute_serial_over_usb_bulk_link_interface const *interface,
    void *cookie,
    size_t cookie_size
) {
    cahute_link *link = NULL;
    int err = CAHUTE_ERROR_ALLOC;

    err = cahute_alloc_link(open_params->context, &link, cookie, cookie_size);
    if (err)
        goto fail;

    link->transport = CAHUTE_LINK_TRANSPORT_SERIAL_OVER_USB_BULK;
    link->transport_name = interface->name;
    link->transport_close_func = interface->close_func;
    link->transport_receive_func = interface->receive_func;
    link->transport_send_func = interface->send_func;

    *open_params->linkp = link;
    err = cahute_initialize_link_protocol(
        link,
        open_params->serial_protocol,
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
 * Open an UMS link from an interface.
 *
 * @param open_params Parameters passed by ``cahute_open_usb_link()``.
 * @param interface Interface of the underlying transport.
 * @param cookie Cookie.
 * @param cookie_size Cookie size.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INTERNAL(int)
cahute_open_ums_link_from_interface(
    cahute_usb_link_open_params *open_params,
    cahute_ums_link_interface const *interface,
    void *cookie,
    size_t cookie_size
) {
    cahute_link *link = NULL;
    int err = CAHUTE_ERROR_ALLOC;

    err = cahute_alloc_link(open_params->context, &link, cookie, cookie_size);
    if (err)
        goto fail;

    link->transport = CAHUTE_LINK_TRANSPORT_UMS;
    link->transport_name = interface->name;
    link->transport_close_func = interface->close_func;
    link->transport_scsi_request_from_func = interface->request_from_func;
    link->transport_scsi_request_to_func = interface->request_to_func;

    /* HACK: We actually use send and receive callbacks for SCSI with a
     * different cookie, pointing to the link directly. */
    link->transport_stream_cookie = link;
    link->transport_send_func =
        (cahute_link_send_func *)&cahute_send_on_ums_link;
    link->transport_receive_func =
        (cahute_link_receive_func *)&cahute_receive_on_ums_link;

    *open_params->linkp = link;
    err = cahute_initialize_link_protocol(
        link,
        open_params->serial_protocol,
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
 * Open a link over a USB transport.
 *
 * @param context Context in which the link is opened.
 * @param linkp Pointer to the link to set with the opened link.
 * @param flags Flags to open the link and underlying transport with.
 * @param name Name or path of the device to open.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_usb_link(
    cahute_context *context,
    cahute_link **linkp,
    unsigned long flags,
    char const *name
) {
    cahute_usb_link_open_params params;
    unsigned long unsupported_flags, init_flags = 0;
    int serial_protocol = PROTOCOL_USB_AUTO,
        ums_protocol = PROTOCOL_USB_MASS_STORAGE;

    unsupported_flags =
        flags
        & ~(CAHUTE_USB_NOCHECK | CAHUTE_USB_NODISC | CAHUTE_USB_NOTERM
            | CAHUTE_USB_RECEIVER | CAHUTE_USB_OHP | CAHUTE_USB_NOPROTO
            | CAHUTE_USB_SEVEN | CAHUTE_USB_CAS300);
    if (unsupported_flags)
        CAHUTE_RETURN_IMPL(
            context,
            "At least one unsupported flag was present."
        );

    if (flags & CAHUTE_USB_NOPROTO) {
        unsupported_flags =
            flags
            & (CAHUTE_USB_NOCHECK | CAHUTE_USB_NODISC | CAHUTE_USB_NOTERM
               | CAHUTE_USB_RECEIVER | CAHUTE_USB_OHP | CAHUTE_USB_SEVEN
               | CAHUTE_USB_CAS300);
        if (unsupported_flags) {
            msg(context,
                ll_error,
                "The following flags are not supported by the generic "
                "protocol: 0x%08lX",
                unsupported_flags);
            return CAHUTE_ERROR_UNKNOWN;
        }
    } else if (flags & CAHUTE_USB_OHP) {
        /* TODO */
        if (~flags & CAHUTE_USB_RECEIVER)
            CAHUTE_RETURN_IMPL(
                context,
                "Sender mode not available for screenstreaming."
            );

        if (flags & CAHUTE_USB_CAS300)
            CAHUTE_RETURN_IMPL(
                context,
                "No screenstreaming is available with CAS300."
            );

        init_flags |= PROTOCOL_FLAG_RECEIVER;
    } else if (flags & CAHUTE_USB_RECEIVER)
        CAHUTE_RETURN_IMPL(
            context,
            "Receiver mode not available for data protocols."
        );

    if ((flags & CAHUTE_USB_SEVEN) && (flags & CAHUTE_USB_CAS300)) {
        msg(context,
            ll_error,
            "SEVEN and CAS300 USB flags cannot be used at the same time.");
        return CAHUTE_ERROR_UNKNOWN;
    } else if ((flags & CAHUTE_USB_NOCHECK) && !(flags & (CAHUTE_USB_SEVEN | CAHUTE_USB_CAS300 | CAHUTE_USB_OHP))) {
        msg(context,
            ll_error,
            "SEVEN or CAS300 USB flag must be set if check is disabled.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (flags & CAHUTE_USB_NOPROTO) {
        serial_protocol = PROTOCOL_USB_NONE;
        ums_protocol = PROTOCOL_USB_NONE;
    } else if (flags & CAHUTE_USB_OHP) {
        serial_protocol = PROTOCOL_USB_SEVEN_OHP;
        ums_protocol = PROTOCOL_USB_SEVEN_OHP;
    } else if (flags & CAHUTE_USB_SEVEN)
        serial_protocol = PROTOCOL_USB_SEVEN;
    else if (flags & CAHUTE_USB_CAS300)
        serial_protocol = PROTOCOL_USB_CAS300;

    if (flags & CAHUTE_USB_NOCHECK)
        init_flags |= PROTOCOL_FLAG_NOCHECK;
    if (flags & CAHUTE_USB_NOTERM)
        init_flags |= PROTOCOL_FLAG_NOTERM;
    if (flags & CAHUTE_USB_NODISC)
        init_flags |= PROTOCOL_FLAG_NODISC;
    if (flags & CAHUTE_USB_RECEIVER)
        init_flags |= PROTOCOL_FLAG_RECEIVER;

    params.context = context;
    params.linkp = linkp;
    params.serial_protocol = serial_protocol;
    params.ums_protocol = ums_protocol;
    params.init_flags = init_flags;

#if CAHUTE_PLATFORM_LIBUSB
    return cahute_open_libusb_link(context, &params, name);
#elif CAHUTE_PLATFORM_WIN32
    return cahute_open_win32_usb_device(context, &params, name);
#else
    (void)params;
    CAHUTE_RETURN_IMPL(
        context,
        "No method available for opening an USB device."
    );
#endif
}
