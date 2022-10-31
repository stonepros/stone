from libc.stdint cimport *
from types cimport *

cdef extern from "stonefs/stone_ll_client.h":
    cdef struct statx "stone_statx":
        uint32_t    stx_mask
        uint32_t    stx_blksize
        uint32_t    stx_nlink
        uint32_t    stx_uid
        uint32_t    stx_gid
        uint16_t    stx_mode
        uint64_t    stx_ino
        uint64_t    stx_size
        uint64_t    stx_blocks
        uint64_t    stx_dev
        uint64_t    stx_rdev
        timespec    stx_atime
        timespec    stx_ctime
        timespec    stx_mtime
        timespec    stx_btime
        uint64_t    stx_version

cdef extern from "stonefs/libstonefs.h" nogil:
    cdef struct stone_mount_info:
        pass

    cdef struct stone_dir_result:
        pass

    cdef struct snap_metadata:
        const char *key
        const char *value

    cdef struct snap_info:
        uint64_t id
        size_t nr_snap_metadata
        snap_metadata *snap_metadata

    ctypedef void* rados_t

    const char *stone_version(int *major, int *minor, int *patch)

    int stone_create(stone_mount_info **cmount, const char * const id)
    int stone_create_from_rados(stone_mount_info **cmount, rados_t cluster)
    int stone_init(stone_mount_info *cmount)
    void stone_shutdown(stone_mount_info *cmount)

    int stone_getaddrs(stone_mount_info* cmount, char** addrs)
    int64_t stone_get_fs_cid(stone_mount_info *cmount)
    int stone_conf_read_file(stone_mount_info *cmount, const char *path_list)
    int stone_conf_parse_argv(stone_mount_info *cmount, int argc, const char **argv)
    int stone_conf_get(stone_mount_info *cmount, const char *option, char *buf, size_t len)
    int stone_conf_set(stone_mount_info *cmount, const char *option, const char *value)
    int stone_set_mount_timeout(stone_mount_info *cmount, uint32_t timeout)

    int stone_mount(stone_mount_info *cmount, const char *root)
    int stone_select_filesystem(stone_mount_info *cmount, const char *fs_name)
    int stone_unmount(stone_mount_info *cmount)
    int stone_abort_conn(stone_mount_info *cmount)
    uint64_t stone_get_instance_id(stone_mount_info *cmount)
    int stone_fstatx(stone_mount_info *cmount, int fd, statx *stx, unsigned want, unsigned flags)
    int stone_statx(stone_mount_info *cmount, const char *path, statx *stx, unsigned want, unsigned flags)
    int stone_statfs(stone_mount_info *cmount, const char *path, statvfs *stbuf)

    int stone_setattrx(stone_mount_info *cmount, const char *relpath, statx *stx, int mask, int flags)
    int stone_fsetattrx(stone_mount_info *cmount, int fd, statx *stx, int mask)
    int stone_mds_command(stone_mount_info *cmount, const char *mds_spec, const char **cmd, size_t cmdlen,
                         const char *inbuf, size_t inbuflen, char **outbuf, size_t *outbuflen,
                         char **outs, size_t *outslen)
    int stone_rename(stone_mount_info *cmount, const char *from_, const char *to)
    int stone_link(stone_mount_info *cmount, const char *existing, const char *newname)
    int stone_unlink(stone_mount_info *cmount, const char *path)
    int stone_symlink(stone_mount_info *cmount, const char *existing, const char *newname)
    int stone_readlink(stone_mount_info *cmount, const char *path, char *buf, int64_t size)
    int stone_setxattr(stone_mount_info *cmount, const char *path, const char *name,
                      const void *value, size_t size, int flags)
    int stone_fsetxattr(stone_mount_info *cmount, int fd, const char *name,
                       const void *value, size_t size, int flags)
    int stone_lsetxattr(stone_mount_info *cmount, const char *path, const char *name,
                       const void *value, size_t size, int flags)
    int stone_getxattr(stone_mount_info *cmount, const char *path, const char *name,
                      void *value, size_t size)
    int stone_fgetxattr(stone_mount_info *cmount, int fd, const char *name,
                       void *value, size_t size)
    int stone_lgetxattr(stone_mount_info *cmount, const char *path, const char *name,
                       void *value, size_t size)
    int stone_removexattr(stone_mount_info *cmount, const char *path, const char *name)
    int stone_fremovexattr(stone_mount_info *cmount, int fd, const char *name)
    int stone_lremovexattr(stone_mount_info *cmount, const char *path, const char *name)
    int stone_listxattr(stone_mount_info *cmount, const char *path, char *list, size_t size)
    int stone_flistxattr(stone_mount_info *cmount, int fd, char *list, size_t size)
    int stone_llistxattr(stone_mount_info *cmount, const char *path, char *list, size_t size)
    int stone_write(stone_mount_info *cmount, int fd, const char *buf, int64_t size, int64_t offset)
    int stone_pwritev(stone_mount_info *cmount, int fd, iovec *iov, int iovcnt, int64_t offset)
    int stone_read(stone_mount_info *cmount, int fd, char *buf, int64_t size, int64_t offset)
    int stone_preadv(stone_mount_info *cmount, int fd, iovec *iov, int iovcnt, int64_t offset)
    int stone_flock(stone_mount_info *cmount, int fd, int operation, uint64_t owner)
    int stone_mknod(stone_mount_info *cmount, const char *path, mode_t mode, dev_t rdev)
    int stone_close(stone_mount_info *cmount, int fd)
    int stone_open(stone_mount_info *cmount, const char *path, int flags, mode_t mode)
    int stone_mkdir(stone_mount_info *cmount, const char *path, mode_t mode)
    int stone_mksnap(stone_mount_info *cmount, const char *path, const char *name, mode_t mode, snap_metadata *snap_metadata, size_t nr_snap_metadata)
    int stone_rmsnap(stone_mount_info *cmount, const char *path, const char *name)
    int stone_get_snap_info(stone_mount_info *cmount, const char *path, snap_info *snap_info)
    void stone_free_snap_info_buffer(snap_info *snap_info)
    int stone_mkdirs(stone_mount_info *cmount, const char *path, mode_t mode)
    int stone_closedir(stone_mount_info *cmount, stone_dir_result *dirp)
    int stone_opendir(stone_mount_info *cmount, const char *name, stone_dir_result **dirpp)
    void stone_rewinddir(stone_mount_info *cmount, stone_dir_result *dirp)
    int64_t stone_telldir(stone_mount_info *cmount, stone_dir_result *dirp)
    void stone_seekdir(stone_mount_info *cmount, stone_dir_result *dirp, int64_t offset)
    int stone_chdir(stone_mount_info *cmount, const char *path)
    dirent * stone_readdir(stone_mount_info *cmount, stone_dir_result *dirp)
    int stone_rmdir(stone_mount_info *cmount, const char *path)
    const char* stone_getcwd(stone_mount_info *cmount)
    int stone_sync_fs(stone_mount_info *cmount)
    int stone_fsync(stone_mount_info *cmount, int fd, int syncdataonly)
    int stone_lazyio(stone_mount_info *cmount, int fd, int enable)
    int stone_lazyio_propagate(stone_mount_info *cmount, int fd, int64_t offset, size_t count)
    int stone_lazyio_synchronize(stone_mount_info *cmount, int fd, int64_t offset, size_t count)
    int stone_fallocate(stone_mount_info *cmount, int fd, int mode, int64_t offset, int64_t length)
    int stone_chmod(stone_mount_info *cmount, const char *path, mode_t mode)
    int stone_lchmod(stone_mount_info *cmount, const char *path, mode_t mode)
    int stone_fchmod(stone_mount_info *cmount, int fd, mode_t mode)
    int stone_chown(stone_mount_info *cmount, const char *path, int uid, int gid)
    int stone_lchown(stone_mount_info *cmount, const char *path, int uid, int gid)
    int stone_fchown(stone_mount_info *cmount, int fd, int uid, int gid)
    int64_t stone_lseek(stone_mount_info *cmount, int fd, int64_t offset, int whence)
    void stone_buffer_free(char *buf)
    mode_t stone_umask(stone_mount_info *cmount, mode_t mode)
    int stone_utime(stone_mount_info *cmount, const char *path, utimbuf *buf)
    int stone_futime(stone_mount_info *cmount, int fd, utimbuf *buf)
    int stone_utimes(stone_mount_info *cmount, const char *path, timeval times[2])
    int stone_lutimes(stone_mount_info *cmount, const char *path, timeval times[2])
    int stone_futimes(stone_mount_info *cmount, int fd, timeval times[2])
    int stone_futimens(stone_mount_info *cmount, int fd, timespec times[2])
    int stone_get_file_replication(stone_mount_info *cmount, int fh)
    int stone_get_path_replication(stone_mount_info *cmount, const char *path)
    int stone_get_pool_id(stone_mount_info *cmount, const char *pool_name)
    int stone_get_pool_replication(stone_mount_info *cmount, int pool_id)
    int stone_debug_get_fd_caps(stone_mount_info *cmount, int fd)
    int stone_debug_get_file_caps(stone_mount_info *cmount, const char *path)
    uint32_t stone_get_cap_return_timeout(stone_mount_info *cmount)
    void stone_set_uuid(stone_mount_info *cmount, const char *uuid)
    void stone_set_session_timeout(stone_mount_info *cmount, unsigned timeout)
    int stone_get_file_layout(stone_mount_info *cmount, int fh, int *stripe_unit, int *stripe_count, int *object_size, int *pg_pool)
    int stone_get_file_pool_name(stone_mount_info *cmount, int fh, char *buf, size_t buflen)
    int stone_get_default_data_pool_name(stone_mount_info *cmount, char *buf, size_t buflen)
