:orphan:

========================================
 mount.stone -- mount a Stone file system
========================================

.. program:: mount.stone

Synopsis
========

| **mount.stone** [*mon1_socket*\ ,\ *mon2_socket*\ ,...]:/[*subdir*] *dir* [
  -o *options* ]


Description
===========

**mount.stone** is a helper for mounting the Stone file system on a Linux host.
It serves to resolve monitor hostname(s) into IP addresses and read
authentication keys from disk; the Linux kernel client component does most of
the real work. In fact, it is possible to mount a non-authenticated Stone file
system without mount.stone by specifying monitor address(es) by IP::

        mount -t stone 1.2.3.4:/ /mnt/mystonefs

The first argument is the device part of the mount command. It includes host's
socket and path within StoneFS that will be mounted at the mount point. The
socket, obviously, takes the form ip_address[:port]. If the port is not
specified, the Stone default of 6789 is assumed. Multiple monitor addresses can
be passed by separating them by commas. Only one monitor is needed to mount
successfully; the client will learn about all monitors from any responsive
monitor. However, it is a good idea to specify more than one in case the one
happens to be down at the time of mount.

If the host portion of the device is left blank, then **mount.stone** will
attempt to determine monitor addresses using local configuration files
and/or DNS SRV records. In similar way, if authentication is enabled on Stone
cluster (which is done using StoneX) and options ``secret`` and ``secretfile``
are not specified in the command, the mount helper will spawn a child process
that will use the standard Stone library routines to find a keyring and fetch
the secret from it.

A sub-directory of the file system can be mounted by specifying the (absolute)
path to the sub-directory right after ":" after the socket in the device part
of the mount command.

Mount helper application conventions dictate that the first two options are
device to be mounted and the mountpoint for that device. Options must be
passed only after these fixed arguments.


Options
=======

Basic
-----

:command:`conf`
    Path to a stone.conf file. This is used to initialize the Stone context
    for autodiscovery of monitor addresses and auth secrets. The default is
    to use the standard search path for stone.conf files.

:command: `fs=<fs-name>`
    Specify the non-default file system to be mounted. Not passing this
    option mounts the default file system.

:command: `mds_namespace=<fs-name>`
    A synonym of "fs=" and its use is deprecated.

:command:`mount_timeout`
    int (seconds), Default: 60

:command:`ms_mode=<legacy|crc|secure|prefer-crc|prefer-secure>`
    Set the connection mode that the client uses for transport. The available
    modes are:

    - ``legacy``: use messenger v1 protocol to talk to the cluster

    - ``crc``: use messenger v2, without on-the-wire encryption

    - ``secure``: use messenger v2, with on-the-wire encryption

    - ``prefer-crc``: crc mode, if denied agree to secure mode

    - ``prefer-secure``: secure mode, if denied agree to crc mode

:command:`name`
    RADOS user to authenticate as when using StoneX. Default: guest

:command:`secret`
    secret key for use with StoneX. This option is insecure because it exposes
    the secret on the command line. To avoid this, use the secretfile option.

:command:`secretfile`
    path to file containing the secret key to use with StoneX

:command:`recover_session=<no|clean>`
    Set auto reconnect mode in the case where the client is blocklisted. The
    available modes are ``no`` and ``clean``. The default is ``no``.

    - ``no``: never attempt to reconnect when client detects that it has been
       blocklisted. Blocklisted clients will not attempt to reconnect and
       their operations will fail too.

    - ``clean``: client reconnects to the Stone cluster automatically when it
      detects that it has been blocklisted. During reconnect, client drops
      dirty data/metadata, invalidates page caches and writable file handles.
      After reconnect, file locks become stale because the MDS loses track of
      them. If an inode contains any stale file locks, read/write on the inode
      is not allowed until applications release all stale file locks.

Advanced
--------
:command:`cap_release_safety`
    int, Default: calculated

:command:`caps_wanted_delay_max`
    int, cap release delay, Default: 60

:command:`caps_wanted_delay_min`
    int, cap release delay, Default: 5

:command:`dirstat`
    funky `cat dirname` for stats, Default: off

:command:`nodirstat`
    no funky `cat dirname` for stats

:command:`ip`
    my ip

:command:`noasyncreaddir`
    no dcache readdir

:command:`nocrc`
    no data crc on writes

:command:`noshare`
    create a new client instance, instead of sharing an existing instance of
    a client mounting the same cluster

:command:`osdkeepalive`
    int, Default: 5

:command:`osd_idle_ttl`
    int (seconds), Default: 60

:command:`rasize`
    int (bytes), max readahead. Default: 8388608 (8192*1024)

:command:`rbytes`
    Report the recursive size of the directory contents for st_size on
    directories.  Default: off

:command:`norbytes`
    Do not report the recursive size of the directory contents for
    st_size on directories.

:command:`readdir_max_bytes`
    int, Default: 524288 (512*1024)

:command:`readdir_max_entries`
    int, Default: 1024

:command:`rsize`
    int (bytes), max read size. Default: 16777216 (16*1024*1024)

:command:`snapdirname`
    string, set the name of the hidden snapdir. Default: .snap

:command:`write_congestion_kb`
    int (kb), max writeback in flight. scale with available
    memory. Default: calculated from available memory

:command:`wsize`
    int (bytes), max write size. Default: 16777216 (16*1024*1024) (writeback
    uses smaller of wsize and stripe unit)

:command:`wsync`
    Execute all namespace operations synchronously. This ensures that the
    namespace operation will only complete after receiving a reply from
    the MDS. This is the default.

:command:`nowsync`
    Allow the client to do namespace operations asynchronously. When this
    option is enabled, a namespace operation may complete before the MDS
    replies, if it has sufficient capabilities to do so.

Examples
========

Mount the full file system::

    mount.stone :/ /mnt/mystonefs

Assuming mount.stone is installed properly, it should be automatically invoked
by mount(8)::

    mount -t stone :/ /mnt/mystonefs

Mount only part of the namespace/file system::

    mount.stone :/some/directory/in/stonefs /mnt/mystonefs

Mount non-default FS, in case cluster has multiple FSs::
    mount -t stone :/ /mnt/mystonefs2 -o fs=mystonefs2
    
    or
    
    mount -t stone :/ /mnt/mystonefs2 -o mds_namespace=mystonefs2 # This option name is deprecated.

Pass the monitor host's IP address, optionally::

    mount.stone 192.168.0.1:/ /mnt/mystonefs

Pass the port along with IP address if it's running on a non-standard port::

    mount.stone 192.168.0.1:7000:/ /mnt/mystonefs

If there are multiple monitors, passes addresses separated by a comma::

   mount.stone 192.168.0.1,192.168.0.2,192.168.0.3:/ /mnt/mystonefs

If authentication is enabled on Stone cluster::

    mount.stone :/ /mnt/mystonefs -o name=fs_username

Pass secret key for StoneX user optionally::

    mount.stone :/ /mnt/mystonefs -o name=fs_username,secret=AQATSKdNGBnwLhAAnNDKnH65FmVKpXZJVasUeQ==

Pass file containing secret key to avoid leaving secret key in shell's command
history::

    mount.stone :/ /mnt/mystonefs -o name=fs_username,secretfile=/etc/stone/fs_username.secret


Availability
============

**mount.stone** is part of Stone, a massively scalable, open-source, distributed
storage system. Please refer to the Stone documentation at http://stone.com/docs
for more information.

Feature Availability
====================

The ``recover_session=`` option was added to mainline Linux kernels in v5.4.
``wsync`` and ``nowsync`` were added in v5.7.

See also
========

:doc:`stone-fuse <stone-fuse>`\(8),
:doc:`stone <stone>`\(8)
