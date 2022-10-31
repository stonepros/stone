Hello World Module
==================

This is a simple module skeleton for documentation purposes.

Enabling
--------

The *hello* module is enabled with::

  stone mgr module enable hello

To check that it is enabled, run::

  stone mgr module ls

After editing the module file (found in ``src/pybind/mgr/hello/module.py``), you can see changes by running::

  stone mgr module disable hello
  stone mgr module enable hello

or::

  init-stone restart mgr

To execute the module, run::

  stone hello

The log is found at::

  build/out/mgr.x.log


Documenting
-----------

After adding a new mgr module, be sure to add its documentation to ``doc/mgr/module_name.rst``.
Also, add a link to your new module into ``doc/mgr/index.rst``.
