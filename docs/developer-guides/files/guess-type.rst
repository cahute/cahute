Guessing the type of a file
===========================

In order to guess the type of a file, the steps are the following:

#. Create the context using :c:func:`cahute_create_context`.
#. Open the file using :c:func:`cahute_open_file`.
#. Use :c:func:`cahute_guess_file_type` to obtain the guessed file type.
#. Close the file using :c:func:`cahute_close_file`.
#. Destroy the context using :c:func:`cahute_destroy_context`.

An example program to do this is the following:

.. literalinclude:: guess-type.c
    :language: c
