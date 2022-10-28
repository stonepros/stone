/*
 * Stone string constants
 */
#include "stone_strings.h"
#include "include/types.h"
#include "include/stone_features.h"

const char *stone_entity_type_name(int type)
{
	switch (type) {
	case STONE_ENTITY_TYPE_MDS: return "mds";
	case STONE_ENTITY_TYPE_OSD: return "osd";
	case STONE_ENTITY_TYPE_MON: return "mon";
	case STONE_ENTITY_TYPE_MGR: return "mgr";
	case STONE_ENTITY_TYPE_CLIENT: return "client";
	case STONE_ENTITY_TYPE_AUTH: return "auth";
	default: return "unknown";
	}
}

const char *stone_con_mode_name(int con_mode)
{
	switch (con_mode) {
	case STONE_CON_MODE_UNKNOWN: return "unknown";
	case STONE_CON_MODE_CRC: return "crc";
	case STONE_CON_MODE_SECURE: return "secure";
	default: return "???";
	}
}

const char *stone_osd_op_name(int op)
{
	switch (op) {
#define GENERATE_CASE(op, opcode, str)	case STONE_OSD_OP_##op: return (str);
__STONE_FORALL_OSD_OPS(GENERATE_CASE)
#undef GENERATE_CASE
	default:
		return "???";
	}
}

const char *stone_osd_state_name(int s)
{
	switch (s) {
	case STONE_OSD_EXISTS:
		return "exists";
	case STONE_OSD_UP:
		return "up";
	case STONE_OSD_AUTOOUT:
		return "autoout";
	case STONE_OSD_NEW:
		return "new";
	case STONE_OSD_FULL:
		return "full";
	case STONE_OSD_NEARFULL:
		return "nearfull";
	case STONE_OSD_BACKFILLFULL:
		return "backfillfull";
        case STONE_OSD_DESTROYED:
                return "destroyed";
        case STONE_OSD_NOUP:
                return "noup";
        case STONE_OSD_NODOWN:
                return "nodown";
        case STONE_OSD_NOIN:
                return "noin";
        case STONE_OSD_NOOUT:
                return "noout";
        case STONE_OSD_STOP:
                return "stop";
	default:
		return "???";
	}
}

const char *stone_release_name(int r)
{
	switch (r) {
	case STONE_RELEASE_ARGONAUT:
		return "argonaut";
	case STONE_RELEASE_BOBTAIL:
		return "bobtail";
	case STONE_RELEASE_CUTTLEFISH:
		return "cuttlefish";
	case STONE_RELEASE_DUMPLING:
		return "dumpling";
	case STONE_RELEASE_EMPEROR:
		return "emperor";
	case STONE_RELEASE_FIREFLY:
		return "firefly";
	case STONE_RELEASE_GIANT:
		return "giant";
	case STONE_RELEASE_HAMMER:
		return "hammer";
	case STONE_RELEASE_INFERNALIS:
		return "infernalis";
	case STONE_RELEASE_JEWEL:
		return "jewel";
	case STONE_RELEASE_KRAKEN:
		return "kraken";
	case STONE_RELEASE_LUMINOUS:
		return "luminous";
	case STONE_RELEASE_MIMIC:
		return "mimic";
	case STONE_RELEASE_NAUTILUS:
		return "nautilus";
	case STONE_RELEASE_OCTOPUS:
		return "octopus";
	case STONE_RELEASE_PACIFIC:
		return "pacific";
	default:
		if (r < 0)
			return "unspecified";
		return "unknown";
	}
}

uint64_t stone_release_features(int r)
{
	uint64_t req = 0;

	req |= STONE_FEATURE_CRUSH_TUNABLES;
	if (r <= STONE_RELEASE_CUTTLEFISH)
		return req;

	req |= STONE_FEATURE_CRUSH_TUNABLES2 |
		STONE_FEATURE_OSDHASHPSPOOL;
	if (r <= STONE_RELEASE_EMPEROR)
		return req;

	req |= STONE_FEATURE_CRUSH_TUNABLES3 |
		STONE_FEATURE_OSD_PRIMARY_AFFINITY |
		STONE_FEATURE_OSD_CACHEPOOL;
	if (r <= STONE_RELEASE_GIANT)
		return req;

	req |= STONE_FEATURE_CRUSH_V4;
	if (r <= STONE_RELEASE_INFERNALIS)
		return req;

	req |= STONE_FEATURE_CRUSH_TUNABLES5;
	if (r <= STONE_RELEASE_JEWEL)
		return req;

	req |= STONE_FEATURE_MSG_ADDR2;
	if (r <= STONE_RELEASE_KRAKEN)
		return req;

	req |= STONE_FEATUREMASK_CRUSH_CHOOSE_ARGS; // and overlaps
	if (r <= STONE_RELEASE_LUMINOUS)
		return req;

	return req;
}

/* return oldest/first release that supports these features */
int stone_release_from_features(uint64_t features)
{
	int r = 1;
	while (true) {
		uint64_t need = stone_release_features(r);
		if ((need & features) != need ||
		    r == STONE_RELEASE_MAX) {
			r--;
			need = stone_release_features(r);
			/* we want the first release that looks like this */
			while (r > 1 && stone_release_features(r - 1) == need) {
				r--;
			}
			break;
		}
		++r;
	}
	return r;
}

const char *stone_osd_watch_op_name(int o)
{
	switch (o) {
	case STONE_OSD_WATCH_OP_UNWATCH:
		return "unwatch";
	case STONE_OSD_WATCH_OP_WATCH:
		return "watch";
	case STONE_OSD_WATCH_OP_RECONNECT:
		return "reconnect";
	case STONE_OSD_WATCH_OP_PING:
		return "ping";
	default:
		return "???";
	}
}

const char *stone_osd_alloc_hint_flag_name(int f)
{
	switch (f) {
	case STONE_OSD_ALLOC_HINT_FLAG_SEQUENTIAL_WRITE:
		return "sequential_write";
	case STONE_OSD_ALLOC_HINT_FLAG_RANDOM_WRITE:
		return "random_write";
	case STONE_OSD_ALLOC_HINT_FLAG_SEQUENTIAL_READ:
		return "sequential_read";
	case STONE_OSD_ALLOC_HINT_FLAG_RANDOM_READ:
		return "random_read";
	case STONE_OSD_ALLOC_HINT_FLAG_APPEND_ONLY:
		return "append_only";
	case STONE_OSD_ALLOC_HINT_FLAG_IMMUTABLE:
		return "immutable";
	case STONE_OSD_ALLOC_HINT_FLAG_SHORTLIVED:
		return "shortlived";
	case STONE_OSD_ALLOC_HINT_FLAG_LONGLIVED:
		return "longlived";
	case STONE_OSD_ALLOC_HINT_FLAG_COMPRESSIBLE:
		return "compressible";
	case STONE_OSD_ALLOC_HINT_FLAG_INCOMPRESSIBLE:
		return "incompressible";
	default:
		return "???";
	}
}

const char *stone_mds_state_name(int s)
{
	switch (s) {
		/* down and out */
	case STONE_MDS_STATE_DNE:        return "down:dne";
	case STONE_MDS_STATE_STOPPED:    return "down:stopped";
	case STONE_MDS_STATE_DAMAGED:   return "down:damaged";
		/* up and out */
	case STONE_MDS_STATE_BOOT:       return "up:boot";
	case STONE_MDS_STATE_STANDBY:    return "up:standby";
	case STONE_MDS_STATE_STANDBY_REPLAY:    return "up:standby-replay";
	case STONE_MDS_STATE_REPLAYONCE: return "up:oneshot-replay";
	case STONE_MDS_STATE_CREATING:   return "up:creating";
	case STONE_MDS_STATE_STARTING:   return "up:starting";
		/* up and in */
	case STONE_MDS_STATE_REPLAY:     return "up:replay";
	case STONE_MDS_STATE_RESOLVE:    return "up:resolve";
	case STONE_MDS_STATE_RECONNECT:  return "up:reconnect";
	case STONE_MDS_STATE_REJOIN:     return "up:rejoin";
	case STONE_MDS_STATE_CLIENTREPLAY: return "up:clientreplay";
	case STONE_MDS_STATE_ACTIVE:     return "up:active";
	case STONE_MDS_STATE_STOPPING:   return "up:stopping";
               /* misc */
	case STONE_MDS_STATE_NULL:       return "null";
	}
	return "???";
}

const char *stone_session_op_name(int op)
{
	switch (op) {
	case STONE_SESSION_REQUEST_OPEN: return "request_open";
	case STONE_SESSION_OPEN: return "open";
	case STONE_SESSION_REQUEST_CLOSE: return "request_close";
	case STONE_SESSION_CLOSE: return "close";
	case STONE_SESSION_REQUEST_RENEWCAPS: return "request_renewcaps";
	case STONE_SESSION_RENEWCAPS: return "renewcaps";
	case STONE_SESSION_STALE: return "stale";
	case STONE_SESSION_RECALL_STATE: return "recall_state";
	case STONE_SESSION_FLUSHMSG: return "flushmsg";
	case STONE_SESSION_FLUSHMSG_ACK: return "flushmsg_ack";
	case STONE_SESSION_FORCE_RO: return "force_ro";
	case STONE_SESSION_REJECT: return "reject";
	case STONE_SESSION_REQUEST_FLUSH_MDLOG: return "request_flushmdlog";
	}
	return "???";
}

const char *stone_mds_op_name(int op)
{
	switch (op) {
	case STONE_MDS_OP_LOOKUP:  return "lookup";
	case STONE_MDS_OP_LOOKUPHASH:  return "lookuphash";
	case STONE_MDS_OP_LOOKUPPARENT:  return "lookupparent";
	case STONE_MDS_OP_LOOKUPINO:  return "lookupino";
	case STONE_MDS_OP_LOOKUPNAME:  return "lookupname";
	case STONE_MDS_OP_GETATTR:  return "getattr";
	case STONE_MDS_OP_SETXATTR: return "setxattr";
	case STONE_MDS_OP_SETATTR: return "setattr";
	case STONE_MDS_OP_RMXATTR: return "rmxattr";
	case STONE_MDS_OP_SETLAYOUT: return "setlayou";
	case STONE_MDS_OP_SETDIRLAYOUT: return "setdirlayout";
	case STONE_MDS_OP_READDIR: return "readdir";
	case STONE_MDS_OP_MKNOD: return "mknod";
	case STONE_MDS_OP_LINK: return "link";
	case STONE_MDS_OP_UNLINK: return "unlink";
	case STONE_MDS_OP_RENAME: return "rename";
	case STONE_MDS_OP_MKDIR: return "mkdir";
	case STONE_MDS_OP_RMDIR: return "rmdir";
	case STONE_MDS_OP_SYMLINK: return "symlink";
	case STONE_MDS_OP_CREATE: return "create";
	case STONE_MDS_OP_OPEN: return "open";
	case STONE_MDS_OP_LOOKUPSNAP: return "lookupsnap";
	case STONE_MDS_OP_LSSNAP: return "lssnap";
	case STONE_MDS_OP_MKSNAP: return "mksnap";
	case STONE_MDS_OP_RMSNAP: return "rmsnap";
	case STONE_MDS_OP_RENAMESNAP: return "renamesnap";
	case STONE_MDS_OP_SETFILELOCK: return "setfilelock";
	case STONE_MDS_OP_GETFILELOCK: return "getfilelock";
	case STONE_MDS_OP_FRAGMENTDIR: return "fragmentdir";
	case STONE_MDS_OP_EXPORTDIR: return "exportdir";
	case STONE_MDS_OP_FLUSH: return "flush_path";
	case STONE_MDS_OP_ENQUEUE_SCRUB: return "enqueue_scrub";
	case STONE_MDS_OP_REPAIR_FRAGSTATS: return "repair_fragstats";
	case STONE_MDS_OP_REPAIR_INODESTATS: return "repair_inodestats";
	}
	return "???";
}

const char *stone_cap_op_name(int op)
{
	switch (op) {
	case STONE_CAP_OP_GRANT: return "grant";
	case STONE_CAP_OP_REVOKE: return "revoke";
	case STONE_CAP_OP_TRUNC: return "trunc";
	case STONE_CAP_OP_EXPORT: return "export";
	case STONE_CAP_OP_IMPORT: return "import";
	case STONE_CAP_OP_UPDATE: return "update";
	case STONE_CAP_OP_DROP: return "drop";
	case STONE_CAP_OP_FLUSH: return "flush";
	case STONE_CAP_OP_FLUSH_ACK: return "flush_ack";
	case STONE_CAP_OP_FLUSHSNAP: return "flushsnap";
	case STONE_CAP_OP_FLUSHSNAP_ACK: return "flushsnap_ack";
	case STONE_CAP_OP_RELEASE: return "release";
	case STONE_CAP_OP_RENEW: return "renew";
	}
	return "???";
}

const char *stone_lease_op_name(int o)
{
	switch (o) {
	case STONE_MDS_LEASE_REVOKE: return "revoke";
	case STONE_MDS_LEASE_RELEASE: return "release";
	case STONE_MDS_LEASE_RENEW: return "renew";
	case STONE_MDS_LEASE_REVOKE_ACK: return "revoke_ack";
	}
	return "???";
}

const char *stone_snap_op_name(int o)
{
	switch (o) {
	case STONE_SNAP_OP_UPDATE: return "update";
	case STONE_SNAP_OP_CREATE: return "create";
	case STONE_SNAP_OP_DESTROY: return "destroy";
	case STONE_SNAP_OP_SPLIT: return "split";
	}
	return "???";
}

const char *stone_watch_event_name(int e)
{
	switch (e) {
	case STONE_WATCH_EVENT_NOTIFY: return "notify";
	case STONE_WATCH_EVENT_NOTIFY_COMPLETE: return "notify_complete";
	case STONE_WATCH_EVENT_DISCONNECT: return "disconnect";
	}
	return "???";
}

const char *stone_pool_op_name(int op)
{
	switch (op) {
	case POOL_OP_CREATE: return "create";
	case POOL_OP_DELETE: return "delete";
	case POOL_OP_AUID_CHANGE: return "auid change";  // (obsolete)
	case POOL_OP_CREATE_SNAP: return "create snap";
	case POOL_OP_DELETE_SNAP: return "delete snap";
	case POOL_OP_CREATE_UNMANAGED_SNAP: return "create unmanaged snap";
	case POOL_OP_DELETE_UNMANAGED_SNAP: return "delete unmanaged snap";
	}
	return "???";
}

const char *stone_osd_backoff_op_name(int op)
{
	switch (op) {
	case STONE_OSD_BACKOFF_OP_BLOCK: return "block";
	case STONE_OSD_BACKOFF_OP_ACK_BLOCK: return "ack-block";
	case STONE_OSD_BACKOFF_OP_UNBLOCK: return "unblock";
	}
	return "???";
}
