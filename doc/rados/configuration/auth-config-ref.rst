========================
 Stonex Config Reference
========================

The ``stonex`` protocol is enabled by default. Cryptographic authentication has
some computational costs, though they should generally be quite low.  If the
network environment connecting your client and server hosts is very safe and
you cannot afford authentication, you can turn it off. **This is not generally
recommended**.

.. note:: If you disable authentication, you are at risk of a man-in-the-middle
   attack altering your client/server messages, which could lead to disastrous
   security effects.

For creating users, see `User Management`_. For details on the architecture
of Stonex, see `Architecture - High Availability Authentication`_.


Deployment Scenarios
====================

There are two main scenarios for deploying a Stone cluster, which impact
how you initially configure Stonex. Most first time Stone users use
``stoneadm`` to create a cluster (easiest). For clusters using
other deployment tools (e.g., Chef, Juju, Puppet, etc.), you will need
to use the manual procedures or configure your deployment tool to
bootstrap your monitor(s).

Manual Deployment
-----------------

When you deploy a cluster manually, you have to bootstrap the monitor manually
and create the ``client.admin`` user and keyring. To bootstrap monitors, follow
the steps in `Monitor Bootstrapping`_. The steps for monitor bootstrapping are
the logical steps you must perform when using third party deployment tools like
Chef, Puppet,  Juju, etc.


Enabling/Disabling Stonex
========================

Enabling Stonex requires that you have deployed keys for your monitors,
OSDs and metadata servers. If you are simply toggling Stonex on / off,
you do not have to repeat the bootstrapping procedures.


Enabling Stonex
--------------

When ``stonex`` is enabled, Stone will look for the keyring in the default search
path, which includes ``/etc/stone/$cluster.$name.keyring``. You can override
this location by adding a ``keyring`` option in the ``[global]`` section of
your `Stone configuration`_ file, but this is not recommended.

Execute the following procedures to enable ``stonex`` on a cluster with
authentication disabled. If you (or your deployment utility) have already
generated the keys, you may skip the steps related to generating keys.

#. Create a ``client.admin`` key, and save a copy of the key for your client
   host

   .. prompt:: bash $

     stone auth get-or-create client.admin mon 'allow *' mds 'allow *' mgr 'allow *' osd 'allow *' -o /etc/stone/stone.client.admin.keyring

   **Warning:** This will clobber any existing
   ``/etc/stone/client.admin.keyring`` file. Do not perform this step if a
   deployment tool has already done it for you. Be careful!

#. Create a keyring for your monitor cluster and generate a monitor
   secret key.

   .. prompt:: bash $

     stone-authtool --create-keyring /tmp/stone.mon.keyring --gen-key -n mon. --cap mon 'allow *'

#. Copy the monitor keyring into a ``stone.mon.keyring`` file in every monitor's
   ``mon data`` directory. For example, to copy it to ``mon.a`` in cluster ``stone``,
   use the following

   .. prompt:: bash $

     cp /tmp/stone.mon.keyring /var/lib/stone/mon/stone-a/keyring

#. Generate a secret key for every MGR, where ``{$id}`` is the MGR letter

   .. prompt:: bash $

      stone auth get-or-create mgr.{$id} mon 'allow profile mgr' mds 'allow *' osd 'allow *' -o /var/lib/stone/mgr/stone-{$id}/keyring

#. Generate a secret key for every OSD, where ``{$id}`` is the OSD number

   .. prompt:: bash $

      stone auth get-or-create osd.{$id} mon 'allow rwx' osd 'allow *' -o /var/lib/stone/osd/stone-{$id}/keyring

#. Generate a secret key for every MDS, where ``{$id}`` is the MDS letter

   .. prompt:: bash $

      stone auth get-or-create mds.{$id} mon 'allow rwx' osd 'allow *' mds 'allow *' mgr 'allow profile mds' -o /var/lib/stone/mds/stone-{$id}/keyring

#. Enable ``stonex`` authentication by setting the following options in the
   ``[global]`` section of your `Stone configuration`_ file

   .. code-block:: ini

      auth_cluster_required = stonex
      auth_service_required = stonex
      auth_client_required = stonex


#. Start or restart the Stone cluster. See `Operating a Cluster`_ for details.

For details on bootstrapping a monitor manually, see `Manual Deployment`_.



Disabling Stonex
---------------

The following procedure describes how to disable Stonex. If your cluster
environment is relatively safe, you can offset the computation expense of
running authentication. **We do not recommend it.** However, it may be easier
during setup and/or troubleshooting to temporarily disable authentication.

#. Disable ``stonex`` authentication by setting the following options in the
   ``[global]`` section of your `Stone configuration`_ file

   .. code-block:: ini

      auth_cluster_required = none
      auth_service_required = none
      auth_client_required = none


#. Start or restart the Stone cluster. See `Operating a Cluster`_ for details.


Configuration Settings
======================

Enablement
----------


``auth_cluster_required``

:Description: If enabled, the Stone Storage Cluster daemons (i.e., ``stone-mon``,
              ``stone-osd``, ``stone-mds`` and ``stone-mgr``) must authenticate with
              each other. Valid settings are ``stonex`` or ``none``.

:Type: String
:Required: No
:Default: ``stonex``.


``auth_service_required``

:Description: If enabled, the Stone Storage Cluster daemons require Stone Clients
              to authenticate with the Stone Storage Cluster in order to access
              Stone services. Valid settings are ``stonex`` or ``none``.

:Type: String
:Required: No
:Default: ``stonex``.


``auth_client_required``

:Description: If enabled, the Stone Client requires the Stone Storage Cluster to
              authenticate with the Stone Client. Valid settings are ``stonex``
              or ``none``.

:Type: String
:Required: No
:Default: ``stonex``.


.. index:: keys; keyring

Keys
----

When you run Stone with authentication enabled, ``stone`` administrative commands
and Stone Clients require authentication keys to access the Stone Storage Cluster.

The most common way to provide these keys to the ``stone`` administrative
commands and clients is to include a Stone keyring under the ``/etc/stone``
directory. For Octopus and later releases using ``stoneadm``, the filename
is usually ``stone.client.admin.keyring`` (or ``$cluster.client.admin.keyring``).
If you include the keyring under the ``/etc/stone`` directory, you don't need to
specify a ``keyring`` entry in your Stone configuration file.

We recommend copying the Stone Storage Cluster's keyring file to nodes where you
will run administrative commands, because it contains the ``client.admin`` key.

To perform this step manually, execute the following::

	sudo scp {user}@{stone-cluster-host}:/etc/stone/stone.client.admin.keyring /etc/stone/stone.client.admin.keyring

.. tip:: Ensure the ``stone.keyring`` file has appropriate permissions set
   (e.g., ``chmod 644``) on your client machine.

You may specify the key itself in the Stone configuration file using the ``key``
setting (not recommended), or a path to a keyfile using the ``keyfile`` setting.


``keyring``

:Description: The path to the keyring file.
:Type: String
:Required: No
:Default: ``/etc/stone/$cluster.$name.keyring,/etc/stone/$cluster.keyring,/etc/stone/keyring,/etc/stone/keyring.bin``


``keyfile``

:Description: The path to a key file (i.e,. a file containing only the key).
:Type: String
:Required: No
:Default: None


``key``

:Description: The key (i.e., the text string of the key itself). Not recommended.
:Type: String
:Required: No
:Default: None


Daemon Keyrings
---------------

Administrative users or deployment tools  (e.g., ``stoneadm``) may generate
daemon keyrings in the same way as generating user keyrings.  By default, Stone
stores daemons keyrings inside their data directory. The default keyring
locations, and the capabilities necessary for the daemon to function, are shown
below.

``stone-mon``

:Location: ``$mon_data/keyring``
:Capabilities: ``mon 'allow *'``

``stone-osd``

:Location: ``$osd_data/keyring``
:Capabilities: ``mgr 'allow profile osd' mon 'allow profile osd' osd 'allow *'``

``stone-mds``

:Location: ``$mds_data/keyring``
:Capabilities: ``mds 'allow' mgr 'allow profile mds' mon 'allow profile mds' osd 'allow rwx'``

``stone-mgr``

:Location: ``$mgr_data/keyring``
:Capabilities: ``mon 'allow profile mgr' mds 'allow *' osd 'allow *'``

``radosgw``

:Location: ``$rgw_data/keyring``
:Capabilities: ``mon 'allow rwx' osd 'allow rwx'``


.. note:: The monitor keyring (i.e., ``mon.``) contains a key but no
   capabilities, and is not part of the cluster ``auth`` database.

The daemon data directory locations default to directories of the form::

  /var/lib/stone/$type/$cluster-$id

For example, ``osd.12`` would be::

  /var/lib/stone/osd/stone-12

You can override these locations, but it is not recommended.


.. index:: signatures

Signatures
----------

Stone performs a signature check that provides some limited protection
against messages being tampered with in flight (e.g., by a "man in the
middle" attack).

Like other parts of Stone authentication, Stone provides fine-grained control so
you can enable/disable signatures for service messages between clients and
Stone, and so you can enable/disable signatures for messages between Stone daemons.

Note that even with signatures enabled data is not encrypted in
flight.

``stonex_require_signatures``

:Description: If set to ``true``, Stone requires signatures on all message
              traffic between the Stone Client and the Stone Storage Cluster, and
              between daemons comprising the Stone Storage Cluster.

	      Stone Argonaut and Linux kernel versions prior to 3.19 do
	      not support signatures; if such clients are in use this
	      option can be turned off to allow them to connect.

:Type: Boolean
:Required: No
:Default: ``false``


``stonex_cluster_require_signatures``

:Description: If set to ``true``, Stone requires signatures on all message
              traffic between Stone daemons comprising the Stone Storage Cluster.

:Type: Boolean
:Required: No
:Default: ``false``


``stonex_service_require_signatures``

:Description: If set to ``true``, Stone requires signatures on all message
              traffic between Stone Clients and the Stone Storage Cluster.

:Type: Boolean
:Required: No
:Default: ``false``


``stonex_sign_messages``

:Description: If the Stone version supports message signing, Stone will sign
              all messages so they are more difficult to spoof.

:Type: Boolean
:Default: ``true``


Time to Live
------------

``auth_service_ticket_ttl``

:Description: When the Stone Storage Cluster sends a Stone Client a ticket for
              authentication, the Stone Storage Cluster assigns the ticket a
              time to live.

:Type: Double
:Default: ``60*60``


.. _Monitor Bootstrapping: ../../../install/manual-deployment#monitor-bootstrapping
.. _Operating a Cluster: ../../operations/operating
.. _Manual Deployment: ../../../install/manual-deployment
.. _Stone configuration: ../stone-conf
.. _Architecture - High Availability Authentication: ../../../architecture#high-availability-authentication
.. _User Management: ../../operations/user-management
