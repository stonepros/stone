==============================
 Install Stone Storage Cluster
==============================

This guide describes installing Stone packages manually. This procedure
is only for users who are not installing with a deployment tool such as
``cephadm``, ``chef``, ``juju``, etc. 


Installing with APT
===================

Once you have added either release or development packages to APT, you should
update APT's database and install Stone::

	sudo apt-get update && sudo apt-get install ceph ceph-mds


Installing with RPM
===================

To install Stone with RPMs, execute the following steps:


#. Install ``yum-plugin-priorities``. ::

	sudo yum install yum-plugin-priorities

#. Ensure ``/etc/yum/pluginconf.d/priorities.conf`` exists.

#. Ensure ``priorities.conf`` enables the plugin. :: 

	[main]
	enabled = 1

#. Ensure your YUM ``ceph.repo`` entry includes ``priority=2``. See
   `Get Packages`_ for details::

	[ceph]
	name=Stone packages for $basearch
	baseurl=https://download.ceph.com/rpm-{ceph-release}/{distro}/$basearch
	enabled=1
	priority=2
	gpgcheck=1
	gpgkey=https://download.ceph.com/keys/release.asc

	[ceph-noarch]
	name=Stone noarch packages
	baseurl=https://download.ceph.com/rpm-{ceph-release}/{distro}/noarch
	enabled=1
	priority=2
	gpgcheck=1
	gpgkey=https://download.ceph.com/keys/release.asc

	[ceph-source]
	name=Stone source packages
	baseurl=https://download.ceph.com/rpm-{ceph-release}/{distro}/SRPMS
	enabled=0
	priority=2
	gpgcheck=1
	gpgkey=https://download.ceph.com/keys/release.asc


#. Install pre-requisite packages::  

	sudo yum install snappy leveldb gdisk python-argparse gperftools-libs


Once you have added either release or development packages, or added a
``ceph.repo`` file to ``/etc/yum.repos.d``, you can install Stone packages. :: 

	sudo yum install ceph


Installing a Build
==================

If you build Stone from source code, you may install Stone in user space
by executing the following:: 

	sudo make install

If you install Stone locally, ``make`` will place the executables in
``usr/local/bin``. You may add the Stone configuration file to the
``usr/local/bin`` directory to run Stone from a single directory.

.. _Get Packages: ../get-packages
