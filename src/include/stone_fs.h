/*
 * ceph_fs.h - Stone constants and data types to share between kernel and
 * user space.
 *
 * Most types in this file are defined as little-endian, and are
 * primarily intended to describe data structures that pass over the
 * wire or that are stored on disk.
 *
 * LGPL-2.1 or LGPL-3.0
 */

#ifndef STONE_FS_H
#define STONE_FS_H

#include "msgr.h"
#include "rados.h"

/*
 * The data structures defined here are shared between Linux kernel and
 * user space.  Also, those data structures are maintained always in
 * little-endian byte order, even on big-endian systems.  This is handled
 * differently in kernel vs. user space.  For use as kernel headers, the
 * little-endian fields need to use the __le16/__le32/__le64 types.  These
 * are markers that indicate endian conversion routines must be used
 * whenever such fields are accessed, which can be verified by checker
 * tools like "sparse".  For use as user-space headers, the little-endian
 * fields instead use types ceph_le16/ceph_le32/ceph_le64, which are C++
 * classes that implement automatic endian conversion on every access.
 * To still allow for header sharing, this file uses the __le types, but
 * redefines those to the ceph_ types when compiled in user space.
 */
#ifndef __KERNEL__
#include "byteorder.h"
#define __le16 ceph_le16
#define __le32 ceph_le32
#define __le64 ceph_le64
#endif

/*
 * subprotocol versions.  when specific messages types or high-level
 * protocols change, bump the affected components.  we keep rev
 * internal cluster protocols separately from the public,
 * client-facing protocol.
 */
#define STONE_OSDC_PROTOCOL   24 /* server/client */
#define STONE_MDSC_PROTOCOL   32 /* server/client */
#define STONE_MONC_PROTOCOL   15 /* server/client */


#define STONE_INO_ROOT             1
/*
 * hidden .ceph dir, which is no longer created but
 * recognised in existing filesystems so that we
 * don't try to fragment it.
 */
#define STONE_INO_STONE             2
#define STONE_INO_GLOBAL_SNAPREALM 3
#define STONE_INO_LOST_AND_FOUND   4 /* reserved ino for use in recovery */

/* arbitrary limit on max # of monitors (cluster of 3 is typical) */
#define STONE_MAX_MON   31

/*
 * ceph_file_layout - describe data layout for a file/inode
 */
struct ceph_file_layout {
	/* file -> object mapping */
	__le32 fl_stripe_unit;     /* stripe unit, in bytes.  must be multiple
				      of page size. */
	__le32 fl_stripe_count;    /* over this many objects */
	__le32 fl_object_size;     /* until objects are this big, then move to
				      new objects */
	__le32 fl_cas_hash;        /* UNUSED.  0 = none; 1 = sha256 */

	/* pg -> disk layout */
	__le32 fl_object_stripe_unit;  /* UNUSED.  for per-object parity, if any */

	/* object -> pg layout */
	__le32 fl_unused;       /* unused; used to be preferred primary for pg (-1 for none) */
	__le32 fl_pg_pool;      /* namespace, crush ruleset, rep level */
} __attribute__ ((packed));

#define STONE_MIN_STRIPE_UNIT 65536

struct ceph_dir_layout {
	__u8   dl_dir_hash;   /* see ceph_hash.h for ids */
	__u8   dl_unused1;
	__u16  dl_unused2;
	__u32  dl_unused3;
} __attribute__ ((packed));

/* crypto algorithms */
#define STONE_CRYPTO_NONE 0x0
#define STONE_CRYPTO_AES  0x1

#define STONE_AES_IV "cephsageyudagreg"

/* security/authentication protocols */
#define STONE_AUTH_UNKNOWN	0x0
#define STONE_AUTH_NONE	 	0x1
#define STONE_AUTH_STONEX	 	0x2

/* msgr2 protocol modes */
#define STONE_CON_MODE_UNKNOWN 0x0
#define STONE_CON_MODE_CRC     0x1
#define STONE_CON_MODE_SECURE  0x2

extern const char *ceph_con_mode_name(int con_mode);

/*  For options with "_", like: GSS_GSS
    which means: Mode/Protocol to validate "authentication_authorization",
    where:
      - Authentication: Verifying the identity of an entity.
      - Authorization:  Verifying that an authenticated entity has
                        the right to access a particular resource.
*/ 
#define STONE_AUTH_GSS     0x4
#define STONE_AUTH_GSS_GSS STONE_AUTH_GSS

#define STONE_AUTH_UID_DEFAULT ((__u64) -1)


/*********************************************
 * message layer
 */

/*
 * message types
 */

/* misc */
#define STONE_MSG_SHUTDOWN               1
#define STONE_MSG_PING                   2

/* client <-> monitor */
#define STONE_MSG_MON_MAP                4
#define STONE_MSG_MON_GET_MAP            5
#define STONE_MSG_MON_GET_OSDMAP         6
#define STONE_MSG_MON_METADATA           7
#define STONE_MSG_STATFS                 13
#define STONE_MSG_STATFS_REPLY           14
#define STONE_MSG_MON_SUBSCRIBE          15
#define STONE_MSG_MON_SUBSCRIBE_ACK      16
#define STONE_MSG_AUTH			17
#define STONE_MSG_AUTH_REPLY		18
#define STONE_MSG_MON_GET_VERSION        19
#define STONE_MSG_MON_GET_VERSION_REPLY  20

/* client <-> mds */
#define STONE_MSG_MDS_MAP                21

#define STONE_MSG_CLIENT_SESSION         22
#define STONE_MSG_CLIENT_RECONNECT       23

#define STONE_MSG_CLIENT_REQUEST         24
#define STONE_MSG_CLIENT_REQUEST_FORWARD 25
#define STONE_MSG_CLIENT_REPLY           26
#define STONE_MSG_CLIENT_RECLAIM		27
#define STONE_MSG_CLIENT_RECLAIM_REPLY   28
#define STONE_MSG_CLIENT_METRICS         29
#define STONE_MSG_CLIENT_CAPS            0x310
#define STONE_MSG_CLIENT_LEASE           0x311
#define STONE_MSG_CLIENT_SNAP            0x312
#define STONE_MSG_CLIENT_CAPRELEASE      0x313
#define STONE_MSG_CLIENT_QUOTA           0x314

/* pool ops */
#define STONE_MSG_POOLOP_REPLY           48
#define STONE_MSG_POOLOP                 49


/* osd */
#define STONE_MSG_OSD_MAP                41
#define STONE_MSG_OSD_OP                 42
#define STONE_MSG_OSD_OPREPLY            43
#define STONE_MSG_WATCH_NOTIFY           44
#define STONE_MSG_OSD_BACKOFF            61

/* FSMap subscribers (see all MDS clusters at once) */
#define STONE_MSG_FS_MAP                 45
/* FSMapUser subscribers (get MDS clusters name->ID mapping) */
#define STONE_MSG_FS_MAP_USER		103

/* watch-notify operations */
enum {
	STONE_WATCH_EVENT_NOTIFY		  = 1, /* notifying watcher */
	STONE_WATCH_EVENT_NOTIFY_COMPLETE  = 2, /* notifier notified when done */
	STONE_WATCH_EVENT_DISCONNECT       = 3, /* we were disconnected */
};

const char *ceph_watch_event_name(int o);

/* pool operations */
enum {
  POOL_OP_CREATE			= 0x01,
  POOL_OP_DELETE			= 0x02,
  POOL_OP_AUID_CHANGE			= 0x03,
  POOL_OP_CREATE_SNAP			= 0x11,
  POOL_OP_DELETE_SNAP			= 0x12,
  POOL_OP_CREATE_UNMANAGED_SNAP		= 0x21,
  POOL_OP_DELETE_UNMANAGED_SNAP		= 0x22,
};

struct ceph_mon_request_header {
	__le64 have_version;
	__le16 session_mon;
	__le64 session_mon_tid;
} __attribute__ ((packed));

struct ceph_mon_statfs {
	struct ceph_mon_request_header monhdr;
	struct ceph_fsid fsid;
} __attribute__ ((packed));

struct ceph_statfs {
	__le64 kb, kb_used, kb_avail;
	__le64 num_objects;
} __attribute__ ((packed));

struct ceph_mon_statfs_reply {
	struct ceph_fsid fsid;
	__le64 version;
	struct ceph_statfs st;
} __attribute__ ((packed));

const char *ceph_pool_op_name(int op);

struct ceph_mon_poolop {
	struct ceph_mon_request_header monhdr;
	struct ceph_fsid fsid;
	__le32 pool;
	__le32 op;
	__le64 __old_auid;  // obsolete
	__le64 snapid;
	__le32 name_len;
} __attribute__ ((packed));

struct ceph_mon_poolop_reply {
	struct ceph_mon_request_header monhdr;
	struct ceph_fsid fsid;
	__le32 reply_code;
	__le32 epoch;
	char has_data;
	char data[0];
} __attribute__ ((packed));

struct ceph_mon_unmanaged_snap {
	__le64 snapid;
} __attribute__ ((packed));

struct ceph_osd_getmap {
	struct ceph_mon_request_header monhdr;
	struct ceph_fsid fsid;
	__le32 start;
} __attribute__ ((packed));

struct ceph_mds_getmap {
	struct ceph_mon_request_header monhdr;
	struct ceph_fsid fsid;
} __attribute__ ((packed));

struct ceph_client_mount {
	struct ceph_mon_request_header monhdr;
} __attribute__ ((packed));

#define STONE_SUBSCRIBE_ONETIME    1  /* i want only 1 update after have */

struct ceph_mon_subscribe_item {
	__le64 start;
	__u8 flags;
} __attribute__ ((packed));

struct ceph_mon_subscribe_ack {
	__le32 duration;         /* seconds */
	struct ceph_fsid fsid;
} __attribute__ ((packed));

/*
 * mdsmap flags
 */
#define STONE_MDSMAP_NOT_JOINABLE                 (1<<0)  /* standbys cannot join */
#define STONE_MDSMAP_DOWN                         (STONE_MDSMAP_NOT_JOINABLE) /* backwards compat */
#define STONE_MDSMAP_ALLOW_SNAPS                  (1<<1)  /* cluster allowed to create snapshots */
/* deprecated #define STONE_MDSMAP_ALLOW_MULTIMDS (1<<2) cluster allowed to have >1 active MDS */
/* deprecated #define STONE_MDSMAP_ALLOW_DIRFRAGS (1<<3) cluster allowed to fragment directories */
#define STONE_MDSMAP_ALLOW_MULTIMDS_SNAPS	     (1<<4)  /* cluster alllowed to enable MULTIMDS
                                                            and SNAPS at the same time */
#define STONE_MDSMAP_ALLOW_STANDBY_REPLAY         (1<<5)  /* cluster alllowed to enable MULTIMDS */

#define STONE_MDSMAP_DEFAULTS (STONE_MDSMAP_ALLOW_SNAPS | \
			      STONE_MDSMAP_ALLOW_MULTIMDS_SNAPS)

/*
 * mds states
 *   > 0 -> in
 *  <= 0 -> out
 */
#define STONE_MDS_STATE_DNE          0  /* down, does not exist. */
#define STONE_MDS_STATE_STOPPED     -1  /* down, once existed, but no subtrees.
					  empty log. */
#define STONE_MDS_STATE_BOOT        -4  /* up, boot announcement. */
#define STONE_MDS_STATE_STANDBY     -5  /* up, idle.  waiting for assignment. */
#define STONE_MDS_STATE_CREATING    -6  /* up, creating MDS instance. */
#define STONE_MDS_STATE_STARTING    -7  /* up, starting previously stopped mds */
#define STONE_MDS_STATE_STANDBY_REPLAY -8 /* up, tailing active node's journal */
#define STONE_MDS_STATE_REPLAYONCE   -9 /* Legacy, unused */
#define STONE_MDS_STATE_NULL         -10

#define STONE_MDS_STATE_REPLAY       8  /* up, replaying journal. */
#define STONE_MDS_STATE_RESOLVE      9  /* up, disambiguating distributed
					  operations (import, rename, etc.) */
#define STONE_MDS_STATE_RECONNECT    10 /* up, reconnect to clients */
#define STONE_MDS_STATE_REJOIN       11 /* up, rejoining distributed cache */
#define STONE_MDS_STATE_CLIENTREPLAY 12 /* up, replaying client operations */
#define STONE_MDS_STATE_ACTIVE       13 /* up, active */
#define STONE_MDS_STATE_STOPPING     14 /* up, but exporting metadata */
#define STONE_MDS_STATE_DAMAGED      15 /* rank not replayable, need repair */

extern const char *ceph_mds_state_name(int s);


/*
 * metadata lock types.
 *  - these are bitmasks.. we can compose them
 *  - they also define the lock ordering by the MDS
 *  - a few of these are internal to the mds
 */
#define STONE_LOCK_DN          (1 << 0)
#define STONE_LOCK_DVERSION    (1 << 1)
#define STONE_LOCK_ISNAP       (1 << 4)  /* snapshot lock. MDS internal */
#define STONE_LOCK_IPOLICY     (1 << 5)  /* policy lock on dirs. MDS internal */
#define STONE_LOCK_IFILE       (1 << 6)
#define STONE_LOCK_INEST       (1 << 7)  /* mds internal */
#define STONE_LOCK_IDFT        (1 << 8)  /* dir frag tree */
#define STONE_LOCK_IAUTH       (1 << 9)
#define STONE_LOCK_ILINK       (1 << 10)
#define STONE_LOCK_IXATTR      (1 << 11)
#define STONE_LOCK_IFLOCK      (1 << 12)  /* advisory file locks */
#define STONE_LOCK_IVERSION    (1 << 13)  /* mds internal */

#define STONE_LOCK_IFIRST      STONE_LOCK_ISNAP


/* client_session ops */
enum {
	STONE_SESSION_REQUEST_OPEN,
	STONE_SESSION_OPEN,
	STONE_SESSION_REQUEST_CLOSE,
	STONE_SESSION_CLOSE,
	STONE_SESSION_REQUEST_RENEWCAPS,
	STONE_SESSION_RENEWCAPS,
	STONE_SESSION_STALE,
	STONE_SESSION_RECALL_STATE,
	STONE_SESSION_FLUSHMSG,
	STONE_SESSION_FLUSHMSG_ACK,
	STONE_SESSION_FORCE_RO,
    // A response to REQUEST_OPEN indicating that the client should
    // permanently desist from contacting the MDS
	STONE_SESSION_REJECT,
        STONE_SESSION_REQUEST_FLUSH_MDLOG
};

// flags for state reclaim
#define STONE_RECLAIM_RESET	1

extern const char *ceph_session_op_name(int op);

struct ceph_mds_session_head {
	__le32 op;
	__le64 seq;
	struct ceph_timespec stamp;
	__le32 max_caps, max_leases;
} __attribute__ ((packed));

/* client_request */
/*
 * metadata ops.
 *  & 0x001000 -> write op
 *  & 0x010000 -> follow symlink (e.g. stat(), not lstat()).
 &  & 0x100000 -> use weird ino/path trace
 */
#define STONE_MDS_OP_WRITE        0x001000
enum {
	STONE_MDS_OP_LOOKUP     = 0x00100,
	STONE_MDS_OP_GETATTR    = 0x00101,
	STONE_MDS_OP_LOOKUPHASH = 0x00102,
	STONE_MDS_OP_LOOKUPPARENT = 0x00103,
	STONE_MDS_OP_LOOKUPINO  = 0x00104,
	STONE_MDS_OP_LOOKUPNAME = 0x00105,
	STONE_MDS_OP_GETVXATTR  = 0x00106,

	STONE_MDS_OP_SETXATTR   = 0x01105,
	STONE_MDS_OP_RMXATTR    = 0x01106,
	STONE_MDS_OP_SETLAYOUT  = 0x01107,
	STONE_MDS_OP_SETATTR    = 0x01108,
	STONE_MDS_OP_SETFILELOCK= 0x01109,
	STONE_MDS_OP_GETFILELOCK= 0x00110,
	STONE_MDS_OP_SETDIRLAYOUT=0x0110a,

	STONE_MDS_OP_MKNOD      = 0x01201,
	STONE_MDS_OP_LINK       = 0x01202,
	STONE_MDS_OP_UNLINK     = 0x01203,
	STONE_MDS_OP_RENAME     = 0x01204,
	STONE_MDS_OP_MKDIR      = 0x01220,
	STONE_MDS_OP_RMDIR      = 0x01221,
	STONE_MDS_OP_SYMLINK    = 0x01222,

	STONE_MDS_OP_CREATE     = 0x01301,
	STONE_MDS_OP_OPEN       = 0x00302,
	STONE_MDS_OP_READDIR    = 0x00305,

	STONE_MDS_OP_LOOKUPSNAP = 0x00400,
	STONE_MDS_OP_MKSNAP     = 0x01400,
	STONE_MDS_OP_RMSNAP     = 0x01401,
	STONE_MDS_OP_LSSNAP     = 0x00402,
	STONE_MDS_OP_RENAMESNAP = 0x01403,

	// internal op
	STONE_MDS_OP_FRAGMENTDIR= 0x01500,
	STONE_MDS_OP_EXPORTDIR  = 0x01501,
	STONE_MDS_OP_FLUSH      = 0x01502,
	STONE_MDS_OP_ENQUEUE_SCRUB  = 0x01503,
	STONE_MDS_OP_REPAIR_FRAGSTATS = 0x01504,
	STONE_MDS_OP_REPAIR_INODESTATS = 0x01505,
	STONE_MDS_OP_RDLOCK_FRAGSSTATS = 0x01507
};

extern const char *ceph_mds_op_name(int op);

#ifndef STONE_SETATTR_MODE
#define STONE_SETATTR_MODE	(1 << 0)
#define STONE_SETATTR_UID	(1 << 1)
#define STONE_SETATTR_GID	(1 << 2)
#define STONE_SETATTR_MTIME	(1 << 3)
#define STONE_SETATTR_ATIME	(1 << 4)
#define STONE_SETATTR_SIZE	(1 << 5)
#define STONE_SETATTR_CTIME	(1 << 6)
#define STONE_SETATTR_MTIME_NOW	(1 << 7)
#define STONE_SETATTR_ATIME_NOW	(1 << 8)
#define STONE_SETATTR_BTIME	(1 << 9)
#endif
#define STONE_SETATTR_KILL_SGUID	(1 << 10)

/*
 * open request flags
 */
#define STONE_O_RDONLY          00000000
#define STONE_O_WRONLY          00000001
#define STONE_O_RDWR            00000002
#define STONE_O_CREAT           00000100
#define STONE_O_EXCL            00000200
#define STONE_O_TRUNC           00001000
#define STONE_O_LAZY            00020000
#define STONE_O_DIRECTORY       00200000
#define STONE_O_NOFOLLOW        00400000

int ceph_flags_sys2wire(int flags);

/*
 * Stone setxattr request flags.
 */
#define STONE_XATTR_CREATE  (1 << 0)
#define STONE_XATTR_REPLACE (1 << 1)
#define STONE_XATTR_REMOVE  (1 << 31)

/*
 * readdir request flags;
 */
#define STONE_READDIR_REPLY_BITFLAGS	(1<<0)

/*
 * readdir reply flags.
 */
#define STONE_READDIR_FRAG_END		(1<<0)
#define STONE_READDIR_FRAG_COMPLETE	(1<<8)
#define STONE_READDIR_HASH_ORDER		(1<<9)
#define STONE_READDIR_OFFSET_HASH       (1<<10)

/* Note that this is embedded wthin ceph_mds_request_head_legacy. */
union ceph_mds_request_args_legacy {
	struct {
		__le32 mask;                 /* STONE_CAP_* */
	} __attribute__ ((packed)) getattr;
	struct {
		__le32 mode;
		__le32 uid;
		__le32 gid;
		struct ceph_timespec mtime;
		struct ceph_timespec atime;
		__le64 size, old_size;       /* old_size needed by truncate */
		__le32 mask;                 /* STONE_SETATTR_* */
	} __attribute__ ((packed)) setattr;
	struct {
		__le32 frag;                 /* which dir fragment */
		__le32 max_entries;          /* how many dentries to grab */
		__le32 max_bytes;
		__le16 flags;
               __le32 offset_hash;
	} __attribute__ ((packed)) readdir;
	struct {
		__le32 mode;
		__le32 rdev;
	} __attribute__ ((packed)) mknod;
	struct {
		__le32 mode;
	} __attribute__ ((packed)) mkdir;
	struct {
		__le32 flags;
		__le32 mode;
		__le32 stripe_unit;          /* layout for newly created file */
		__le32 stripe_count;         /* ... */
		__le32 object_size;
		__le32 pool;                 /* if >= 0 and CREATEPOOLID feature */
		__le32 mask;                 /* STONE_CAP_* */
		__le64 old_size;             /* if O_TRUNC */
	} __attribute__ ((packed)) open;
	struct {
		__le32 flags;
		__le32 osdmap_epoch; 	    /* use for set file/dir layout */
	} __attribute__ ((packed)) setxattr;
	struct {
		struct ceph_file_layout layout;
	} __attribute__ ((packed)) setlayout;
	struct {
		__u8 rule; /* currently fcntl or flock */
		__u8 type; /* shared, exclusive, remove*/
		__le64 owner; /* who requests/holds the lock */
		__le64 pid; /* process id requesting the lock */
		__le64 start; /* initial location to lock */
		__le64 length; /* num bytes to lock from start */
		__u8 wait; /* will caller wait for lock to become available? */
	} __attribute__ ((packed)) filelock_change;
} __attribute__ ((packed));

#define STONE_MDS_FLAG_REPLAY        1  /* this is a replayed op */
#define STONE_MDS_FLAG_WANT_DENTRY   2  /* want dentry in reply */
#define STONE_MDS_FLAG_ASYNC         4  /* request is async */

struct ceph_mds_request_head_legacy {
	__le64 oldest_client_tid;
	__le32 mdsmap_epoch;           /* on client */
	__le32 flags;                  /* STONE_MDS_FLAG_* */
	__u8 num_retry, num_fwd;       /* count retry, fwd attempts */
	__le16 num_releases;           /* # include cap/lease release records */
	__le32 op;                     /* mds op code */
	__le32 caller_uid, caller_gid;
	__le64 ino;                    /* use this ino for openc, mkdir, mknod,
					  etc. (if replaying) */
	union ceph_mds_request_args_legacy args;
} __attribute__ ((packed));

/*
 * Note that this is embedded wthin ceph_mds_request_head. Also, compatibility
 * with the ceph_mds_request_args_legacy must be maintained!
 */
union ceph_mds_request_args {
	struct {
		__le32 mask;                 /* STONE_CAP_* */
	} __attribute__ ((packed)) getattr;
	struct {
		__le32 mode;
		__le32 uid;
		__le32 gid;
		struct ceph_timespec mtime;
		struct ceph_timespec atime;
		__le64 size, old_size;       /* old_size needed by truncate */
		__le32 mask;                 /* STONE_SETATTR_* */
		struct ceph_timespec btime;
	} __attribute__ ((packed)) setattr;
	struct {
		__le32 frag;                 /* which dir fragment */
		__le32 max_entries;          /* how many dentries to grab */
		__le32 max_bytes;
		__le16 flags;
               __le32 offset_hash;
	} __attribute__ ((packed)) readdir;
	struct {
		__le32 mode;
		__le32 rdev;
	} __attribute__ ((packed)) mknod;
	struct {
		__le32 mode;
	} __attribute__ ((packed)) mkdir;
	struct {
		__le32 flags;
		__le32 mode;
		__le32 stripe_unit;          /* layout for newly created file */
		__le32 stripe_count;         /* ... */
		__le32 object_size;
		__le32 pool;                 /* if >= 0 and CREATEPOOLID feature */
		__le32 mask;                 /* STONE_CAP_* */
		__le64 old_size;             /* if O_TRUNC */
	} __attribute__ ((packed)) open;
	struct {
		__le32 flags;
		__le32 osdmap_epoch; 	    /* use for set file/dir layout */
	} __attribute__ ((packed)) setxattr;
	struct {
		struct ceph_file_layout layout;
	} __attribute__ ((packed)) setlayout;
	struct {
		__u8 rule; /* currently fcntl or flock */
		__u8 type; /* shared, exclusive, remove*/
		__le64 owner; /* who requests/holds the lock */
		__le64 pid; /* process id requesting the lock */
		__le64 start; /* initial location to lock */
		__le64 length; /* num bytes to lock from start */
		__u8 wait; /* will caller wait for lock to become available? */
	} __attribute__ ((packed)) filelock_change;
	struct {
		__le32 mask;                 /* STONE_CAP_* */
		__le64 snapid;
		__le64 parent;
		__le32 hash;
	} __attribute__ ((packed)) lookupino;
} __attribute__ ((packed));

#define STONE_MDS_REQUEST_HEAD_VERSION	1

/*
 * Note that any change to this structure must ensure that it is compatible
 * with ceph_mds_request_head_legacy.
 */
struct ceph_mds_request_head {
	__le16 version;
	__le64 oldest_client_tid;
	__le32 mdsmap_epoch;           /* on client */
	__le32 flags;                  /* STONE_MDS_FLAG_* */
	__u8 num_retry, num_fwd;       /* count retry, fwd attempts */
	__le16 num_releases;           /* # include cap/lease release records */
	__le32 op;                     /* mds op code */
	__le32 caller_uid, caller_gid;
	__le64 ino;                    /* use this ino for openc, mkdir, mknod,
					  etc. (if replaying) */
	union ceph_mds_request_args args;
} __attribute__ ((packed));

/* cap/lease release record */
struct ceph_mds_request_release {
	__le64 ino, cap_id;            /* ino and unique cap id */
	__le32 caps, wanted;           /* new issued, wanted */
	__le32 seq, issue_seq, mseq;
	__le32 dname_seq;              /* if releasing a dentry lease, a */
	__le32 dname_len;              /* string follows. */
} __attribute__ ((packed));

static inline void
copy_from_legacy_head(struct ceph_mds_request_head *head,
			struct ceph_mds_request_head_legacy *legacy)
{
	struct ceph_mds_request_head_legacy *embedded_legacy =
		(struct ceph_mds_request_head_legacy *)&head->oldest_client_tid;
	*embedded_legacy = *legacy;
}

static inline void
copy_to_legacy_head(struct ceph_mds_request_head_legacy *legacy,
			struct ceph_mds_request_head *head)
{
	struct ceph_mds_request_head_legacy *embedded_legacy =
		(struct ceph_mds_request_head_legacy *)&head->oldest_client_tid;
	*legacy = *embedded_legacy;
}

/* client reply */
struct ceph_mds_reply_head {
	__le32 op;
	__le32 result;
	__le32 mdsmap_epoch;
	__u8 safe;                     /* true if committed to disk */
	__u8 is_dentry, is_target;     /* true if dentry, target inode records
					  are included with reply */
} __attribute__ ((packed));

/* one for each node split */
struct ceph_frag_tree_split {
	__le32 frag;                   /* this frag splits... */
	__le32 by;                     /* ...by this many bits */
} __attribute__ ((packed));

struct ceph_frag_tree_head {
	__le32 nsplits;                /* num ceph_frag_tree_split records */
	struct ceph_frag_tree_split splits[];
} __attribute__ ((packed));

/* capability issue, for bundling with mds reply */
struct ceph_mds_reply_cap {
	__le32 caps, wanted;           /* caps issued, wanted */
	__le64 cap_id;
	__le32 seq, mseq;
	__le64 realm;                  /* snap realm */
	__u8 flags;                    /* STONE_CAP_FLAG_* */
} __attribute__ ((packed));

#define STONE_CAP_FLAG_AUTH	(1 << 0)	/* cap is issued by auth mds */
#define STONE_CAP_FLAG_RELEASE	(1 << 1)        /* ask client to release the cap */

/* reply_lease follows dname, and reply_inode */
struct ceph_mds_reply_lease {
	__le16 mask;            /* lease type(s) */
	__le32 duration_ms;     /* lease duration */
	__le32 seq;
} __attribute__ ((packed));

#define STONE_LEASE_VALID	(1 | 2) /* old and new bit values */
#define STONE_LEASE_PRIMARY_LINK	4	/* primary linkage */

struct ceph_mds_reply_dirfrag {
	__le32 frag;            /* fragment */
	__le32 auth;            /* auth mds, if this is a delegation point */
	__le32 ndist;           /* number of mds' this is replicated on */
	__le32 dist[];
} __attribute__ ((packed));

#define STONE_LOCK_FCNTL		1
#define STONE_LOCK_FLOCK		2
#define STONE_LOCK_FCNTL_INTR	3
#define STONE_LOCK_FLOCK_INTR	4

#define STONE_LOCK_SHARED   1
#define STONE_LOCK_EXCL     2
#define STONE_LOCK_UNLOCK   4

struct ceph_filelock {
	__le64 start;/* file offset to start lock at */
	__le64 length; /* num bytes to lock; 0 for all following start */
	__le64 client; /* which client holds the lock */
	__le64 owner; /* who requests/holds the lock */
	__le64 pid; /* process id holding the lock on the client */
	__u8 type; /* shared lock, exclusive lock, or unlock */
} __attribute__ ((packed));


/* file access modes */
#define STONE_FILE_MODE_PIN        0
#define STONE_FILE_MODE_RD         1
#define STONE_FILE_MODE_WR         2
#define STONE_FILE_MODE_RDWR       3  /* RD | WR */
#define STONE_FILE_MODE_LAZY       4  /* lazy io */
#define STONE_FILE_MODE_NUM        8  /* bc these are bit fields.. mostly */

int ceph_flags_to_mode(int flags);

/* inline data state */
#define STONE_INLINE_NONE	((__u64)-1)
#define STONE_INLINE_MAX_SIZE	STONE_MIN_STRIPE_UNIT

/* capability bits */
#define STONE_CAP_PIN         1  /* no specific capabilities beyond the pin */

/* generic cap bits */
/* note: these definitions are duplicated in mds/locks.c */
#define STONE_CAP_GSHARED     1  /* client can reads */
#define STONE_CAP_GEXCL       2  /* client can read and update */
#define STONE_CAP_GCACHE      4  /* (file) client can cache reads */
#define STONE_CAP_GRD         8  /* (file) client can read */
#define STONE_CAP_GWR        16  /* (file) client can write */
#define STONE_CAP_GBUFFER    32  /* (file) client can buffer writes */
#define STONE_CAP_GWREXTEND  64  /* (file) client can extend EOF */
#define STONE_CAP_GLAZYIO   128  /* (file) client can perform lazy io */

#define STONE_CAP_SIMPLE_BITS  2
#define STONE_CAP_FILE_BITS    8

/* per-lock shift */
#define STONE_CAP_SAUTH      2
#define STONE_CAP_SLINK      4
#define STONE_CAP_SXATTR     6
#define STONE_CAP_SFILE      8

/* composed values */
#define STONE_CAP_AUTH_SHARED  (STONE_CAP_GSHARED  << STONE_CAP_SAUTH)
#define STONE_CAP_AUTH_EXCL     (STONE_CAP_GEXCL     << STONE_CAP_SAUTH)
#define STONE_CAP_LINK_SHARED  (STONE_CAP_GSHARED  << STONE_CAP_SLINK)
#define STONE_CAP_LINK_EXCL     (STONE_CAP_GEXCL     << STONE_CAP_SLINK)
#define STONE_CAP_XATTR_SHARED (STONE_CAP_GSHARED  << STONE_CAP_SXATTR)
#define STONE_CAP_XATTR_EXCL    (STONE_CAP_GEXCL     << STONE_CAP_SXATTR)
#define STONE_CAP_FILE(x)       ((x) << STONE_CAP_SFILE)
#define STONE_CAP_FILE_SHARED   (STONE_CAP_GSHARED   << STONE_CAP_SFILE)
#define STONE_CAP_FILE_EXCL     (STONE_CAP_GEXCL     << STONE_CAP_SFILE)
#define STONE_CAP_FILE_CACHE    (STONE_CAP_GCACHE    << STONE_CAP_SFILE)
#define STONE_CAP_FILE_RD       (STONE_CAP_GRD       << STONE_CAP_SFILE)
#define STONE_CAP_FILE_WR       (STONE_CAP_GWR       << STONE_CAP_SFILE)
#define STONE_CAP_FILE_BUFFER   (STONE_CAP_GBUFFER   << STONE_CAP_SFILE)
#define STONE_CAP_FILE_WREXTEND (STONE_CAP_GWREXTEND << STONE_CAP_SFILE)
#define STONE_CAP_FILE_LAZYIO   (STONE_CAP_GLAZYIO   << STONE_CAP_SFILE)

/* cap masks (for getattr) */
#define STONE_STAT_CAP_INODE    STONE_CAP_PIN
#define STONE_STAT_CAP_TYPE     STONE_CAP_PIN  /* mode >> 12 */
#define STONE_STAT_CAP_SYMLINK  STONE_CAP_PIN
#define STONE_STAT_CAP_UID      STONE_CAP_AUTH_SHARED
#define STONE_STAT_CAP_GID      STONE_CAP_AUTH_SHARED
#define STONE_STAT_CAP_MODE     STONE_CAP_AUTH_SHARED
#define STONE_STAT_CAP_NLINK    STONE_CAP_LINK_SHARED
#define STONE_STAT_CAP_LAYOUT   STONE_CAP_FILE_SHARED
#define STONE_STAT_CAP_MTIME    STONE_CAP_FILE_SHARED
#define STONE_STAT_CAP_SIZE     STONE_CAP_FILE_SHARED
#define STONE_STAT_CAP_ATIME    STONE_CAP_FILE_SHARED  /* fixme */
#define STONE_STAT_CAP_XATTR    STONE_CAP_XATTR_SHARED
#define STONE_STAT_CAP_INODE_ALL (STONE_CAP_PIN |			\
				 STONE_CAP_AUTH_SHARED |	\
				 STONE_CAP_LINK_SHARED |	\
				 STONE_CAP_FILE_SHARED |	\
				 STONE_CAP_XATTR_SHARED)
#define STONE_STAT_CAP_INLINE_DATA (STONE_CAP_FILE_SHARED | \
				   STONE_CAP_FILE_RD)
#define STONE_STAT_RSTAT        STONE_CAP_FILE_WREXTEND

#define STONE_CAP_ANY_SHARED (STONE_CAP_AUTH_SHARED |			\
			      STONE_CAP_LINK_SHARED |			\
			      STONE_CAP_XATTR_SHARED |			\
			      STONE_CAP_FILE_SHARED)
#define STONE_CAP_ANY_RD   (STONE_CAP_ANY_SHARED | STONE_CAP_FILE_RD |	\
			   STONE_CAP_FILE_CACHE)

#define STONE_CAP_ANY_EXCL (STONE_CAP_AUTH_EXCL |		\
			   STONE_CAP_LINK_EXCL |		\
			   STONE_CAP_XATTR_EXCL |	\
			   STONE_CAP_FILE_EXCL)
#define STONE_CAP_ANY_FILE_RD (STONE_CAP_FILE_RD | STONE_CAP_FILE_CACHE | \
                              STONE_CAP_FILE_SHARED)
#define STONE_CAP_ANY_FILE_WR (STONE_CAP_FILE_WR | STONE_CAP_FILE_BUFFER |	\
			      STONE_CAP_FILE_EXCL)
#define STONE_CAP_ANY_WR   (STONE_CAP_ANY_EXCL | STONE_CAP_ANY_FILE_WR)
#define STONE_CAP_ANY      (STONE_CAP_ANY_RD | STONE_CAP_ANY_EXCL | \
			   STONE_CAP_ANY_FILE_WR | STONE_CAP_FILE_LAZYIO | \
			   STONE_CAP_PIN)

#define STONE_CAP_LOCKS (STONE_LOCK_IFILE | STONE_LOCK_IAUTH | STONE_LOCK_ILINK | \
			STONE_LOCK_IXATTR)

/* cap masks async dir operations */
#define STONE_CAP_DIR_CREATE    STONE_CAP_FILE_CACHE
#define STONE_CAP_DIR_UNLINK    STONE_CAP_FILE_RD
#define STONE_CAP_ANY_DIR_OPS   (STONE_CAP_FILE_CACHE | STONE_CAP_FILE_RD | \
				STONE_CAP_FILE_WREXTEND | STONE_CAP_FILE_LAZYIO)


int ceph_caps_for_mode(int mode);

enum {
	STONE_CAP_OP_GRANT,         /* mds->client grant */
	STONE_CAP_OP_REVOKE,        /* mds->client revoke */
	STONE_CAP_OP_TRUNC,         /* mds->client trunc notify */
	STONE_CAP_OP_EXPORT,        /* mds has exported the cap */
	STONE_CAP_OP_IMPORT,        /* mds has imported the cap */
	STONE_CAP_OP_UPDATE,        /* client->mds update */
	STONE_CAP_OP_DROP,          /* client->mds drop cap bits */
	STONE_CAP_OP_FLUSH,         /* client->mds cap writeback */
	STONE_CAP_OP_FLUSH_ACK,     /* mds->client flushed */
	STONE_CAP_OP_FLUSHSNAP,     /* client->mds flush snapped metadata */
	STONE_CAP_OP_FLUSHSNAP_ACK, /* mds->client flushed snapped metadata */
	STONE_CAP_OP_RELEASE,       /* client->mds release (clean) cap */
	STONE_CAP_OP_RENEW,         /* client->mds renewal request */
};

extern const char *ceph_cap_op_name(int op);

/* extra info for cap import/export */
struct ceph_mds_cap_peer {
	__le64 cap_id;
	__le32 seq;
	__le32 mseq;
	__le32 mds;
	__u8   flags;
} __attribute__ ((packed));

/*
 * caps message, used for capability callbacks, acks, requests, etc.
 */
struct ceph_mds_caps_head {
	__le32 op;                  /* STONE_CAP_OP_* */
	__le64 ino, realm;
	__le64 cap_id;
	__le32 seq, issue_seq;
	__le32 caps, wanted, dirty; /* latest issued/wanted/dirty */
	__le32 migrate_seq;
	__le64 snap_follows;
	__le32 snap_trace_len;

	/* authlock */
	__le32 uid, gid, mode;

	/* linklock */
	__le32 nlink;

	/* xattrlock */
	__le32 xattr_len;
	__le64 xattr_version;
} __attribute__ ((packed));

struct ceph_mds_caps_non_export_body {
    /* all except export */
    /* filelock */
    __le64 size, max_size, truncate_size;
    __le32 truncate_seq;
    struct ceph_timespec mtime, atime, ctime;
    struct ceph_file_layout layout;
    __le32 time_warp_seq;
} __attribute__ ((packed));

struct ceph_mds_caps_export_body {
    /* export message */
    struct ceph_mds_cap_peer peer;
} __attribute__ ((packed));

/* cap release msg head */
struct ceph_mds_cap_release {
	__le32 num;                /* number of cap_items that follow */
} __attribute__ ((packed));

struct ceph_mds_cap_item {
	__le64 ino;
	__le64 cap_id;
	__le32 migrate_seq, seq;
} __attribute__ ((packed));

#define STONE_MDS_LEASE_REVOKE           1  /*    mds  -> client */
#define STONE_MDS_LEASE_RELEASE          2  /* client  -> mds    */
#define STONE_MDS_LEASE_RENEW            3  /* client <-> mds    */
#define STONE_MDS_LEASE_REVOKE_ACK       4  /* client  -> mds    */

extern const char *ceph_lease_op_name(int o);

/* lease msg header */
struct ceph_mds_lease {
	__u8 action;            /* STONE_MDS_LEASE_* */
	__le16 mask;            /* which lease */
	__le64 ino;
	__le64 first, last;     /* snap range */
	__le32 seq;
	__le32 duration_ms;     /* duration of renewal */
} __attribute__ ((packed));
/* followed by a __le32+string for dname */

/* client reconnect */
struct ceph_mds_cap_reconnect {
	__le64 cap_id;
	__le32 wanted;
	__le32 issued;
	__le64 snaprealm;
	__le64 pathbase;        /* base ino for our path to this ino */
	__le32 flock_len;       /* size of flock state blob, if any */
} __attribute__ ((packed));
/* followed by flock blob */

struct ceph_mds_cap_reconnect_v1 {
	__le64 cap_id;
	__le32 wanted;
	__le32 issued;
	__le64 size;
	struct ceph_timespec mtime, atime;
	__le64 snaprealm;
	__le64 pathbase;        /* base ino for our path to this ino */
} __attribute__ ((packed));

struct ceph_mds_snaprealm_reconnect {
	__le64 ino;     /* snap realm base */
	__le64 seq;     /* snap seq for this snap realm */
	__le64 parent;  /* parent realm */
} __attribute__ ((packed));

/*
 * snaps
 */
enum {
	STONE_SNAP_OP_UPDATE,  /* CREATE or DESTROY */
	STONE_SNAP_OP_CREATE,
	STONE_SNAP_OP_DESTROY,
	STONE_SNAP_OP_SPLIT,
};

extern const char *ceph_snap_op_name(int o);

/* snap msg header */
struct ceph_mds_snap_head {
	__le32 op;                /* STONE_SNAP_OP_* */
	__le64 split;             /* ino to split off, if any */
	__le32 num_split_inos;    /* # inos belonging to new child realm */
	__le32 num_split_realms;  /* # child realms udner new child realm */
	__le32 trace_len;         /* size of snap trace blob */
} __attribute__ ((packed));
/* followed by split ino list, then split realms, then the trace blob */

/*
 * encode info about a snaprealm, as viewed by a client
 */
struct ceph_mds_snap_realm {
	__le64 ino;           /* ino */
	__le64 created;       /* snap: when created */
	__le64 parent;        /* ino: parent realm */
	__le64 parent_since;  /* snap: same parent since */
	__le64 seq;           /* snap: version */
	__le32 num_snaps;
	__le32 num_prior_parent_snaps;
} __attribute__ ((packed));
/* followed by my snap list, then prior parent snap list */

#ifndef __KERNEL__
#undef __le16
#undef __le32
#undef __le64
#endif

#endif
