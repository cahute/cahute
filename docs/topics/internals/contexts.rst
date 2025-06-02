.. _internals-topic-contexts:

Context internals
=================

See :ref:`feature-topic-contexts` for more information about what contexts are.

This document describe internal mechanisms for contexts.

.. _internals-topic-context-pointers:

Context pointers
----------------

Contexts can behave as a cache for system resources, by associating numbers
with universal pointers and an optional initialization pointer. The idea for
any other internal context user, including platform-specific utilities, is to
use the following function:

.. c:function:: int cahute_get_context_pointer(cahute_context *context, \
    void **valuep, int key, cahute_context_init_func *init_func)

    Initialize a cached resource if necessary, referenced by the key provided
    in ``key``, and provide it by setting ``*valuep``.

    If the context pointer has already been initialized, the value is provided
    directly; otherwise, it is initialized first using ``init_func``.

    :param context: Context serving as cache for the resource.
    :param valuep: Pointer to the pointer to set to the initialized resource.
    :param key: Key for the context pointer to get.
    :param init_func: Function to use to initialize the resource, if necessary.
    :return: Error, or 0 if successful.

The function types used with context pointers are the following:

.. c:type:: int (cahute_context_init_func)(cahute_context *context, \
    void **valuep, cahute_context_destroy_func **destroy_funcp)

    Function to use to initialize a context pointer.

    The function **must** set ``*valuep`` to the obtained value.
    It **can** also set ``*destroy_funcp`` to the function to use to destroy
    the value on context destruction, if necessary.

    .. warning::

        If the function returns an error, ``*valuep`` and ``*destroy_funcp``
        will be **ignored by the caller**; you must destroy the obtained
        resources if you are to return an error.

        This is designed to make destroy callbacks easier to make, since you
        can always assume that the resource has been fully created / loaded.

    :param context: Context in which the resource is being created / loaded.
    :param valuep: Pointer to the pointer to set with the created / loaded
        resource, to be used as a context pointer.
    :param destroy_funcp: Pointer to the destroy callback pointer, to
        optionally set if the resource is to be destroyed on output.
    :return: Error, or 0 if successful.

.. c:type:: void (cahute_context_destroy_func)(cahute_context *context, \
    void *value)

    Function to use to destroy a context pointer.

    :param context: Context in which the resource is being destroyed
        / unloaded.
    :param value: Value to destroy / unload.

Keys are global to all of the library, for easier introspection.
They are defined in ``lib/internals.h``, using the ``CAHUTE_CONTEXT_POINTER_``
prefix.

.. warning::

    If you are adding a context pointer, ``CAHUTE_CONTEXT_POINTER_COUNT`` must
    be updated to reflect the actual number of possible context pointers.

Example uses of context pointers within the codebase are the following:

* ``lib/platform/libusb/context.c`` for
  ``CAHUTE_CONTEXT_POINTER_LIBUSB_CONTEXT``;
* ``lib/platform/amigaos/timer.c`` for
  ``CAHUTE_CONTEXT_POINTER_AMIGAOS_TIMER``.
