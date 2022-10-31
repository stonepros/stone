.. _mgr-iostat-overview:

iostat
======

This module shows the current throughput and IOPS done on the Stone cluster.

Enabling
--------

To check if the *iostat* module is enabled, run::

  stone mgr module ls

The module can be enabled with::

  stone mgr module enable iostat

To execute the module, run::

  stone iostat

To change the frequency at which the statistics are printed, use the ``-p``
option::

  stone iostat -p <period in seconds>

For example, use the following command to print the statistics every 5 seconds::

  stone iostat -p 5

To stop the module, press Ctrl-C.
