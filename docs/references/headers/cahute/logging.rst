.. _header-ref-cahute-logging:

``<cahute/logging.h>`` -- Logging control for Cahute
====================================================

Macro definitions
-----------------

.. c:macro:: CAHUTE_LOGLEVEL_DEBUG

    Constant representing the ``debug`` logging level; see
    :ref:`feature-topic-logging` for more information.

.. c:macro:: CAHUTE_LOGLEVEL_INFO

    Constant representing the ``info`` logging level; see
    :ref:`feature-topic-logging` for more information.

.. c:macro:: CAHUTE_LOGLEVEL_WARNING

    Constant representing the ``warning`` logging level; see
    :ref:`feature-topic-logging` for more information.

.. c:macro:: CAHUTE_LOGLEVEL_ERROR

    Constant representing the ``error`` logging level; see
    :ref:`feature-topic-logging` for more information.

.. c:macro:: CAHUTE_LOGLEVEL_FATAL

    Constant representing the ``fatal`` logging level; see
    :ref:`feature-topic-logging` for more information.

.. c:macro:: CAHUTE_LOGLEVEL_NONE

    Constant representing the special ``none`` logging level; see
    :ref:`feature-topic-logging` for more information.

Type definitions
----------------

.. c:type:: void (cahute_log_func)(void *cookie, int level, \
    char const *source, char const *func, char const *message)

    Function that can be called to emit a log message with parameters.

    When called, this function is passed the following parameters:

    ``cookie``
        Cookie set using :c:func:`cahute_set_log_func`.

    ``level``
        Level with which the message was emitted.

    ``source``
        Source of the message, either set to ``"cahute"`` or the name of
        the third party library the log comes from.

    ``func``
        Name or prototype of the function that emitted the message; may be
        set to ``NULL`` in some cases.

    ``message``
        Formatted content of the message that was emitted.

Function declarations
---------------------

.. c:function:: int cahute_get_log_level(cahute_context *context)

    Get the current logging level, i.e. threshold of emitted messages,
    in the provided context.

    :param context: Context from which to get the current logging level.
    :return: The current logging level.

.. c:function:: void cahute_set_log_level(cahute_context *context, int level)

    Set the current logging level, i.e. threshold of emitted message,
    in the provided context.

    :param context: Context in which to set the current logging level.
    :param level: The logging level to set as the current one.

.. c:function:: int cahute_set_log_func(cahute_context *context, \
    cahute_log_func *func, void *cookie)

    Set the function and related cookie used to emit logging messages.

    :param context: Context in which to set the logging function.
    :param func: Pointer to the function to use.
    :param cookie: Cookie to pass to the function on every call.
    :return: Cahute error.

.. c:function:: int cahute_reset_log_func(cahute_context *context)

    Reset the function and related cookie used to emit logging messages
    to the default one.

    :param context: Context in which to reset the logging function.
