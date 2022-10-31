// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2013 Inktank Storage, Inc.
 * Copyright (C) 2013,2014 Cloudwatt <libre.licensing@cloudwatt.com>
 *
 * Author: Loic Dachary <loic@dachary.org>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

/* no guard; may be included multiple times */

/*
 * Define commands that are reported by the monitor's
 * "get_command_descriptions" command, and parsed by the Python
 * frontend 'stone' (and perhaps by other frontends, such as a RESTful
 * server). The format is:
 *
 * COMMAND(signature, helpstring, modulename, req perms, availability)
 * where:
 * signature:  describes the command and its parameters (more below)
 * helpstring: displays in CLI help, API help (nice if it refers to
 *             parameter names from signature, 40-a few hundred chars)
 * modulename: the monitor module or daemon this applies to:
 *             mds, osd, pg (osd), mon, auth, log, config-key, mgr
 * req perms:  required permission in that modulename space to execute command
 *             this also controls what type of REST command is accepted
 *
 * The commands describe themselves completely enough for the separate
 * frontend(s) to be able to accept user input and validate it against
 * the command descriptions, and generate a JSON object that contains
 * key:value mappings of parameter names to validated parameter values.
 *
 * 'signature' is a space-separated list of individual command descriptors;
 * each descriptor is either a literal string, which can contain no spaces or
 * '=' signs (for instance, in "pg stat", both "pg" and "stat" are literal
 * strings representing one descriptor each), or a list of key=val[,key=val...]
 * which also includes no spaces.
 *
 * The key=val form describes a non-literal parameter.  Each will have at
 * least a name= and type=, and each type can have its own type-specific
 * parameters.  The parser is the arbiter of these types and their
 * interpretation.  A few more non-type-specific key=val pairs exist:
 *
 *    req=false marks an optional parameter (default for req is 'true')
 *    n=<n> is a repeat count for how many of this argument must be supplied.
 *          n=1 is the default.
 *          n=N is a special case that means "1 or more".
 *
 * A perhaps-incomplete list of types:
 *
 * StoneInt: Optional: range=min[|max]
 * StoneFloat: Optional range
 * StoneString: optional badchars
 * StoneSocketpath: validation involves "is it S_ISSOCK"
 * StoneIPAddr: v4 or v6 addr with optional port, syntax validated
 * StoneEntityAddr: StoneIPAddr + optional '/nonce'
 * StonePoolname: Plainold string
 * StoneObjectname: Another plainold string
 * StonePgid: n.xxx where n is an int > 0, xxx is a hex number > 0
 * StoneName: daemon name, '*' or '<type>.<id>' (id must be int for type osd)
 * StoneOsdName: osd name, '*' or '<id> or 'osd.<id>' (id must be int)
 * StoneChoices: strings="foo|bar" means this param can be either
 * StoneFilepath: openable file
 * StoneFragment: stonefs 'fragID': val/bits, val in hex 0xnnn, bits in dec
 * StoneUUID: uuid in text matching Python uuid.UUID()
 * StonePrefix: special type assigned to literals
 *
 * Example:
 *
 * COMMAND("auth add "
 *   	   "name=entity,type=StoneString "
 *   	   "name=caps,type=StoneString,n=N,req=false",
 *   	   "add auth info for <name> from input file, or random key "
 *   	   "if no input given, and/or any caps specified in the command")
 *
 * defines a command "auth add" that takes a required argument "entity"
 * of type "StoneString", and from 1 to N arguments named "caps" of type
 * StoneString, at least one of which is required.  The front end will
 * validate user input against this description.  Let's say the user
 * enters auth add client.admin 'mon rwx' 'osd *'.  The result will be a
 * JSON object like {"prefix":"auth add", "entity":"client.admin",
 * "caps":["mon rwx", "osd *"]}.
 * Note that
 * 	- string literals are accumulated into 'prefix'
 * 	- n=1 descriptors are given normal string or int object values
 * 	- n=N descriptors are given array values
 *
 * NOTE: be careful with spaces.  Each descriptor must be separated by
 * one space, no other characters, so if you split lines as above, be
 * sure to close and reopen the quotes, and be careful to include the '
 * separating spaces in the quoted string.
 *
 * The monitor marshals this JSON into a std::map<string, cmd_vartype>
 * where cmd_vartype is a boost::variant type-enforcing discriminated
 * type, so the monitor is expected to know the type of each argument.
 * See cmdparse.cc/h for more details.
 *
 * The flag parameter for COMMAND_WITH_FLAGS macro must be passed using
 * FLAG(f), where 'f' may be one of the following:
 *
 *  NONE      - no flag assigned
 *  NOFORWARD - command may not be forwarded
 *  OBSOLETE  - command is considered obsolete
 *  DEPRECATED - command is considered deprecated
 *  MGR       - command goes to stone-mgr (for luminous+)
 *  POLL      - command is intended to be called periodically by the
 *              client (see iostat)
 *  HIDDEN    - command is hidden (no reported by help etc)
 *  TELL      - tell/asok command. it's an alias of (NOFORWARD | HIDDEN)
 *
 * A command should always be first considered DEPRECATED before being
 * considered OBSOLETE, giving due consideration to users and conforming
 * to any guidelines regarding deprecating commands.
 */

COMMAND("pg map name=pgid,type=StonePgid", "show mapping of pg to osds", \
	"pg", "r")
COMMAND("pg repeer name=pgid,type=StonePgid", "force a PG to repeer",
	"osd", "rw")
COMMAND("osd last-stat-seq name=id,type=StoneOsdName", \
	"get the last pg stats sequence number reported for this osd", \
	"osd", "r")

/*
 * auth commands AuthMonitor.cc
 */

COMMAND("auth export name=entity,type=StoneString,req=false", \
       	"write keyring for requested entity, or master keyring if none given", \
	"auth", "rx")
COMMAND("auth get name=entity,type=StoneString", \
	"write keyring file with requested key", "auth", "rx")
COMMAND("auth get-key name=entity,type=StoneString", "display requested key", \
	"auth", "rx")
COMMAND("auth print-key name=entity,type=StoneString", "display requested key", \
	"auth", "rx")
COMMAND("auth print_key name=entity,type=StoneString", "display requested key", \
	"auth", "rx")
COMMAND_WITH_FLAG("auth list", "list authentication state", "auth", "rx",
		  FLAG(DEPRECATED))
COMMAND("auth ls", "list authentication state", "auth", "rx")
COMMAND("auth import", "auth import: read keyring file from -i <file>",
	"auth", "rwx")
COMMAND("auth add "
	"name=entity,type=StoneString "
	"name=caps,type=StoneString,n=N,req=false",
	"add auth info for <entity> from input file, or random key if no "
        "input is given, and/or any caps specified in the command",
	"auth", "rwx")
COMMAND("auth get-or-create-key "
	"name=entity,type=StoneString "
	"name=caps,type=StoneString,n=N,req=false",
	"get, or add, key for <name> from system/caps pairs specified in the command.  If key already exists, any given caps must match the existing caps for that key.",
	"auth", "rwx")
COMMAND("auth get-or-create "
	"name=entity,type=StoneString "
	"name=caps,type=StoneString,n=N,req=false",
	"add auth info for <entity> from input file, or random key if no input given, and/or any caps specified in the command",
	"auth", "rwx")
COMMAND("fs authorize "
   "name=filesystem,type=StoneString "
   "name=entity,type=StoneString "
	"name=caps,type=StoneString,n=N",
	"add auth for <entity> to access file system <filesystem> based on following directory and permissions pairs",
	"auth", "rwx")
COMMAND("auth caps "
	"name=entity,type=StoneString "
	"name=caps,type=StoneString,n=N",
	"update caps for <name> from caps specified in the command",
	"auth", "rwx")
COMMAND_WITH_FLAG("auth del "
	"name=entity,type=StoneString",
	"delete all caps for <name>",
	"auth", "rwx",
    FLAG(DEPRECATED))
COMMAND("auth rm "
	"name=entity,type=StoneString",
	"remove all caps for <name>",
	"auth", "rwx")

/*
 * Monitor commands (Monitor.cc)
 */
COMMAND_WITH_FLAG("compact", "cause compaction of monitor's leveldb/rocksdb storage",
	     "mon", "rw",
             FLAG(TELL))
COMMAND_WITH_FLAG("scrub", "scrub the monitor stores",
             "mon", "rw",
             FLAG(OBSOLETE))
COMMAND("fsid", "show cluster FSID/UUID", "mon", "r")
COMMAND("log name=logtext,type=StoneString,n=N",
	"log supplied text to the monitor log", "mon", "rw")
COMMAND("log last "
        "name=num,type=StoneInt,range=1,req=false "
        "name=level,type=StoneChoices,strings=debug|info|sec|warn|error,req=false "
        "name=channel,type=StoneChoices,strings=*|cluster|audit|stoneadm,req=false",
	"print last few lines of the cluster log",
	"mon", "r")

COMMAND("status", "show cluster status", "mon", "r")
COMMAND("health name=detail,type=StoneChoices,strings=detail,req=false",
	"show cluster health", "mon", "r")
COMMAND("health mute "\
	"name=code,type=StoneString "
	"name=ttl,type=StoneString,req=false "
	"name=sticky,type=StoneBool,req=false",
	"mute health alert", "mon", "w")
COMMAND("health unmute "\
	"name=code,type=StoneString,req=false",
	"unmute existing health alert mute(s)", "mon", "w")
COMMAND("time-sync-status", "show time sync status", "mon", "r")
COMMAND("df name=detail,type=StoneChoices,strings=detail,req=false",
	"show cluster free space stats", "mon", "r")
COMMAND("report name=tags,type=StoneString,n=N,req=false",
	"report full status of cluster, optional title tag strings",
	"mon", "r")
COMMAND("features", "report of connected features",
        "mon", "r")
COMMAND("quorum_status", "report status of monitor quorum",
	"mon", "r")
COMMAND("mon ok-to-stop "
	"name=ids,type=StoneString,n=N",
	"check whether mon(s) can be safely stopped without reducing immediate "
	"availability",
	"mon", "r")
COMMAND("mon ok-to-add-offline",
	"check whether adding a mon and not starting it would break quorum",
	"mon", "r")
COMMAND("mon ok-to-rm "
	"name=id,type=StoneString",
	"check whether removing the specified mon would break quorum",
	"mon", "r")

COMMAND("tell "
	"name=target,type=StoneName "
	"name=args,type=StoneString,n=N",
	"send a command to a specific daemon", "mon", "rw")
COMMAND_WITH_FLAG("version", "show mon daemon version", "mon", "r",
                  FLAG(TELL))

COMMAND("node ls "
	"name=type,type=StoneChoices,strings=all|osd|mon|mds|mgr,req=false",
	"list all nodes in cluster [type]", "mon", "r")
/*
 * Monitor-specific commands under module 'mon'
 */
COMMAND_WITH_FLAG("mon scrub",
    "scrub the monitor stores",
    "mon", "rw",
    FLAG(NONE))
COMMAND("mon metadata name=id,type=StoneString,req=false",
	"fetch metadata for mon <id>",
	"mon", "r")
COMMAND("mon count-metadata name=property,type=StoneString",
	"count mons by metadata field property",
	"mon", "r")
COMMAND("mon versions",
	"check running versions of monitors",
	"mon", "r")
COMMAND("versions",
	"check running versions of stone daemons",
	"mon", "r")



/*
 * MDS commands (MDSMonitor.cc)
 */

COMMAND_WITH_FLAG("mds stat", "show MDS status", "mds", "r", FLAG(HIDDEN))
COMMAND_WITH_FLAG("mds dump "
	"name=epoch,type=StoneInt,req=false,range=0",
	"dump legacy MDS cluster info, optionally from epoch",
        "mds", "r", FLAG(OBSOLETE))
COMMAND("fs dump "
	"name=epoch,type=StoneInt,req=false,range=0",
	"dump all StoneFS status, optionally from epoch", "mds", "r")
COMMAND_WITH_FLAG("mds getmap "
	"name=epoch,type=StoneInt,req=false,range=0",
	"get MDS map, optionally from epoch", "mds", "r", FLAG(OBSOLETE))
COMMAND("mds metadata name=who,type=StoneString,req=false",
	"fetch metadata for mds <role>",
	"mds", "r")
COMMAND("mds count-metadata name=property,type=StoneString",
	"count MDSs by metadata field property",
	"mds", "r")
COMMAND("mds versions",
	"check running versions of MDSs",
	"mds", "r")
COMMAND_WITH_FLAG("mds tell "
	"name=who,type=StoneString "
	"name=args,type=StoneString,n=N",
	"send command to particular mds", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds stop name=role,type=StoneString", "stop mds",
	"mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds deactivate name=role,type=StoneString",
        "clean up specified MDS rank (use with `set max_mds` to shrink cluster)",
	"mds", "rw", FLAG(OBSOLETE))
COMMAND("mds ok-to-stop name=ids,type=StoneString,n=N",
	"check whether stopping the specified MDS would reduce immediate availability",
	"mds", "r")
COMMAND_WITH_FLAG("mds set_max_mds "
	"name=maxmds,type=StoneInt,range=0",
	"set max MDS index", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds set "
	"name=var,type=StoneChoices,strings=max_mds|max_file_size|inline_data|"
	"allow_new_snaps|allow_multimds|allow_multimds_snaps|allow_dirfrags "
	"name=val,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"set mds parameter <var> to <val>", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds freeze name=role_or_gid,type=StoneString"
	" name=val,type=StoneString",
	"freeze MDS yes/no", "mds", "rw", FLAG(HIDDEN))
// arbitrary limit 0-20 below; worth standing on head to make it
// relate to actual state definitions?
// #include "include/stone_fs.h"
COMMAND_WITH_FLAG("mds set_state "
	"name=gid,type=StoneInt,range=0 "
	"name=state,type=StoneInt,range=0|20",
	"set mds state of <gid> to <numeric-state>", "mds", "rw", FLAG(HIDDEN))
COMMAND("mds fail name=role_or_gid,type=StoneString",
	"Mark MDS failed: trigger a failover if a standby is available",
        "mds", "rw")
COMMAND("mds repaired name=role,type=StoneString",
	"mark a damaged MDS rank as no longer damaged", "mds", "rw")
COMMAND("mds rm "
	"name=gid,type=StoneInt,range=0",
	"remove nonactive mds", "mds", "rw")
COMMAND_WITH_FLAG("mds rmfailed name=role,type=StoneString "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
	"remove failed rank", "mds", "rw", FLAG(HIDDEN))
COMMAND_WITH_FLAG("mds cluster_down", "take MDS cluster down", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds cluster_up", "bring MDS cluster up", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds compat show", "show mds compatibility settings",
	"mds", "r", FLAG(DEPRECATED))
COMMAND("fs compat show "
        "name=fs_name,type=StoneString ",
        "show fs compatibility settings",
	"mds", "r")
COMMAND_WITH_FLAG("mds compat rm_compat "
	"name=feature,type=StoneInt,range=0",
	"remove compatible feature", "mds", "rw", FLAG(DEPRECATED))
COMMAND_WITH_FLAG("mds compat rm_incompat "
	"name=feature,type=StoneInt,range=0",
	"remove incompatible feature", "mds", "rw", FLAG(DEPRECATED))
COMMAND_WITH_FLAG("mds add_data_pool "
	"name=pool,type=StoneString",
	"add data pool <pool>", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds rm_data_pool "
	"name=pool,type=StoneString",
	"remove data pool <pool>", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds remove_data_pool "
	"name=pool,type=StoneString",
	"remove data pool <pool>", "mds", "rw", FLAG(OBSOLETE))
COMMAND_WITH_FLAG("mds newfs "
	"name=metadata,type=StoneInt,range=0 "
	"name=data,type=StoneInt,range=0 "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"make new filesystem using pools <metadata> and <data>",
	"mds", "rw", FLAG(OBSOLETE))
COMMAND("fs new "
	"name=fs_name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=metadata,type=StoneString "
	"name=data,type=StoneString "
	"name=force,type=StoneBool,req=false "
	"name=allow_dangerous_metadata_overlay,type=StoneBool,req=false "
	"name=fscid,type=StoneInt,range=0,req=false "
	"name=recover,type=StoneBool,req=false",
	"make new filesystem using named pools <metadata> and <data>",
	"fs", "rw")
COMMAND("fs fail "
	"name=fs_name,type=StoneString ",
	"bring the file system down and all of its ranks",
	"fs", "rw")
COMMAND("fs rm "
	"name=fs_name,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"disable the named filesystem",
	"fs", "rw")
COMMAND("fs reset "
	"name=fs_name,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"disaster recovery only: reset to a single-MDS map",
	"fs", "rw")
COMMAND("fs ls ",
	"list filesystems",
	"fs", "r")
COMMAND("fs get name=fs_name,type=StoneString",
	"get info about one filesystem",
	"fs", "r")
COMMAND("fs set "
	"name=fs_name,type=StoneString "
	"name=var,type=StoneChoices,strings=max_mds|max_file_size"
        "|allow_new_snaps|inline_data|cluster_down|allow_dirfrags|balancer"
        "|standby_count_wanted|session_timeout|session_autoclose"
        "|allow_standby_replay|down|joinable|min_compat_client "
	"name=val,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false "
	"name=yes_i_really_really_mean_it,type=StoneBool,req=false",
	"set fs parameter <var> to <val>", "mds", "rw")
COMMAND("fs flag set name=flag_name,type=StoneChoices,strings=enable_multiple "
        "name=val,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"Set a global StoneFS flag",
	"fs", "rw")

COMMAND("fs feature ls",
        "list available stonefs features to be set/unset",
	"mds", "r")

COMMAND("fs compat "
        "name=fs_name,type=StoneString "
        "name=subop,type=StoneChoices,strings=rm_compat|rm_incompat|add_compat|add_incompat "
        "name=feature,type=StoneInt "
        "name=feature_str,type=StoneString,req=false ",
        "manipulate compat settings", "fs", "rw")

COMMAND("fs required_client_features "
        "name=fs_name,type=StoneString "
        "name=subop,type=StoneChoices,strings=add|rm "
        "name=val,type=StoneString ",
        "add/remove required features of clients", "mds", "rw")

COMMAND("fs add_data_pool name=fs_name,type=StoneString "
	"name=pool,type=StoneString",
	"add data pool <pool>", "mds", "rw")
COMMAND("fs rm_data_pool name=fs_name,type=StoneString "
	"name=pool,type=StoneString",
	"remove data pool <pool>", "mds", "rw")
COMMAND_WITH_FLAG("fs set_default name=fs_name,type=StoneString",
		  "set the default to the named filesystem",
		  "fs", "rw",
		  FLAG(DEPRECATED))
COMMAND("fs set-default name=fs_name,type=StoneString",
	"set the default to the named filesystem",
	"fs", "rw")
COMMAND("fs mirror enable "
	"name=fs_name,type=StoneString ",
	"enable mirroring for a stone filesystem", "mds", "rw")
COMMAND("fs mirror disable "
	"name=fs_name,type=StoneString ",
	"disable mirroring for a stone filesystem", "mds", "rw")
COMMAND("fs mirror peer_add "
	"name=fs_name,type=StoneString "
	"name=uuid,type=StoneString "
	"name=remote_cluster_spec,type=StoneString "
	"name=remote_fs_name,type=StoneString",
	"add a mirror peer for a stone filesystem", "mds", "rw")
COMMAND("fs mirror peer_remove "
	"name=fs_name,type=StoneString "
	"name=uuid,type=StoneString ",
	"remove a mirror peer for a stone filesystem", "mds", "rw")

/*
 * Monmap commands
 */
COMMAND("mon dump "
	"name=epoch,type=StoneInt,range=0,req=false",
	"dump formatted monmap (optionally from epoch)",
	"mon", "r")
COMMAND("mon stat", "summarize monitor status", "mon", "r")
COMMAND("mon getmap "
	"name=epoch,type=StoneInt,range=0,req=false",
	"get monmap", "mon", "r")
COMMAND("mon add "
	"name=name,type=StoneString "
	"name=addr,type=StoneIPAddr "
	"name=location,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=],req=false",
	"add new monitor named <name> at <addr>, possibly with CRUSH location <location>", "mon", "rw")
COMMAND("mon rm "
	"name=name,type=StoneString",
	"remove monitor named <name>", "mon", "rw")
COMMAND_WITH_FLAG("mon remove "
	"name=name,type=StoneString",
	"remove monitor named <name>", "mon", "rw",
    FLAG(DEPRECATED))
COMMAND("mon feature ls "
        "name=with_value,type=StoneChoices,strings=--with-value,req=false",
        "list available mon map features to be set/unset",
        "mon", "r")
COMMAND("mon feature set "
        "name=feature_name,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
        "set provided feature on mon map",
        "mon", "rw")
COMMAND("mon set-rank "
	"name=name,type=StoneString "
	"name=rank,type=StoneInt",
	"set the rank for the specified mon",
	"mon", "rw")
COMMAND("mon set-addrs "
	"name=name,type=StoneString "
	"name=addrs,type=StoneString",
	"set the addrs (IPs and ports) a specific monitor binds to",
	"mon", "rw")
COMMAND("mon set-weight "
        "name=name,type=StoneString "
        "name=weight,type=StoneInt,range=0|65535",
        "set the weight for the specified mon",
        "mon", "rw")
COMMAND("mon enable-msgr2",
	"enable the msgr2 protocol on port 3300",
	"mon", "rw")
COMMAND("mon set election_strategy " \
	"name=strategy,type=StoneString", \
	"set the election strategy to use; choices classic, disallow, connectivity", \
	"mon", "rw")
COMMAND("mon add disallowed_leader " \
	"name=name,type=StoneString", \
	"prevent the named mon from being a leader", \
	"mon", "rw")
COMMAND("mon rm disallowed_leader " \
	"name=name,type=StoneString", \
	"allow the named mon to be a leader again", \
	"mon", "rw")
COMMAND("mon set_location " \
	"name=name,type=StoneString "
	"name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=]",
	"specify location <args> for the monitor <name>, using CRUSH bucket names", \
	"mon", "rw")
COMMAND("mon enable_stretch_mode " \
	"name=tiebreaker_mon,type=StoneString, "
	"name=new_crush_rule,type=StoneString, "
	"name=dividing_bucket,type=StoneString, ",
	"enable stretch mode, changing the peering rules and "
	"failure handling on all pools with <tiebreaker_mon> "
	"as the tiebreaker and setting <dividing_bucket> locations "
	"as the units for stretching across",
	"mon", "rw")
COMMAND("mon set_new_tiebreaker " \
	"name=name,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"switch the stretch tiebreaker to be the named mon", \
	"mon", "rw")

/*
 * OSD commands
 */
COMMAND("osd stat", "print summary of OSD map", "osd", "r")
COMMAND("osd dump "
	"name=epoch,type=StoneInt,range=0,req=false",
	"print summary of OSD map", "osd", "r")
COMMAND("osd info "
	"name=id,type=StoneOsdName,req=false",
	"print osd's {id} information (instead of all osds from map)",
	"osd", "r")
COMMAND("osd tree "
	"name=epoch,type=StoneInt,range=0,req=false "
	"name=states,type=StoneChoices,strings=up|down|in|out|destroyed,n=N,req=false",
	"print OSD tree", "osd", "r")
COMMAND("osd tree-from "
	"name=epoch,type=StoneInt,range=0,req=false "
	"name=bucket,type=StoneString "
	"name=states,type=StoneChoices,strings=up|down|in|out|destroyed,n=N,req=false",
	"print OSD tree in bucket", "osd", "r")
COMMAND("osd ls "
	"name=epoch,type=StoneInt,range=0,req=false",
	"show all OSD ids", "osd", "r")
COMMAND("osd getmap "
	"name=epoch,type=StoneInt,range=0,req=false",
	"get OSD map", "osd", "r")
COMMAND("osd getcrushmap "
	"name=epoch,type=StoneInt,range=0,req=false",
	"get CRUSH map", "osd", "r")
COMMAND("osd getmaxosd", "show largest OSD id", "osd", "r")
COMMAND("osd ls-tree "
        "name=epoch,type=StoneInt,range=0,req=false "
        "name=name,type=StoneString,req=true",
        "show OSD ids under bucket <name> in the CRUSH map",
        "osd", "r")
COMMAND("osd find "
	"name=id,type=StoneOsdName",
	"find osd <id> in the CRUSH map and show its location",
	"osd", "r")
COMMAND("osd metadata "
	"name=id,type=StoneOsdName,req=false",
	"fetch metadata for osd {id} (default all)",
	"osd", "r")
COMMAND("osd count-metadata name=property,type=StoneString",
	"count OSDs by metadata field property",
	"osd", "r")
COMMAND("osd versions",
	"check running versions of OSDs",
	"osd", "r")
COMMAND("osd numa-status",
	"show NUMA status of OSDs",
	"osd", "r")
COMMAND("osd map "
	"name=pool,type=StonePoolname "
	"name=object,type=StoneObjectname "
	"name=nspace,type=StoneString,req=false",
	"find pg for <object> in <pool> with [namespace]", "osd", "r")
COMMAND_WITH_FLAG("osd lspools",
		  "list pools", "osd", "r", FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd crush rule list", "list crush rules", "osd", "r",
		  FLAG(DEPRECATED))
COMMAND("osd crush rule ls", "list crush rules", "osd", "r")
COMMAND("osd crush rule ls-by-class "
        "name=class,type=StoneString,goodchars=[A-Za-z0-9-_.]",
        "list all crush rules that reference the same <class>",
        "osd", "r")
COMMAND("osd crush rule dump "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.],req=false",
	"dump crush rule <name> (default all)",
	"osd", "r")
COMMAND("osd crush dump",
	"dump crush map",
	"osd", "r")
COMMAND("osd setcrushmap name=prior_version,type=StoneInt,req=false",
	"set crush map from input file",
	"osd", "rw")
COMMAND("osd crush set name=prior_version,type=StoneInt,req=false",
	"set crush map from input file",
	"osd", "rw")
COMMAND("osd crush add-bucket "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
        "name=type,type=StoneString "
        "name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=],req=false",
	"add no-parent (probably root) crush bucket <name> of type <type> "
        "to location <args>",
	"osd", "rw")
COMMAND("osd crush rename-bucket "
	"name=srcname,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=dstname,type=StoneString,goodchars=[A-Za-z0-9-_.]",
	"rename bucket <srcname> to <dstname>",
	"osd", "rw")
COMMAND("osd crush set "
	"name=id,type=StoneOsdName "
	"name=weight,type=StoneFloat,range=0.0 "
	"name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=]",
	"update crushmap position and weight for <name> to <weight> with location <args>",
	"osd", "rw")
COMMAND("osd crush add "
	"name=id,type=StoneOsdName "
	"name=weight,type=StoneFloat,range=0.0 "
	"name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=]",
	"add or update crushmap position and weight for <name> with <weight> and location <args>",
	"osd", "rw")
COMMAND("osd crush set-all-straw-buckets-to-straw2",
        "convert all CRUSH current straw buckets to use the straw2 algorithm",
	"osd", "rw")
COMMAND("osd crush class create "
        "name=class,type=StoneString,goodchars=[A-Za-z0-9-_]",
        "create crush device class <class>",
        "osd", "rw")
COMMAND("osd crush class rm "
        "name=class,type=StoneString,goodchars=[A-Za-z0-9-_]",
        "remove crush device class <class>",
        "osd", "rw")
COMMAND("osd crush set-device-class "
        "name=class,type=StoneString "
	"name=ids,type=StoneString,n=N",
	"set the <class> of the osd(s) <id> [<id>...],"
        "or use <all|any> to set all.",
	"osd", "rw")
COMMAND("osd crush rm-device-class "
        "name=ids,type=StoneString,n=N",
        "remove class of the osd(s) <id> [<id>...],"
        "or use <all|any> to remove all.",
        "osd", "rw")
COMMAND("osd crush class rename "
        "name=srcname,type=StoneString,goodchars=[A-Za-z0-9-_] "
        "name=dstname,type=StoneString,goodchars=[A-Za-z0-9-_]",
        "rename crush device class <srcname> to <dstname>",
        "osd", "rw")
COMMAND("osd crush create-or-move "
	"name=id,type=StoneOsdName "
	"name=weight,type=StoneFloat,range=0.0 "
	"name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=]",
	"create entry or move existing entry for <name> <weight> at/to location <args>",
	"osd", "rw")
COMMAND("osd crush move "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=]",
	"move existing entry for <name> to location <args>",
	"osd", "rw")
COMMAND("osd crush swap-bucket "
	"name=source,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=dest,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"swap existing bucket contents from (orphan) bucket <source> and <target>",
	"osd", "rw")
COMMAND("osd crush link "
	"name=name,type=StoneString "
	"name=args,type=StoneString,n=N,goodchars=[A-Za-z0-9-_.=]",
	"link existing entry for <name> under location <args>",
	"osd", "rw")
COMMAND("osd crush rm "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=ancestor,type=StoneString,req=false,goodchars=[A-Za-z0-9-_.]",
	"remove <name> from crush map (everywhere, or just at <ancestor>)",\
	"osd", "rw")
COMMAND_WITH_FLAG("osd crush remove "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=ancestor,type=StoneString,req=false,goodchars=[A-Za-z0-9-_.]",
	"remove <name> from crush map (everywhere, or just at <ancestor>)",
	"osd", "rw",
    FLAG(DEPRECATED))
COMMAND("osd crush unlink "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=ancestor,type=StoneString,req=false,goodchars=[A-Za-z0-9-_.]",
	"unlink <name> from crush map (everywhere, or just at <ancestor>)",
	"osd", "rw")
COMMAND("osd crush reweight-all",
	"recalculate the weights for the tree to ensure they sum correctly",
	"osd", "rw")
COMMAND("osd crush reweight "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=weight,type=StoneFloat,range=0.0",
	"change <name>'s weight to <weight> in crush map",
	"osd", "rw")
COMMAND("osd crush reweight-subtree "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=weight,type=StoneFloat,range=0.0",
	"change all leaf items beneath <name> to <weight> in crush map",
	"osd", "rw")
COMMAND("osd crush tunables "
	"name=profile,type=StoneChoices,strings=legacy|argonaut|bobtail|firefly|hammer|jewel|optimal|default",
	"set crush tunables values to <profile>", "osd", "rw")
COMMAND("osd crush set-tunable "
	"name=tunable,type=StoneChoices,strings=straw_calc_version "
	"name=value,type=StoneInt",
	"set crush tunable <tunable> to <value>",
	"osd", "rw")
COMMAND("osd crush get-tunable "
	"name=tunable,type=StoneChoices,strings=straw_calc_version",
	"get crush tunable <tunable>",
	"osd", "r")
COMMAND("osd crush show-tunables",
	"show current crush tunables", "osd", "r")
COMMAND("osd crush rule create-simple "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=root,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=type,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=mode,type=StoneChoices,strings=firstn|indep,req=false",
	"create crush rule <name> to start from <root>, replicate across buckets of type <type>, using a choose mode of <firstn|indep> (default firstn; indep best for erasure pools)",
	"osd", "rw")
COMMAND("osd crush rule create-replicated "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=root,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=type,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=class,type=StoneString,goodchars=[A-Za-z0-9-_.],req=false",
	"create crush rule <name> for replicated pool to start from <root>, replicate across buckets of type <type>, use devices of type <class> (ssd or hdd)",
	"osd", "rw")
COMMAND("osd crush rule create-erasure "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=profile,type=StoneString,req=false,goodchars=[A-Za-z0-9-_.=]",
	"create crush rule <name> for erasure coded pool created with <profile> (default default)",
	"osd", "rw")
COMMAND("osd crush rule rm "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] ",
	"remove crush rule <name>", "osd", "rw")
COMMAND("osd crush rule rename "
        "name=srcname,type=StoneString,goodchars=[A-Za-z0-9-_.] "
        "name=dstname,type=StoneString,goodchars=[A-Za-z0-9-_.]",
        "rename crush rule <srcname> to <dstname>",
        "osd", "rw")
COMMAND("osd crush tree "
        "name=shadow,type=StoneChoices,strings=--show-shadow,req=false",
	"dump crush buckets and items in a tree view",
	"osd", "r")
COMMAND("osd crush ls name=node,type=StoneString,goodchars=[A-Za-z0-9-_.]",
	"list items beneath a node in the CRUSH tree",
	"osd", "r")
COMMAND("osd crush class ls",
	"list all crush device classes",
	"osd", "r")
COMMAND("osd crush class ls-osd "
        "name=class,type=StoneString,goodchars=[A-Za-z0-9-_]",
        "list all osds belonging to the specific <class>",
        "osd", "r")
COMMAND("osd crush get-device-class "
        "name=ids,type=StoneString,n=N",
        "get classes of specified osd(s) <id> [<id>...]",
        "osd", "r")
COMMAND("osd crush weight-set ls",
	"list crush weight sets",
	"osd", "r")
COMMAND("osd crush weight-set dump",
	"dump crush weight sets",
	"osd", "r")
COMMAND("osd crush weight-set create-compat",
	"create a default backward-compatible weight-set",
	"osd", "rw")
COMMAND("osd crush weight-set create "
        "name=pool,type=StonePoolname "\
        "name=mode,type=StoneChoices,strings=flat|positional",
	"create a weight-set for a given pool",
	"osd", "rw")
COMMAND("osd crush weight-set rm name=pool,type=StonePoolname",
	"remove the weight-set for a given pool",
	"osd", "rw")
COMMAND("osd crush weight-set rm-compat",
	"remove the backward-compatible weight-set",
	"osd", "rw")
COMMAND("osd crush weight-set reweight "
        "name=pool,type=StonePoolname "
	"name=item,type=StoneString "
        "name=weight,type=StoneFloat,range=0.0,n=N",
	"set weight for an item (bucket or osd) in a pool's weight-set",
	"osd", "rw")
COMMAND("osd crush weight-set reweight-compat "
	"name=item,type=StoneString "
        "name=weight,type=StoneFloat,range=0.0,n=N",
	"set weight for an item (bucket or osd) in the backward-compatible weight-set",
	"osd", "rw")
COMMAND("osd setmaxosd "
	"name=newmax,type=StoneInt,range=0",
	"set new maximum osd value", "osd", "rw")
COMMAND("osd set-full-ratio "
	"name=ratio,type=StoneFloat,range=0.0|1.0",
	"set usage ratio at which OSDs are marked full",
	"osd", "rw")
COMMAND("osd set-backfillfull-ratio "
	"name=ratio,type=StoneFloat,range=0.0|1.0",
	"set usage ratio at which OSDs are marked too full to backfill",
	"osd", "rw")
COMMAND("osd set-nearfull-ratio "
	"name=ratio,type=StoneFloat,range=0.0|1.0",
	"set usage ratio at which OSDs are marked near-full",
	"osd", "rw")
COMMAND("osd get-require-min-compat-client",
        "get the minimum client version we will maintain compatibility with",
        "osd", "r")
COMMAND("osd set-require-min-compat-client "
	"name=version,type=StoneString "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
	"set the minimum client version we will maintain compatibility with",
	"osd", "rw")
COMMAND("osd pause", "pause osd", "osd", "rw")
COMMAND("osd unpause", "unpause osd", "osd", "rw")
COMMAND("osd erasure-code-profile set "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=profile,type=StoneString,n=N,req=false "
	"name=force,type=StoneBool,req=false",
	"create erasure code profile <name> with [<key[=value]> ...] pairs. Add a --force at the end to override an existing profile (VERY DANGEROUS)",
	"osd", "rw")
COMMAND("osd erasure-code-profile get "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.]",
	"get erasure code profile <name>",
	"osd", "r")
COMMAND("osd erasure-code-profile rm "
	"name=name,type=StoneString,goodchars=[A-Za-z0-9-_.]",
	"remove erasure code profile <name>",
	"osd", "rw")
COMMAND("osd erasure-code-profile ls",
	"list all erasure code profiles",
	"osd", "r")
COMMAND("osd set "
	"name=key,type=StoneChoices,strings=full|pause|noup|nodown|"
	"noout|noin|nobackfill|norebalance|norecover|noscrub|nodeep-scrub|"
	"notieragent|nosnaptrim|pglog_hardlimit "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
	"set <key>", "osd", "rw")
COMMAND("osd unset "
	"name=key,type=StoneChoices,strings=full|pause|noup|nodown|"\
	"noout|noin|nobackfill|norebalance|norecover|noscrub|nodeep-scrub|"
	"notieragent|nosnaptrim",
	"unset <key>", "osd", "rw")
COMMAND("osd require-osd-release "\
	"name=release,type=StoneChoices,strings=luminous|mimic|nautilus|octopus|pacific "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
	"set the minimum allowed OSD release to participate in the cluster",
	"osd", "rw")
COMMAND("osd down "
	"name=ids,type=StoneString,n=N "
	"name=definitely_dead,type=StoneBool,req=false",
	"set osd(s) <id> [<id>...] down, "
        "or use <any|all> to set all osds down",
        "osd", "rw")
COMMAND("osd stop "
        "type=StoneString,name=ids,n=N",
        "stop the corresponding osd daemons and mark them as down",
        "osd", "rw")
COMMAND("osd out "
	"name=ids,type=StoneString,n=N",
	"set osd(s) <id> [<id>...] out, "
        "or use <any|all> to set all osds out",
        "osd", "rw")
COMMAND("osd in "
	"name=ids,type=StoneString,n=N",
	"set osd(s) <id> [<id>...] in, "
        "can use <any|all> to automatically set all previously out osds in",
        "osd", "rw")
COMMAND_WITH_FLAG("osd rm "
	"name=ids,type=StoneString,n=N",
	"remove osd(s) <id> [<id>...], "
        "or use <any|all> to remove all osds",
	"osd", "rw",
	FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd add-noup "
        "name=ids,type=StoneString,n=N",
        "mark osd(s) <id> [<id>...] as noup, "
        "or use <all|any> to mark all osds as noup",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd add-nodown "
        "name=ids,type=StoneString,n=N",
        "mark osd(s) <id> [<id>...] as nodown, "
        "or use <all|any> to mark all osds as nodown",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd add-noin "
        "name=ids,type=StoneString,n=N",
        "mark osd(s) <id> [<id>...] as noin, "
        "or use <all|any> to mark all osds as noin",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd add-noout "
        "name=ids,type=StoneString,n=N",
        "mark osd(s) <id> [<id>...] as noout, "
        "or use <all|any> to mark all osds as noout",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd rm-noup "
        "name=ids,type=StoneString,n=N",
        "allow osd(s) <id> [<id>...] to be marked up "
        "(if they are currently marked as noup), "
        "can use <all|any> to automatically filter out all noup osds",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd rm-nodown "
        "name=ids,type=StoneString,n=N",
        "allow osd(s) <id> [<id>...] to be marked down "
        "(if they are currently marked as nodown), "
        "can use <all|any> to automatically filter out all nodown osds",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd rm-noin "
        "name=ids,type=StoneString,n=N",
        "allow osd(s) <id> [<id>...] to be marked in "
        "(if they are currently marked as noin), "
        "can use <all|any> to automatically filter out all noin osds",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd rm-noout "
        "name=ids,type=StoneString,n=N",
        "allow osd(s) <id> [<id>...] to be marked out "
        "(if they are currently marked as noout), "
        "can use <all|any> to automatically filter out all noout osds",
        "osd", "rw",
        FLAG(DEPRECATED))
COMMAND("osd set-group "
        "name=flags,type=StoneString "
        "name=who,type=StoneString,n=N",
        "set <flags> for batch osds or crush nodes, "
        "<flags> must be a comma-separated subset of {noup,nodown,noin,noout}",
        "osd", "rw")
COMMAND("osd unset-group "
        "name=flags,type=StoneString "
        "name=who,type=StoneString,n=N",
        "unset <flags> for batch osds or crush nodes, "
        "<flags> must be a comma-separated subset of {noup,nodown,noin,noout}",
        "osd", "rw")
COMMAND("osd reweight "
	"name=id,type=StoneOsdName "
	"type=StoneFloat,name=weight,range=0.0|1.0",
	"reweight osd to 0.0 < <weight> < 1.0", "osd", "rw")
COMMAND("osd reweightn "
	"name=weights,type=StoneString",
	"reweight osds with {<id>: <weight>,...}",
	"osd", "rw")
COMMAND("osd force-create-pg "
	"name=pgid,type=StonePgid "\
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"force creation of pg <pgid>",
        "osd", "rw")
COMMAND("osd pg-temp "
	"name=pgid,type=StonePgid "
	"name=id,type=StoneOsdName,n=N,req=false",
	"set pg_temp mapping pgid:[<id> [<id>...]] (developers only)",
        "osd", "rw")
COMMAND("osd pg-upmap "
	"name=pgid,type=StonePgid "
	"name=id,type=StoneOsdName,n=N",
	"set pg_upmap mapping <pgid>:[<id> [<id>...]] (developers only)",
        "osd", "rw")
COMMAND("osd rm-pg-upmap "
	"name=pgid,type=StonePgid",
	"clear pg_upmap mapping for <pgid> (developers only)",
        "osd", "rw")
COMMAND("osd pg-upmap-items "
	"name=pgid,type=StonePgid "
	"name=id,type=StoneOsdName,n=N",
	"set pg_upmap_items mapping <pgid>:{<id> to <id>, [...]} (developers only)",
        "osd", "rw")
COMMAND("osd rm-pg-upmap-items "
	"name=pgid,type=StonePgid",
	"clear pg_upmap_items mapping for <pgid> (developers only)",
        "osd", "rw")
COMMAND("osd primary-temp "
	"name=pgid,type=StonePgid "
	"name=id,type=StoneOsdName",
        "set primary_temp mapping pgid:<id>|-1 (developers only)",
        "osd", "rw")
COMMAND("osd primary-affinity "
	"name=id,type=StoneOsdName "
	"type=StoneFloat,name=weight,range=0.0|1.0",
	"adjust osd primary-affinity from 0.0 <= <weight> <= 1.0",
	"osd", "rw")
COMMAND_WITH_FLAG("osd destroy-actual "
        "name=id,type=StoneOsdName "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
        "mark osd as being destroyed. Keeps the ID intact (allowing reuse), "
        "but removes stonex keys, config-key data and lockbox keys, "\
        "rendering data permanently unreadable.",
		  "osd", "rw", FLAG(HIDDEN))
COMMAND("osd purge-new "
        "name=id,type=StoneOsdName "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
        "purge all traces of an OSD that was partially created but never "
	"started",
        "osd", "rw")
COMMAND_WITH_FLAG("osd purge-actual "
        "name=id,type=StoneOsdName "
        "name=yes_i_really_mean_it,type=StoneBool,req=false",
        "purge all osd data from the monitors. Combines `osd destroy`, "
        "`osd rm`, and `osd crush rm`.",
		  "osd", "rw", FLAG(HIDDEN))
COMMAND("osd lost "
	"name=id,type=StoneOsdName "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"mark osd as permanently lost. THIS DESTROYS DATA IF NO MORE REPLICAS EXIST, BE CAREFUL",
	"osd", "rw")
COMMAND_WITH_FLAG("osd create "
	"name=uuid,type=StoneUUID,req=false "
	"name=id,type=StoneOsdName,req=false",
	"create new osd (with optional UUID and ID)", "osd", "rw",
	FLAG(DEPRECATED))
COMMAND("osd new "
        "name=uuid,type=StoneUUID,req=true "
        "name=id,type=StoneOsdName,req=false",
        "Create a new OSD. If supplied, the `id` to be replaced needs to "
        "exist and have been previously destroyed. "
        "Reads secrets from JSON file via `-i <file>` (see man page).",
        "osd", "rw")
COMMAND("osd blocklist "
	"name=blocklistop,type=StoneChoices,strings=add|rm "
	"name=addr,type=StoneEntityAddr "
	"name=expire,type=StoneFloat,range=0.0,req=false",
	"add (optionally until <expire> seconds from now) or remove <addr> from blocklist",
	"osd", "rw")
COMMAND("osd blocklist ls", "show blocklisted clients", "osd", "r")
COMMAND("osd blocklist clear", "clear all blocklisted clients", "osd", "rw")

COMMAND_WITH_FLAG("osd blacklist "
	"name=blacklistop,type=StoneChoices,strings=add|rm "
	"name=addr,type=StoneEntityAddr "
	"name=expire,type=StoneFloat,range=0.0,req=false",
	"add (optionally until <expire> seconds from now) or remove <addr> from blacklist",
	"osd", "rw",
	FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd blacklist ls", "show blacklisted clients", "osd", "r",
	FLAG(DEPRECATED))
COMMAND_WITH_FLAG("osd blacklist clear", "clear all blacklisted clients", "osd", "rw",
	FLAG(DEPRECATED))

COMMAND("osd pool mksnap "
	"name=pool,type=StonePoolname "
	"name=snap,type=StoneString",
	"make snapshot <snap> in <pool>", "osd", "rw")
COMMAND("osd pool rmsnap "
	"name=pool,type=StonePoolname "
	"name=snap,type=StoneString",
	"remove snapshot <snap> from <pool>", "osd", "rw")
COMMAND("osd pool ls "
	"name=detail,type=StoneChoices,strings=detail,req=false",
	"list pools", "osd", "r")
COMMAND("osd pool create "
	"name=pool,type=StonePoolname "
	"name=pg_num,type=StoneInt,range=0,req=false "
	"name=pgp_num,type=StoneInt,range=0,req=false "
        "name=pool_type,type=StoneChoices,strings=replicated|erasure,req=false "
	"name=erasure_code_profile,type=StoneString,req=false,goodchars=[A-Za-z0-9-_.] "
	"name=rule,type=StoneString,req=false "
        "name=expected_num_objects,type=StoneInt,range=0,req=false "
        "name=size,type=StoneInt,range=0,req=false "
	"name=pg_num_min,type=StoneInt,range=0,req=false "
	"name=pg_num_max,type=StoneInt,range=0,req=false "
	"name=autoscale_mode,type=StoneChoices,strings=on|off|warn,req=false "
	"name=bulk,type=StoneBool,req=false "
	"name=target_size_bytes,type=StoneInt,range=0,req=false "
	"name=target_size_ratio,type=StoneFloat,range=0.0,req=false",\
	"create pool", "osd", "rw")
COMMAND_WITH_FLAG("osd pool delete "
	"name=pool,type=StonePoolname "
	"name=pool2,type=StonePoolname,req=false "
	"name=yes_i_really_really_mean_it,type=StoneBool,req=false "
	"name=yes_i_really_really_mean_it_not_faking,type=StoneBool,req=false ",
	"delete pool",
	"osd", "rw",
    FLAG(DEPRECATED))
COMMAND("osd pool rm "
	"name=pool,type=StonePoolname "
	"name=pool2,type=StonePoolname,req=false "
	"name=yes_i_really_really_mean_it,type=StoneBool,req=false "
	"name=yes_i_really_really_mean_it_not_faking,type=StoneBool,req=false ",
	"remove pool",
	"osd", "rw")
COMMAND("osd pool rename "
	"name=srcpool,type=StonePoolname "
	"name=destpool,type=StonePoolname",
	"rename <srcpool> to <destpool>", "osd", "rw")
COMMAND("osd pool get "
	"name=pool,type=StonePoolname "
	"name=var,type=StoneChoices,strings=size|min_size|pg_num|pgp_num|crush_rule|hashpspool|nodelete|nopgchange|nosizechange|write_fadvise_dontneed|noscrub|nodeep-scrub|hit_set_type|hit_set_period|hit_set_count|hit_set_fpp|use_gmt_hitset|target_max_objects|target_max_bytes|cache_target_dirty_ratio|cache_target_dirty_high_ratio|cache_target_full_ratio|cache_min_flush_age|cache_min_evict_age|erasure_code_profile|min_read_recency_for_promote|all|min_write_recency_for_promote|fast_read|hit_set_grade_decay_rate|hit_set_search_last_n|scrub_min_interval|scrub_max_interval|deep_scrub_interval|recovery_priority|recovery_op_priority|scrub_priority|compression_mode|compression_algorithm|compression_required_ratio|compression_max_blob_size|compression_min_blob_size|csum_type|csum_min_block|csum_max_block|allow_ec_overwrites|fingerprint_algorithm|pg_autoscale_mode|pg_autoscale_bias|pg_num_min|pg_num_max|target_size_bytes|target_size_ratio|dedup_tier|dedup_chunk_algorithm|dedup_cdc_chunk_size|bulk",
	"get pool parameter <var>", "osd", "r")
COMMAND("osd pool set "
	"name=pool,type=StonePoolname "
	"name=var,type=StoneChoices,strings=size|min_size|pg_num|pgp_num|pgp_num_actual|crush_rule|hashpspool|nodelete|nopgchange|nosizechange|write_fadvise_dontneed|noscrub|nodeep-scrub|hit_set_type|hit_set_period|hit_set_count|hit_set_fpp|use_gmt_hitset|target_max_bytes|target_max_objects|cache_target_dirty_ratio|cache_target_dirty_high_ratio|cache_target_full_ratio|cache_min_flush_age|cache_min_evict_age|min_read_recency_for_promote|min_write_recency_for_promote|fast_read|hit_set_grade_decay_rate|hit_set_search_last_n|scrub_min_interval|scrub_max_interval|deep_scrub_interval|recovery_priority|recovery_op_priority|scrub_priority|compression_mode|compression_algorithm|compression_required_ratio|compression_max_blob_size|compression_min_blob_size|csum_type|csum_min_block|csum_max_block|allow_ec_overwrites|fingerprint_algorithm|pg_autoscale_mode|pg_autoscale_bias|pg_num_min|pg_num_max|target_size_bytes|target_size_ratio|dedup_tier|dedup_chunk_algorithm|dedup_cdc_chunk_size|bulk "
	"name=val,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"set pool parameter <var> to <val>", "osd", "rw")
// 'val' is a StoneString because it can include a unit.  Perhaps
// there should be a Python type for validation/conversion of strings
// with units.
COMMAND("osd pool set-quota "
	"name=pool,type=StonePoolname "
	"name=field,type=StoneChoices,strings=max_objects|max_bytes "
	"name=val,type=StoneString",
	"set object or byte limit on pool", "osd", "rw")
COMMAND("osd pool get-quota "
        "name=pool,type=StonePoolname ",
        "obtain object or byte limits for pool",
        "osd", "r")
COMMAND("osd pool application enable "
        "name=pool,type=StonePoolname "
        "name=app,type=StoneString,goodchars=[A-Za-z0-9-_.] "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
        "enable use of an application <app> [stonefs,rbd,rgw] on pool <poolname>",
        "osd", "rw")
COMMAND("osd pool application disable "
        "name=pool,type=StonePoolname "
        "name=app,type=StoneString "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
        "disables use of an application <app> on pool <poolname>",
        "osd", "rw")
COMMAND("osd pool application set "
        "name=pool,type=StonePoolname "
        "name=app,type=StoneString "
        "name=key,type=StoneString,goodchars=[A-Za-z0-9-_.] "
        "name=value,type=StoneString,goodchars=[A-Za-z0-9-_.=]",
        "sets application <app> metadata key <key> to <value> on pool <poolname>",
        "osd", "rw")
COMMAND("osd pool application rm "
        "name=pool,type=StonePoolname "
        "name=app,type=StoneString "
        "name=key,type=StoneString",
        "removes application <app> metadata key <key> on pool <poolname>",
        "osd", "rw")
COMMAND("osd pool application get "
        "name=pool,type=StonePoolname,req=fasle "
        "name=app,type=StoneString,req=false "
        "name=key,type=StoneString,req=false",
        "get value of key <key> of application <app> on pool <poolname>",
        "osd", "r")
COMMAND("osd utilization",
	"get basic pg distribution stats",
	"osd", "r")
COMMAND("osd force_healthy_stretch_mode " \
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"force a healthy stretch mode, requiring the full number of CRUSH buckets "
	"to peer and letting all non-tiebreaker monitors be elected leader ",
	"osd", "rw")
COMMAND("osd force_recovery_stretch_mode " \
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"try and force a recovery stretch mode, increasing the "
	"pool size to its non-failure value if currently degraded and "
	"all monitor buckets are up",
	"osd", "rw")


// tiering
COMMAND("osd tier add "
	"name=pool,type=StonePoolname "
	"name=tierpool,type=StonePoolname "
	"name=force_nonempty,type=StoneChoices,strings=--force-nonempty,req=false",
	"add the tier <tierpool> (the second one) to base pool <pool> (the first one)",
	"osd", "rw")
COMMAND("osd tier rm "
	"name=pool,type=StonePoolname "
	"name=tierpool,type=StonePoolname",
	"remove the tier <tierpool> (the second one) from base pool <pool> (the first one)",
	"osd", "rw")
COMMAND_WITH_FLAG("osd tier remove "
	"name=pool,type=StonePoolname "
	"name=tierpool,type=StonePoolname",
	"remove the tier <tierpool> (the second one) from base pool <pool> (the first one)",
	"osd", "rw",
    FLAG(DEPRECATED))
COMMAND("osd tier cache-mode "
	"name=pool,type=StonePoolname "
	"name=mode,type=StoneChoices,strings=writeback|readproxy|readonly|none "
	"name=yes_i_really_mean_it,type=StoneBool,req=false",
	"specify the caching mode for cache tier <pool>", "osd", "rw")
COMMAND("osd tier set-overlay "
	"name=pool,type=StonePoolname "
	"name=overlaypool,type=StonePoolname",
	"set the overlay pool for base pool <pool> to be <overlaypool>", "osd", "rw")
COMMAND("osd tier rm-overlay "
	"name=pool,type=StonePoolname ",
	"remove the overlay pool for base pool <pool>", "osd", "rw")
COMMAND_WITH_FLAG("osd tier remove-overlay "
	"name=pool,type=StonePoolname ",
	"remove the overlay pool for base pool <pool>", "osd", "rw",
    FLAG(DEPRECATED))

COMMAND("osd tier add-cache "
	"name=pool,type=StonePoolname "
	"name=tierpool,type=StonePoolname "
	"name=size,type=StoneInt,range=0",
	"add a cache <tierpool> (the second one) of size <size> to existing pool <pool> (the first one)",
	"osd", "rw")

/*
 * mon/KVMonitor.cc
 */

COMMAND("config-key get "
	"name=key,type=StoneString",
	"get <key>", "config-key", "r")
COMMAND("config-key set "
	"name=key,type=StoneString "
	"name=val,type=StoneString,req=false",
	"set <key> to value <val>", "config-key", "rw")
COMMAND_WITH_FLAG("config-key put "
		  "name=key,type=StoneString "
		  "name=val,type=StoneString,req=false",
		  "put <key>, value <val>", "config-key", "rw",
		  FLAG(DEPRECATED))
COMMAND_WITH_FLAG("config-key del "
	"name=key,type=StoneString",
	"delete <key>", "config-key", "rw",
    FLAG(DEPRECATED))
COMMAND("config-key rm "
	"name=key,type=StoneString",
	"rm <key>", "config-key", "rw")
COMMAND("config-key exists "
	"name=key,type=StoneString",
	"check for <key>'s existence", "config-key", "r")
COMMAND_WITH_FLAG("config-key list ", "list keys", "config-key", "r",
		  FLAG(DEPRECATED))
COMMAND("config-key ls ", "list keys", "config-key", "r")
COMMAND("config-key dump "
	"name=key,type=StoneString,req=false", "dump keys and values (with optional prefix)", "config-key", "r")


/*
 * mon/MgrMonitor.cc
 */
COMMAND("mgr stat",
	"dump basic info about the mgr cluster state",
	"mgr", "r")
COMMAND("mgr dump "
	"name=epoch,type=StoneInt,range=0,req=false",
	"dump the latest MgrMap",
	"mgr", "r")
COMMAND("mgr fail name=who,type=StoneString,req=false",
	"treat the named manager daemon as failed", "mgr", "rw")
COMMAND("mgr module ls",
	"list active mgr modules", "mgr", "r")
COMMAND("mgr services",
	"list service endpoints provided by mgr modules",
        "mgr", "r")
COMMAND("mgr module enable "
	"name=module,type=StoneString "
	"name=force,type=StoneChoices,strings=--force,req=false",
	"enable mgr module", "mgr", "rw")
COMMAND("mgr module disable "
	"name=module,type=StoneString",
	"disable mgr module", "mgr", "rw")
COMMAND("mgr metadata name=who,type=StoneString,req=false",
	"dump metadata for all daemons or a specific daemon",
	"mgr", "r")
COMMAND("mgr count-metadata name=property,type=StoneString",
	"count stone-mgr daemons by metadata field property",
	"mgr", "r")
COMMAND("mgr versions",
	"check running versions of stone-mgr daemons",
	"mgr", "r")

// ConfigMonitor
COMMAND("config set"
	" name=who,type=StoneString"
	" name=name,type=StoneString"
	" name=value,type=StoneString"
	" name=force,type=StoneBool,req=false",
	"Set a configuration option for one or more entities",
	"config", "rw")
COMMAND("config rm"
	" name=who,type=StoneString"
	" name=name,type=StoneString",
	"Clear a configuration option for one or more entities",
	"config", "rw")
COMMAND("config get "
	"name=who,type=StoneString "
	"name=key,type=StoneString,req=False",
	"Show configuration option(s) for an entity",
	"config", "r")
COMMAND("config dump",
	"Show all configuration option(s)",
	"mon", "r")
COMMAND("config help "
	"name=key,type=StoneString",
	"Describe a configuration option",
	"config", "r")
COMMAND("config ls",
	"List available configuration options",
	"config", "r")
COMMAND("config assimilate-conf",
	"Assimilate options from a conf, and return a new, minimal conf file",
	"config", "rw")
COMMAND("config log name=num,type=StoneInt,req=False",
	"Show recent history of config changes",
	"config", "r")
COMMAND("config reset "
	"name=num,type=StoneInt,range=0",
	"Revert configuration to a historical version specified by <num>",
	"config", "rw")
COMMAND("config generate-minimal-conf",
	"Generate a minimal stone.conf file",
	"config", "r")




// these are tell commands that were implemented as CLI commands in
// the broken pre-octopus way that we want to allow to work when a
// monitor has upgraded to octopus+ but the monmap min_mon_release is
// still < octopus.  we exclude things that weren't well supported
// before and that aren't implemented by the octopus mon anymore.
//
// the command set below matches the kludge in Monitor::handle_command
// that shunts these off to the asok machinery.

COMMAND_WITH_FLAG("injectargs "
	    "name=injected_args,type=StoneString,n=N",
	    "inject config arguments into monitor", "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("smart name=devid,type=StoneString,req=false",
            "Query health metrics for underlying device",
	    "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("mon_status",
	    "report status of monitors",
	    "mon", "r",
            FLAG(TELL))
COMMAND_WITH_FLAG("heap "
            "name=heapcmd,type=StoneChoices,strings=dump|start_profiler|stop_profiler|release|stats "
            "name=value,type=StoneString,req=false",
            "show heap usage info (available only if compiled with tcmalloc)",
	    "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("connection scores dump",
		  "show the scores used in connectivity-based elections",
		  "mon", "rwx",
		  FLAG(TELL))
COMMAND_WITH_FLAG("connection scores reset",
		  "reset the scores used in connectivity-based elections",
		  "mon", "rwx",
		  FLAG(TELL))
COMMAND_WITH_FLAG("sync_force "
            "name=validate,type=StoneChoices,strings=--yes-i-really-mean-it,req=false",
            "force sync of and clear monitor store",
            "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("add_bootstrap_peer_hint "
            "name=addr,type=StoneIPAddr",
            "add peer address as potential bootstrap "
            "peer for cluster bringup",
            "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("add_bootstrap_peer_hintv "
            "name=addrv,type=StoneString",
            "add peer address vector as potential bootstrap "
            "peer for cluster bringup",
            "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("quorum enter ",
            "force monitor back into quorum",
            "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("quorum exit",
            "force monitor out of the quorum",
            "mon", "rw",
            FLAG(TELL))
COMMAND_WITH_FLAG("ops",
            "show the ops currently in flight",
            "mon", "r",
            FLAG(TELL))
COMMAND_WITH_FLAG("sessions",
            "list existing sessions",
            "mon", "r",
            FLAG(TELL))
COMMAND_WITH_FLAG("dump_historic_ops",
            "dump_historic_ops",
            "mon", "r",
            FLAG(TELL))
