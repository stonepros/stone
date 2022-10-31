CLI API Commands Module
=======================

The CLI API module exposes most stone-mgr python API via CLI. Furthermore, this API can be
benchmarked for further testing.

Enabling
--------

The *cli api commands* module is enabled with::

  stone mgr module enable cli_api

To check that it is enabled, run::

  stone mgr module ls | grep cli_api

Usage
--------

To run a mgr module command, run::

  stone mgr cli <command> <param>

For example, use the following command to print the list of servers::

  stone mgr cli list_servers

List all available mgr module commands with::

  stone mgr cli --help

To benchmark a command, run::

  stone mgr cli_benchmark <number of calls> <number of threads> <command> <param>

For example, use the following command to benchmark the command to get osd_map::

  stone mgr cli_benchmark 100 10 get osd_map
