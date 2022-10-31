// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Stone - scalable distributed file system
 *
 * Copyright (C) 2009-2011 New Dream Network
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifndef STONE_LIB_H
#define STONE_LIB_H

#if defined(__linux__)
#include <features.h>
#endif
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

#include "stone_ll_client.h"

#ifdef __cplusplus
namespace stone::common {
  class StoneContext;
}
using StoneContext = stone::common::StoneContext;
extern "C" {
#endif

#define LIBSTONEFS_VER_MAJOR 10
#define LIBSTONEFS_VER_MINOR 0
#define LIBSTONEFS_VER_EXTRA 2

#define LIBSTONEFS_VERSION(maj, min, extra) ((maj << 16) + (min << 8) + extra)
#define LIBSTONEFS_VERSION_CODE LIBSTONEFS_VERSION(LIBSTONEFS_VER_MAJOR, LIBSTONEFS_VER_MINOR, LIBSTONEFS_VER_EXTRA)

#if __GNUC__ >= 4
  #define LIBSTONEFS_DEPRECATED   __attribute__((deprecated))
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#else
  #define LIBSTONEFS_DEPRECATED
#endif

/*
 * If using glibc check that file offset is 64-bit.
 */
#if defined(__GLIBC__) && !defined(__USE_FILE_OFFSET64)
# error libstone: glibc must define __USE_FILE_OFFSET64 or readdir results will be corrupted
#endif

/*
 * XXXX redeclarations from stone_fs.h, rados.h, etc.  We need more of this
 * in the interface, but shouldn't be re-typing it (and using different
 * C data types).
 */
#ifndef __cplusplus

#define STONE_INO_ROOT  1
#define STONE_NOSNAP  ((uint64_t)(-2))

struct stone_file_layout {
	/* file -> object mapping */
	uint32_t fl_stripe_unit;     /* stripe unit, in bytes.  must be multiple
				      of page size. */
	uint32_t fl_stripe_count;    /* over this many objects */
	uint32_t fl_object_size;     /* until objects are this big, then move to
				      new objects */
	uint32_t fl_cas_hash;        /* 0 = none; 1 = sha256 */

	/* pg -> disk layout */
	uint32_t fl_object_stripe_unit;  /* for per-object parity, if any */

	/* object -> pg layout */
	uint32_t fl_pg_preferred; /* preferred primary for pg (-1 for none) */
	uint32_t fl_pg_pool;      /* namespace, crush ruleset, rep level */
} __attribute__ ((packed));

struct StoneContext;
#endif /* ! __cplusplus */

struct UserPerm;
typedef struct UserPerm UserPerm;

struct Inode;
typedef struct Inode Inode;

struct stone_mount_info;
struct stone_dir_result;

// user supplied key,value pair to be associated with a snapshot.
// callers can supply an array of this struct via stone_mksnap().
struct snap_metadata {
  const char *key;
  const char *value;
};

struct snap_info {
  uint64_t id;
  size_t nr_snap_metadata;
  struct snap_metadata *snap_metadata;
};

/* setattr mask bits */
#ifndef STONE_SETATTR_MODE
# define STONE_SETATTR_MODE	1
# define STONE_SETATTR_UID	2
# define STONE_SETATTR_GID	4
# define STONE_SETATTR_MTIME	8
# define STONE_SETATTR_ATIME	16
# define STONE_SETATTR_SIZE	32
# define STONE_SETATTR_CTIME	64
# define STONE_SETATTR_MTIME_NOW	128
# define STONE_SETATTR_ATIME_NOW	256
# define STONE_SETATTR_BTIME	512
#endif

/* define error codes for the mount function*/
# define STONEFS_ERROR_MON_MAP_BUILD 1000
# define STONEFS_ERROR_NEW_CLIENT 1002
# define STONEFS_ERROR_MESSENGER_START 1003

/**
 * Create a UserPerm credential object.
 *
 * Some calls (most notably, the stone_ll_* ones), take a credential object
 * that represents the credentials that the calling program is using. This
 * function creates a new credential object for this purpose. Returns a
 * pointer to the object, or NULL if it can't be allocated.
 *
 * Note that the gidlist array is used directly and is not copied. It must
 * remain valid over the lifetime of the created UserPerm object.
 *
 * @param uid uid to be used
 * @param gid gid to be used
 * @param ngids number of gids in supplemental grouplist
 * @param gidlist array of gid_t's in the list of groups
 */
UserPerm *stone_userperm_new(uid_t uid, gid_t gid, int ngids, gid_t *gidlist);

/**
 * Destroy a UserPerm credential object.
 *
 * @param perm pointer to object to be destroyed
 *
 * Currently this just frees the object. Note that the gidlist array is not
 * freed. The caller must do so if it's necessary.
 */
void stone_userperm_destroy(UserPerm *perm);

/**
 * Get a pointer to the default UserPerm object for the mount.
 *
 * @param cmount the mount info handle
 *
 * Every cmount has a default set of credentials. This returns a pointer to
 * that object.
 *
 * Unlike with stone_userperm_new, this object should not be freed.
 */
struct UserPerm *stone_mount_perms(struct stone_mount_info *cmount);

/**
 * Set cmount's default permissions
 *
 * @param cmount the mount info handle
 * @param perm permissions to set to default for mount
 *
 * Every cmount has a default set of credentials. This does a deep copy of
 * the given permissions to the ones in the cmount. Must be done after
 * stone_init but before stone_mount.
 *
 * Returns 0 on success, and -EISCONN if the cmount is already mounted.
 */
int stone_mount_perms_set(struct stone_mount_info *cmount, UserPerm *perm);

/**
 * @defgroup libstonefs_h_init Setup and Teardown
 * These are the first and last functions that should be called
 * when using libstonefs.
 *
 * @{
 */

/**
 * Get the version of libstonefs.
 *
 * The version number is major.minor.patch.
 *
 * @param major where to store the major version number
 * @param minor where to store the minor version number
 * @param patch where to store the extra version number
 */
const char *stone_version(int *major, int *minor, int *patch);

/**
 * Create a mount handle for interacting with Stone.  All libstonefs
 * functions operate on a mount info handle.
 *
 * @param cmount the mount info handle to initialize
 * @param id the id of the client.  This can be a unique id that identifies
 *           this client, and will get appended onto "client.".  Callers can
 *           pass in NULL, and the id will be the process id of the client.
 * @returns 0 on success, negative error code on failure
 */
int stone_create(struct stone_mount_info **cmount, const char * const id);

/**
 * Create a mount handle from a StoneContext, which holds the configuration
 * for the stone cluster.  A StoneContext can be acquired from an existing stone_mount_info
 * handle, using the @ref stone_get_mount_context call.  Note that using the same StoneContext
 * for two different mount handles results in the same client entity id being used.
 *
 * @param cmount the mount info handle to initialize
 * @param conf reuse this pre-existing StoneContext config
 * @returns 0 on success, negative error code on failure
 */
#ifdef __cplusplus
int stone_create_with_context(struct stone_mount_info **cmount, StoneContext *conf);
#else
int stone_create_with_context(struct stone_mount_info **cmount, struct StoneContext *conf);
#endif

#ifndef VOIDPTR_RADOS_T
#define VOIDPTR_RADOS_T
typedef void *rados_t;
#endif // VOIDPTR_RADOS_T

/**
 * Create a mount handle from a rados_t, for using libstonefs in the
 * same process as librados.
 *
 * @param cmount the mount info handle to initialize
 * @param cluster reference to already-initialized librados handle
 * @returns 0 on success, negative error code on failure
 */
int stone_create_from_rados(struct stone_mount_info **cmount, rados_t cluster);

/**
 * Initialize the filesystem client (but do not mount the filesystem yet)
 *
 * @returns 0 on success, negative error code on failure
 */
int stone_init(struct stone_mount_info *cmount);

/**
 * Optionally set which filesystem to mount, before calling mount.
 *
 * An error will be returned if this libstonefs instance is already
 * mounted. This function is an alternative to setting the global
 * client_fs setting.  Using this function enables multiple libstonefs
 * instances in the same process to mount different filesystems.
 *
 * The filesystem name is *not* validated in this function.  That happens
 * during mount(), where an ENOENT error will result if a non-existent
 * filesystem was specified here.
 *
 * @param cmount the mount info handle
 * @returns 0 on success, negative error code on failure
 */
int stone_select_filesystem(struct stone_mount_info *cmount, const char *fs_name);


/**
 * Perform a mount using the path for the root of the mount.
 *
 * It is optional to call stone_init before this.  If stone_init has
 * not already been called, it will be called in the course of this operation.
 *
 * @param cmount the mount info handle
 * @param root the path for the root of the mount.  This can be an existing
 *	       directory within the stone cluster, but most likely it will
 * 	       be "/".  Passing in NULL is equivalent to "/".
 * @returns 0 on success, negative error code on failure
 */
int stone_mount(struct stone_mount_info *cmount, const char *root);

/**
 * Return cluster ID for a mounted stone filesystem
 *
 * Every stone filesystem has a filesystem ID associated with it. This
 * function returns that value. If the stone_mount_info does not refer to a
 * mounted filesystem, this returns a negative error code.
 */
int64_t stone_get_fs_cid(struct stone_mount_info *cmount);

/**
 * Execute a management command remotely on an MDS.
 *
 * Must have called stone_init or stone_mount before calling this.
 *
 * @param mds_spec string representing rank, MDS name, GID or '*'
 * @param cmd array of null-terminated strings
 * @param cmdlen length of cmd array
 * @param inbuf non-null-terminated input data to command
 * @param inbuflen length in octets of inbuf
 * @param outbuf populated with pointer to buffer (command output data)
 * @param outbuflen length of allocated outbuf
 * @param outs populated with pointer to buffer (command error strings)
 * @param outslen length of allocated outs
 *
 * @return 0 on success, negative error code on failure
 *
 */
int stone_mds_command(struct stone_mount_info *cmount,
    const char *mds_spec,
    const char **cmd,
    size_t cmdlen,
    const char *inbuf, size_t inbuflen,
    char **outbuf, size_t *outbuflen,
    char **outs, size_t *outslen);

/**
 * Free a buffer, such as those used for output arrays from stone_mds_command
 */
void stone_buffer_free(char *buf);

/**
 * Unmount a mount handle.
 *
 * @param cmount the mount handle
 * @return 0 on success, negative error code on failure
 */
int stone_unmount(struct stone_mount_info *cmount);

/**
 * Abort mds connections
 *
 * @param cmount the mount handle
 * @return 0 on success, negative error code on failure
 */
int stone_abort_conn(struct stone_mount_info *cmount);

/**
 * Destroy the mount handle.
 *
 * The handle should not be mounted. This should be called on completion of
 * all libstonefs functions.
 *
 * @param cmount the mount handle
 * @return 0 on success, negative error code on failure.
 */
int stone_release(struct stone_mount_info *cmount);

/**
 * Deprecated. Unmount and destroy the stone mount handle. This should be
 * called on completion of all libstonefs functions.
 *
 * Equivalent to stone_unmount() + stone_release() without error handling.
 *
 * @param cmount the mount handle to shutdown
 */
void stone_shutdown(struct stone_mount_info *cmount);

/**
 * Return associated client addresses
 *
 * @param cmount the mount handle
 * @param addrs the output addresses
 * @returns 0 on success, a negative error code on failure
 * @note the returned addrs should be free by the caller
 */
int stone_getaddrs(struct stone_mount_info *cmount, char** addrs);

/**
 * Get a global id for current instance
 *
 * The handle should not be mounted. This should be called on completion of
 * all libstonefs functions.
 *
 * @param cmount the mount handle
 * @returns instance global id
 */
uint64_t stone_get_instance_id(struct stone_mount_info *cmount);

/**
 * Extract the StoneContext from the mount point handle.
 *
 * @param cmount the stone mount handle to get the context from.
 * @returns the StoneContext associated with the mount handle.
 */
#ifdef __cplusplus
StoneContext *stone_get_mount_context(struct stone_mount_info *cmount);
#else
struct StoneContext *stone_get_mount_context(struct stone_mount_info *cmount);
#endif
/*
 * Check mount status.
 *
 * Return non-zero value if mounted. Otherwise, zero.
 */
int stone_is_mounted(struct stone_mount_info *cmount);

/** @} init */

/**
 * @defgroup libstonefs_h_config Config
 * Functions for manipulating the Stone configuration at runtime.
 *
 * @{
 */

/**
 * Load the stone configuration from the specified config file.
 *
 * @param cmount the mount handle to load the configuration into.
 * @param path_list the configuration file path
 * @returns 0 on success, negative error code on failure
 */
int stone_conf_read_file(struct stone_mount_info *cmount, const char *path_list);

/**
 * Parse the command line arguments and load the configuration parameters.
 *
 * @param cmount the mount handle to load the configuration parameters into.
 * @param argc count of the arguments in argv
 * @param argv the argument list
 * @returns 0 on success, negative error code on failure
 */
int stone_conf_parse_argv(struct stone_mount_info *cmount, int argc, const char **argv);

/**
 * Configure the cluster handle based on an environment variable
 *
 * The contents of the environment variable are parsed as if they were
 * Stone command line options. If var is NULL, the STONE_ARGS
 * environment variable is used.
 *
 * @pre stone_mount() has not been called on the handle
 *
 * @note BUG: this is not threadsafe - it uses a static buffer
 *
 * @param cmount handle to configure
 * @param var name of the environment variable to read
 * @returns 0 on success, negative error code on failure
 */
int stone_conf_parse_env(struct stone_mount_info *cmount, const char *var);

/** Sets a configuration value from a string.
 *
 * @param cmount the mount handle to set the configuration value on
 * @param option the configuration option to set
 * @param value the value of the configuration option to set
 * 
 * @returns 0 on success, negative error code otherwise.
 */
int stone_conf_set(struct stone_mount_info *cmount, const char *option, const char *value);

/** Set mount timeout.
 *
 * @param cmount mount handle to set the configuration value on
 * @param timeout mount timeout interval
 *
 * @returns 0 on success, negative error code otherwise.
 */
int stone_set_mount_timeout(struct stone_mount_info *cmount, uint32_t timeout);

/**
 * Gets the configuration value as a string.
 *
 * @param cmount the mount handle to set the configuration value on
 * @param option the config option to get
 * @param buf the buffer to fill with the value
 * @param len the length of the buffer.
 * @returns the size of the buffer filled in with the value, or negative error code on failure
 */
int stone_conf_get(struct stone_mount_info *cmount, const char *option, char *buf, size_t len);

/** @} config */

/**
 * @defgroup libstonefs_h_fsops File System Operations.
 * Functions for getting/setting file system wide information specific to a particular
 * mount handle.
 *
 * @{
 */

/**
 * Perform a statfs on the stone file system.  This call fills in file system wide statistics
 * into the passed in buffer.
 *
 * @param cmount the stone mount handle to use for performing the statfs.
 * @param path can be any path within the mounted filesystem
 * @param stbuf the file system statistics filled in by this function.
 * @return 0 on success, negative error code otherwise.
 */
int stone_statfs(struct stone_mount_info *cmount, const char *path, struct statvfs *stbuf);

/**
 * Synchronize all filesystem data to persistent media.
 *
 * @param cmount the stone mount handle to use for performing the sync_fs.
 * @returns 0 on success or negative error code on failure.
 */
int stone_sync_fs(struct stone_mount_info *cmount);

/**
 * Get the current working directory.
 *
 * @param cmount the stone mount to get the current working directory for.
 * @returns the path to the current working directory
 */
const char* stone_getcwd(struct stone_mount_info *cmount);

/**
 * Change the current working directory.
 *
 * @param cmount the stone mount to change the current working directory for.
 * @param path the path to the working directory to change into.
 * @returns 0 on success, negative error code otherwise.
 */
int stone_chdir(struct stone_mount_info *cmount, const char *path);

/** @} fsops */

/**
 * @defgroup libstonefs_h_dir Directory Operations.
 * Functions for manipulating and listing directories.
 *
 * @{
 */

/**
 * Open the given directory.
 *
 * @param cmount the stone mount handle to use to open the directory
 * @param name the path name of the directory to open.  Must be either an absolute path
 *        or a path relative to the current working directory.
 * @param dirpp the directory result pointer structure to fill in.
 * @returns 0 on success or negative error code otherwise.
 */
int stone_opendir(struct stone_mount_info *cmount, const char *name, struct stone_dir_result **dirpp);

/**
 * Open a directory referred to by a file descriptor
 *
 * @param cmount the stone mount handle to use to open the directory
 * @param dirfd open file descriptor for the directory
 * @param dirpp the directory result pointer structure to fill in
 * @returns 0 on success or negative error code otherwise
 */
int stone_fdopendir(struct stone_mount_info *cmount, int dirfd, struct stone_dir_result **dirpp);

/**
 * Close the open directory.
 *
 * @param cmount the stone mount handle to use for closing the directory
 * @param dirp the directory result pointer (set by stone_opendir) to close
 * @returns 0 on success or negative error code on failure.
 */
int stone_closedir(struct stone_mount_info *cmount, struct stone_dir_result *dirp);

/**
 * Get the next entry in an open directory.
 *
 * @param cmount the stone mount handle to use for performing the readdir.
 * @param dirp the directory stream pointer from an opendir holding the state of the
 *        next entry to return.
 * @returns the next directory entry or NULL if at the end of the directory (or the directory
 *          is empty.  This pointer should not be freed by the caller, and is only safe to
 *          access between return and the next call to stone_readdir or stone_closedir.
 */
struct dirent * stone_readdir(struct stone_mount_info *cmount, struct stone_dir_result *dirp);

/**
 * A safe version of stone_readdir, where the directory entry struct is allocated by the caller.
 *
 * @param cmount the stone mount handle to use for performing the readdir.
 * @param dirp the directory stream pointer from an opendir holding the state of the
 *        next entry to return.
 * @param de the directory entry pointer filled in with the next directory entry of the dirp state.
 * @returns 1 if the next entry was filled in, 0 if the end of the directory stream was reached,
 *          and a negative error code on failure.
 */
int stone_readdir_r(struct stone_mount_info *cmount, struct stone_dir_result *dirp, struct dirent *de);

/**
 * A safe version of stone_readdir that also returns the file statistics (readdir+stat).
 *
 * @param cmount the stone mount handle to use for performing the readdir_plus_r.
 * @param dirp the directory stream pointer from an opendir holding the state of the
 *        next entry to return.
 * @param de the directory entry pointer filled in with the next directory entry of the dirp state.
 * @param stx the stats of the file/directory of the entry returned
 * @param want mask showing desired inode attrs for returned entry
 * @param flags bitmask of flags to use when filling out attributes
 * @param out optional returned Inode argument. If non-NULL, then a reference will be taken on
 *            the inode and the pointer set on success.
 * @returns 1 if the next entry was filled in, 0 if the end of the directory stream was reached,
 *          and a negative error code on failure.
 */
int stone_readdirplus_r(struct stone_mount_info *cmount, struct stone_dir_result *dirp, struct dirent *de,
		       struct stone_statx *stx, unsigned want, unsigned flags, struct Inode **out);

/**
 * Gets multiple directory entries.
 *
 * @param cmount the stone mount handle to use for performing the getdents.
 * @param dirp the directory stream pointer from an opendir holding the state of the
 *        next entry/entries to return.
 * @param name an array of struct dirent that gets filled in with the  to fill returned directory entries into.
 * @param buflen the length of the buffer, which should be the number of dirent structs * sizeof(struct dirent).
 * @returns the length of the buffer that was filled in, will always be multiples of sizeof(struct dirent), or a
 *          negative error code.  If the buffer is not large enough for a single entry, -ERANGE is returned.
 */
int stone_getdents(struct stone_mount_info *cmount, struct stone_dir_result *dirp, char *name, int buflen);

/**
 * Gets multiple directory names.
 * 
 * @param cmount the stone mount handle to use for performing the getdents.
 * @param dirp the directory stream pointer from an opendir holding the state of the
 *        next entry/entries to return.
 * @param name a buffer to fill in with directory entry names.
 * @param buflen the length of the buffer that can be filled in.
 * @returns the length of the buffer filled in with entry names, or a negative error code on failure.
 *          If the buffer isn't large enough for a single entry, -ERANGE is returned.
 */
int stone_getdnames(struct stone_mount_info *cmount, struct stone_dir_result *dirp, char *name, int buflen);

/**
 * Rewind the directory stream to the beginning of the directory.
 *
 * @param cmount the stone mount handle to use for performing the rewinddir.
 * @param dirp the directory stream pointer to rewind.
 */
void stone_rewinddir(struct stone_mount_info *cmount, struct stone_dir_result *dirp);

/**
 * Get the current position of a directory stream.
 *
 * @param cmount the stone mount handle to use for performing the telldir.
 * @param dirp the directory stream pointer to get the current position of.
 * @returns the position of the directory stream.  Note that the offsets returned
 *          by stone_telldir do not have a particular order (cannot be compared with
 *          inequality).
 */
int64_t stone_telldir(struct stone_mount_info *cmount, struct stone_dir_result *dirp);

/**
 * Move the directory stream to a position specified by the given offset.
 *
 * @param cmount the stone mount handle to use for performing the seekdir.
 * @param dirp the directory stream pointer to move.
 * @param offset the position to move the directory stream to.  This offset should be
 *        a value returned by telldir.  Note that this value does not refer to the nth
 *        entry in a directory, and can not be manipulated with plus or minus.
 */
void stone_seekdir(struct stone_mount_info *cmount, struct stone_dir_result *dirp, int64_t offset);

/**
 * Create a directory.
 *
 * @param cmount the stone mount handle to use for making the directory.
 * @param path the path of the directory to create.  This must be either an
 *        absolute path or a relative path off of the current working directory.
 * @param mode the permissions the directory should have once created.
 * @returns 0 on success or a negative return code on error.
 */
int stone_mkdir(struct stone_mount_info *cmount, const char *path, mode_t mode);

/**
 * Create a directory relative to a file descriptor
 *
 * @param cmount the stone mount handle to use for making the directory.
 * @param dirfd open file descriptor for a directory (or STONEFS_AT_FDCWD)
 * @param relpath the path of the directory to create.
 * @param mode the permissions the directory should have once created.
 * @returns 0 on success or a negative return code on error.
 */
int stone_mkdirat(struct stone_mount_info *cmount, int dirfd, const char *relpath, mode_t mode);

/**
 * Create a snapshot
 *
 * @param cmount the stone mount handle to use for making the directory.
 * @param path the path of the directory to create snapshot.  This must be either an
 *        absolute path or a relative path off of the current working directory.
 * @param name snapshot name
 * @param mode the permissions the directory should have once created.
 * @param snap_metadata array of snap metadata structs
 * @param nr_snap_metadata number of snap metadata struct entries
 * @returns 0 on success or a negative return code on error.
 */
int stone_mksnap(struct stone_mount_info *cmount, const char *path, const char *name,
                mode_t mode, struct snap_metadata *snap_metadata, size_t nr_snap_metadata);

/**
 * Remove a snapshot
 *
 * @param cmount the stone mount handle to use for making the directory.
 * @param path the path of the directory to create snapshot.  This must be either an
 *        absolute path or a relative path off of the current working directory.
 * @param name snapshot name
 * @returns 0 on success or a negative return code on error.
 */
int stone_rmsnap(struct stone_mount_info *cmount, const char *path, const char *name);

/**
 * Create multiple directories at once.
 *
 * @param cmount the stone mount handle to use for making the directories.
 * @param path the full path of directories and sub-directories that should
 *        be created.
 * @param mode the permissions the directory should have once created.
 * @returns 0 on success or a negative return code on error.
 */
int stone_mkdirs(struct stone_mount_info *cmount, const char *path, mode_t mode);

/**
 * Remove a directory.
 *
 * @param cmount the stone mount handle to use for removing directories.
 * @param path the path of the directory to remove.
 * @returns 0 on success or a negative return code on error.
 */
int stone_rmdir(struct stone_mount_info *cmount, const char *path);

/** @} dir */

/**
 * @defgroup libstonefs_h_links Links and Link Handling.
 * Functions for creating and manipulating hard links and symbolic inks.
 *
 * @{
 */

/**
 * Create a link.
 *
 * @param cmount the stone mount handle to use for creating the link.
 * @param existing the path to the existing file/directory to link to.
 * @param newname the path to the new file/directory to link from.
 * @returns 0 on success or a negative return code on error.
 */
int stone_link(struct stone_mount_info *cmount, const char *existing, const char *newname);

/**
 * Read a symbolic link.
 *
 * @param cmount the stone mount handle to use for creating the link.
 * @param path the path to the symlink to read
 * @param buf the buffer to hold the path of the file that the symlink points to.
 * @param size the length of the buffer
 * @returns number of bytes copied on success or negative error code on failure
 */
int stone_readlink(struct stone_mount_info *cmount, const char *path, char *buf, int64_t size);

/**
 * Read a symbolic link relative to a file descriptor
 *
 * @param cmount the stone mount handle to use for creating the link.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath the path to the symlink to read
 * @param buf the buffer to hold the path of the file that the symlink points to.
 * @param size the length of the buffer
 * @returns number of bytes copied on success or negative error code on failure
 */
int stone_readlinkat(struct stone_mount_info *cmount, int dirfd, const char *relpath, char *buf,
                    int64_t size);

/**
 * Creates a symbolic link.
 *
 * @param cmount the stone mount handle to use for creating the symbolic link.
 * @param existing the path to the existing file/directory to link to.
 * @param newname the path to the new file/directory to link from.
 * @returns 0 on success or a negative return code on failure.
 */
int stone_symlink(struct stone_mount_info *cmount, const char *existing, const char *newname);

/**
 * Creates a symbolic link relative to a file descriptor
 *
 * @param cmount the stone mount handle to use for creating the symbolic link.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param existing the path to the existing file/directory to link to.
 * @param newname the path to the new file/directory to link from.
 * @returns 0 on success or a negative return code on failure.
 */
int stone_symlinkat(struct stone_mount_info *cmount, const char *existing, int dirfd,
                   const char *newname);

/** @} links */

/**
 * @defgroup libstonefs_h_files File manipulation and handling.
 * Functions for creating and manipulating files.
 *
 * @{
 */


/**
 * Checks if deleting a file, link or directory is allowed.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file, link or directory.
 * @returns 0 on success or negative error code on failure.
 */
int stone_may_delete(struct stone_mount_info *cmount, const char *path);

/**
 * Removes a file, link, or symbolic link.  If the file/link has multiple links to it, the
 * file will not disappear from the namespace until all references to it are removed.
 * 
 * @param cmount the stone mount handle to use for performing the unlink.
 * @param path the path of the file or link to unlink.
 * @returns 0 on success or negative error code on failure.
 */
int stone_unlink(struct stone_mount_info *cmount, const char *path);

/**
 * Removes a file, link, or symbolic link relative to a file descriptor.
 * If the file/link has multiple links to it, the file will not
 * disappear from the namespace until all references to it are removed.
 *
 * @param cmount the stone mount handle to use for performing the unlink.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath the path of the file or link to unlink.
 * @param flags bitfield that can be used to set AT_* modifier flags (only AT_REMOVEDIR)
 * @returns 0 on success or negative error code on failure.
 */
int stone_unlinkat(struct stone_mount_info *cmount, int dirfd, const char *relpath, int flags);

/**
 * Rename a file or directory.
 *
 * @param cmount the stone mount handle to use for performing the rename.
 * @param from the path to the existing file or directory.
 * @param to the new name of the file or directory
 * @returns 0 on success or negative error code on failure.
 */
int stone_rename(struct stone_mount_info *cmount, const char *from, const char *to);

/**
 * Get an open file's extended statistics and attributes.
 *
 * @param cmount the stone mount handle to use for performing the stat.
 * @param fd the file descriptor of the file to get statistics of.
 * @param stx the stone_statx struct that will be filled in with the file's statistics.
 * @param want bitfield of STONE_STATX_* flags showing designed attributes
 * @param flags bitfield that can be used to set AT_* modifier flags (only AT_NO_ATTR_SYNC and AT_SYMLINK_NOFOLLOW)
 * @returns 0 on success or negative error code on failure.
 */
int stone_fstatx(struct stone_mount_info *cmount, int fd, struct stone_statx *stx,
		unsigned int want, unsigned int flags);

/**
 * Get attributes of a file relative to a file descriptor
 *
 * @param cmount the stone mount handle to use for performing the stat.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath to the file/directory to get statistics of
 * @param stx the stone_statx struct that will be filled in with the file's statistics.
 * @param want bitfield of STONE_STATX_* flags showing designed attributes
 * @param flags bitfield that can be used to set AT_* modifier flags (only AT_NO_ATTR_SYNC and AT_SYMLINK_NOFOLLOW)
 * @returns 0 on success or negative error code on failure.
 */
int stone_statxat(struct stone_mount_info *cmount, int dirfd, const char *relpath,
                 struct stone_statx *stx, unsigned int want, unsigned int flags);

/**
 * Get a file's extended statistics and attributes.
 *
 * @param cmount the stone mount handle to use for performing the stat.
 * @param path the file or directory to get the statistics of.
 * @param stx the stone_statx struct that will be filled in with the file's statistics.
 * @param want bitfield of STONE_STATX_* flags showing designed attributes
 * @param flags bitfield that can be used to set AT_* modifier flags (only AT_NO_ATTR_SYNC and AT_SYMLINK_NOFOLLOW)
 * @returns 0 on success or negative error code on failure.
 */
int stone_statx(struct stone_mount_info *cmount, const char *path, struct stone_statx *stx,
	       unsigned int want, unsigned int flags);

/**
 * Get a file's statistics and attributes.
 *
 * stone_stat() is deprecated, use stone_statx() instead.
 *
 * @param cmount the stone mount handle to use for performing the stat.
 * @param path the file or directory to get the statistics of.
 * @param stbuf the stat struct that will be filled in with the file's statistics.
 * @returns 0 on success or negative error code on failure.
 */
int stone_stat(struct stone_mount_info *cmount, const char *path, struct stat *stbuf)
  LIBSTONEFS_DEPRECATED;

/**
 * Get a file's statistics and attributes, without following symlinks.
 *
 * stone_lstat() is deprecated, use stone_statx(.., AT_SYMLINK_NOFOLLOW) instead.
 *
 * @param cmount the stone mount handle to use for performing the stat.
 * @param path the file or directory to get the statistics of.
 * @param stbuf the stat struct that will be filled in with the file's statistics.
 * @returns 0 on success or negative error code on failure.
 */
int stone_lstat(struct stone_mount_info *cmount, const char *path, struct stat *stbuf)
  LIBSTONEFS_DEPRECATED;

/**
 * Get the open file's statistics.
 *
 * stone_fstat() is deprecated, use stone_fstatx() instead.
 *
 * @param cmount the stone mount handle to use for performing the fstat.
 * @param fd the file descriptor of the file to get statistics of.
 * @param stbuf the stat struct of the file's statistics, filled in by the
 *    function.
 * @returns 0 on success or a negative error code on failure
 */
int stone_fstat(struct stone_mount_info *cmount, int fd, struct stat *stbuf)
  LIBSTONEFS_DEPRECATED;

/**
 * Set a file's attributes.
 *
 * @param cmount the stone mount handle to use for performing the setattr.
 * @param relpath the path to the file/directory to set the attributes of.
 * @param stx the statx struct that must include attribute values to set on the file.
 * @param mask a mask of all the STONE_SETATTR_* values that have been set in the statx struct.
 * @param flags mask of AT_* flags (only AT_ATTR_NOFOLLOW is respected for now)
 * @returns 0 on success or negative error code on failure.
 */
int stone_setattrx(struct stone_mount_info *cmount, const char *relpath, struct stone_statx *stx, int mask, int flags);

/**
 * Set a file's attributes (extended version).
 * 
 * @param cmount the stone mount handle to use for performing the setattr.
 * @param fd the fd of the open file/directory to set the attributes of.
 * @param stx the statx struct that must include attribute values to set on the file.
 * @param mask a mask of all the stat values that have been set on the stat struct.
 * @returns 0 on success or negative error code on failure.
 */
int stone_fsetattrx(struct stone_mount_info *cmount, int fd, struct stone_statx *stx, int mask);

/**
 * Change the mode bits (permissions) of a file/directory.
 *
 * @param cmount the stone mount handle to use for performing the chmod.
 * @param path the path to the file/directory to change the mode bits on.
 * @param mode the new permissions to set.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_chmod(struct stone_mount_info *cmount, const char *path, mode_t mode);

/**
 * Change the mode bits (permissions) of a file/directory. If the path is a
 * symbolic link, it's not de-referenced.
 *
 * @param cmount the stone mount handle to use for performing the chmod.
 * @param path the path of file/directory to change the mode bits on.
 * @param mode the new permissions to set.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_lchmod(struct stone_mount_info *cmount, const char *path, mode_t mode);

/**
 * Change the mode bits (permissions) of an open file.
 *
 * @param cmount the stone mount handle to use for performing the chmod.
 * @param fd the open file descriptor to change the mode bits on.
 * @param mode the new permissions to set.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_fchmod(struct stone_mount_info *cmount, int fd, mode_t mode);

/**
 * Change the mode bits (permissions) of a file relative to a file descriptor.
 *
 * @param cmount the stone mount handle to use for performing the chown.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath the relpath of the file/directory to change the ownership of.
 * @param mode the new permissions to set.
 * @param flags bitfield that can be used to set AT_* modifier flags (AT_SYMLINK_NOFOLLOW)
 * @returns 0 on success or negative error code on failure.
 */
int stone_chmodat(struct stone_mount_info *cmount, int dirfd, const char *relpath,
                 mode_t mode, int flags);

/**
 * Change the ownership of a file/directory.
 * 
 * @param cmount the stone mount handle to use for performing the chown.
 * @param path the path of the file/directory to change the ownership of.
 * @param uid the user id to set on the file/directory.
 * @param gid the group id to set on the file/directory.
 * @returns 0 on success or negative error code on failure.
 */
int stone_chown(struct stone_mount_info *cmount, const char *path, int uid, int gid);

/**
 * Change the ownership of a file from an open file descriptor.
 *
 * @param cmount the stone mount handle to use for performing the chown.
 * @param fd the fd of the open file/directory to change the ownership of.
 * @param uid the user id to set on the file/directory.
 * @param gid the group id to set on the file/directory.
 * @returns 0 on success or negative error code on failure.
 */
int stone_fchown(struct stone_mount_info *cmount, int fd, int uid, int gid);

/**
 * Change the ownership of a file/directory, don't follow symlinks.
 * 
 * @param cmount the stone mount handle to use for performing the chown.
 * @param path the path of the file/directory to change the ownership of.
 * @param uid the user id to set on the file/directory.
 * @param gid the group id to set on the file/directory.
 * @returns 0 on success or negative error code on failure.
 */
int stone_lchown(struct stone_mount_info *cmount, const char *path, int uid, int gid);

/**
 * Change the ownership of a file/directory releative to a file descriptor.
 *
 * @param cmount the stone mount handle to use for performing the chown.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath the relpath of the file/directory to change the ownership of.
 * @param uid the user id to set on the file/directory.
 * @param gid the group id to set on the file/directory.
 * @param flags bitfield that can be used to set AT_* modifier flags (AT_SYMLINK_NOFOLLOW)
 * @returns 0 on success or negative error code on failure.
 */
int stone_chownat(struct stone_mount_info *cmount, int dirfd, const char *relpath,
                 uid_t uid, gid_t gid, int flags);

/**
 * Change file/directory last access and modification times.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param path the path to the file/directory to set the time values of.
 * @param buf holding the access and modification times to set on the file.
 * @returns 0 on success or negative error code on failure.
 */
int stone_utime(struct stone_mount_info *cmount, const char *path, struct utimbuf *buf);

/**
 * Change file/directory last access and modification times.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param fd the fd of the open file/directory to set the time values of.
 * @param buf holding the access and modification times to set on the file.
 * @returns 0 on success or negative error code on failure.
 */
int stone_futime(struct stone_mount_info *cmount, int fd, struct utimbuf *buf);

/**
 * Change file/directory last access and modification times.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param path the path to the file/directory to set the time values of.
 * @param times holding the access and modification times to set on the file.
 * @returns 0 on success or negative error code on failure.
 */
int stone_utimes(struct stone_mount_info *cmount, const char *path, struct timeval times[2]);

/**
 * Change file/directory last access and modification times, don't follow symlinks.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param path the path to the file/directory to set the time values of.
 * @param times holding the access and modification times to set on the file.
 * @returns 0 on success or negative error code on failure.
 */
int stone_lutimes(struct stone_mount_info *cmount, const char *path, struct timeval times[2]);

/**
 * Change file/directory last access and modification times.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param fd the fd of the open file/directory to set the time values of.
 * @param times holding the access and modification times to set on the file.
 * @returns 0 on success or negative error code on failure.
 */
int stone_futimes(struct stone_mount_info *cmount, int fd, struct timeval times[2]);

/**
 * Change file/directory last access and modification times.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param fd the fd of the open file/directory to set the time values of.
 * @param times holding the access and modification times to set on the file.
 * @returns 0 on success or negative error code on failure.
 */
int stone_futimens(struct stone_mount_info *cmount, int fd, struct timespec times[2]);

/**
 * Change file/directory last access and modification times relative
 * to a file descriptor.
 *
 * @param cmount the stone mount handle to use for performing the utime.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath the relpath of the file/directory to change the ownership of.
 * @param dirfd the fd of the open file/directory to set the time values of.
 * @param times holding the access and modification times to set on the file.
 * @param flags bitfield that can be used to set AT_* modifier flags (AT_SYMLINK_NOFOLLOW)
 * @returns 0 on success or negative error code on failure.
 */
int stone_utimensat(struct stone_mount_info *cmount, int dirfd, const char *relpath,
                   struct timespec times[2], int flags);

/**
 * Apply or remove an advisory lock.
 *
 * @param cmount the stone mount handle to use for performing the lock.
 * @param fd the open file descriptor to change advisory lock.
 * @param operation the advisory lock operation to be performed on the file
 * descriptor among LOCK_SH (shared lock), LOCK_EX (exclusive lock),
 * or LOCK_UN (remove lock). The LOCK_NB value can be ORed to perform a
 * non-blocking operation.
 * @param owner the user-supplied owner identifier (an arbitrary integer)
 * @returns 0 on success or negative error code on failure.
 */
int stone_flock(struct stone_mount_info *cmount, int fd, int operation,
	       uint64_t owner);

/**
 * Truncate the file to the given size.  If this operation causes the
 * file to expand, the empty bytes will be filled in with zeros.
 *
 * @param cmount the stone mount handle to use for performing the truncate.
 * @param path the path to the file to truncate.
 * @param size the new size of the file.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_truncate(struct stone_mount_info *cmount, const char *path, int64_t size);

/**
 * Make a block or character special file.
 *
 * @param cmount the stone mount handle to use for performing the mknod.
 * @param path the path to the special file.
 * @param mode the permissions to use and the type of special file.  The type can be
 *        one of S_IFREG, S_IFCHR, S_IFBLK, S_IFIFO.
 * @param rdev If the file type is S_IFCHR or S_IFBLK then this parameter specifies the
 *        major and minor numbers of the newly created device special file.  Otherwise, 
 *        it is ignored.
 * @returns 0 on success or negative error code on failure.
 */
int stone_mknod(struct stone_mount_info *cmount, const char *path, mode_t mode, dev_t rdev);
/**
 * Create and/or open a file.
 *
 * @param cmount the stone mount handle to use for performing the open.
 * @param path the path of the file to open.  If the flags parameter includes O_CREAT,
 *        the file will first be created before opening.
 * @param flags a set of option masks that control how the file is created/opened.
 * @param mode the permissions to place on the file if the file does not exist and O_CREAT
 *        is specified in the flags.
 * @returns a non-negative file descriptor number on success or a negative error code on failure.
 */
int stone_open(struct stone_mount_info *cmount, const char *path, int flags, mode_t mode);

/**
 * Create and/or open a file relative to a directory
 *
 * @param cmount the stone mount handle to use for performing the open.
 * @param dirfd open file descriptor (or STONEFS_AT_FDCWD)
 * @param relpath the path of the file to open.  If the flags parameter includes O_CREAT,
 *        the file will first be created before opening.
 * @param flags a set of option masks that control how the file is created/opened.
 * @param mode the permissions to place on the file if the file does not exist and O_CREAT
 *        is specified in the flags.
 * @returns a non-negative file descriptor number on success or a negative error code on failure.
 */
int stone_openat(struct stone_mount_info *cmount, int dirfd, const char *relpath, int flags, mode_t mode);

/**
 * Create and/or open a file with a specific file layout.
 *
 * @param cmount the stone mount handle to use for performing the open.
 * @param path the path of the file to open.  If the flags parameter includes O_CREAT,
 *        the file will first be created before opening.
 * @param flags a set of option masks that control how the file is created/opened.
 * @param mode the permissions to place on the file if the file does not exist and O_CREAT
 *        is specified in the flags.
 * @param stripe_unit the stripe unit size (option, 0 for default)
 * @param stripe_count the stripe count (optional, 0 for default)
 * @param object_size the object size (optional, 0 for default)
 * @param data_pool name of target data pool name (optional, NULL or empty string for default)
 * @returns a non-negative file descriptor number on success or a negative error code on failure.
 */
int stone_open_layout(struct stone_mount_info *cmount, const char *path, int flags,
 		     mode_t mode, int stripe_unit, int stripe_count, int object_size,
 		     const char *data_pool);

/**
 * Close the open file.
 *
 * @param cmount the stone mount handle to use for performing the close.
 * @param fd the file descriptor referring to the open file.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_close(struct stone_mount_info *cmount, int fd);

/**
 * Reposition the open file stream based on the given offset.
 *
 * @param cmount the stone mount handle to use for performing the lseek.
 * @param fd the open file descriptor referring to the open file and holding the
 *        current position of the stream.
 * @param offset the offset to set the stream to
 * @param whence the flag to indicate what type of seeking to perform:
 *	SEEK_SET: the offset is set to the given offset in the file.
 *      SEEK_CUR: the offset is set to the current location plus @e offset bytes.
 *      SEEK_END: the offset is set to the end of the file plus @e offset bytes.
 * @returns 0 on success or a negative error code on failure.
 */
int64_t stone_lseek(struct stone_mount_info *cmount, int fd, int64_t offset, int whence);
/**
 * Read data from the file.
 *
 * @param cmount the stone mount handle to use for performing the read.
 * @param fd the file descriptor of the open file to read from.
 * @param buf the buffer to read data into
 * @param size the initial size of the buffer
 * @param offset the offset in the file to read from.  If this value is negative, the
 *        function reads from the current offset of the file descriptor.
 * @returns the number of bytes read into buf, or a negative error code on failure.
 */
int stone_read(struct stone_mount_info *cmount, int fd, char *buf, int64_t size, int64_t offset);

/**
 * Read data from the file.
 * @param cmount the stone mount handle to use for performing the read.
 * @param fd the file descriptor of the open file to read from.
 * @param iov the iov structure to read data into
 * @param iovcnt the number of items that iov includes
 * @param offset the offset in the file to read from.  If this value is negative, the
 *        function reads from the current offset of the file descriptor.
 * @returns the number of bytes read into buf, or a negative error code on failure.
 */
int stone_preadv(struct stone_mount_info *cmount, int fd, const struct iovec *iov, int iovcnt,
           int64_t offset);

/**
 * Write data to a file.
 *
 * @param cmount the stone mount handle to use for performing the write.
 * @param fd the file descriptor of the open file to write to
 * @param buf the bytes to write to the file
 * @param size the size of the buf array
 * @param offset the offset of the file write into.  If this value is negative, the
 *        function writes to the current offset of the file descriptor.
 * @returns the number of bytes written, or a negative error code
 */
int stone_write(struct stone_mount_info *cmount, int fd, const char *buf, int64_t size,
	       int64_t offset);

/**
 * Write data to a file.
 *
 * @param cmount the stone mount handle to use for performing the write.
 * @param fd the file descriptor of the open file to write to
 * @param iov the iov structure to read data into
 * @param iovcnt the number of items that iov includes
 * @param offset the offset of the file write into.  If this value is negative, the
 *        function writes to the current offset of the file descriptor.
 * @returns the number of bytes written, or a negative error code
 */
int stone_pwritev(struct stone_mount_info *cmount, int fd, const struct iovec *iov, int iovcnt,
           int64_t offset);

/**
 * Truncate a file to the given size.
 *
 * @param cmount the stone mount handle to use for performing the ftruncate.
 * @param fd the file descriptor of the file to truncate
 * @param size the new size of the file
 * @returns 0 on success or a negative error code on failure.
 */
int stone_ftruncate(struct stone_mount_info *cmount, int fd, int64_t size);

/**
 * Synchronize an open file to persistent media.
 *
 * @param cmount the stone mount handle to use for performing the fsync.
 * @param fd the file descriptor of the file to sync.
 * @param syncdataonly a boolean whether to synchronize metadata and data (0)
 *        or just data (1).
 * @return 0 on success or a negative error code on failure.
 */
int stone_fsync(struct stone_mount_info *cmount, int fd, int syncdataonly);

/**
 * Preallocate or release disk space for the file for the byte range.
 *
 * @param cmount the stone mount handle to use for performing the fallocate.
 * @param fd the file descriptor of the file to fallocate.
 * @param mode the flags determines the operation to be performed on the given range.
 *        default operation (0) allocate and initialize to zero the file in the byte range,
 *        and the file size will be changed if offset + length is greater than
 *        the file size. if the FALLOC_FL_KEEP_SIZE flag is specified in the mode,
 *        the file size will not be changed. if the FALLOC_FL_PUNCH_HOLE flag is
 *        specified in the mode, the operation is deallocate space and zero the byte range.
 * @param offset the byte range starting.
 * @param length the length of the range.
 * @return 0 on success or a negative error code on failure.
 */
int stone_fallocate(struct stone_mount_info *cmount, int fd, int mode,
	                      int64_t offset, int64_t length);

/**
 * Enable/disable lazyio for the file.
 *
 * @param cmount the stone mount handle to use for performing the fsync.
 * @param fd the file descriptor of the file to sync.
 * @param enable a boolean to enable lazyio or disable lazyio.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_lazyio(struct stone_mount_info *cmount, int fd, int enable);


/**
 * Flushes the write buffer for the file thereby propogating the buffered write to the file.
 *
 * @param cmount the stone mount handle to use for performing the fsync.
 * @param fd the file descriptor of the file to sync.
 * @param offset a boolean to enable lazyio or disable lazyio.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_lazyio_propagate(struct stone_mount_info *cmount, int fd, int64_t offset, size_t count);


/**
 * Flushes the write buffer for the file and invalidate the read cache. This allows a subsequent read operation to read and cache data directly from the file and hence everyone's propagated writes would be visible. 
 *
 * @param cmount the stone mount handle to use for performing the fsync.
 * @param fd the file descriptor of the file to sync.
 * @param offset a boolean to enable lazyio or disable lazyio.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_lazyio_synchronize(struct stone_mount_info *cmount, int fd, int64_t offset, size_t count);

/** @} file */

/**
 * @defgroup libstonefs_h_xattr Extended Attribute manipulation and handling.
 * Functions for creating and manipulating extended attributes on files.
 *
 * @{
 */

/**
 * Get an extended attribute.
 *
 * @param cmount the stone mount handle to use for performing the getxattr.
 * @param path the path to the file
 * @param name the name of the extended attribute to get
 * @param value a pre-allocated buffer to hold the xattr's value
 * @param size the size of the pre-allocated buffer
 * @returns the size of the value or a negative error code on failure.
 */
int stone_getxattr(struct stone_mount_info *cmount, const char *path, const char *name, 
	void *value, size_t size);

/**
 * Get an extended attribute.
 *
 * @param cmount the stone mount handle to use for performing the getxattr.
 * @param fd the open file descriptor referring to the file to get extended attribute from.
 * @param name the name of the extended attribute to get
 * @param value a pre-allocated buffer to hold the xattr's value
 * @param size the size of the pre-allocated buffer
 * @returns the size of the value or a negative error code on failure.
 */
int stone_fgetxattr(struct stone_mount_info *cmount, int fd, const char *name,
	void *value, size_t size);

/**
 * Get an extended attribute without following symbolic links.  This function is
 * identical to stone_getxattr, but if the path refers to a symbolic link,
 * we get the extended attributes of the symlink rather than the attributes
 * of the link itself.
 *
 * @param cmount the stone mount handle to use for performing the lgetxattr.
 * @param path the path to the file
 * @param name the name of the extended attribute to get
 * @param value a pre-allocated buffer to hold the xattr's value
 * @param size the size of the pre-allocated buffer
 * @returns the size of the value or a negative error code on failure.
 */
int stone_lgetxattr(struct stone_mount_info *cmount, const char *path, const char *name, 
	void *value, size_t size);

/**
 * List the extended attribute keys on a file.
 *
 * @param cmount the stone mount handle to use for performing the listxattr.
 * @param path the path to the file.
 * @param list a buffer to be filled in with the list of extended attributes keys.
 * @param size the size of the list buffer.
 * @returns the size of the resulting list filled in.
 */
int stone_listxattr(struct stone_mount_info *cmount, const char *path, char *list, size_t size);

/**
 * List the extended attribute keys on a file.
 *
 * @param cmount the stone mount handle to use for performing the listxattr.
 * @param fd the open file descriptor referring to the file to list extended attributes on.
 * @param list a buffer to be filled in with the list of extended attributes keys.
 * @param size the size of the list buffer.
 * @returns the size of the resulting list filled in.
 */
int stone_flistxattr(struct stone_mount_info *cmount, int fd, char *list, size_t size);

/**
 * Get the list of extended attribute keys on a file, but do not follow symbolic links.
 *
 * @param cmount the stone mount handle to use for performing the llistxattr.
 * @param path the path to the file.
 * @param list a buffer to be filled in with the list of extended attributes keys.
 * @param size the size of the list buffer.
 * @returns the size of the resulting list filled in.
 */
int stone_llistxattr(struct stone_mount_info *cmount, const char *path, char *list, size_t size);

/**
 * Remove an extended attribute from a file.
 *
 * @param cmount the stone mount handle to use for performing the removexattr.
 * @param path the path to the file.
 * @param name the name of the extended attribute to remove.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_removexattr(struct stone_mount_info *cmount, const char *path, const char *name);

/**
 * Remove an extended attribute from a file.
 *
 * @param cmount the stone mount handle to use for performing the removexattr.
 * @param fd the open file descriptor referring to the file to remove extended attribute from.
 * @param name the name of the extended attribute to remove.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_fremovexattr(struct stone_mount_info *cmount, int fd, const char *name);

/**
 * Remove the extended attribute from a file, do not follow symbolic links.
 *
 * @param cmount the stone mount handle to use for performing the lremovexattr.
 * @param path the path to the file.
 * @param name the name of the extended attribute to remove.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_lremovexattr(struct stone_mount_info *cmount, const char *path, const char *name);

/**
 * Set an extended attribute on a file.
 *
 * @param cmount the stone mount handle to use for performing the setxattr.
 * @param path the path to the file.
 * @param name the name of the extended attribute to set.
 * @param value the bytes of the extended attribute value
 * @param size the size of the extended attribute value
 * @param flags the flags can be:
 *	STONE_XATTR_CREATE: create the extended attribute.  Must not exist.
 *      STONE_XATTR_REPLACE: replace the extended attribute, Must already exist.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_setxattr(struct stone_mount_info *cmount, const char *path, const char *name, 
	const void *value, size_t size, int flags);

/**
 * Set an extended attribute on a file.
 *
 * @param cmount the stone mount handle to use for performing the setxattr.
 * @param fd the open file descriptor referring to the file to set extended attribute on.
 * @param name the name of the extended attribute to set.
 * @param value the bytes of the extended attribute value
 * @param size the size of the extended attribute value
 * @param flags the flags can be:
 *	STONE_XATTR_CREATE: create the extended attribute.  Must not exist.
 *      STONE_XATTR_REPLACE: replace the extended attribute, Must already exist.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_fsetxattr(struct stone_mount_info *cmount, int fd, const char *name,
	const void *value, size_t size, int flags);

/**
 * Set an extended attribute on a file, do not follow symbolic links.
 *
 * @param cmount the stone mount handle to use for performing the lsetxattr.
 * @param path the path to the file.
 * @param name the name of the extended attribute to set.
 * @param value the bytes of the extended attribute value
 * @param size the size of the extended attribute value
 * @param flags the flags can be:
 *	STONE_XATTR_CREATE: create the extended attribute.  Must not exist.
 *      STONE_XATTR_REPLACE: replace the extended attribute, Must already exist.
 * @returns 0 on success or a negative error code on failure.
 */
int stone_lsetxattr(struct stone_mount_info *cmount, const char *path, const char *name, 
	const void *value, size_t size, int flags);

/** @} xattr */

/**
 * @defgroup libstonefs_h_filelayout Control File Layout.
 * Functions for setting and getting the file layout of existing files.
 *
 * @{
 */

/**
 * Get the file striping unit from an open file descriptor.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file to get the striping unit of.
 * @returns the striping unit of the file or a negative error code on failure.
 */
int stone_get_file_stripe_unit(struct stone_mount_info *cmount, int fh);

/**
 * Get the file striping unit.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory get the striping unit of.
 * @returns the striping unit of the file or a negative error code on failure.
 */
int stone_get_path_stripe_unit(struct stone_mount_info *cmount, const char *path);

/**
 * Get the file striping count from an open file descriptor.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file to get the striping count of.
 * @returns the striping count of the file or a negative error code on failure.
 */
int stone_get_file_stripe_count(struct stone_mount_info *cmount, int fh);

/**
 * Get the file striping count.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory get the striping count of.
 * @returns the striping count of the file or a negative error code on failure.
 */
int stone_get_path_stripe_count(struct stone_mount_info *cmount, const char *path);

/**
 * Get the file object size from an open file descriptor.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file to get the object size of.
 * @returns the object size of the file or a negative error code on failure.
 */
int stone_get_file_object_size(struct stone_mount_info *cmount, int fh);

/**
 * Get the file object size.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory get the object size of.
 * @returns the object size of the file or a negative error code on failure.
 */
int stone_get_path_object_size(struct stone_mount_info *cmount, const char *path);

/**
 * Get the file pool information from an open file descriptor.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file to get the pool information of.
 * @returns the stone pool id that the file is in
 */
int stone_get_file_pool(struct stone_mount_info *cmount, int fh);

/**
 * Get the file pool information.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory get the pool information of.
 * @returns the stone pool id that the file is in
 */
int stone_get_path_pool(struct stone_mount_info *cmount, const char *path);

/**
 * Get the name of the pool a opened file is stored in,
 *
 * Write the name of the file's pool to the buffer.  If buflen is 0, return
 * a suggested length for the buffer.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file
 * @param buf buffer to store the name in
 * @param buflen size of the buffer
 * @returns length in bytes of the pool name, or -ERANGE if the buffer is not large enough.
 */
int stone_get_file_pool_name(struct stone_mount_info *cmount, int fh, char *buf, size_t buflen);

/**
 * get the name of a pool by id
 *
 * Given a pool's numeric identifier, get the pool's alphanumeric name.
 *
 * @param cmount the stone mount handle to use
 * @param pool the numeric pool id
 * @param buf buffer to sore the name in
 * @param buflen size of the buffer
 * @returns length in bytes of the pool name, or -ERANGE if the buffer is not large enough
 */
int stone_get_pool_name(struct stone_mount_info *cmount, int pool, char *buf, size_t buflen);

/**
 * Get the name of the pool a file is stored in
 *
 * Write the name of the file's pool to the buffer.  If buflen is 0, return
 * a suggested length for the buffer.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory
 * @param buf buffer to store the name in
 * @param buflen size of the buffer
 * @returns length in bytes of the pool name, or -ERANGE if the buffer is not large enough.
 */
int stone_get_path_pool_name(struct stone_mount_info *cmount, const char *path, char *buf, size_t buflen);

/**
 * Get the default pool name of stonefs
 * Write the name of the default pool to the buffer. If buflen is 0, return
 * a suggested length for the buffer.
 * @param cmount the stone mount handle to use.
 * @param buf buffer to store the name in
 * @param buflen size of the buffer
 * @returns length in bytes of the pool name, or -ERANGE if the buffer is not large enough.
 */
int stone_get_default_data_pool_name(struct stone_mount_info *cmount, char *buf, size_t buflen);

/**
 * Get the file layout from an open file descriptor.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file to get the layout of.
 * @param stripe_unit where to store the striping unit of the file
 * @param stripe_count where to store the striping count of the file
 * @param object_size where to store the object size of the file
 * @param pg_pool where to store the stone pool id that the file is in
 * @returns 0 on success or a negative error code on failure.
 */
int stone_get_file_layout(struct stone_mount_info *cmount, int fh, int *stripe_unit, int *stripe_count, int *object_size, int *pg_pool);

/**
 * Get the file layout.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory get the layout of.
 * @param stripe_unit where to store the striping unit of the file
 * @param stripe_count where to store the striping count of the file
 * @param object_size where to store the object size of the file
 * @param pg_pool where to store the stone pool id that the file is in
 * @returns 0 on success or a negative error code on failure.
 */
int stone_get_path_layout(struct stone_mount_info *cmount, const char *path, int *stripe_unit, int *stripe_count, int *object_size, int *pg_pool);

/**
 * Get the file replication information from an open file descriptor.
 *
 * @param cmount the stone mount handle to use.
 * @param fh the open file descriptor referring to the file to get the replication information of.
 * @returns the replication factor of the file.
 */
int stone_get_file_replication(struct stone_mount_info *cmount, int fh);

/**
 * Get the file replication information.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path of the file/directory get the replication information of.
 * @returns the replication factor of the file.
 */
int stone_get_path_replication(struct stone_mount_info *cmount, const char *path);

/**
 * Get the id of the named pool.
 *
 * @param cmount the stone mount handle to use.
 * @param pool_name the name of the pool.
 * @returns the pool id, or a negative error code on failure.
 */
int stone_get_pool_id(struct stone_mount_info *cmount, const char *pool_name);

/**
 * Get the pool replication factor.
 *
 * @param cmount the stone mount handle to use.
 * @param pool_id the pool id to look up
 * @returns the replication factor, or a negative error code on failure.
 */
int stone_get_pool_replication(struct stone_mount_info *cmount, int pool_id);

/**
 * Get the OSD address where the primary copy of a file stripe is located.
 *
 * @param cmount the stone mount handle to use.
 * @param fd the open file descriptor referring to the file to get the striping unit of.
 * @param offset the offset into the file to specify the stripe.  The offset can be
 *	anywhere within the stripe unit.
 * @param addr the address of the OSD holding that stripe
 * @param naddr the capacity of the address passed in.
 * @returns the size of the addressed filled into the @e addr parameter, or a negative
 *	error code on failure.
 */
int stone_get_file_stripe_address(struct stone_mount_info *cmount, int fd, int64_t offset,
				 struct sockaddr_storage *addr, int naddr);

/**
 * Get the list of OSDs where the objects containing a file offset are located.
 *
 * @param cmount the stone mount handle to use.
 * @param fd the open file descriptor referring to the file.
 * @param offset the offset within the file.
 * @param length return the number of bytes between the offset and the end of
 * the stripe unit (optional).
 * @param osds an integer array to hold the OSD ids.
 * @param nosds the size of the integer array.
 * @returns the number of items stored in the output array, or -ERANGE if the
 * array is not large enough.
 */
int stone_get_file_extent_osds(struct stone_mount_info *cmount, int fd,
                              int64_t offset, int64_t *length, int *osds, int nosds);

/**
 * Get the fully qualified CRUSH location of an OSD.
 *
 * Returns (type, name) string pairs for each device in the CRUSH bucket
 * hierarchy starting from the given osd to the root. Each pair element is
 * separated by a NULL character.
 *
 * @param cmount the stone mount handle to use.
 * @param osd the OSD id.
 * @param path buffer to store location.
 * @param len size of buffer.
 * @returns the amount of bytes written into the buffer, or -ERANGE if the
 * array is not large enough.
 */
int stone_get_osd_crush_location(struct stone_mount_info *cmount,
    int osd, char *path, size_t len);

/**
 * Get the network address of an OSD.
 *
 * @param cmount the stone mount handle.
 * @param osd the OSD id.
 * @param addr the OSD network address.
 * @returns zero on success, other returns a negative error code.
 */
int stone_get_osd_addr(struct stone_mount_info *cmount, int osd,
    struct sockaddr_storage *addr);

/**
 * Get the file layout stripe unit granularity.
 * @param cmount the stone mount handle.
 * @returns the stripe unit granularity or a negative error code on failure.
 */
int stone_get_stripe_unit_granularity(struct stone_mount_info *cmount);

/** @} filelayout */

/**
 * No longer available.  Do not use.
 * These functions will return -EOPNOTSUPP.
 */
int stone_set_default_file_stripe_unit(struct stone_mount_info *cmount, int stripe);
int stone_set_default_file_stripe_count(struct stone_mount_info *cmount, int count);
int stone_set_default_object_size(struct stone_mount_info *cmount, int size);
int stone_set_default_preferred_pg(struct stone_mount_info *cmount, int osd);
int stone_set_default_file_replication(struct stone_mount_info *cmount, int replication);

/**
 * Read from local replicas when possible.
 *
 * @param cmount the stone mount handle to use.
 * @param val a boolean to set (1) or clear (0) the option to favor local objects
 *     for reads.
 * @returns 0
 */
int stone_localize_reads(struct stone_mount_info *cmount, int val);

/**
 * Get the osd id of the local osd (if any)
 *
 * @param cmount the stone mount handle to use.
 * @returns the osd (if any) local to the node where this call is made, otherwise
 *	-1 is returned.
 */
int stone_get_local_osd(struct stone_mount_info *cmount);

/** @} default_filelayout */

/**
 * Get the capabilities currently issued to the client.
 *
 * @param cmount the stone mount handle to use.
 * @param fd the file descriptor to get issued
 * @returns the current capabilities issued to this client
 *       for the open file
 */
int stone_debug_get_fd_caps(struct stone_mount_info *cmount, int fd);

/**
 * Get the capabilities currently issued to the client.
 *
 * @param cmount the stone mount handle to use.
 * @param path the path to the file
 * @returns the current capabilities issued to this client
 *       for the file
 */
int stone_debug_get_file_caps(struct stone_mount_info *cmount, const char *path);

/* Low Level */
struct Inode *stone_ll_get_inode(struct stone_mount_info *cmount,
				vinodeno_t vino);

int stone_ll_lookup_vino(struct stone_mount_info *cmount, vinodeno_t vino,
			Inode **inode);

int stone_ll_lookup_inode(
    struct stone_mount_info *cmount,
    struct inodeno_t ino,
    Inode **inode);

/**
 * Get the root inode of FS. Increase counter of references for root Inode. You must call stone_ll_forget for it!
 *
 * @param cmount the stone mount handle to use.
 * @param parent pointer to pointer to Inode struct. Pointer to root inode will be returned
 * @returns 0 if all good
 */
int stone_ll_lookup_root(struct stone_mount_info *cmount,
                  Inode **parent);
int stone_ll_lookup(struct stone_mount_info *cmount, Inode *parent,
		   const char *name, Inode **out, struct stone_statx *stx,
		   unsigned want, unsigned flags, const UserPerm *perms);
int stone_ll_put(struct stone_mount_info *cmount, struct Inode *in);
int stone_ll_forget(struct stone_mount_info *cmount, struct Inode *in,
		   int count);
int stone_ll_walk(struct stone_mount_info *cmount, const char* name, Inode **i,
		 struct stone_statx *stx, unsigned int want, unsigned int flags,
		 const UserPerm *perms);
int stone_ll_getattr(struct stone_mount_info *cmount, struct Inode *in,
		    struct stone_statx *stx, unsigned int want, unsigned int flags,
		    const UserPerm *perms);
int stone_ll_setattr(struct stone_mount_info *cmount, struct Inode *in,
		    struct stone_statx *stx, int mask, const UserPerm *perms);
int stone_ll_open(struct stone_mount_info *cmount, struct Inode *in, int flags,
		 struct Fh **fh, const UserPerm *perms);
off_t stone_ll_lseek(struct stone_mount_info *cmount, struct Fh* filehandle,
		     off_t offset, int whence);
int stone_ll_read(struct stone_mount_info *cmount, struct Fh* filehandle,
		 int64_t off, uint64_t len, char* buf);
int stone_ll_fsync(struct stone_mount_info *cmount, struct Fh *fh,
		  int syncdataonly);
int stone_ll_sync_inode(struct stone_mount_info *cmount, struct Inode *in,
		  int syncdataonly);
int stone_ll_fallocate(struct stone_mount_info *cmount, struct Fh *fh,
		      int mode, int64_t offset, int64_t length);
int stone_ll_write(struct stone_mount_info *cmount, struct Fh* filehandle,
		  int64_t off, uint64_t len, const char *data);
int64_t stone_ll_readv(struct stone_mount_info *cmount, struct Fh *fh,
		      const struct iovec *iov, int iovcnt, int64_t off);
int64_t stone_ll_writev(struct stone_mount_info *cmount, struct Fh *fh,
		       const struct iovec *iov, int iovcnt, int64_t off);
int stone_ll_close(struct stone_mount_info *cmount, struct Fh* filehandle);
int stone_ll_iclose(struct stone_mount_info *cmount, struct Inode *in, int mode);
/**
 * Get xattr value by xattr name.
 *
 * @param cmount the stone mount handle to use.
 * @param in file handle
 * @param name name of attribute
 * @param value pointer to begin buffer
 * @param size buffer size
 * @param perms pointer to UserPerms object
 * @returns size of returned buffer. Negative number in error case
 */
int stone_ll_getxattr(struct stone_mount_info *cmount, struct Inode *in,
		     const char *name, void *value, size_t size,
		     const UserPerm *perms);
int stone_ll_setxattr(struct stone_mount_info *cmount, struct Inode *in,
		     const char *name, const void *value, size_t size,
		     int flags, const UserPerm *perms);
int stone_ll_listxattr(struct stone_mount_info *cmount, struct Inode *in,
                      char *list, size_t buf_size, size_t *list_size,
		      const UserPerm *perms);
int stone_ll_removexattr(struct stone_mount_info *cmount, struct Inode *in,
			const char *name, const UserPerm *perms);
int stone_ll_create(struct stone_mount_info *cmount, Inode *parent,
		   const char *name, mode_t mode, int oflags, Inode **outp,
		   Fh **fhp, struct stone_statx *stx, unsigned want,
		   unsigned lflags, const UserPerm *perms);
int stone_ll_mknod(struct stone_mount_info *cmount, Inode *parent,
		  const char *name, mode_t mode, dev_t rdev, Inode **out,
		  struct stone_statx *stx, unsigned want, unsigned flags,
		  const UserPerm *perms);
int stone_ll_mkdir(struct stone_mount_info *cmount, Inode *parent,
		  const char *name, mode_t mode, Inode **out,
		  struct stone_statx *stx, unsigned want,
		  unsigned flags, const UserPerm *perms);
int stone_ll_link(struct stone_mount_info *cmount, struct Inode *in,
		 struct Inode *newparent, const char *name,
		 const UserPerm *perms);
int stone_ll_opendir(struct stone_mount_info *cmount, struct Inode *in,
		    struct stone_dir_result **dirpp, const UserPerm *perms);
int stone_ll_releasedir(struct stone_mount_info *cmount,
		       struct stone_dir_result* dir);
int stone_ll_rename(struct stone_mount_info *cmount, struct Inode *parent,
		   const char *name, struct Inode *newparent,
		   const char *newname, const UserPerm *perms);
int stone_ll_unlink(struct stone_mount_info *cmount, struct Inode *in,
		   const char *name, const UserPerm *perms);
int stone_ll_statfs(struct stone_mount_info *cmount, struct Inode *in,
		   struct statvfs *stbuf);
int stone_ll_readlink(struct stone_mount_info *cmount, struct Inode *in,
		     char *buf, size_t bufsize, const UserPerm *perms);
int stone_ll_symlink(struct stone_mount_info *cmount,
		    Inode *in, const char *name, const char *value,
		    Inode **out, struct stone_statx *stx,
		    unsigned want, unsigned flags,
		    const UserPerm *perms);
int stone_ll_rmdir(struct stone_mount_info *cmount, struct Inode *in,
		  const char *name, const UserPerm *perms);
uint32_t stone_ll_stripe_unit(struct stone_mount_info *cmount,
			     struct Inode *in);
uint32_t stone_ll_file_layout(struct stone_mount_info *cmount,
			     struct Inode *in,
			     struct stone_file_layout *layout);
uint64_t stone_ll_snap_seq(struct stone_mount_info *cmount,
			  struct Inode *in);
int stone_ll_get_stripe_osd(struct stone_mount_info *cmount,
			   struct Inode *in,
			   uint64_t blockno,
			   struct stone_file_layout* layout);
int stone_ll_num_osds(struct stone_mount_info *cmount);
int stone_ll_osdaddr(struct stone_mount_info *cmount,
		    int osd, uint32_t *addr);
uint64_t stone_ll_get_internal_offset(struct stone_mount_info *cmount,
				     struct Inode *in, uint64_t blockno);
int stone_ll_read_block(struct stone_mount_info *cmount,
		       struct Inode *in, uint64_t blockid,
		       char* bl, uint64_t offset, uint64_t length,
		       struct stone_file_layout* layout);
int stone_ll_write_block(struct stone_mount_info *cmount,
			struct Inode *in, uint64_t blockid,
			char* buf, uint64_t offset,
			uint64_t length, struct stone_file_layout* layout,
			uint64_t snapseq, uint32_t sync);
int stone_ll_commit_blocks(struct stone_mount_info *cmount,
			  struct Inode *in, uint64_t offset, uint64_t range);


int stone_ll_getlk(struct stone_mount_info *cmount,
		  Fh *fh, struct flock *fl, uint64_t owner);
int stone_ll_setlk(struct stone_mount_info *cmount,
		  Fh *fh, struct flock *fl, uint64_t owner, int sleep);

int stone_ll_lazyio(struct stone_mount_info *cmount, Fh *fh, int enable);

/*
 * Delegation support
 *
 * Delegations are way for an application to request exclusive or
 * semi-exclusive access to an Inode. The client requests the delegation and
 * if it's successful it can reliably cache file data and metadata until the
 * delegation is recalled.
 *
 * Recalls are issued via a callback function, provided by the application.
 * Callback functions should act something like signal handlers.  You want to
 * do as little as possible in the callback. Any major work should be deferred
 * in some fashion as it's difficult to predict the context in which this
 * function will be called.
 *
 * Once the delegation has been recalled, the application should return it as
 * soon as possible. The application has client_deleg_timeout seconds to
 * return it, after which the cmount structure is forcibly unmounted and
 * further calls into it fail.
 *
 * The application can set the client_deleg_timeout config option to suit its
 * needs, but it should take care to choose a value that allows it to avoid
 * forcible eviction from the cluster in the event of an application bug.
 */

/* Commands for manipulating delegation state */
#ifndef STONE_DELEGATION_NONE
# define STONE_DELEGATION_NONE	0
# define STONE_DELEGATION_RD	1
# define STONE_DELEGATION_WR	2
#endif

/**
 * Get the amount of time that the client has to return caps
 * @param cmount the stone mount handle to use.
 *
 * In the event that a client does not return its caps, the MDS may blocklist
 * it after this timeout. Applications should check this value and ensure
 * that they set the delegation timeout to a value lower than this.
 *
 * This call returns the cap return timeout (in seconds) for this cmount, or
 * zero if it's not mounted.
 */
uint32_t stone_get_cap_return_timeout(struct stone_mount_info *cmount);

/**
 * Set the delegation timeout for the mount (thereby enabling delegations)
 * @param cmount the stone mount handle to use.
 * @param timeout the delegation timeout (in seconds)
 *
 * Since the client could end up blocklisted if it doesn't return delegations
 * in time, we mandate that any application wanting to use delegations
 * explicitly set the timeout beforehand. Until this call is done on the
 * mount, attempts to set a delegation will return -ETIME.
 *
 * Once a delegation is recalled, if it is not returned in this amount of
 * time, the cmount will be forcibly unmounted and further access attempts
 * will fail (usually with -ENOTCONN errors).
 *
 * This value is further vetted against the cap return timeout, and this call
 * can fail with -EINVAL if the timeout value is too long. Delegations can be
 * disabled again by setting the timeout to 0.
 */
int stone_set_deleg_timeout(struct stone_mount_info *cmount, uint32_t timeout);

/**
 * Request a delegation on an open Fh
 * @param cmount the stone mount handle to use.
 * @param fh file handle
 * @param cmd STONE_DELEGATION_* command
 * @param cb callback function for recalling delegation
 * @param priv opaque token passed back during recalls
 *
 * Returns 0 if the delegation was granted, -EAGAIN if there was a conflict
 * and other error codes if there is a fatal error of some sort (e.g. -ENOMEM,
 * -ETIME)
 */
int stone_ll_delegation(struct stone_mount_info *cmount, Fh *fh,
		       unsigned int cmd, stone_deleg_cb_t cb, void *priv);

mode_t stone_umask(struct stone_mount_info *cmount, mode_t mode);

/* state reclaim */
#define STONE_RECLAIM_RESET 	1

/**
 * Set stone client uuid
 * @param cmount the stone mount handle to use.
 * @param uuid the uuid to set
 *
 * Must be called before mount.
 */
void stone_set_uuid(struct stone_mount_info *cmount, const char *uuid);

/**
 * Set stone client session timeout
 * @param cmount the stone mount handle to use.
 * @param timeout the timeout to set
 *
 * Must be called before mount.
 */
void stone_set_session_timeout(struct stone_mount_info *cmount, unsigned timeout);

/**
 * Start to reclaim states of other client
 * @param cmount the stone mount handle to use.
 * @param uuid uuid of client whose states need to be reclaimed
 * @param flags flags that control how states get reclaimed
 *
 * Returns 0 success, -EOPNOTSUPP if mds does not support the operation,
 * -ENOENT if STONE_RECLAIM_RESET is specified and there is no client
 * with the given uuid, -ENOTRECOVERABLE in all other error cases.
 */
int stone_start_reclaim(struct stone_mount_info *cmount,
		       const char *uuid, unsigned flags);

/**
 * finish reclaiming states of other client (
 * @param cmount the stone mount handle to use.
 */
void stone_finish_reclaim(struct stone_mount_info *cmount);

/**
 * Register a set of callbacks to be used with this cmount
 * @param cmount the stone mount handle on which the cb's should be registerd
 * @param args   callback arguments to register with the cmount
 *
 * Any fields set to NULL will be ignored. There currently is no way to
 * unregister these callbacks, so this is a one-way change.
 */
void stone_ll_register_callbacks(struct stone_mount_info *cmount,
				struct stone_client_callback_args *args);

/**
 * Get snapshot info
 *
 * @param cmount the stone mount handle to use for making the directory.
 * @param path the path of the snapshot.  This must be either an
 *        absolute path or a relative path off of the current working directory.
 * @returns 0 on success or a negative return code on error.
 */
int stone_get_snap_info(struct stone_mount_info *cmount,
                       const char *path, struct snap_info *snap_info);

/**
 * Free snapshot info buffers
 *
 * @param snap_info snapshot info struct (fetched via call to stone_get_snap_info()).
 */
void stone_free_snap_info_buffer(struct snap_info *snap_info);
#ifdef __cplusplus
}
#endif

#endif
