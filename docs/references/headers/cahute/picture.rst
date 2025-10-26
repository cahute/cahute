.. _header-ref-cahute-picture:

``<cahute/picture.h>`` -- Picture format related utilities for Cahute
=====================================================================

Macro definitions
-----------------

``CAHUTE_PICTURE_FORMAT_*`` are constants representing how a given
picture's data is encoded.

.. c:macro:: CAHUTE_PICTURE_FORMAT_1BIT_MONO

    Constant representing the :ref:`format-topic-picture-1bit`.

.. c:macro:: CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50

    Constant representing the :ref:`format-topic-picture-1bit-cas50`.

.. c:macro:: CAHUTE_PICTURE_FORMAT_1BIT_DUAL

    Constant representing the :ref:`format-topic-picture-1bit-dual`.

.. c:macro:: CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50

    Constant representing the :ref:`format-topic-picture-1bit-multiple-cas50`.

.. c:macro:: CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED

    Constant representing the :ref:`format-topic-picture-4bit-rgb-packed`.

.. c:macro:: CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5

    Constant representing the :ref:`format-topic-picture-r5g6b5`.

.. c:macro:: CAHUTE_PICTURE_FORMAT_32BIT_ARGB_HOST

    Constant representing the :ref:`format-topic-picture-32bit-argb-host`.

Type definitions
----------------

.. c:struct:: cahute_frame

    Screenstreaming frame details for screenstreaming.

    .. c:member:: int cahute_frame_width

        Width of the frame, in pixels.

    .. c:member:: int cahute_frame_height

        Height of the frame, in pixels.

    .. c:member:: int cahute_frame_format

        Format of the frame, as a ``CAHUTE_PICTURE_FORMAT_*`` constant.
        See :ref:`header-ref-cahute-picture` for more information.

    .. c:member:: cahute_u8 const *cahute_frame_data

        Frame contents encoded with the format described above.

Function declarations
---------------------

.. c:function:: int cahute_convert_picture(cahute_context *context, \
    void *dest, int dest_format, void const *src, int src_format, \
    int width, int height)

    Convert picture data from a source to a destination format.

    :param context: Context in which to run the function.
    :param dest: Destination picture data.
    :param dest_format: Format to write picture data in on the destination.
    :param src: Source picture data.
    :param src_format: Format of the source picture data.
    :param width: Picture width.
    :param height: Picture height.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_convert_picture_from_frame( \
    cahute_context *context, void *dest, \
    int dest_format, cahute_frame const *frame)

    Convert picture data from a frame to a destination format.

    :param context: Context in which to run the function.
    :param dest: Destination picture data.
    :param dest_format: Format to write picture data in on the destination.
    :param frame: Frame to get source picture data and metadata from.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_blit_picture( \
    cahute_context *context, void *dest, int dest_format, int dest_width, \
    int dest_height, void const *src, int src_format, int src_width, \
    int src_height, int y, int x)

    Blit a source picture onto a destination picture.

    :param context: Context in which to run the function.
    :param dest: Destination picture data.
    :param dest_format: Format of the destination picture.
    :param dest_width: Width of the destination picture.
    :param dest_height: Height of the destination picture.
    :param src: Source picture data.
    :param src_format: Format of the source picture.
    :param src_width: Width of the source picture.
    :param src_height: Height of the source picture.
    :param y: Y coordinate on the destination picture at which to blit the
        source picture.
    :param x: X coordinate on the destination picture at which to blit the
        source picture.
    :return: Error, or 0 if the operation was successful.
