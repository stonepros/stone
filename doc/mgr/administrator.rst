.. _mgr-administrator-guide:

stone-mgr administrator's guide
==============================

Manual setup
------------

Usually, you would set up a stone-mgr daemon using a tool such
as stone-ansible.  These instructions describe how to set up
a stone-mgr daemon manually.

First, create an authentication key for your daemon::

    stone auth get-or-create mgr.$name mon 'allow profile mgr' osd 'allow *' mds 'allow *'

Place that key into ``mgr data`` path, which for a cluster "stone"
and mgr $name "foo" would be ``/var/lib/stone/mgr/stone-foo``.

Start the stone-mgr daemon::

    stone-mgr -i $name

Check that the mgr has come up by looking at the output
of ``stone status``, which should now include a mgr status line::

    mgr active: $name

Client authentication
---------------------

The manager is a new daemon which requires new StoneX capabilities. If you upgrade
a cluster from an old version of Stone, or use the default install/deploy tools,
your admin client should get this capability automatically. If you use tooling from
elsewhere, you may get EACCES errors when invoking certain stone cluster commands.
To fix that, add a "mgr allow \*" stanza to your client's stonex capabilities by
`Modifying User Capabilities`_.

High availability
-----------------

In general, you should set up a stone-mgr on each of the hosts
running a stone-mon daemon to achieve the same level of availability.

By default, whichever stone-mgr instance comes up first will be made
active by the monitors, and the others will be standbys.  There is
no requirement for quorum among the stone-mgr daemons.

If the active daemon fails to send a beacon to the monitors for
more than ``mon mgr beacon grace`` (default 30s), then it will be replaced
by a standby.

If you want to pre-empt failover, you can explicitly mark a stone-mgr
daemon as failed using ``stone mgr fail <mgr name>``.

Performance and Scalability
---------------------------

All the mgr modules share a cache that can be enabled with
``stone config set mgr mgr_ttl_cache_expire_seconds <seconds>``, where seconds
is the time to live of the cached python objects.

It is recommended to enable the cache with a 10 seconds TTL when there are 500+
osds or 10k+ pgs as internal structures might increase in size, and cause latency
issues when requesting large structures. As an example, an OSDMap with 1000 osds
has a aproximate size of 4MiB. With heavy load, on a 3000 osd cluster there has
been a 1.5x improvement enabling the cache.

Furthermore, you can run ``stone daemon mgr.${MGRNAME} perf dump`` to retrieve perf
counters of a mgr module. In ``mgr.cache_hit`` and ``mgr.cache_miss`` you'll find the
hit/miss ratio of the mgr cache.

Using modules
-------------

Use the command ``stone mgr module ls`` to see which modules are
available, and which are currently enabled.  Enable or disable modules
using the commands ``stone mgr module enable <module>`` and
``stone mgr module disable <module>`` respectively.

If a module is *enabled* then the active stone-mgr daemon will load
and execute it.  In the case of modules that provide a service,
such as an HTTP server, the module may publish its address when it
is loaded.  To see the addresses of such modules, use the command
``stone mgr services``.

Some modules may also implement a special standby mode which runs on
standby stone-mgr daemons as well as the active daemon.  This enables
modules that provide services to redirect their clients to the active
daemon, if the client tries to connect to a standby.

Consult the documentation pages for individual manager modules for more
information about what functionality each module provides.

Here is an example of enabling the :term:`Dashboard` module:

::

	$ stone mgr module ls
	{
		"enabled_modules": [
			"restful",
			"status"
		],
		"disabled_modules": [
			"dashboard"
		]
	}

	$ stone mgr module enable dashboard
	$ stone mgr module ls
	{
		"enabled_modules": [
			"restful",
			"status",
			"dashboard"
		],
		"disabled_modules": [
		]
	}

	$ stone mgr services
	{
		"dashboard": "http://myserver.com:7789/",
		"restful": "https://myserver.com:8789/"
	}


The first time the cluster starts, it uses the ``mgr_initial_modules``
setting to override which modules to enable.  However, this setting
is ignored through the rest of the lifetime of the cluster: only
use it for bootstrapping.  For example, before starting your
monitor daemons for the first time, you might add a section like
this to your ``stone.conf``:

::

    [mon]
        mgr_initial_modules = dashboard balancer

Calling module commands
-----------------------

Where a module implements command line hooks, the commands will
be accessible as ordinary Stone commands.  Stone will automatically incorporate
module commands into the standard CLI interface and route them appropriately to
the module.::

    stone <command | help>

Configuration
-------------

``mgr_module_path``

:Description: Path to load modules from
:Type: String
:Default: ``"<library dir>/mgr"``

``mgr_data``

:Description: Path to load daemon data (such as keyring)
:Type: String
:Default: ``"/var/lib/stone/mgr/$cluster-$id"``

``mgr_tick_period``

:Description: How many seconds between mgr beacons to monitors, and other
              periodic checks.
:Type: Integer
:Default: ``5``

``mon_mgr_beacon_grace``

:Description: How long after last beacon should a mgr be considered failed
:Type: Integer
:Default: ``30``

.. _Modifying User Capabilities: ../../rados/operations/user-management/#modify-user-capabilities
