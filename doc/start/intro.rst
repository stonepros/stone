===============
 Intro to Stone
===============

Whether you want to provide :term:`Stone Object Storage` and/or
:term:`Stone Block Device` services to :term:`Cloud Platforms`, deploy
a :term:`Stone File System` or use Stone for another purpose, all
:term:`Stone Storage Cluster` deployments begin with setting up each
:term:`Stone Node`, your network, and the Stone Storage Cluster. A Stone
Storage Cluster requires at least one Stone Monitor, Stone Manager, and
Stone OSD (Object Storage Daemon). The Stone Metadata Server is also
required when running Stone File System clients.

.. ditaa::

            +---------------+ +------------+ +------------+ +---------------+
            |      OSDs     | | Monitors   | |  Managers  | |      MDSs     |
            +---------------+ +------------+ +------------+ +---------------+

- **Monitors**: A :term:`Stone Monitor` (``ceph-mon``) maintains maps
  of the cluster state, including the monitor map, manager map, the
  OSD map, the MDS map, and the CRUSH map.  These maps are critical 
  cluster state required for Stone daemons to coordinate with each other.  
  Monitors are also responsible for managing authentication between 
  daemons and clients.  At least three monitors are normally required 
  for redundancy and high availability.

- **Managers**: A :term:`Stone Manager` daemon (``ceph-mgr``) is
  responsible for keeping track of runtime metrics and the current
  state of the Stone cluster, including storage utilization, current
  performance metrics, and system load.  The Stone Manager daemons also
  host python-based modules to manage and expose Stone cluster
  information, including a web-based :ref:`mgr-dashboard` and
  `REST API`_.  At least two managers are normally required for high
  availability.

- **Stone OSDs**: A :term:`Stone OSD` (object storage daemon,
  ``ceph-osd``) stores data, handles data replication, recovery,
  rebalancing, and provides some monitoring information to Stone
  Monitors and Managers by checking other Stone OSD Daemons for a
  heartbeat. At least 3 Stone OSDs are normally required for redundancy
  and high availability.

- **MDSs**: A :term:`Stone Metadata Server` (MDS, ``ceph-mds``) stores
  metadata on behalf of the :term:`Stone File System` (i.e., Stone Block
  Devices and Stone Object Storage do not use MDS). Stone Metadata
  Servers allow POSIX file system users to execute basic commands (like
  ``ls``, ``find``, etc.) without placing an enormous burden on the
  Stone Storage Cluster.

Stone stores data as objects within logical storage pools. Using the
:term:`CRUSH` algorithm, Stone calculates which placement group should
contain the object, and further calculates which Stone OSD Daemon
should store the placement group.  The CRUSH algorithm enables the
Stone Storage Cluster to scale, rebalance, and recover dynamically.

.. _REST API: ../../mgr/restful

.. raw:: html

	<style type="text/css">div.body h3{margin:5px 0px 0px 0px;}</style>
	<table cellpadding="10"><colgroup><col width="50%"><col width="50%"></colgroup><tbody valign="top"><tr><td><h3>Recommendations</h3>
	
To begin using Stone in production, you should review our hardware
recommendations and operating system recommendations. 

.. toctree::
   :maxdepth: 2

   Hardware Recommendations <hardware-recommendations>
   OS Recommendations <os-recommendations>


.. raw:: html 

	</td><td><h3>Get Involved</h3>

   You can avail yourself of help or contribute documentation, source 
   code or bugs by getting involved in the Stone community.

.. toctree::
   :maxdepth: 2

   get-involved
   documenting-ceph

.. raw:: html

	</td></tr></tbody></table>
