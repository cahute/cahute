.. warning::

    It is possible the build may be running into an error, in which case
    you should select "No" for debugging, then go into the output to see
    the error:

    .. figure:: winvserr1.png

        The build has failed, what to do now?

    If your error regards the Python ``toml`` module not being installed,
    it will look like this:

    .. figure:: winvserr2.png

        An error output that indicates the ``toml`` Python module is missing!

    In which case you will need to go into the "PowerShell" tab, and type
    the following:

    .. code-block:: bash

        py -m pip install toml

    The output will resemble the following:

    .. figure:: winvserr3.png

        A successful Python module install from the PowerShell tab in
        Visual Studio.
