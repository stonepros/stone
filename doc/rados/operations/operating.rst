=====================
 Operating a Cluster
=====================

.. index:: systemd; operating a cluster


Running Stone with systemd
==========================

For all distributions that support systemd (CentOS 7, Fedora, Debian
Jessie 8 and later, SUSE), stone daemons are now managed using native
systemd files instead of the legacy sysvinit scripts.  For example::

        sudo systemctl start stone.target       # start all daemons
        sudo systemctl status stone-osd@12      # check status of osd.12

To list the Stone systemd units on a node, execute::

        sudo systemctl status stone\*.service stone\*.target

Starting all Daemons
--------------------

To start all daemons on a Stone Node (irrespective of type), execute the
following::

	sudo systemctl start stone.target


Stopping all Daemons
--------------------

To stop all daemons on a Stone Node (irrespective of type), execute the
following::

        sudo systemctl stop stone\*.service stone\*.target


Starting all Daemons by Type
----------------------------

To start all daemons of a particular type on a Stone Node, execute one of the
following::

        sudo systemctl start stone-osd.target
        sudo systemctl start stone-mon.target
        sudo systemctl start stone-mds.target


Stopping all Daemons by Type
----------------------------

To stop all daemons of a particular type on a Stone Node, execute one of the
following::

        sudo systemctl stop stone-mon\*.service stone-mon.target
        sudo systemctl stop stone-osd\*.service stone-osd.target
        sudo systemctl stop stone-mds\*.service stone-mds.target


Starting a Daemon
-----------------

To start a specific daemon instance on a Stone Node, execute one of the
following::

	sudo systemctl start stone-osd@{id}
	sudo systemctl start stone-mon@{hostname}
	sudo systemctl start stone-mds@{hostname}

For example::

	sudo systemctl start stone-osd@1
	sudo systemctl start stone-mon@stone-server
	sudo systemctl start stone-mds@stone-server


Stopping a Daemon
-----------------

To stop a specific daemon instance on a Stone Node, execute one of the
following::

	sudo systemctl stop stone-osd@{id}
	sudo systemctl stop stone-mon@{hostname}
	sudo systemctl stop stone-mds@{hostname}

For example::

	sudo systemctl stop stone-osd@1
	sudo systemctl stop stone-mon@stone-server
	sudo systemctl stop stone-mds@stone-server


.. index:: Upstart; operating a cluster

Running Stone with Upstart
==========================

Starting all Daemons
--------------------

To start all daemons on a Stone Node (irrespective of type), execute the
following:: 

	sudo start stone-all
	

Stopping all Daemons	
--------------------

To stop all daemons on a Stone Node (irrespective of type), execute the
following:: 

	sudo stop stone-all
	

Starting all Daemons by Type
----------------------------

To start all daemons of a particular type on a Stone Node, execute one of the
following:: 

	sudo start stone-osd-all
	sudo start stone-mon-all
	sudo start stone-mds-all


Stopping all Daemons by Type
----------------------------

To stop all daemons of a particular type on a Stone Node, execute one of the
following::

	sudo stop stone-osd-all
	sudo stop stone-mon-all
	sudo stop stone-mds-all


Starting a Daemon
-----------------

To start a specific daemon instance on a Stone Node, execute one of the
following:: 

	sudo start stone-osd id={id}
	sudo start stone-mon id={hostname}
	sudo start stone-mds id={hostname}

For example:: 

	sudo start stone-osd id=1
	sudo start stone-mon id=stone-server
	sudo start stone-mds id=stone-server


Stopping a Daemon
-----------------

To stop a specific daemon instance on a Stone Node, execute one of the
following:: 

	sudo stop stone-osd id={id}
	sudo stop stone-mon id={hostname}
	sudo stop stone-mds id={hostname}

For example:: 

	sudo stop stone-osd id=1
	sudo start stone-mon id=stone-server
	sudo start stone-mds id=stone-server


.. index:: sysvinit; operating a cluster

Running Stone with sysvinit
==========================

Each time you to **start**, **restart**, and  **stop** Stone daemons (or your
entire cluster) you must specify at least one option and one command. You may
also specify a daemon type or a daemon instance. ::

	{commandline} [options] [commands] [daemons]


The ``stone`` options include:

+-----------------+----------+-------------------------------------------------+
| Option          | Shortcut | Description                                     |
+=================+==========+=================================================+
| ``--verbose``   |  ``-v``  | Use verbose logging.                            |
+-----------------+----------+-------------------------------------------------+
| ``--valgrind``  | ``N/A``  | (Dev and QA only) Use `Valgrind`_ debugging.    |
+-----------------+----------+-------------------------------------------------+
| ``--allhosts``  |  ``-a``  | Execute on all nodes in ``stone.conf.``          |
|                 |          | Otherwise, it only executes on ``localhost``.   |
+-----------------+----------+-------------------------------------------------+
| ``--restart``   | ``N/A``  | Automatically restart daemon if it core dumps.  |
+-----------------+----------+-------------------------------------------------+
| ``--norestart`` | ``N/A``  | Don't restart a daemon if it core dumps.        |
+-----------------+----------+-------------------------------------------------+
| ``--conf``      |  ``-c``  | Use an alternate configuration file.            |
+-----------------+----------+-------------------------------------------------+

The ``stone`` commands include:

+------------------+------------------------------------------------------------+
| Command          | Description                                                |
+==================+============================================================+
|    ``start``     | Start the daemon(s).                                       |
+------------------+------------------------------------------------------------+
|    ``stop``      | Stop the daemon(s).                                        |
+------------------+------------------------------------------------------------+
|  ``forcestop``   | Force the daemon(s) to stop. Same as ``kill -9``           |
+------------------+------------------------------------------------------------+
|   ``killall``    | Kill all daemons of a particular type.                     | 
+------------------+------------------------------------------------------------+
|  ``cleanlogs``   | Cleans out the log directory.                              |
+------------------+------------------------------------------------------------+
| ``cleanalllogs`` | Cleans out **everything** in the log directory.            |
+------------------+------------------------------------------------------------+

For subsystem operations, the ``stone`` service can target specific daemon types
by adding a particular daemon type for the ``[daemons]`` option. Daemon types
include: 

- ``mon``
- ``osd``
- ``mds``



.. _Valgrind: http://www.valgrind.org/
.. _initctl: http://manpages.ubuntu.com/manpages/raring/en/man8/initctl.8.html
