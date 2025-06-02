.. _header-ref-cahute-context:

``<cahute/context.h>`` -- Context management for Cahute
=======================================================

This header declares context-related utilities for Cahute.

Type definitions
----------------

.. c:struct:: cahute_context

    Context; see :ref:`feature-topic-contexts` for more information.

    This type is opaque. It must be instantiated using
    :c:func:`cahute_create_context`, and destroyed using
    :c:func:`cahute_destroy_context`.

Function declarations
---------------------

.. c:function:: int cahute_create_context(cahute_context **contextp)

    Create a context.

    .. warning::

        In case of error, the value of ``*contextp`` mustn't be used nor freed.

    :param contextp: Pointer to set to the created context.
    :return: Error, or 0 if the context was successfully created.

.. c:function:: void cahute_destroy_context(cahute_context *context)

    Destroy a context.

    :param context: Context to destroy.
