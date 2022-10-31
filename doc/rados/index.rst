.. _rados-index:

======================
 Stone Storage Cluster
======================

The :term:`Stone Storage Cluster` is the foundation for all Stone deployments.
Based upon :abbr:`RADOS (Reliable Autonomic Distributed Object Store)`, Stone
Storage Clusters consist of two types of daemons: a :term:`Stone OSD Daemon`
(OSD) stores data as objects on a storage node; and a :term:`Stone Monitor` (MON)
maintains a master copy of the cluster map. A Stone Storage Cluster may contain
thousands of storage nodes. A minimal system will have at least one 
Stone Monitor and two Stone OSD Daemons for data replication. 

The Stone File System, Stone Object Storage and Stone Block Devices read data from
and write data to the Stone Storage Cluster.

.. raw:: html

	<style type="text/css">div.body h3{margin:5px 0px 0px 0px;}</style>
	<table cellpadding="10"><colgroup><col width="33%"><col width="33%"><col width="33%"></colgroup><tbody valign="top"><tr><td><h3>Config and Deploy</h3>

Stone Storage Clusters have a few required settings, but most configuration
settings have default values. A typical deployment uses a deployment tool 
to define a cluster and bootstrap a monitor. See `Deployment`_ for details 
on ``stoneadm.``

.. toctree::
	:maxdepth: 2

	Configuration <configuration/index>
	Deployment <../stoneadm/index>

.. raw:: html 

	</td><td><h3>Operations</h3>

Once you have deployed a Stone Storage Cluster, you may begin operating 
your cluster.

.. toctree::
	:maxdepth: 2
	
	
	Operations <operations/index>

.. toctree::
	:maxdepth: 1

	Man Pages <man/index>


.. toctree:: 
	:hidden:
	
	troubleshooting/index

.. raw:: html 

	</td><td><h3>APIs</h3>

Most Stone deployments use `Stone Block Devices`_, `Stone Object Storage`_ and/or the
`Stone File System`_. You  may also develop applications that talk directly to
the Stone Storage Cluster.

.. toctree::
	:maxdepth: 2

	APIs <api/index>
	
.. raw:: html

	</td></tr></tbody></table>

.. _Stone Block Devices: ../rbd/
.. _Stone File System: ../stonefs/
.. _Stone Object Storage: ../radosgw/
.. _Deployment: ../stoneadm/
