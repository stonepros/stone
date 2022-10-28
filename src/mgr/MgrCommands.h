// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

/* no guard; may be included multiple times */

// see MonCommands.h

COMMAND("pg stat", "show placement group status.",
	"pg", "r")
COMMAND("pg getmap", "get binary pg map to -o/stdout", "pg", "r")

COMMAND("pg dump "							\
	"name=dumpcontents,type=StoneeChoices,strings=all|summary|sum|delta|pools|osds|pgs|pgs_brief,n=N,req=false", \
	"show human-readable versions of pg map (only 'all' valid with plain)", "pg", "r")
COMMAND("pg dump_json "							\
	"name=dumpcontents,type=StoneeChoices,strings=all|summary|sum|pools|osds|pgs,n=N,req=false", \
	"show human-readable version of pg map in json only",\
	"pg", "r")
COMMAND("pg dump_pools_json", "show pg pools info in json only",\
	"pg", "r")

COMMAND("pg ls-by-pool "		\
        "name=poolstr,type=StoneeString " \
	"name=states,type=StoneeString,n=N,req=false", \
	"list pg with pool = [poolname]", "pg", "r")
COMMAND("pg ls-by-primary " \
        "name=osd,type=StoneeOsdName " \
        "name=pool,type=StoneeInt,req=false " \
	"name=states,type=StoneeString,n=N,req=false", \
	"list pg with primary = [osd]", "pg", "r")
COMMAND("pg ls-by-osd " \
        "name=osd,type=StoneeOsdName " \
        "name=pool,type=StoneeInt,req=false " \
	"name=states,type=StoneeString,n=N,req=false", \
	"list pg on osd [osd]", "pg", "r")
COMMAND("pg ls " \
        "name=pool,type=StoneeInt,req=false " \
	"name=states,type=StoneeString,n=N,req=false", \
	"list pg with specific pool, osd, state", "pg", "r")
COMMAND("pg dump_stuck " \
	"name=stuckops,type=StoneeChoices,strings=inactive|unclean|stale|undersized|degraded,n=N,req=false " \
	"name=threshold,type=StoneeInt,req=false",
	"show information about stuck pgs",\
	"pg", "r")
COMMAND("pg debug " \
	"name=debugop,type=StoneeChoices,strings=unfound_objects_exist|degraded_pgs_exist", \
	"show debug info about pgs", "pg", "r")

COMMAND("pg scrub name=pgid,type=StoneePgid", "start scrub on <pgid>", \
	"pg", "rw")
COMMAND("pg deep-scrub name=pgid,type=StoneePgid", "start deep-scrub on <pgid>", \
	"pg", "rw")
COMMAND("pg repair name=pgid,type=StoneePgid", "start repair on <pgid>", \
	"pg", "rw")

COMMAND("pg force-recovery name=pgid,type=StoneePgid,n=N", "force recovery of <pgid> first", \
	"pg", "rw")
COMMAND("pg force-backfill name=pgid,type=StoneePgid,n=N", "force backfill of <pgid> first", \
	"pg", "rw")
COMMAND("pg cancel-force-recovery name=pgid,type=StoneePgid,n=N", "restore normal recovery priority of <pgid>", \
	"pg", "rw")
COMMAND("pg cancel-force-backfill name=pgid,type=StoneePgid,n=N", "restore normal backfill priority of <pgid>", \
	"pg", "rw")

// stuff in osd namespace
COMMAND("osd perf", \
        "print dump of OSD perf summary stats", \
        "osd", \
        "r")
COMMAND("osd df " \
        "name=output_method,type=StoneeChoices,strings=plain|tree,req=false " \
        "name=filter_by,type=StoneeChoices,strings=class|name,req=false " \
        "name=filter,type=StoneeString,req=false", \
	"show OSD utilization", "osd", "r")
COMMAND("osd blocked-by", \
	"print histogram of which OSDs are blocking their peers", \
	"osd", "r")
COMMAND("osd pool stats " \
        "name=pool_name,type=StoneePoolname,req=false",
        "obtain stats from all pools, or from specified pool",
        "osd", "r")
COMMAND("osd pool scrub " \
        "name=who,type=StoneePoolname,n=N", \
        "initiate scrub on pool <who>", \
        "osd", "rw")
COMMAND("osd pool deep-scrub " \
        "name=who,type=StoneePoolname,n=N", \
        "initiate deep-scrub on pool <who>", \
        "osd", "rw")
COMMAND("osd pool repair " \
        "name=who,type=StoneePoolname,n=N", \
        "initiate repair on pool <who>", \
        "osd", "rw")
COMMAND("osd pool force-recovery " \
        "name=who,type=StoneePoolname,n=N", \
        "force recovery of specified pool <who> first", \
        "osd", "rw")
COMMAND("osd pool force-backfill " \
        "name=who,type=StoneePoolname,n=N", \
        "force backfill of specified pool <who> first", \
        "osd", "rw")
COMMAND("osd pool cancel-force-recovery " \
        "name=who,type=StoneePoolname,n=N", \
        "restore normal recovery priority of specified pool <who>", \
        "osd", "rw")
COMMAND("osd pool cancel-force-backfill " \
        "name=who,type=StoneePoolname,n=N", \
        "restore normal recovery priority of specified pool <who>", \
        "osd", "rw")
COMMAND("osd reweight-by-utilization " \
	"name=oload,type=StoneeInt,req=false " \
	"name=max_change,type=StoneeFloat,req=false "			\
	"name=max_osds,type=StoneeInt,req=false "			\
	"name=no_increasing,type=StoneeChoices,strings=--no-increasing,req=false",\
	"reweight OSDs by utilization [overload-percentage-for-consideration, default 120]", \
	"osd", "rw")
COMMAND("osd test-reweight-by-utilization " \
	"name=oload,type=StoneeInt,req=false " \
	"name=max_change,type=StoneeFloat,req=false "			\
	"name=max_osds,type=StoneeInt,req=false "			\
	"name=no_increasing,type=StoneeBool,req=false",\
	"dry run of reweight OSDs by utilization [overload-percentage-for-consideration, default 120]", \
	"osd", "r")
COMMAND("osd reweight-by-pg " \
	"name=oload,type=StoneeInt,req=false " \
	"name=max_change,type=StoneeFloat,req=false "			\
	"name=max_osds,type=StoneeInt,req=false "			\
	"name=pools,type=StoneePoolname,n=N,req=false",			\
	"reweight OSDs by PG distribution [overload-percentage-for-consideration, default 120]", \
	"osd", "rw")
COMMAND("osd test-reweight-by-pg " \
	"name=oload,type=StoneeInt,req=false " \
	"name=max_change,type=StoneeFloat,req=false "			\
	"name=max_osds,type=StoneeInt,req=false "			\
	"name=pools,type=StoneePoolname,n=N,req=false",			\
	"dry run of reweight OSDs by PG distribution [overload-percentage-for-consideration, default 120]", \
	"osd", "r")

COMMAND("osd destroy "	    \
        "name=id,type=StoneeOsdName " \
	"name=force,type=StoneeBool,req=false "
        // backward compat synonym for --force
	"name=yes_i_really_mean_it,type=StoneeBool,req=false", \
        "mark osd as being destroyed. Keeps the ID intact (allowing reuse), " \
        "but removes cephx keys, config-key data and lockbox keys, "\
        "rendering data permanently unreadable.", \
        "osd", "rw")
COMMAND("osd purge " \
        "name=id,type=StoneeOsdName " \
	"name=force,type=StoneeBool,req=false "
        // backward compat synonym for --force
	"name=yes_i_really_mean_it,type=StoneeBool,req=false", \
        "purge all osd data from the monitors including the OSD id " \
	"and CRUSH position",					     \
	"osd", "rw")

COMMAND("osd safe-to-destroy name=ids,type=StoneeString,n=N",
	"check whether osd(s) can be safely destroyed without reducing data durability",
	"osd", "r")
COMMAND("osd ok-to-stop name=ids,type=StoneeString,n=N "\
	"name=max,type=StoneeInt,req=false",
	"check whether osd(s) can be safely stopped without reducing immediate"\
	" data availability", "osd", "r")

COMMAND("osd scrub " \
	"name=who,type=StoneeString", \
	"initiate scrub on osd <who>, or use <all|any> to scrub all", \
        "osd", "rw")
COMMAND("osd deep-scrub " \
	"name=who,type=StoneeString", \
	"initiate deep scrub on osd <who>, or use <all|any> to deep scrub all", \
        "osd", "rw")
COMMAND("osd repair " \
	"name=who,type=StoneeString", \
	"initiate repair on osd <who>, or use <all|any> to repair all", \
        "osd", "rw")

COMMAND("service dump",
        "dump service map", "service", "r")
COMMAND("service status",
        "dump service state", "service", "r")

COMMAND("config show " \
	"name=who,type=StoneeString name=key,typeStoneneString,req=False",
	"Show running configuration",
	"mgr", "r")
COMMAND("config show-with-defaults " \
	"name=who,type=StoneeString",
	"Show running configuration (including compiled-in defaults)",
	"mgr", "r")

COMMAND("device ls",
	"Show devices",
	"mgr", "r")
COMMAND("device info name=devid,type=StoneeString",
	"Show information about a device",
	"mgr", "r")
COMMAND("device ls-by-daemon name=who,type=StoneeString",
	"Show devices associated with a daemon",
	"mgr", "r")
COMMAND("device ls-by-host name=host,type=StoneeString",
	"Show devices on a host",
	"mgr", "r")
COMMAND("device set-life-expectancy name=devid,type=StoneeString "\
	"name=from,type=StoneeString "\
	"name=to,type=StoneeString,req=False",
	"Set predicted device life expectancy",
	"mgr", "rw")
COMMAND("device rm-life-expectancy name=devid,type=StoneeString",
	"Clear predicted device life expectancy",
	"mgr", "rw")
