.. _telemetry:

Telemetry Module
================

The telemetry module sends anonymous data about the cluster back to the Stone
developers to help understand how Stone is used and what problems users may
be experiencing.

This data is visualized on `public dashboards <https://telemetry-public.stone.com/>`_
that allow the community to quickly see summary statistics on how many clusters
are reporting, their total capacity and OSD count, and version distribution
trends.

Channels
--------

The telemetry report is broken down into several "channels", each with
a different type of information.  Assuming telemetry has been enabled,
individual channels can be turned on and off.  (If telemetry is off,
the per-channel setting has no effect.)

* **basic** (default: on): Basic information about the cluster

    - capacity of the cluster
    - number of monitors, managers, OSDs, MDSs, object gateways, or other daemons
    - software version currently being used
    - number and types of RADOS pools and StoneFS file systems
    - names of configuration options that have been changed from their
      default (but *not* their values)

* **crash** (default: on): Information about daemon crashes, including

    - type of daemon
    - version of the daemon
    - operating system (OS distribution, kernel version)
    - stack trace identifying where in the Stone code the crash occurred

* **device** (default: on): Information about device metrics, including

    - anonymized SMART metrics

* **ident** (default: off): User-provided identifying information about
  the cluster

    - cluster description
    - contact email address

The data being reported does *not* contain any sensitive
data like pool names, object names, object contents, hostnames, or device
serial numbers.

It contains counters and statistics on how the cluster has been
deployed, the version of Stone, the distribution of the hosts and other
parameters which help the project to gain a better understanding of
the way Stone is used.

Data is sent secured to *https://telemetry.stone.com*.

Sample report
-------------

You can look at what data is reported at any time with the command::

  stone telemetry show

To protect your privacy, device reports are generated separately, and data such
as hostname and device serial number is anonymized. The device telemetry is
sent to a different endpoint and does not associate the device data with a
particular cluster. To see a preview of the device report use the command::

  stone telemetry show-device

Please note: In order to generate the device report we use Smartmontools
version 7.0 and up, which supports JSON output. 
If you have any concerns about privacy with regard to the information included in
this report, please contact the Stone developers.

Channels
--------

Individual channels can be enabled or disabled with::

  stone config set mgr mgr/telemetry/channel_ident false
  stone config set mgr mgr/telemetry/channel_basic false
  stone config set mgr mgr/telemetry/channel_crash false
  stone config set mgr mgr/telemetry/channel_device false
  stone telemetry show
  stone telemetry show-device

Enabling Telemetry
------------------

To allow the *telemetry* module to start sharing data::

  stone telemetry on

Please note: Telemetry data is licensed under the Community Data License
Agreement - Sharing - Version 1.0 (https://cdla.io/sharing-1-0/). Hence,
telemetry module can be enabled only after you add '--license sharing-1-0' to
the 'stone telemetry on' command.

Telemetry can be disabled at any time with::

  stone telemetry off

Interval
--------

The module compiles and sends a new report every 24 hours by default.
You can adjust this interval with::

  stone config set mgr mgr/telemetry/interval 72    # report every three days

Status
--------

The see the current configuration::

  stone telemetry status

Manually sending telemetry
--------------------------

To ad hoc send telemetry data::

  stone telemetry send

In case telemetry is not enabled (with 'stone telemetry on'), you need to add
'--license sharing-1-0' to 'stone telemetry send' command.

Sending telemetry through a proxy
---------------------------------

If the cluster cannot directly connect to the configured telemetry
endpoint (default *telemetry.stone.com*), you can configure a HTTP/HTTPS
proxy server with::

  stone config set mgr mgr/telemetry/proxy https://10.0.0.1:8080

You can also include a *user:pass* if needed::

  stone config set mgr mgr/telemetry/proxy https://stone:telemetry@10.0.0.1:8080


Contact and Description
-----------------------

A contact and description can be added to the report.  This is
completely optional, and disabled by default.::

  stone config set mgr mgr/telemetry/contact 'John Doe <john.doe@example.com>'
  stone config set mgr mgr/telemetry/description 'My first Stone cluster'
  stone config set mgr mgr/telemetry/channel_ident true

