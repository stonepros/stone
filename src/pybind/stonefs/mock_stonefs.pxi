# cython: embedsignature=True

from libc.stdint cimport *
from types cimport timespec


cdef:
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

cdef nogil:
    cdef struct stone_mount_info:
        int dummy

    cdef struct stone_dir_result:
        int dummy

    cdef struct snap_metadata:
        const char *key
        const char *value

    cdef struct snap_info:
        uint64_t id
        size_t nr_snap_metadata
        snap_metadata *snap_metadata

    ctypedef void* rados_t

    const char *stone_version(int *major, int *minor, int *patch):
        pass

    int stone_create(stone_mount_info **cmount, const char * const id):
        pass
    int stone_create_from_rados(stone_mount_info **cmount, rados_t cluster):
        pass
    int stone_init(stone_mount_info *cmount):
        pass
    void stone_shutdown(stone_mount_info *cmount):
        pass

    int stone_getaddrs(stone_mount_info* cmount, char** addrs):
        pass
    int64_t stone_get_fs_cid(stone_mount_info *cmount):
        pass
    int stone_conf_read_file(stone_mount_info *cmount, const char *path_list):
        pass
    int stone_conf_parse_argv(stone_mount_info *cmount, int argc, const char **argv):
        pass
    int stone_conf_get(stone_mount_info *cmount, const char *option, char *buf, size_t len):
        pass
    int stone_conf_set(stone_mount_info *cmount, const char *option, const char *value):
        pass
    int stone_set_mount_timeout(stone_mount_info *cmount, uint32_t timeout):
        pass

    int stone_mount(stone_mount_info *cmount, const char *root):
        pass
    int stone_select_filesystem(stone_mount_info *cmount, const char *fs_name):
        pass
    int stone_unmount(stone_mount_info *cmount):
        pass
    int stone_abort_conn(stone_mount_info *cmount):
        pass
    uint64_t stone_get_instance_id(stone_mount_info *cmount):
        pass
    int stone_fstatx(stone_mount_info *cmount, int fd, statx *stx, unsigned want, unsigned flags):
        pass
    int stone_statx(stone_mount_info *cmount, const char *path, statx *stx, unsigned want, unsigned flags):
        pass
    int stone_statfs(stone_mount_info *cmount, const char *path, statvfs *stbuf):
        pass

    int stone_setattrx(stone_mount_info *cmount, const char *relpath, statx *stx, int mask, int flags):
        pass
    int stone_fsetattrx(stone_mount_info *cmount, int fd, statx *stx, int mask):
        pass
    int stone_mds_command(stone_mount_info *cmount, const char *mds_spec, const char **cmd, size_t cmdlen,
                         const char *inbuf, size_t inbuflen, char **outbuf, size_t *outbuflen,
                         char **outs, size_t *outslen):
        pass
    int stone_rename(stone_mount_info *cmount, const char *from_, const char *to):
        pass
    int stone_link(stone_mount_info *cmount, const char *existing, const char *newname):
        pass
    int stone_unlink(stone_mount_info *cmount, const char *path):
        pass
    int stone_symlink(stone_mount_info *cmount, const char *existing, const char *newname):
        pass
    int stone_readlink(stone_mount_info *cmount, const char *path, char *buf, int64_t size):
        pass
    int stone_setxattr(stone_mount_info *cmount, const char *path, const char *name,
                      const void *value, size_t size, int flags):
        pass
    int stone_fsetxattr(stone_mount_info *cmount, int fd, const char *name,
                       const void *value, size_t size, int flags):
        pass
    int stone_lsetxattr(stone_mount_info *cmount, const char *path, const char *name,
                       const void *value, size_t size, int flags):
        pass
    int stone_getxattr(stone_mount_info *cmount, const char *path, const char *name,
                      void *value, size_t size):
        pass
    int stone_fgetxattr(stone_mount_info *cmount, int fd, const char *name,
                       void *value, size_t size):
        pass
    int stone_lgetxattr(stone_mount_info *cmount, const char *path, const char *name,
                       void *value, size_t size):
        pass
    int stone_removexattr(stone_mount_info *cmount, const char *path, const char *name):
        pass
    int stone_fremovexattr(stone_mount_info *cmount, int fd, const char *name):
        pass
    int stone_lremovexattr(stone_mount_info *cmount, const char *path, const char *name):
        pass
    int stone_listxattr(stone_mount_info *cmount, const char *path, char *list, size_t size):
        pass
    int stone_flistxattr(stone_mount_info *cmount, int fd, char *list, size_t size):
        pass
    int stone_llistxattr(stone_mount_info *cmount, const char *path, char *list, size_t size):
        pass
    int stone_write(stone_mount_info *cmount, int fd, const char *buf, int64_t size, int64_t offset):
        pass
    int stone_pwritev(stone_mount_info *cmount, int fd, iovec *iov, int iovcnt, int64_t offset):
        pass
    int stone_read(stone_mount_info *cmount, int fd, char *buf, int64_t size, int64_t offset):
        pass
    int stone_preadv(stone_mount_info *cmount, int fd, iovec *iov, int iovcnt, int64_t offset):
        pass
    int stone_flock(stone_mount_info *cmount, int fd, int operation, uint64_t owner):
        pass
    int stone_mknod(stone_mount_info *cmount, const char *path, mode_t mode, dev_t rdev):
        pass
    int stone_close(stone_mount_info *cmount, int fd):
        pass
    int stone_open(stone_mount_info *cmount, const char *path, int flags, mode_t mode):
        pass
    int stone_mkdir(stone_mount_info *cmount, const char *path, mode_t mode):
        pass
    int stone_mksnap(stone_mount_info *cmount, const char *path, const char *name, mode_t mode, snap_metadata *snap_metadata, size_t nr_snap_metadata):
        pass
    int stone_rmsnap(stone_mount_info *cmount, const char *path, const char *name):
        pass
    int stone_get_snap_info(stone_mount_info *cmount, const char *path, snap_info *snap_info):
        pass
    void stone_free_snap_info_buffer(snap_info *snap_info):
        pass
    int stone_mkdirs(stone_mount_info *cmount, const char *path, mode_t mode):
        pass
    int stone_closedir(stone_mount_info *cmount, stone_dir_result *dirp):
        pass
    int stone_opendir(stone_mount_info *cmount, const char *name, stone_dir_result **dirpp):
        pass
    void stone_rewinddir(stone_mount_info *cmount, stone_dir_result *dirp):
        pass
    int64_t stone_telldir(stone_mount_info *cmount, stone_dir_result *dirp):
        pass
    void stone_seekdir(stone_mount_info *cmount, stone_dir_result *dirp, int64_t offset):
        pass
    int stone_chdir(stone_mount_info *cmount, const char *path):
        pass
    dirent * stone_readdir(stone_mount_info *cmount, stone_dir_result *dirp):
        pass
    int stone_rmdir(stone_mount_info *cmount, const char *path):
        pass
    const char* stone_getcwd(stone_mount_info *cmount):
        pass
    int stone_sync_fs(stone_mount_info *cmount):
        pass
    int stone_fsync(stone_mount_info *cmount, int fd, int syncdataonly):
        pass
    int stone_lazyio(stone_mount_info *cmount, int fd, int enable):
        pass
    int stone_lazyio_propagate(stone_mount_info *cmount, int fd, int64_t offset, size_t count):
        pass
    int stone_lazyio_synchronize(stone_mount_info *cmount, int fd, int64_t offset, size_t count):
        pass
    int stone_fallocate(stone_mount_info *cmount, int fd, int mode, int64_t offset, int64_t length):
        pass
    int stone_chmod(stone_mount_info *cmount, const char *path, mode_t mode):
        pass
    int stone_lchmod(stone_mount_info *cmount, const char *path, mode_t mode):
        pass
    int stone_fchmod(stone_mount_info *cmount, int fd, mode_t mode):
        pass
    int stone_chown(stone_mount_info *cmount, const char *path, int uid, int gid):
        pass
    int stone_lchown(stone_mount_info *cmount, const char *path, int uid, int gid):
        pass
    int stone_fchown(stone_mount_info *cmount, int fd, int uid, int gid):
        pass
    int64_t stone_lseek(stone_mount_info *cmount, int fd, int64_t offset, int whence):
        pass
    void stone_buffer_free(char *buf):
        pass
    mode_t stone_umask(stone_mount_info *cmount, mode_t mode):
        pass
    int stone_utime(stone_mount_info *cmount, const char *path, utimbuf *buf):
        pass
    int stone_futime(stone_mount_info *cmount, int fd, utimbuf *buf):
        pass
    int stone_utimes(stone_mount_info *cmount, const char *path, timeval times[2]):
        pass
    int stone_lutimes(stone_mount_info *cmount, const char *path, timeval times[2]):
        pass
    int stone_futimes(stone_mount_info *cmount, int fd, timeval times[2]):
        pass
    int stone_futimens(stone_mount_info *cmount, int fd, timespec times[2]):
        pass
    int stone_get_file_replication(stone_mount_info *cmount, int fh):
        pass
    int stone_get_path_replication(stone_mount_info *cmount, const char *path):
        pass
    int stone_get_pool_id(stone_mount_info *cmount, const char *pool_name):
        pass
    int stone_get_pool_replication(stone_mount_info *cmount, int pool_id):
        pass
    int stone_debug_get_fd_caps(stone_mount_info *cmount, int fd):
        pass
    int stone_debug_get_file_caps(stone_mount_info *cmount, const char *path):
        pass
    uint32_t stone_get_cap_return_timeout(stone_mount_info *cmount):
        pass
    void stone_set_uuid(stone_mount_info *cmount, const char *uuid):
        pass
    void stone_set_session_timeout(stone_mount_info *cmount, unsigned timeout):
        pass
    int stone_get_file_layout(stone_mount_info *cmount, int fh, int *stripe_unit, int *stripe_count, int *object_size, int *pg_pool):
        pass
    int stone_get_file_pool_name(stone_mount_info *cmount, int fh, char *buf, size_t buflen):
        pass
    int stone_get_default_data_pool_name(stone_mount_info *cmount, char *buf, size_t buflen):
        pass
