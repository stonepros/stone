:orphan:

==================================
 stone -- stone administration tool
==================================

.. program:: stone

Synopsis
========

| **stone** **auth** [ *add* \| *caps* \| *del* \| *export* \| *get* \| *get-key* \| *get-or-create* \| *get-or-create-key* \| *import* \| *list* \| *print-key* \| *print_key* ] ...

| **stone** **compact**

| **stone** **config** [ *dump* | *ls* | *help* | *get* | *show* | *show-with-defaults* | *set* | *rm* | *log* | *reset* | *assimilate-conf* | *generate-minimal-conf* ] ...

| **stone** **config-key** [ *rm* | *exists* | *get* | *ls* | *dump* | *set* ] ...

| **stone** **daemon** *<name>* \| *<path>* *<command>* ...

| **stone** **daemonperf** *<name>* \| *<path>* [ *interval* [ *count* ] ]

| **stone** **df** *{detail}*

| **stone** **fs** [ *ls* \| *new* \| *reset* \| *rm* \| *authorize* ] ...

| **stone** **fsid**

| **stone** **health** *{detail}*

| **stone** **injectargs** *<injectedargs>* [ *<injectedargs>*... ]

| **stone** **log** *<logtext>* [ *<logtext>*... ]

| **stone** **mds** [ *compat* \| *fail* \| *rm* \| *rmfailed* \| *set_state* \| *stat* \| *repaired* ] ...

| **stone** **mon** [ *add* \| *dump* \| *getmap* \| *remove* \| *stat* ] ...

| **stone** **osd** [ *blocklist* \| *blocked-by* \| *create* \| *new* \| *deep-scrub* \| *df* \| *down* \| *dump* \| *erasure-code-profile* \| *find* \| *getcrushmap* \| *getmap* \| *getmaxosd* \| *in* \| *ls* \| *lspools* \| *map* \| *metadata* \| *ok-to-stop* \| *out* \| *pause* \| *perf* \| *pg-temp* \| *force-create-pg* \| *primary-affinity* \| *primary-temp* \| *repair* \| *reweight* \| *reweight-by-pg* \| *rm* \| *destroy* \| *purge* \| *safe-to-destroy* \| *scrub* \| *set* \| *setcrushmap* \| *setmaxosd*  \| *stat* \| *tree* \| *unpause* \| *unset* ] ...

| **stone** **osd** **crush** [ *add* \| *add-bucket* \| *create-or-move* \| *dump* \| *get-tunable* \| *link* \| *move* \| *remove* \| *rename-bucket* \| *reweight* \| *reweight-all* \| *reweight-subtree* \| *rm* \| *rule* \| *set* \| *set-tunable* \| *show-tunables* \| *tunables* \| *unlink* ] ...

| **stone** **osd** **pool** [ *create* \| *delete* \| *get* \| *get-quota* \| *ls* \| *mksnap* \| *rename* \| *rmsnap* \| *set* \| *set-quota* \| *stats* ] ...

| **stone** **osd** **pool** **application** [ *disable* \| *enable* \| *get* \| *rm* \| *set* ] ...

| **stone** **osd** **tier** [ *add* \| *add-cache* \| *cache-mode* \| *remove* \| *remove-overlay* \| *set-overlay* ] ...

| **stone** **pg** [ *debug* \| *deep-scrub* \| *dump* \| *dump_json* \| *dump_pools_json* \| *dump_stuck* \| *getmap* \| *ls* \| *ls-by-osd* \| *ls-by-pool* \| *ls-by-primary* \| *map* \| *repair* \| *scrub* \| *stat* ] ...

| **stone** **quorum_status**

| **stone** **report** { *<tags>* [ *<tags>...* ] }

| **stone** **status**

| **stone** **sync** **force** {--yes-i-really-mean-it} {--i-know-what-i-am-doing}

| **stone** **tell** *<name (type.id)> <command> [options...]*

| **stone** **version**

Description
===========

:program:`stone` is a control utility which is used for manual deployment and maintenance
of a Stone cluster. It provides a diverse set of commands that allows deployment of
monitors, OSDs, placement groups, MDS and overall maintenance, administration
of the cluster.

Commands
========

auth
----

Manage authentication keys. It is used for adding, removing, exporting
or updating of authentication keys for a particular  entity such as a monitor or
OSD. It uses some additional subcommands.

Subcommand ``add`` adds authentication info for a particular entity from input
file, or random key if no input is given and/or any caps specified in the command.

Usage::

	stone auth add <entity> {<caps> [<caps>...]}

Subcommand ``caps`` updates caps for **name** from caps specified in the command.

Usage::

	stone auth caps <entity> <caps> [<caps>...]

Subcommand ``del`` deletes all caps for ``name``.

Usage::

	stone auth del <entity>

Subcommand ``export`` writes keyring for requested entity, or master keyring if
none given.

Usage::

	stone auth export {<entity>}

Subcommand ``get`` writes keyring file with requested key.

Usage::

	stone auth get <entity>

Subcommand ``get-key`` displays requested key.

Usage::

	stone auth get-key <entity>

Subcommand ``get-or-create`` adds authentication info for a particular entity
from input file, or random key if no input given and/or any caps specified in the
command.

Usage::

	stone auth get-or-create <entity> {<caps> [<caps>...]}

Subcommand ``get-or-create-key`` gets or adds key for ``name`` from system/caps
pairs specified in the command.  If key already exists, any given caps must match
the existing caps for that key.

Usage::

	stone auth get-or-create-key <entity> {<caps> [<caps>...]}

Subcommand ``import`` reads keyring from input file.

Usage::

	stone auth import

Subcommand ``ls`` lists authentication state.

Usage::

	stone auth ls

Subcommand ``print-key`` displays requested key.

Usage::

	stone auth print-key <entity>

Subcommand ``print_key`` displays requested key.

Usage::

	stone auth print_key <entity>


compact
-------

Causes compaction of monitor's leveldb storage.

Usage::

	stone compact


config
------

Configure the cluster. By default, Stone daemons and clients retrieve their
configuration options from monitor when they start, and are updated if any of
the tracked options is changed at run time. It uses following additional
subcommand.

Subcommand ``dump`` to dump all options for the cluster

Usage::

	stone config dump

Subcommand ``ls`` to list all option names for the cluster

Usage::

	stone config ls

Subcommand ``help`` to describe the specified configuration option

Usage::

    stone config help <option>

Subcommand ``get`` to dump the option(s) for the specified entity.

Usage::

    stone config get <who> {<option>}

Subcommand ``show`` to display the running configuration of the specified
entity. Please note, unlike ``get``, which only shows the options managed
by monitor, ``show`` displays all the configurations being actively used.
These options are pulled from several sources, for instance, the compiled-in
default value, the monitor's configuration database, ``stone.conf`` file on
the host. The options can even be overridden at runtime. So, there is chance
that the configuration options in the output of ``show`` could be different
from those in the output of ``get``.

Usage::

	stone config show {<who>}

Subcommand ``show-with-defaults`` to display the running configuration along with the compiled-in defaults of the specified entity

Usage::

	stone config show {<who>}

Subcommand ``set`` to set an option for one or more specified entities

Usage::

    stone config set <who> <option> <value> {--force}

Subcommand ``rm`` to clear an option for one or more entities

Usage::

    stone config rm <who> <option>

Subcommand ``log`` to show recent history of config changes. If `count` option
is omitted it defeaults to 10.

Usage::

    stone config log {<count>}

Subcommand ``reset`` to revert configuration to the specified historical version

Usage::

    stone config reset <version>


Subcommand ``assimilate-conf`` to assimilate options from stdin, and return a
new, minimal conf file

Usage::

    stone config assimilate-conf -i <input-config-path> > <output-config-path>
    stone config assimilate-conf < <input-config-path>

Subcommand ``generate-minimal-conf`` to generate a minimal ``stone.conf`` file,
which can be used for bootstrapping a daemon or a client.

Usage::

    stone config generate-minimal-conf > <minimal-config-path>


config-key
----------

Manage configuration key. Config-key is a general purpose key/value service
offered by the monitors. This service is mainly used by Stone tools and daemons
for persisting various settings. Among which, stone-mgr modules uses it for
storing their options. It uses some additional subcommands.

Subcommand ``rm`` deletes configuration key.

Usage::

	stone config-key rm <key>

Subcommand ``exists`` checks for configuration keys existence.

Usage::

	stone config-key exists <key>

Subcommand ``get`` gets the configuration key.

Usage::

	stone config-key get <key>

Subcommand ``ls`` lists configuration keys.

Usage::

	stone config-key ls

Subcommand ``dump`` dumps configuration keys and values.

Usage::

	stone config-key dump

Subcommand ``set`` puts configuration key and value.

Usage::

	stone config-key set <key> {<val>}


daemon
------

Submit admin-socket commands.

Usage::

	stone daemon {daemon_name|socket_path} {command} ...

Example::

	stone daemon osd.0 help


daemonperf
----------

Watch performance counters from a Stone daemon.

Usage::

	stone daemonperf {daemon_name|socket_path} [{interval} [{count}]]


df
--

Show cluster's free space status.

Usage::

	stone df {detail}

.. _stone features:

features
--------

Show the releases and features of all connected daemons and clients connected
to the cluster, along with the numbers of them in each bucket grouped by the
corresponding features/releases. Each release of Stone supports a different set
of features, expressed by the features bitmask. New cluster features require
that clients support the feature, or else they are not allowed to connect to
these new features. As new features or capabilities are enabled after an
upgrade, older clients are prevented from connecting.

Usage::

    stone features

fs
--

Manage stonefs file systems. It uses some additional subcommands.

Subcommand ``ls`` to list file systems

Usage::

	stone fs ls

Subcommand ``new`` to make a new file system using named pools <metadata> and <data>

Usage::

	stone fs new <fs_name> <metadata> <data>

Subcommand ``reset`` is used for disaster recovery only: reset to a single-MDS map

Usage::

	stone fs reset <fs_name> {--yes-i-really-mean-it}

Subcommand ``rm`` to disable the named file system

Usage::

	stone fs rm <fs_name> {--yes-i-really-mean-it}

Subcommand ``authorize`` creates a new client that will be authorized for the
given path in ``<fs_name>``. Pass ``/`` to authorize for the entire FS.
``<perms>`` below can be ``r``, ``rw`` or ``rwp``.

Usage::

    stone fs authorize <fs_name> client.<client_id> <path> <perms> [<path> <perms>...]

fsid
----

Show cluster's FSID/UUID.

Usage::

	stone fsid


health
------

Show cluster's health.

Usage::

	stone health {detail}


heap
----

Show heap usage info (available only if compiled with tcmalloc)

Usage::

	stone tell <name (type.id)> heap dump|start_profiler|stop_profiler|stats

Subcommand ``release`` to make TCMalloc to releases no-longer-used memory back to the kernel at once. 

Usage::

	stone tell <name (type.id)> heap release

Subcommand ``(get|set)_release_rate`` get or set the TCMalloc memory release rate. TCMalloc releases 
no-longer-used memory back to the kernel gradually. the rate controls how quickly this happens. 
Increase this setting to make TCMalloc to return unused memory more frequently. 0 means never return
memory to system, 1 means wait for 1000 pages after releasing a page to system. It is ``1.0`` by default..

Usage::

	stone tell <name (type.id)> heap get_release_rate|set_release_rate {<val>}

injectargs
----------

Inject configuration arguments into monitor.

Usage::

	stone injectargs <injected_args> [<injected_args>...]


log
---

Log supplied text to the monitor log.

Usage::

	stone log <logtext> [<logtext>...]


mds
---

Manage metadata server configuration and administration. It uses some
additional subcommands.

Subcommand ``compat`` manages compatible features. It uses some additional
subcommands.

Subcommand ``rm_compat`` removes compatible feature.

Usage::

	stone mds compat rm_compat <int[0-]>

Subcommand ``rm_incompat`` removes incompatible feature.

Usage::

	stone mds compat rm_incompat <int[0-]>

Subcommand ``show`` shows mds compatibility settings.

Usage::

	stone mds compat show

Subcommand ``fail`` forces mds to status fail.

Usage::

	stone mds fail <role|gid>

Subcommand ``rm`` removes inactive mds.

Usage::

	stone mds rm <int[0-]> <name> (type.id)>

Subcommand ``rmfailed`` removes failed mds.

Usage::

	stone mds rmfailed <int[0-]>

Subcommand ``set_state`` sets mds state of <gid> to <numeric-state>.

Usage::

	stone mds set_state <int[0-]> <int[0-20]>

Subcommand ``stat`` shows MDS status.

Usage::

	stone mds stat

Subcommand ``repaired`` mark a damaged MDS rank as no longer damaged.

Usage::

	stone mds repaired <role>

mon
---

Manage monitor configuration and administration. It uses some additional
subcommands.

Subcommand ``add`` adds new monitor named <name> at <addr>.

Usage::

	stone mon add <name> <IPaddr[:port]>

Subcommand ``dump`` dumps formatted monmap (optionally from epoch)

Usage::

	stone mon dump {<int[0-]>}

Subcommand ``getmap`` gets monmap.

Usage::

	stone mon getmap {<int[0-]>}

Subcommand ``remove`` removes monitor named <name>.

Usage::

	stone mon remove <name>

Subcommand ``stat`` summarizes monitor status.

Usage::

	stone mon stat

mgr
---

Stone manager daemon configuration and management.

Subcommand ``dump`` dumps the latest MgrMap, which describes the active
and standby manager daemons.

Usage::

  stone mgr dump

Subcommand ``fail`` will mark a manager daemon as failed, removing it
from the manager map.  If it is the active manager daemon a standby
will take its place.

Usage::

  stone mgr fail <name>

Subcommand ``module ls`` will list currently enabled manager modules (plugins).

Usage::

  stone mgr module ls

Subcommand ``module enable`` will enable a manager module.  Available modules are included in MgrMap and visible via ``mgr dump``.

Usage::

  stone mgr module enable <module>

Subcommand ``module disable`` will disable an active manager module.

Usage::

  stone mgr module disable <module>

Subcommand ``metadata`` will report metadata about all manager daemons or, if the name is specified, a single manager daemon.

Usage::

  stone mgr metadata [name]

Subcommand ``versions`` will report a count of running daemon versions.

Usage::

  stone mgr versions

Subcommand ``count-metadata`` will report a count of any daemon metadata field.

Usage::

  stone mgr count-metadata <field>

.. _stone-admin-osd:

osd
---

Manage OSD configuration and administration. It uses some additional
subcommands.

Subcommand ``blocklist`` manage blocklisted clients. It uses some additional
subcommands.

Subcommand ``add`` add <addr> to blocklist (optionally until <expire> seconds
from now)

Usage::

	stone osd blocklist add <EntityAddr> {<float[0.0-]>}

Subcommand ``ls`` show blocklisted clients

Usage::

	stone osd blocklist ls

Subcommand ``rm`` remove <addr> from blocklist

Usage::

	stone osd blocklist rm <EntityAddr>

Subcommand ``blocked-by`` prints a histogram of which OSDs are blocking their peers

Usage::

	stone osd blocked-by

Subcommand ``create`` creates new osd (with optional UUID and ID).

This command is DEPRECATED as of the Luminous release, and will be removed in
a future release.

Subcommand ``new`` should instead be used.

Usage::

	stone osd create {<uuid>} {<id>}

Subcommand ``new`` can be used to create a new OSD or to recreate a previously
destroyed OSD with a specific *id*. The new OSD will have the specified *uuid*,
and the command expects a JSON file containing the base64 stonex key for auth
entity *client.osd.<id>*, as well as optional base64 cepx key for dm-crypt
lockbox access and a dm-crypt key. Specifying a dm-crypt requires specifying
the accompanying lockbox stonex key.

Usage::

    stone osd new {<uuid>} {<id>} -i {<params.json>}

The parameters JSON file is optional but if provided, is expected to maintain
a form of the following format::

    {
        "stonex_secret": "AQBWtwhZdBO5ExAAIDyjK2Bh16ZXylmzgYYEjg==",
	"crush_device_class": "myclass"
    }

Or::

    {
        "stonex_secret": "AQBWtwhZdBO5ExAAIDyjK2Bh16ZXylmzgYYEjg==",
        "stonex_lockbox_secret": "AQDNCglZuaeVCRAAYr76PzR1Anh7A0jswkODIQ==",
        "dmcrypt_key": "<dm-crypt key>",
	"crush_device_class": "myclass"
    }

Or::

    {
	"crush_device_class": "myclass"
    }

The "crush_device_class" property is optional. If specified, it will set the
initial CRUSH device class for the new OSD.


Subcommand ``crush`` is used for CRUSH management. It uses some additional
subcommands.

Subcommand ``add`` adds or updates crushmap position and weight for <name> with
<weight> and location <args>.

Usage::

	stone osd crush add <osdname (id|osd.id)> <float[0.0-]> <args> [<args>...]

Subcommand ``add-bucket`` adds no-parent (probably root) crush bucket <name> of
type <type>.

Usage::

	stone osd crush add-bucket <name> <type>

Subcommand ``create-or-move`` creates entry or moves existing entry for <name>
<weight> at/to location <args>.

Usage::

	stone osd crush create-or-move <osdname (id|osd.id)> <float[0.0-]> <args>
	[<args>...]

Subcommand ``dump`` dumps crush map.

Usage::

	stone osd crush dump

Subcommand ``get-tunable`` get crush tunable straw_calc_version

Usage::

	stone osd crush get-tunable straw_calc_version

Subcommand ``link`` links existing entry for <name> under location <args>.

Usage::

	stone osd crush link <name> <args> [<args>...]

Subcommand ``move`` moves existing entry for <name> to location <args>.

Usage::

	stone osd crush move <name> <args> [<args>...]

Subcommand ``remove`` removes <name> from crush map (everywhere, or just at
<ancestor>).

Usage::

	stone osd crush remove <name> {<ancestor>}

Subcommand ``rename-bucket`` renames bucket <srcname> to <dstname>

Usage::

	stone osd crush rename-bucket <srcname> <dstname>

Subcommand ``reweight`` change <name>'s weight to <weight> in crush map.

Usage::

	stone osd crush reweight <name> <float[0.0-]>

Subcommand ``reweight-all`` recalculate the weights for the tree to
ensure they sum correctly

Usage::

	stone osd crush reweight-all

Subcommand ``reweight-subtree`` changes all leaf items beneath <name>
to <weight> in crush map

Usage::

	stone osd crush reweight-subtree <name> <weight>

Subcommand ``rm`` removes <name> from crush map (everywhere, or just at
<ancestor>).

Usage::

	stone osd crush rm <name> {<ancestor>}

Subcommand ``rule`` is used for creating crush rules. It uses some additional
subcommands.

Subcommand ``create-erasure`` creates crush rule <name> for erasure coded pool
created with <profile> (default default).

Usage::

	stone osd crush rule create-erasure <name> {<profile>}

Subcommand ``create-simple`` creates crush rule <name> to start from <root>,
replicate across buckets of type <type>, using a choose mode of <firstn|indep>
(default firstn; indep best for erasure pools).

Usage::

	stone osd crush rule create-simple <name> <root> <type> {firstn|indep}

Subcommand ``dump`` dumps crush rule <name> (default all).

Usage::

	stone osd crush rule dump {<name>}

Subcommand ``ls`` lists crush rules.

Usage::

	stone osd crush rule ls

Subcommand ``rm`` removes crush rule <name>.

Usage::

	stone osd crush rule rm <name>

Subcommand ``set`` used alone, sets crush map from input file.

Usage::

	stone osd crush set

Subcommand ``set`` with osdname/osd.id update crushmap position and weight
for <name> to <weight> with location <args>.

Usage::

	stone osd crush set <osdname (id|osd.id)> <float[0.0-]> <args> [<args>...]

Subcommand ``set-tunable`` set crush tunable <tunable> to <value>.  The only
tunable that can be set is straw_calc_version.

Usage::

	stone osd crush set-tunable straw_calc_version <value>

Subcommand ``show-tunables`` shows current crush tunables.

Usage::

	stone osd crush show-tunables

Subcommand ``tree`` shows the crush buckets and items in a tree view.

Usage::

	stone osd crush tree

Subcommand ``tunables`` sets crush tunables values to <profile>.

Usage::

	stone osd crush tunables legacy|argonaut|bobtail|firefly|hammer|optimal|default

Subcommand ``unlink`` unlinks <name> from crush map (everywhere, or just at
<ancestor>).

Usage::

	stone osd crush unlink <name> {<ancestor>}

Subcommand ``df`` shows OSD utilization

Usage::

	stone osd df {plain|tree}

Subcommand ``deep-scrub`` initiates deep scrub on specified osd.

Usage::

	stone osd deep-scrub <who>

Subcommand ``down`` sets osd(s) <id> [<id>...] down.

Usage::

	stone osd down <ids> [<ids>...]

Subcommand ``dump`` prints summary of OSD map.

Usage::

	stone osd dump {<int[0-]>}

Subcommand ``erasure-code-profile`` is used for managing the erasure code
profiles. It uses some additional subcommands.

Subcommand ``get`` gets erasure code profile <name>.

Usage::

	stone osd erasure-code-profile get <name>

Subcommand ``ls`` lists all erasure code profiles.

Usage::

	stone osd erasure-code-profile ls

Subcommand ``rm`` removes erasure code profile <name>.

Usage::

	stone osd erasure-code-profile rm <name>

Subcommand ``set`` creates erasure code profile <name> with [<key[=value]> ...]
pairs. Add a --force at the end to override an existing profile (IT IS RISKY).

Usage::

	stone osd erasure-code-profile set <name> {<profile> [<profile>...]}

Subcommand ``find`` find osd <id> in the CRUSH map and shows its location.

Usage::

	stone osd find <int[0-]>

Subcommand ``getcrushmap`` gets CRUSH map.

Usage::

	stone osd getcrushmap {<int[0-]>}

Subcommand ``getmap`` gets OSD map.

Usage::

	stone osd getmap {<int[0-]>}

Subcommand ``getmaxosd`` shows largest OSD id.

Usage::

	stone osd getmaxosd

Subcommand ``in`` sets osd(s) <id> [<id>...] in.

Usage::

	stone osd in <ids> [<ids>...]

Subcommand ``lost`` marks osd as permanently lost. THIS DESTROYS DATA IF NO
MORE REPLICAS EXIST, BE CAREFUL.

Usage::

	stone osd lost <int[0-]> {--yes-i-really-mean-it}

Subcommand ``ls`` shows all OSD ids.

Usage::

	stone osd ls {<int[0-]>}

Subcommand ``lspools`` lists pools.

Usage::

	stone osd lspools {<int>}

Subcommand ``map`` finds pg for <object> in <pool>.

Usage::

	stone osd map <poolname> <objectname>

Subcommand ``metadata`` fetches metadata for osd <id>.

Usage::

	stone osd metadata {int[0-]} (default all)

Subcommand ``out`` sets osd(s) <id> [<id>...] out.

Usage::

	stone osd out <ids> [<ids>...]

Subcommand ``ok-to-stop`` checks whether the list of OSD(s) can be
stopped without immediately making data unavailable.  That is, all
data should remain readable and writeable, although data redundancy
may be reduced as some PGs may end up in a degraded (but active)
state.  It will return a success code if it is okay to stop the
OSD(s), or an error code and informative message if it is not or if no
conclusion can be drawn at the current time.  When ``--max <num>`` is
provided, up to <num> OSDs IDs will return (including the provided
OSDs) that can all be stopped simultaneously.  This allows larger sets
of stoppable OSDs to be generated easily by providing a single
starting OSD and a max.  Additional OSDs are drawn from adjacent locations
in the CRUSH hierarchy.

Usage::

  stone osd ok-to-stop <id> [<ids>...] [--max <num>]

Subcommand ``pause`` pauses osd.

Usage::

	stone osd pause

Subcommand ``perf`` prints dump of OSD perf summary stats.

Usage::

	stone osd perf

Subcommand ``pg-temp`` set pg_temp mapping pgid:[<id> [<id>...]] (developers
only).

Usage::

	stone osd pg-temp <pgid> {<id> [<id>...]}

Subcommand ``force-create-pg`` forces creation of pg <pgid>.

Usage::

	stone osd force-create-pg <pgid>


Subcommand ``pool`` is used for managing data pools. It uses some additional
subcommands.

Subcommand ``create`` creates pool.

Usage::

	stone osd pool create <poolname> {<int[0-]>} {<int[0-]>} {replicated|erasure}
	{<erasure_code_profile>} {<rule>} {<int>} {--autoscale-mode=<on,off,warn>}

Subcommand ``delete`` deletes pool.

Usage::

	stone osd pool delete <poolname> {<poolname>} {--yes-i-really-really-mean-it}

Subcommand ``get`` gets pool parameter <var>.

Usage::

	stone osd pool get <poolname> size|min_size|pg_num|pgp_num|crush_rule|write_fadvise_dontneed

Only for tiered pools::

	stone osd pool get <poolname> hit_set_type|hit_set_period|hit_set_count|hit_set_fpp|
	target_max_objects|target_max_bytes|cache_target_dirty_ratio|cache_target_dirty_high_ratio|
	cache_target_full_ratio|cache_min_flush_age|cache_min_evict_age|
	min_read_recency_for_promote|hit_set_grade_decay_rate|hit_set_search_last_n

Only for erasure coded pools::

	stone osd pool get <poolname> erasure_code_profile

Use ``all`` to get all pool parameters that apply to the pool's type::

	stone osd pool get <poolname> all

Subcommand ``get-quota`` obtains object or byte limits for pool.

Usage::

	stone osd pool get-quota <poolname>

Subcommand ``ls`` list pools

Usage::

	stone osd pool ls {detail}

Subcommand ``mksnap`` makes snapshot <snap> in <pool>.

Usage::

	stone osd pool mksnap <poolname> <snap>

Subcommand ``rename`` renames <srcpool> to <destpool>.

Usage::

	stone osd pool rename <poolname> <poolname>

Subcommand ``rmsnap`` removes snapshot <snap> from <pool>.

Usage::

	stone osd pool rmsnap <poolname> <snap>

Subcommand ``set`` sets pool parameter <var> to <val>.

Usage::

	stone osd pool set <poolname> size|min_size|pg_num|
	pgp_num|crush_rule|hashpspool|nodelete|nopgchange|nosizechange|
	hit_set_type|hit_set_period|hit_set_count|hit_set_fpp|debug_fake_ec_pool|
	target_max_bytes|target_max_objects|cache_target_dirty_ratio|
	cache_target_dirty_high_ratio|
	cache_target_full_ratio|cache_min_flush_age|cache_min_evict_age|
	min_read_recency_for_promote|write_fadvise_dontneed|hit_set_grade_decay_rate|
	hit_set_search_last_n
	<val> {--yes-i-really-mean-it}

Subcommand ``set-quota`` sets object or byte limit on pool.

Usage::

	stone osd pool set-quota <poolname> max_objects|max_bytes <val>

Subcommand ``stats`` obtain stats from all pools, or from specified pool.

Usage::

	stone osd pool stats {<name>}

Subcommand ``application`` is used for adding an annotation to the given
pool. By default, the possible applications are object, block, and file
storage (corresponding app-names are "rgw", "rbd", and "stonefs"). However,
there might be other applications as well. Based on the application, there
may or may not be some processing conducted.

Subcommand ``disable`` disables the given application on the given pool.

Usage::

        stone osd pool application disable <pool-name> <app> {--yes-i-really-mean-it}

Subcommand ``enable`` adds an annotation to the given pool for the mentioned
application.

Usage::

        stone osd pool application enable <pool-name> <app> {--yes-i-really-mean-it}

Subcommand ``get`` displays the value for the given key that is associated
with the given application of the given pool. Not passing the optional
arguments would display all key-value pairs for all applications for all
pools.

Usage::

        stone osd pool application get {<pool-name>} {<app>} {<key>}

Subcommand ``rm`` removes the key-value pair for the given key in the given
application of the given pool.

Usage::

        stone osd pool application rm <pool-name> <app> <key>

Subcommand ``set`` associates or updates, if it already exists, a key-value
pair with the given application for the given pool.

Usage::

        stone osd pool application set <pool-name> <app> <key> <value>

Subcommand ``primary-affinity`` adjust osd primary-affinity from 0.0 <=<weight>
<= 1.0

Usage::

	stone osd primary-affinity <osdname (id|osd.id)> <float[0.0-1.0]>

Subcommand ``primary-temp`` sets primary_temp mapping pgid:<id>|-1 (developers
only).

Usage::

	stone osd primary-temp <pgid> <id>

Subcommand ``repair`` initiates repair on a specified osd.

Usage::

	stone osd repair <who>

Subcommand ``reweight`` reweights osd to 0.0 < <weight> < 1.0.

Usage::

	osd reweight <int[0-]> <float[0.0-1.0]>

Subcommand ``reweight-by-pg`` reweight OSDs by PG distribution
[overload-percentage-for-consideration, default 120].

Usage::

	stone osd reweight-by-pg {<int[100-]>} {<poolname> [<poolname...]}
	{--no-increasing}

Subcommand ``reweight-by-utilization`` reweights OSDs by utilization.  It only reweights
outlier OSDs whose utilization exceeds the average, eg. the default 120%
limits reweight to those OSDs that are more than 20% over the average.
[overload-threshold, default 120 [max_weight_change, default 0.05 [max_osds_to_adjust, default 4]]] 

Usage::

	stone osd reweight-by-utilization {<int[100-]> {<float[0.0-]> {<int[0-]>}}}
	{--no-increasing}

Subcommand ``rm`` removes osd(s) <id> [<id>...] from the OSD map.


Usage::

	stone osd rm <ids> [<ids>...]

Subcommand ``destroy`` marks OSD *id* as *destroyed*, removing its stonex
entity's keys and all of its dm-crypt and daemon-private config key
entries.

This command will not remove the OSD from crush, nor will it remove the
OSD from the OSD map. Instead, once the command successfully completes,
the OSD will show marked as *destroyed*.

In order to mark an OSD as destroyed, the OSD must first be marked as
**lost**.

Usage::

    stone osd destroy <id> {--yes-i-really-mean-it}


Subcommand ``purge`` performs a combination of ``osd destroy``,
``osd rm`` and ``osd crush remove``.

Usage::

    stone osd purge <id> {--yes-i-really-mean-it}

Subcommand ``safe-to-destroy`` checks whether it is safe to remove or
destroy an OSD without reducing overall data redundancy or durability.
It will return a success code if it is definitely safe, or an error
code and informative message if it is not or if no conclusion can be
drawn at the current time.

Usage::

  stone osd safe-to-destroy <id> [<ids>...]

Subcommand ``scrub`` initiates scrub on specified osd.

Usage::

	stone osd scrub <who>

Subcommand ``set`` sets cluster-wide <flag> by updating OSD map.
The ``full`` flag is not honored anymore since the Mimic release, and
``stone osd set full`` is not supported in the Octopus release.

Usage::

	stone osd set pause|noup|nodown|noout|noin|nobackfill|
	norebalance|norecover|noscrub|nodeep-scrub|notieragent

Subcommand ``setcrushmap`` sets crush map from input file.

Usage::

	stone osd setcrushmap

Subcommand ``setmaxosd`` sets new maximum osd value.

Usage::

	stone osd setmaxosd <int[0-]>

Subcommand ``set-require-min-compat-client`` enforces the cluster to be backward
compatible with the specified client version. This subcommand prevents you from
making any changes (e.g., crush tunables, or using new features) that
would violate the current setting. Please note, This subcommand will fail if
any connected daemon or client is not compatible with the features offered by
the given <version>. To see the features and releases of all clients connected
to cluster, please see `stone features`_.

Usage::

    stone osd set-require-min-compat-client <version>

Subcommand ``stat`` prints summary of OSD map.

Usage::

	stone osd stat

Subcommand ``tier`` is used for managing tiers. It uses some additional
subcommands.

Subcommand ``add`` adds the tier <tierpool> (the second one) to base pool <pool>
(the first one).

Usage::

	stone osd tier add <poolname> <poolname> {--force-nonempty}

Subcommand ``add-cache`` adds a cache <tierpool> (the second one) of size <size>
to existing pool <pool> (the first one).

Usage::

	stone osd tier add-cache <poolname> <poolname> <int[0-]>

Subcommand ``cache-mode`` specifies the caching mode for cache tier <pool>.

Usage::

	stone osd tier cache-mode <poolname> writeback|readproxy|readonly|none

Subcommand ``remove`` removes the tier <tierpool> (the second one) from base pool
<pool> (the first one).

Usage::

	stone osd tier remove <poolname> <poolname>

Subcommand ``remove-overlay`` removes the overlay pool for base pool <pool>.

Usage::

	stone osd tier remove-overlay <poolname>

Subcommand ``set-overlay`` set the overlay pool for base pool <pool> to be
<overlaypool>.

Usage::

	stone osd tier set-overlay <poolname> <poolname>

Subcommand ``tree`` prints OSD tree.

Usage::

	stone osd tree {<int[0-]>}

Subcommand ``unpause`` unpauses osd.

Usage::

	stone osd unpause

Subcommand ``unset`` unsets cluster-wide <flag> by updating OSD map.

Usage::

	stone osd unset pause|noup|nodown|noout|noin|nobackfill|
	norebalance|norecover|noscrub|nodeep-scrub|notieragent


pg
--

It is used for managing the placement groups in OSDs. It uses some
additional subcommands.

Subcommand ``debug`` shows debug info about pgs.

Usage::

	stone pg debug unfound_objects_exist|degraded_pgs_exist

Subcommand ``deep-scrub`` starts deep-scrub on <pgid>.

Usage::

	stone pg deep-scrub <pgid>

Subcommand ``dump`` shows human-readable versions of pg map (only 'all' valid
with plain).

Usage::

	stone pg dump {all|summary|sum|delta|pools|osds|pgs|pgs_brief} [{all|summary|sum|delta|pools|osds|pgs|pgs_brief...]}

Subcommand ``dump_json`` shows human-readable version of pg map in json only.

Usage::

	stone pg dump_json {all|summary|sum|delta|pools|osds|pgs|pgs_brief} [{all|summary|sum|delta|pools|osds|pgs|pgs_brief...]}

Subcommand ``dump_pools_json`` shows pg pools info in json only.

Usage::

	stone pg dump_pools_json

Subcommand ``dump_stuck`` shows information about stuck pgs.

Usage::

	stone pg dump_stuck {inactive|unclean|stale|undersized|degraded [inactive|unclean|stale|undersized|degraded...]}
	{<int>}

Subcommand ``getmap`` gets binary pg map to -o/stdout.

Usage::

	stone pg getmap

Subcommand ``ls`` lists pg with specific pool, osd, state

Usage::

	stone pg ls {<int>} {<pg-state> [<pg-state>...]}

Subcommand ``ls-by-osd`` lists pg on osd [osd]

Usage::

	stone pg ls-by-osd <osdname (id|osd.id)> {<int>}
	{<pg-state> [<pg-state>...]}

Subcommand ``ls-by-pool`` lists pg with pool = [poolname]

Usage::

	stone pg ls-by-pool <poolstr> {<int>} {<pg-state> [<pg-state>...]}

Subcommand ``ls-by-primary`` lists pg with primary = [osd]

Usage::

	stone pg ls-by-primary <osdname (id|osd.id)> {<int>}
	{<pg-state> [<pg-state>...]}

Subcommand ``map`` shows mapping of pg to osds.

Usage::

	stone pg map <pgid>

Subcommand ``repair`` starts repair on <pgid>.

Usage::

	stone pg repair <pgid>

Subcommand ``scrub`` starts scrub on <pgid>.

Usage::

	stone pg scrub <pgid>

Subcommand ``stat`` shows placement group status.

Usage::

	stone pg stat


quorum
------

Cause a specific MON to enter or exit quorum.

Usage::

	stone tell mon.<id> quorum enter|exit

quorum_status
-------------

Reports status of monitor quorum.

Usage::

	stone quorum_status


report
------

Reports full status of cluster, optional title tag strings.

Usage::

	stone report {<tags> [<tags>...]}


status
------

Shows cluster status.

Usage::

	stone status


tell
----

Sends a command to a specific daemon.

Usage::

	stone tell <name (type.id)> <command> [options...]


List all available commands.

Usage::

 	stone tell <name (type.id)> help

version
-------

Show mon daemon version

Usage::

	stone version

Options
=======

.. option:: -i infile

   will specify an input file to be passed along as a payload with the
   command to the monitor cluster. This is only used for specific
   monitor commands.

.. option:: -o outfile

   will write any payload returned by the monitor cluster with its
   reply to outfile.  Only specific monitor commands (e.g. osd getmap)
   return a payload.

.. option:: --setuser user

   will apply the appropriate user ownership to the file specified by
   the option '-o'.

.. option:: --setgroup group

   will apply the appropriate group ownership to the file specified by
   the option '-o'.

.. option:: -c stone.conf, --conf=stone.conf

   Use stone.conf configuration file instead of the default
   ``/etc/stone/stone.conf`` to determine monitor addresses during startup.

.. option:: --id CLIENT_ID, --user CLIENT_ID

   Client id for authentication.

.. option:: --name CLIENT_NAME, -n CLIENT_NAME

	Client name for authentication.

.. option:: --cluster CLUSTER

	Name of the Stone cluster.

.. option:: --admin-daemon ADMIN_SOCKET, daemon DAEMON_NAME

	Submit admin-socket commands via admin sockets in /var/run/stone.

.. option:: --admin-socket ADMIN_SOCKET_NOPE

	You probably mean --admin-daemon

.. option:: -s, --status

	Show cluster status.

.. option:: -w, --watch

	Watch live cluster changes on the default 'cluster' channel

.. option:: -W, --watch-channel

	Watch live cluster changes on any channel (cluster, audit, stoneadm, or * for all)

.. option:: --watch-debug

	Watch debug events.

.. option:: --watch-info

	Watch info events.

.. option:: --watch-sec

	Watch security events.

.. option:: --watch-warn

	Watch warning events.

.. option:: --watch-error

	Watch error events.

.. option:: --version, -v

	Display version.

.. option:: --verbose

	Make verbose.

.. option:: --concise

	Make less verbose.

.. option:: -f {json,json-pretty,xml,xml-pretty,plain}, --format

	Format of output.

.. option:: --connect-timeout CLUSTER_TIMEOUT

	Set a timeout for connecting to the cluster.

.. option:: --no-increasing

	 ``--no-increasing`` is off by default. So increasing the osd weight is allowed
         using the ``reweight-by-utilization`` or ``test-reweight-by-utilization`` commands.
         If this option is used with these commands, it will help not to increase osd weight
         even the osd is under utilized.

.. option:: --block

	 block until completion (scrub and deep-scrub only)

Availability
============

:program:`stone` is part of Stone, a massively scalable, open-source, distributed storage system. Please refer to
the Stone documentation at http://stone.com/docs for more information.


See also
========

:doc:`stone-mon <stone-mon>`\(8),
:doc:`stone-osd <stone-osd>`\(8),
:doc:`stone-mds <stone-mds>`\(8)
