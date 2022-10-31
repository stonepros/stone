// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * scalable distributed file system
 *
 * Copyright (C) Jeff Layton <jlayton@redhat.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 */

#ifndef STONE_STONE_LL_CLIENT_H
#define STONE_STONE_LL_CLIENT_H
#include <stdint.h>

#ifdef _WIN32
#include "include/win32/fs_compat.h"
#endif

#ifdef __cplusplus
extern "C" {

class Fh;

struct inodeno_t;
struct vinodeno_t;
typedef struct vinodeno_t vinodeno;

#else /* __cplusplus */

typedef struct Fh Fh;

typedef struct inodeno_t {
  uint64_t val;
} inodeno_t;

typedef struct _snapid_t {
  uint64_t val;
} snapid_t;

typedef struct vinodeno_t {
  inodeno_t ino;
  snapid_t snapid;
} vinodeno_t;

#endif /* __cplusplus */

/*
 * Heavily borrowed from David Howells' draft statx patchset.
 *
 * Since the xstat patches are still a work in progress, we borrow its data
 * structures and #defines to implement stone_getattrx. Once the xstat stuff
 * has been merged we should drop this and switch over to using that instead.
 */
struct stone_statx {
	uint32_t	stx_mask;
	uint32_t	stx_blksize;
	uint32_t	stx_nlink;
	uint32_t	stx_uid;
	uint32_t	stx_gid;
	uint16_t	stx_mode;
	uint64_t	stx_ino;
	uint64_t	stx_size;
	uint64_t	stx_blocks;
	dev_t		stx_dev;
	dev_t		stx_rdev;
	struct timespec	stx_atime;
	struct timespec	stx_ctime;
	struct timespec	stx_mtime;
	struct timespec	stx_btime;
	uint64_t	stx_version;
};

#define STONE_STATX_MODE		0x00000001U     /* Want/got stx_mode */
#define STONE_STATX_NLINK	0x00000002U     /* Want/got stx_nlink */
#define STONE_STATX_UID		0x00000004U     /* Want/got stx_uid */
#define STONE_STATX_GID		0x00000008U     /* Want/got stx_gid */
#define STONE_STATX_RDEV		0x00000010U     /* Want/got stx_rdev */
#define STONE_STATX_ATIME	0x00000020U     /* Want/got stx_atime */
#define STONE_STATX_MTIME	0x00000040U     /* Want/got stx_mtime */
#define STONE_STATX_CTIME	0x00000080U     /* Want/got stx_ctime */
#define STONE_STATX_INO		0x00000100U     /* Want/got stx_ino */
#define STONE_STATX_SIZE		0x00000200U     /* Want/got stx_size */
#define STONE_STATX_BLOCKS	0x00000400U     /* Want/got stx_blocks */
#define STONE_STATX_BASIC_STATS	0x000007ffU     /* The stuff in the normal stat struct */
#define STONE_STATX_BTIME	0x00000800U     /* Want/got stx_btime */
#define STONE_STATX_VERSION	0x00001000U     /* Want/got stx_version */
#define STONE_STATX_ALL_STATS	0x00001fffU     /* All supported stats */

/*
 * Compatibility macros until these defines make their way into glibc
 */
#ifndef AT_NO_ATTR_SYNC
#define AT_NO_ATTR_SYNC		0x4000 /* Don't sync attributes with the server */
#endif

/*
 * The statx interfaces only allow these flags. In order to allow us to add
 * others in the future, we disallow setting any that aren't recognized.
 */
#define STONE_REQ_FLAG_MASK		(AT_SYMLINK_NOFOLLOW|AT_NO_ATTR_SYNC)

/* delegation recalls */
typedef void (*stone_deleg_cb_t)(Fh *fh, void *priv);

/* inode data/metadata invalidation */
typedef void (*client_ino_callback_t)(void *handle, vinodeno_t ino,
	      int64_t off, int64_t len);

/* dentry invalidation */
typedef void (*client_dentry_callback_t)(void *handle, vinodeno_t dirino,
					 vinodeno_t ino, const char *name,
					 size_t len);

/* remount entire fs */
typedef int (*client_remount_callback_t)(void *handle);

/* lock request interrupted */
typedef void (*client_switch_interrupt_callback_t)(void *handle, void *data);

/* fetch umask of actor */
typedef mode_t (*client_umask_callback_t)(void *handle);

/* request that application release Inode references */
typedef void (*client_ino_release_t)(void *handle, vinodeno_t ino);

/*
 * The handle is an opaque value that gets passed to some callbacks. Any fields
 * set to NULL will be left alone. There is no way to unregister callbacks.
 */
struct stone_client_callback_args {
  void *handle;
  client_ino_callback_t ino_cb;
  client_dentry_callback_t dentry_cb;
  client_switch_interrupt_callback_t switch_intr_cb;
  client_remount_callback_t remount_cb;
  client_umask_callback_t umask_cb;
  client_ino_release_t ino_release_cb;
};

#ifdef __cplusplus
}
#endif

#endif /* STONE_STATX_H */

