In order to get the current released version of the Cahute source, you have
the following options:

* You can download the latest source package:

  .. parsed-literal::

      curl -o cahute-|version|.tar.gz https\://ftp.cahute.org/releases/cahute-|version|.tar.gz
      tar xvf cahute-|version|.tar.gz

* You can clone the repository and checkout the tag corresponding to the
  release:

  .. parsed-literal::

      git clone https\://gitlab.com/cahute/cahute.git cahute-|version|
      (cd cahute-|version| && git checkout -f |version|)

The project is present in the "cahute-|version|" directory.

.. warning::

    If you are building the project in the context of the
    :ref:`contribution-guide-create-merge-request` guide, these commands need
    to be replaced by the following::

        git clone <your-repo-url>

    Where the repository's URL can be obtained through the ``Code`` button
    on the Gitlab.com interface:

    .. figure:: ../contribution/mr4.png

        Gitlab.com's repository interface with "Code" selected, presenting
        the options to clone the repository.
