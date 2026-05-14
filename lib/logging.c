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
#include <stdarg.h>

CAHUTE_LOCAL_DATA(char const *)
hexadecimal_alphabet = "0123456789ABCDEF";

/**
 * Default logging callback for Cahute.
 *
 * This default callback prints the logs on the file pointer provided as the
 * cookie, with a format resembling the following output:
 *
 *     [2024-04-28 13:53:18    cahute info] Without a function.
 *     [2024-04-28 13:53:18 cahute warning] user_func: With a user function.
 *     [2024-04-28 13:53:18   cahute error] open_usb: With an int. function.
 *
 * @param cookie Cookie for the logging function (unused).
 * @param level Log level for the given message.
 * @param source Source of the log.
 * @param func Name of the function.
 * @param message Formatted message.
 */
CAHUTE_LOCAL(void)
cahute_log_to_file(
    void *cookie,
    int level,
    char const *source,
    char const *func,
    char const *message
) {
    cahute_context *context = (void *)cookie;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timebuf[100];
    char levelbuf[20];
    char const *level_name;

    (void)cookie;
    switch (level) {
    case CAHUTE_LOGLEVEL_DEBUG:
        level_name = "debug";
        break;
    case CAHUTE_LOGLEVEL_INFO:
        level_name = "info";
        break;
    case CAHUTE_LOGLEVEL_WARNING:
        level_name = "warning";
        break;
    case CAHUTE_LOGLEVEL_ERROR:
        level_name = "error";
        break;
    case CAHUTE_LOGLEVEL_FATAL:
        level_name = "fatal";
        break;
    case CAHUTE_LOGLEVEL_NONE:
        level_name = "(none)";
        break;
    default:
        level_name = "(unknown)";
    }

    sprintf(
        timebuf,
        "%04d-%02d-%02d %02d:%02d:%02d",
        1900 + tm->tm_year,
        1 + tm->tm_mon,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec
    );
    sprintf(levelbuf, "%s %s", source, level_name);

    if (!func)
        fprintf(
            stderr,
            "%s[%s %14s] ",
            context->log_prefix,
            timebuf,
            levelbuf
        );
    else {
        if (!strncmp(func, "cahute_", 7))
            func = &func[7];

        fprintf(
            stderr,
            "%s[%s %14s] %s: ",
            context->log_prefix,
            timebuf,
            levelbuf,
            func
        );
    }

    fprintf(stderr, "%s\n", message);
}

/**
 * Get the current log level.
 *
 * This is used to expose the static log level variable to the user, as part
 * of the public API.
 *
 * @return Current log level.
 */
CAHUTE_EXTERN(int) cahute_get_log_level(cahute_context *context) {
    return context->log_level;
}

/**
 * Set the current log level.
 *
 * This is used to expose the static log level variable for setting to the
 * user, as part of the public API.
 *
 * @param loglevel Log level to set.
 */
CAHUTE_EXTERN(void)
cahute_set_log_level(cahute_context *context, int loglevel) {
    context->log_level = loglevel;
}

/**
 * Set the current logging function.
 *
 * @param func Pointer to define as the current logging function.
 * @param cookie Cookie to define.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_set_log_func(
    cahute_context *context,
    cahute_log_func *func,
    void *cookie
) {
    if (!func)
        CAHUTE_RETURN_IMPL(
            context,
            "Setting the logging function to NULL is not supported."
        );

    context->log_callback = func;
    context->log_callback_cookie = cookie;
    return CAHUTE_OK;
}

/**
 * Reset the current logging function.
 */
CAHUTE_EXTERN(void) cahute_reset_log_func(cahute_context *context) {
    context->log_callback = &cahute_log_to_file;
    context->log_callback_cookie = (void *)context;
}

/**
 * Emit a log message to the logging system from a different source.
 *
 * @param context Context in which to emit the message.
 * @param loglevel Logging level at which to emit the message.
 * @param source Source for the message.
 * @param func Optional function name for which to emit the message.
 * @param message Message to push.
 * @param len Maximum length of the message to push.
 *        If set to 0, the length will be automatically computed.
 */
CAHUTE_INTERNAL(void)
cahute_log_external_message(
    cahute_context *context,
    int loglevel,
    char const *source,
    char const *func,
    char const *message,
    size_t len
) {
    char buf[64];
    char const *result;

    if (context->log_level > loglevel)
        return;

    if (!len) {
        (*context->log_callback)(
            context->log_callback_cookie,
            loglevel,
            source,
            func,
            message
        );
        return;
    }

    buf[sizeof(buf) - 1] = '\0';
    for (; len > sizeof(buf) - 1; len -= sizeof(buf) - 1) {
        result = memchr(message, '\0', sizeof(buf) - 1);
        if (result) {
            len = (size_t)(result - message);
            break;
        }

        memcpy(buf, message, sizeof(buf) - 1);
        (*context->log_callback)(
            context->log_callback_cookie,
            loglevel,
            source,
            func,
            buf
        );
        message += sizeof(buf) - 1;
    }

    result = memchr(message, '\0', len);
    if (result)
        len = (size_t)(result - message);

    memcpy(buf, message, len);
    buf[len] = '\0';
    (*context->log_callback)(
        context->log_callback_cookie,
        loglevel,
        source,
        func,
        buf
    );
}

/**
 * Output a log message with parameters for a given log level.
 *
 * This is the function called behind the "msg(ll_*, fmt, ...)" macro defined
 * in the common internals for the library.
 *
 * @param context Context in which to output the log message.
 * @param loglevel Logging level at which to emit the message.
 * @param func Optional function name for which to emit the message.
 * @param format Format string to evaluate with the parameters.
 * @param ... Optional parameters for formatting.
 */
CAHUTE_INTERNAL_VA(void)
cahute_log_message(
    cahute_context *context,
    int loglevel,
    char const *func,
    char const *format,
    ...
) {
    char buf[512];
    char const *msg;
    va_list va;
    int ret;

    va_start(va, format);
    if (context->log_level <= loglevel) {
        ret = vsnprintf(buf, sizeof(buf), format, va);

        if (ret >= 0 && (size_t)ret <= sizeof(buf) - 1)
            msg = buf;
        else
            msg = "(message too large)";

        (*context->log_callback)(
            context->log_callback_cookie,
            loglevel,
            "cahute",
            func,
            msg
        );
    }
    va_end(va);
}

/**
 * Output a memory area in displayable format for a given log level.
 *
 * This is the function called behind the "mem((data, data_size))" macro
 * defined in the common internals for the library.
 *
 * @param context Context in which to output the memory messages.
 * @param loglevel Logging level at which to emit the messages.
 * @param func Optional function name for which to emit the message.
 * @param mem Pointer to the memory area to present in the messages.
 * @param size Size of the memory area to present in the messages.
 */
CAHUTE_INTERNAL(void)
cahute_log_memory(
    cahute_context *context,
    int loglevel,
    char const *func,
    void const *mem,
    size_t size
) {
    char linebuf[80];
    cahute_u8 const *p;
    size_t offset = 0;

    if (context->log_level > loglevel)
        return;

    if (!size) {
        (*context->log_callback)(
            context->log_callback_cookie,
            loglevel,
            "cahute",
            func,
            "(nothing)"
        );
        return;
    }

    memcpy(linebuf, "00000000  0000 0000 0000 0000  ", 31);
    for (p = mem; size > 0; offset += 8) {
        char *s = linebuf;

        *s++ = hexadecimal_alphabet[(offset >> 28) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 24) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 20) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 16) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 12) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 8) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 4) & 15];
        *s++ = hexadecimal_alphabet[offset & 15];

        s += 2;

        /* Fill in the hexadecimal part. */
        {
            cahute_u8 const *pc = p;
            size_t l;

            for (l = 0; l < 8; l++) {
                if (l < size) {
                    *s++ = hexadecimal_alphabet[(*pc >> 4) & 15];
                    *s++ = hexadecimal_alphabet[*pc++ & 15];
                } else {
                    *s++ = ' ';
                    *s++ = ' ';
                }

                s += l & 1; /* After an odd index, add a space. */
            }
        }

        ++s; /* Extra space. */

        /* Fill in the ASCII part. */
        {
            int l;

            for (l = 0; l < 8; l++, p++, size--) {
                if (!size)
                    break;

                if (isprint(*p))
                    *s++ = (char)*p;
                else
                    *s++ = '.';
            }
        }

        *s = '\0';

        (*context->log_callback)(
            context->log_callback_cookie,
            loglevel,
            "cahute",
            func,
            linebuf
        );
    }
}
