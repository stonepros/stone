stone-python-common
==================

This library is meant to be used to keep common data structures and
functions usable throughout the Stone project.

Like for example:

- All different Cython bindings.
- MGR modules.
- ``stone`` command line interface and other Stone tools.
- Also external tools.

Usage
=====

From within the Stone git, just import it:

.. code:: python

    from stone.deployment_utils import DriveGroupSpec
    from stone.exceptions import OSError
