# vim: expandtab smarttab shiftwidth=4 softtabstop=4
from nose.tools import assert_raises, assert_equal, assert_not_equal, assert_greater, with_setup
import stonefs as libstonefs
import fcntl
import os
import random
import time
import stat
import uuid
from datetime import datetime

stonefs = None

def setup_module():
    global stonefs
    stonefs = libstonefs.LibStoneFS(conffile='')
    stonefs.mount()

def teardown_module():
    global stonefs
    stonefs.shutdown()

def setup_test():
    d = stonefs.opendir(b"/")
    dent = stonefs.readdir(d)
    while dent:
        if (dent.d_name not in [b".", b".."]):
            if dent.is_dir():
                stonefs.rmdir(b"/" + dent.d_name)
            else:
                stonefs.unlink(b"/" + dent.d_name)

        dent = stonefs.readdir(d)

    stonefs.closedir(d)

    stonefs.chdir(b"/")
    _, ret_buf = stonefs.listxattr("/")
    print(f'ret_buf={ret_buf}')
    xattrs = ret_buf.decode('utf-8').split('\x00')
    for xattr in xattrs[:-1]:
        stonefs.removexattr("/", xattr)

@with_setup(setup_test)
def test_conf_get():
    fsid = stonefs.conf_get("fsid")
    assert(len(fsid) > 0)

@with_setup(setup_test)
def test_version():
    stonefs.version()

@with_setup(setup_test)
def test_fstat():
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stat = stonefs.fstat(fd)
    assert(len(stat) == 13)
    stonefs.close(fd)

@with_setup(setup_test)
def test_statfs():
    stat = stonefs.statfs(b'/')
    assert(len(stat) == 11)

@with_setup(setup_test)
def test_statx():
    stat = stonefs.statx(b'/', libstonefs.STONE_STATX_MODE, 0)
    assert('mode' in stat.keys())
    stat = stonefs.statx(b'/', libstonefs.STONE_STATX_BTIME, 0)
    assert('btime' in stat.keys())
    
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    stonefs.close(fd)
    stonefs.symlink(b'file-1', b'file-2')
    stat = stonefs.statx(b'file-2', libstonefs.STONE_STATX_MODE | libstonefs.STONE_STATX_BTIME, libstonefs.AT_SYMLINK_NOFOLLOW)
    assert('mode' in stat.keys())
    assert('btime' in stat.keys())
    stonefs.unlink(b'file-2')
    stonefs.unlink(b'file-1')

@with_setup(setup_test)
def test_syncfs():
    stat = stonefs.sync_fs()

@with_setup(setup_test)
def test_fsync():
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stonefs.write(fd, b"asdf", 0)
    stat = stonefs.fsync(fd, 0)
    stonefs.write(fd, b"qwer", 0)
    stat = stonefs.fsync(fd, 1)
    stonefs.close(fd)
    #sync on non-existing fd (assume fd 12345 is not exists)
    assert_raises(libstonefs.Error, stonefs.fsync, 12345, 0)

@with_setup(setup_test)
def test_directory():
    stonefs.mkdir(b"/temp-directory", 0o755)
    stonefs.mkdirs(b"/temp-directory/foo/bar", 0o755)
    stonefs.chdir(b"/temp-directory")
    assert_equal(stonefs.getcwd(), b"/temp-directory")
    stonefs.rmdir(b"/temp-directory/foo/bar")
    stonefs.rmdir(b"/temp-directory/foo")
    stonefs.rmdir(b"/temp-directory")
    assert_raises(libstonefs.ObjectNotFound, stonefs.chdir, b"/temp-directory")

@with_setup(setup_test)
def test_walk_dir():
    stonefs.chdir(b"/")
    dirs = [b"dir-1", b"dir-2", b"dir-3"]
    for i in dirs:
        stonefs.mkdir(i, 0o755)
    handler = stonefs.opendir(b"/")
    d = stonefs.readdir(handler)
    dirs += [b".", b".."]
    while d:
        assert(d.d_name in dirs)
        dirs.remove(d.d_name)
        d = stonefs.readdir(handler)
    assert(len(dirs) == 0)
    dirs = [b"/dir-1", b"/dir-2", b"/dir-3"]
    for i in dirs:
        stonefs.rmdir(i)
    stonefs.closedir(handler)

@with_setup(setup_test)
def test_xattr():
    assert_raises(libstonefs.OperationNotSupported, stonefs.setxattr, "/", "key", b"value", 0)
    stonefs.setxattr("/", "user.key", b"value", 0)
    assert_equal(b"value", stonefs.getxattr("/", "user.key"))

    stonefs.setxattr("/", "user.big", b"x" * 300, 0)

    # Default size is 255, get ERANGE
    assert_raises(libstonefs.OutOfRange, stonefs.getxattr, "/", "user.big")

    # Pass explicit size, and we'll get the value
    assert_equal(300, len(stonefs.getxattr("/", "user.big", 300)))

    stonefs.removexattr("/", "user.key")
    # user.key is already removed
    assert_raises(libstonefs.NoData, stonefs.getxattr, "/", "user.key")

    # user.big is only listed
    ret_val, ret_buff = stonefs.listxattr("/")
    assert_equal(9, ret_val)
    assert_equal("user.big\x00", ret_buff.decode('utf-8'))

@with_setup(setup_test)
def test_stone_mirror_xattr():
    def gen_mirror_xattr():
        cluster_id = str(uuid.uuid4())
        fs_id = random.randint(1, 10)
        mirror_xattr = f'cluster_id={cluster_id} fs_id={fs_id}'
        return mirror_xattr.encode('utf-8')

    mirror_xattr_enc_1 = gen_mirror_xattr()

    # mirror xattr is only allowed on root
    stonefs.mkdir('/d0', 0o755)
    assert_raises(libstonefs.InvalidValue, stonefs.setxattr,
                  '/d0', 'stone.mirror.info', mirror_xattr_enc_1, os.XATTR_CREATE)
    stonefs.rmdir('/d0')

    stonefs.setxattr('/', 'stone.mirror.info', mirror_xattr_enc_1, os.XATTR_CREATE)
    assert_equal(mirror_xattr_enc_1, stonefs.getxattr('/', 'stone.mirror.info'))

    # setting again with XATTR_CREATE should fail
    assert_raises(libstonefs.ObjectExists, stonefs.setxattr,
                  '/', 'stone.mirror.info', mirror_xattr_enc_1, os.XATTR_CREATE)

    # stone.mirror.info should not show up in listing
    ret_val, _ = stonefs.listxattr("/")
    assert_equal(0, ret_val)

    mirror_xattr_enc_2 = gen_mirror_xattr()

    stonefs.setxattr('/', 'stone.mirror.info', mirror_xattr_enc_2, os.XATTR_REPLACE)
    assert_equal(mirror_xattr_enc_2, stonefs.getxattr('/', 'stone.mirror.info'))

    stonefs.removexattr('/', 'stone.mirror.info')
    # stone.mirror.info is already removed
    assert_raises(libstonefs.NoData, stonefs.getxattr, '/', 'stone.mirror.info')
    # removing again should throw error
    assert_raises(libstonefs.NoData, stonefs.removexattr, "/", "stone.mirror.info")

    # check mirror info xattr format
    assert_raises(libstonefs.InvalidValue, stonefs.setxattr, '/', 'stone.mirror.info', b"unknown", 0)

@with_setup(setup_test)
def test_fxattr():
    fd = stonefs.open(b'/file-fxattr', 'w', 0o755)
    assert_raises(libstonefs.OperationNotSupported, stonefs.fsetxattr, fd, "key", b"value", 0)
    assert_raises(TypeError, stonefs.fsetxattr, "fd", "user.key", b"value", 0)
    assert_raises(TypeError, stonefs.fsetxattr, fd, "user.key", "value", 0)
    assert_raises(TypeError, stonefs.fsetxattr, fd, "user.key", b"value", "0")
    stonefs.fsetxattr(fd, "user.key", b"value", 0)
    assert_equal(b"value", stonefs.fgetxattr(fd, "user.key"))

    stonefs.fsetxattr(fd, "user.big", b"x" * 300, 0)

    # Default size is 255, get ERANGE
    assert_raises(libstonefs.OutOfRange, stonefs.fgetxattr, fd, "user.big")

    # Pass explicit size, and we'll get the value
    assert_equal(300, len(stonefs.fgetxattr(fd, "user.big", 300)))

    stonefs.fremovexattr(fd, "user.key")
    # user.key is already removed
    assert_raises(libstonefs.NoData, stonefs.fgetxattr, fd, "user.key")

    # user.big is only listed
    ret_val, ret_buff = stonefs.flistxattr(fd)
    assert_equal(9, ret_val)
    assert_equal("user.big\x00", ret_buff.decode('utf-8'))
    stonefs.close(fd)
    stonefs.unlink(b'/file-fxattr')

@with_setup(setup_test)
def test_rename():
    stonefs.mkdir(b"/a", 0o755)
    stonefs.mkdir(b"/a/b", 0o755)
    stonefs.rename(b"/a", b"/b")
    stonefs.stat(b"/b/b")
    stonefs.rmdir(b"/b/b")
    stonefs.rmdir(b"/b")

@with_setup(setup_test)
def test_open():
    assert_raises(libstonefs.ObjectNotFound, stonefs.open, b'file-1', 'r')
    assert_raises(libstonefs.ObjectNotFound, stonefs.open, b'file-1', 'r+')
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stonefs.write(fd, b"asdf", 0)
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', 'r', 0o755)
    assert_equal(stonefs.read(fd, 0, 4), b"asdf")
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', 'r+', 0o755)
    stonefs.write(fd, b"zxcv", 4)
    assert_equal(stonefs.read(fd, 4, 8), b"zxcv")
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', 'w+', 0o755)
    assert_equal(stonefs.read(fd, 0, 4), b"")
    stonefs.write(fd, b"zxcv", 4)
    assert_equal(stonefs.read(fd, 4, 8), b"zxcv")
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', os.O_RDWR, 0o755)
    stonefs.write(fd, b"asdf", 0)
    assert_equal(stonefs.read(fd, 0, 4), b"asdf")
    stonefs.close(fd)
    assert_raises(libstonefs.OperationNotSupported, stonefs.open, b'file-1', 'a')
    stonefs.unlink(b'file-1')

@with_setup(setup_test)
def test_link():
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    stonefs.close(fd)
    stonefs.link(b'file-1', b'file-2')
    fd = stonefs.open(b'file-2', 'r', 0o755)
    assert_equal(stonefs.read(fd, 0, 4), b"1111")
    stonefs.close(fd)
    fd = stonefs.open(b'file-2', 'r+', 0o755)
    stonefs.write(fd, b"2222", 4)
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', 'r', 0o755)
    assert_equal(stonefs.read(fd, 0, 8), b"11112222")
    stonefs.close(fd)
    stonefs.unlink(b'file-2')

@with_setup(setup_test)
def test_symlink():
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    stonefs.close(fd)
    stonefs.symlink(b'file-1', b'file-2')
    fd = stonefs.open(b'file-2', 'r', 0o755)
    assert_equal(stonefs.read(fd, 0, 4), b"1111")
    stonefs.close(fd)
    fd = stonefs.open(b'file-2', 'r+', 0o755)
    stonefs.write(fd, b"2222", 4)
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', 'r', 0o755)
    assert_equal(stonefs.read(fd, 0, 8), b"11112222")
    stonefs.close(fd)
    stonefs.unlink(b'file-2')

@with_setup(setup_test)
def test_readlink():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    stonefs.close(fd)
    stonefs.symlink(b'/file-1', b'/file-2')
    d = stonefs.readlink(b"/file-2",100)
    assert_equal(d, b"/file-1")
    stonefs.unlink(b'/file-2')
    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_delete_cwd():
    assert_equal(b"/", stonefs.getcwd())

    stonefs.mkdir(b"/temp-directory", 0o755)
    stonefs.chdir(b"/temp-directory")
    stonefs.rmdir(b"/temp-directory")

    # getcwd gives you something stale here: it remembers the path string
    # even when things are unlinked.  It's up to the caller to find out
    # whether it really still exists
    assert_equal(b"/temp-directory", stonefs.getcwd())

@with_setup(setup_test)
def test_flock():
    fd = stonefs.open(b'file-1', 'w', 0o755)

    stonefs.flock(fd, fcntl.LOCK_EX, 123);
    fd2 = stonefs.open(b'file-1', 'w', 0o755)

    assert_raises(libstonefs.WouldBlock, stonefs.flock, fd2,
                  fcntl.LOCK_EX | fcntl.LOCK_NB, 456);
    stonefs.close(fd2)

    stonefs.close(fd)

@with_setup(setup_test)
def test_mount_unmount():
    test_directory()
    stonefs.unmount()
    stonefs.mount()
    test_open()

@with_setup(setup_test)
def test_lxattr():
    fd = stonefs.open(b'/file-lxattr', 'w', 0o755)
    stonefs.close(fd)
    stonefs.setxattr(b"/file-lxattr", "user.key", b"value", 0)
    stonefs.symlink(b"/file-lxattr", b"/file-sym-lxattr")
    assert_equal(b"value", stonefs.getxattr(b"/file-sym-lxattr", "user.key"))
    assert_raises(libstonefs.NoData, stonefs.lgetxattr, b"/file-sym-lxattr", "user.key")

    stonefs.lsetxattr(b"/file-sym-lxattr", "trusted.key-sym", b"value-sym", 0)
    assert_equal(b"value-sym", stonefs.lgetxattr(b"/file-sym-lxattr", "trusted.key-sym"))
    stonefs.lsetxattr(b"/file-sym-lxattr", "trusted.big", b"x" * 300, 0)

    # Default size is 255, get ERANGE
    assert_raises(libstonefs.OutOfRange, stonefs.lgetxattr, b"/file-sym-lxattr", "trusted.big")

    # Pass explicit size, and we'll get the value
    assert_equal(300, len(stonefs.lgetxattr(b"/file-sym-lxattr", "trusted.big", 300)))

    stonefs.lremovexattr(b"/file-sym-lxattr", "trusted.key-sym")
    # trusted.key-sym is already removed
    assert_raises(libstonefs.NoData, stonefs.lgetxattr, b"/file-sym-lxattr", "trusted.key-sym")

    # trusted.big is only listed
    ret_val, ret_buff = stonefs.llistxattr(b"/file-sym-lxattr")
    assert_equal(12, ret_val)
    assert_equal("trusted.big\x00", ret_buff.decode('utf-8'))
    stonefs.unlink(b'/file-lxattr')
    stonefs.unlink(b'/file-sym-lxattr')

@with_setup(setup_test)
def test_mount_root():
    stonefs.mkdir(b"/mount-directory", 0o755)
    stonefs.unmount()
    stonefs.mount(mount_root = b"/mount-directory")

    assert_raises(libstonefs.Error, stonefs.mount, mount_root = b"/nowhere")
    stonefs.unmount()
    stonefs.mount()

@with_setup(setup_test)
def test_utime():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)
    stonefs.close(fd)

    stx_pre = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    time.sleep(1)
    stonefs.utime(b'/file-1')

    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_greater(stx_post['atime'], stx_pre['atime'])
    assert_greater(stx_post['mtime'], stx_pre['mtime'])

    atime_pre = int(time.mktime(stx_pre['atime'].timetuple()))
    mtime_pre = int(time.mktime(stx_pre['mtime'].timetuple()))

    stonefs.utime(b'/file-1', (atime_pre, mtime_pre))
    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_equal(stx_post['atime'], stx_pre['atime'])
    assert_equal(stx_post['mtime'], stx_pre['mtime'])

    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_futime():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)

    stx_pre = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    time.sleep(1)
    stonefs.futime(fd)

    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_greater(stx_post['atime'], stx_pre['atime'])
    assert_greater(stx_post['mtime'], stx_pre['mtime'])

    atime_pre = int(time.mktime(stx_pre['atime'].timetuple()))
    mtime_pre = int(time.mktime(stx_pre['mtime'].timetuple()))

    stonefs.futime(fd, (atime_pre, mtime_pre))
    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_equal(stx_post['atime'], stx_pre['atime'])
    assert_equal(stx_post['mtime'], stx_pre['mtime'])

    stonefs.close(fd)
    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_utimes():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)
    stonefs.close(fd)

    stx_pre = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    time.sleep(1)
    stonefs.utimes(b'/file-1')

    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_greater(stx_post['atime'], stx_pre['atime'])
    assert_greater(stx_post['mtime'], stx_pre['mtime'])

    atime_pre = time.mktime(stx_pre['atime'].timetuple())
    mtime_pre = time.mktime(stx_pre['mtime'].timetuple())

    stonefs.utimes(b'/file-1', (atime_pre, mtime_pre))
    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_equal(stx_post['atime'], stx_pre['atime'])
    assert_equal(stx_post['mtime'], stx_pre['mtime'])

    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_lutimes():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)
    stonefs.close(fd)

    stonefs.symlink(b'/file-1', b'/file-2')

    stx_pre_t = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)
    stx_pre_s = stonefs.statx(b'/file-2', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, libstonefs.AT_SYMLINK_NOFOLLOW)

    time.sleep(1)
    stonefs.lutimes(b'/file-2')

    stx_post_t = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)
    stx_post_s = stonefs.statx(b'/file-2', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, libstonefs.AT_SYMLINK_NOFOLLOW)

    assert_equal(stx_post_t['atime'], stx_pre_t['atime'])
    assert_equal(stx_post_t['mtime'], stx_pre_t['mtime'])

    assert_greater(stx_post_s['atime'], stx_pre_s['atime'])
    assert_greater(stx_post_s['mtime'], stx_pre_s['mtime'])

    atime_pre = time.mktime(stx_pre_s['atime'].timetuple())
    mtime_pre = time.mktime(stx_pre_s['mtime'].timetuple())

    stonefs.lutimes(b'/file-2', (atime_pre, mtime_pre))
    stx_post_s = stonefs.statx(b'/file-2', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, libstonefs.AT_SYMLINK_NOFOLLOW)

    assert_equal(stx_post_s['atime'], stx_pre_s['atime'])
    assert_equal(stx_post_s['mtime'], stx_pre_s['mtime'])

    stonefs.unlink(b'/file-2')
    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_futimes():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)

    stx_pre = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    time.sleep(1)
    stonefs.futimes(fd)

    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_greater(stx_post['atime'], stx_pre['atime'])
    assert_greater(stx_post['mtime'], stx_pre['mtime'])

    atime_pre = time.mktime(stx_pre['atime'].timetuple())
    mtime_pre = time.mktime(stx_pre['mtime'].timetuple())

    stonefs.futimes(fd, (atime_pre, mtime_pre))
    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_equal(stx_post['atime'], stx_pre['atime'])
    assert_equal(stx_post['mtime'], stx_pre['mtime'])

    stonefs.close(fd)
    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_futimens():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)

    stx_pre = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    time.sleep(1)
    stonefs.futimens(fd)

    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_greater(stx_post['atime'], stx_pre['atime'])
    assert_greater(stx_post['mtime'], stx_pre['mtime'])

    atime_pre = time.mktime(stx_pre['atime'].timetuple())
    mtime_pre = time.mktime(stx_pre['mtime'].timetuple())

    stonefs.futimens(fd, (atime_pre, mtime_pre))
    stx_post = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_MTIME, 0)

    assert_equal(stx_post['atime'], stx_pre['atime'])
    assert_equal(stx_post['mtime'], stx_pre['mtime'])

    stonefs.close(fd)
    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_lchmod():
    fd = stonefs.open(b'/file-1', 'w', 0o755)
    stonefs.write(fd, b'0000', 0)
    stonefs.close(fd)

    stonefs.symlink(b'/file-1', b'/file-2')

    stx_pre_t = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_MODE, 0)
    stx_pre_s = stonefs.statx(b'/file-2', libstonefs.STONE_STATX_MODE, libstonefs.AT_SYMLINK_NOFOLLOW)

    time.sleep(1)
    stonefs.lchmod(b'/file-2', 0o400)

    stx_post_t = stonefs.statx(b'/file-1', libstonefs.STONE_STATX_MODE, 0)
    stx_post_s = stonefs.statx(b'/file-2', libstonefs.STONE_STATX_MODE, libstonefs.AT_SYMLINK_NOFOLLOW)

    assert_equal(stx_post_t['mode'], stx_pre_t['mode'])
    assert_not_equal(stx_post_s['mode'], stx_pre_s['mode'])
    stx_post_s_perm_bits = stx_post_s['mode'] & ~stat.S_IFMT(stx_post_s["mode"])
    assert_equal(stx_post_s_perm_bits, 0o400)

    stonefs.unlink(b'/file-2')
    stonefs.unlink(b'/file-1')

@with_setup(setup_test)
def test_fchmod():
    fd = stonefs.open(b'/file-fchmod', 'w', 0o655)
    st = stonefs.statx(b'/file-fchmod', libstonefs.STONE_STATX_MODE, 0)
    mode = st["mode"] | stat.S_IXUSR
    stonefs.fchmod(fd, mode)
    st = stonefs.statx(b'/file-fchmod', libstonefs.STONE_STATX_MODE, 0)
    assert_equal(st["mode"] & stat.S_IRWXU, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)
    assert_raises(TypeError, stonefs.fchmod, "/file-fchmod", stat.S_IXUSR)
    assert_raises(TypeError, stonefs.fchmod, fd, "stat.S_IXUSR")
    stonefs.close(fd)
    stonefs.unlink(b'/file-fchmod')

@with_setup(setup_test)
def test_fchown():
    fd = stonefs.open(b'/file-fchown', 'w', 0o655)
    uid = os.getuid()
    gid = os.getgid()
    assert_raises(TypeError, stonefs.fchown, b'/file-fchown', uid, gid)
    assert_raises(TypeError, stonefs.fchown, fd, "uid", "gid")
    stonefs.fchown(fd, uid, gid)
    st = stonefs.statx(b'/file-fchown', libstonefs.STONE_STATX_UID | libstonefs.STONE_STATX_GID, 0)
    assert_equal(st["uid"], uid)
    assert_equal(st["gid"], gid)
    stonefs.fchown(fd, 9999, 9999)
    st = stonefs.statx(b'/file-fchown', libstonefs.STONE_STATX_UID | libstonefs.STONE_STATX_GID, 0)
    assert_equal(st["uid"], 9999)
    assert_equal(st["gid"], 9999)
    stonefs.close(fd)
    stonefs.unlink(b'/file-fchown')

@with_setup(setup_test)
def test_truncate():
    fd = stonefs.open(b'/file-truncate', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    stonefs.truncate(b'/file-truncate', 0)
    stat = stonefs.fsync(fd, 0)
    st = stonefs.statx(b'/file-truncate', libstonefs.STONE_STATX_SIZE, 0)
    assert_equal(st["size"], 0)
    stonefs.close(fd)
    stonefs.unlink(b'/file-truncate')

@with_setup(setup_test)
def test_ftruncate():
    fd = stonefs.open(b'/file-ftruncate', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    assert_raises(TypeError, stonefs.ftruncate, b'/file-ftruncate', 0)
    stonefs.ftruncate(fd, 0)
    stat = stonefs.fsync(fd, 0)
    st = stonefs.fstat(fd)
    assert_equal(st.st_size, 0)
    stonefs.close(fd)
    stonefs.unlink(b'/file-ftruncate')

@with_setup(setup_test)
def test_fallocate():
    fd = stonefs.open(b'/file-fallocate', 'w', 0o755)
    assert_raises(TypeError, stonefs.fallocate, b'/file-fallocate', 0, 10)
    stonefs.fallocate(fd, 0, 10)
    stat = stonefs.fsync(fd, 0)
    st = stonefs.fstat(fd)
    assert_equal(st.st_size, 10)
    stonefs.close(fd)
    stonefs.unlink(b'/file-fallocate')

@with_setup(setup_test)
def test_mknod():
    mode = stat.S_IFIFO | stat.S_IRUSR | stat.S_IWUSR
    stonefs.mknod(b'/file-fifo', mode)
    st = stonefs.statx(b'/file-fifo', libstonefs.STONE_STATX_MODE, 0)
    assert_equal(st["mode"] & mode, mode)
    stonefs.unlink(b'/file-fifo')

@with_setup(setup_test)
def test_lazyio():
    fd = stonefs.open(b'/file-lazyio', 'w', 0o755)
    assert_raises(TypeError, stonefs.lazyio, "fd", 1)
    assert_raises(TypeError, stonefs.lazyio, fd, "1")
    stonefs.lazyio(fd, 1)
    stonefs.write(fd, b"1111", 0)
    assert_raises(TypeError, stonefs.lazyio_propagate, "fd", 0, 4)
    assert_raises(TypeError, stonefs.lazyio_propagate, fd, "0", 4)
    assert_raises(TypeError, stonefs.lazyio_propagate, fd, 0, "4")
    stonefs.lazyio_propagate(fd, 0, 4)
    st = stonefs.fstat(fd)
    assert_equal(st.st_size, 4)
    stonefs.write(fd, b"2222", 4)
    assert_raises(TypeError, stonefs.lazyio_synchronize, "fd", 0, 8)
    assert_raises(TypeError, stonefs.lazyio_synchronize, fd, "0", 8)
    assert_raises(TypeError, stonefs.lazyio_synchronize, fd, 0, "8")
    stonefs.lazyio_synchronize(fd, 0, 8)
    st = stonefs.fstat(fd)
    assert_equal(st.st_size, 8)
    stonefs.close(fd)
    stonefs.unlink(b'/file-lazyio')

@with_setup(setup_test)
def test_replication():
    fd = stonefs.open(b'/file-rep', 'w', 0o755)
    assert_raises(TypeError, stonefs.get_file_replication, "fd")
    l_dict = stonefs.get_layout(fd)
    assert('pool_name' in l_dict.keys())
    cnt = stonefs.get_file_replication(fd)
    get_rep_cnt_cmd = "stone osd pool get " + l_dict["pool_name"] + " size"
    s=os.popen(get_rep_cnt_cmd).read().strip('\n')
    size=int(s.split(" ")[-1])
    assert_equal(cnt, size)
    cnt = stonefs.get_path_replication(b'/file-rep')
    assert_equal(cnt, size)
    stonefs.close(fd)
    stonefs.unlink(b'/file-rep')

@with_setup(setup_test)
def test_caps():
    fd = stonefs.open(b'/file-caps', 'w', 0o755)
    timeout = stonefs.get_cap_return_timeout()
    assert_equal(timeout, 300)
    fd_caps = stonefs.debug_get_fd_caps(fd)
    file_caps = stonefs.debug_get_file_caps(b'/file-caps')
    assert_equal(fd_caps, file_caps)
    stonefs.close(fd)
    stonefs.unlink(b'/file-caps')

@with_setup(setup_test)
def test_setuuid():
    ses_id_uid = uuid.uuid1()
    ses_id_str = str(ses_id_uid)
    stonefs.set_uuid(ses_id_str)

@with_setup(setup_test)
def test_session_timeout():
    assert_raises(TypeError, stonefs.set_session_timeout, "300")
    stonefs.set_session_timeout(300)

@with_setup(setup_test)
def test_readdirops():
    stonefs.chdir(b"/")
    dirs = [b"dir-1", b"dir-2", b"dir-3"]
    for i in dirs:
        stonefs.mkdir(i, 0o755)
    handler = stonefs.opendir(b"/")
    d1 = stonefs.readdir(handler)
    d2 = stonefs.readdir(handler)
    d3 = stonefs.readdir(handler)
    offset_d4 = stonefs.telldir(handler)
    d4 = stonefs.readdir(handler)
    stonefs.rewinddir(handler)
    d = stonefs.readdir(handler)
    assert_equal(d.d_name, d1.d_name)
    stonefs.seekdir(handler, offset_d4)
    d = stonefs.readdir(handler)
    assert_equal(d.d_name, d4.d_name)
    dirs += [b".", b".."]
    stonefs.rewinddir(handler)
    d = stonefs.readdir(handler)
    while d:
        assert(d.d_name in dirs)
        dirs.remove(d.d_name)
        d = stonefs.readdir(handler)
    assert(len(dirs) == 0)
    dirs = [b"/dir-1", b"/dir-2", b"/dir-3"]
    for i in dirs:
        stonefs.rmdir(i)
    stonefs.closedir(handler)

def test_preadv_pwritev():
    fd = stonefs.open(b'file-1', 'w', 0o755)
    stonefs.pwritev(fd, [b"asdf", b"zxcvb"], 0)
    stonefs.close(fd)
    fd = stonefs.open(b'file-1', 'r', 0o755)
    buf = [bytearray(i) for i in [4, 5]]
    stonefs.preadv(fd, buf, 0)
    assert_equal([b"asdf", b"zxcvb"], list(buf))
    stonefs.close(fd)
    stonefs.unlink(b'file-1')

@with_setup(setup_test)
def test_setattrx():
    fd = stonefs.open(b'file-setattrx', 'w', 0o655)
    stonefs.write(fd, b"1111", 0)
    stonefs.close(fd)
    st = stonefs.statx(b'file-setattrx', libstonefs.STONE_STATX_MODE, 0)
    mode = st["mode"] | stat.S_IXUSR
    assert_raises(TypeError, stonefs.setattrx, b'file-setattrx', "dict", 0, 0)

    time.sleep(1)
    statx_dict = dict()
    statx_dict["mode"] = mode
    statx_dict["uid"] = 9999
    statx_dict["gid"] = 9999
    dt = datetime.now()
    statx_dict["mtime"] = dt
    statx_dict["atime"] = dt
    statx_dict["ctime"] = dt
    statx_dict["size"] = 10
    statx_dict["btime"] = dt
    stonefs.setattrx(b'file-setattrx', statx_dict, libstonefs.STONE_SETATTR_MODE | libstonefs.STONE_SETATTR_UID |
                                                  libstonefs.STONE_SETATTR_GID | libstonefs.STONE_SETATTR_MTIME |
                                                  libstonefs.STONE_SETATTR_ATIME | libstonefs.STONE_SETATTR_CTIME |
                                                  libstonefs.STONE_SETATTR_SIZE | libstonefs.STONE_SETATTR_BTIME, 0)
    st1 = stonefs.statx(b'file-setattrx', libstonefs.STONE_STATX_MODE | libstonefs.STONE_STATX_UID |
                                         libstonefs.STONE_STATX_GID | libstonefs.STONE_STATX_MTIME |
                                         libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_CTIME |
                                         libstonefs.STONE_STATX_SIZE | libstonefs.STONE_STATX_BTIME, 0)
    assert_equal(mode, st1["mode"])
    assert_equal(9999, st1["uid"])
    assert_equal(9999, st1["gid"])
    assert_equal(int(dt.timestamp()), int(st1["mtime"].timestamp()))
    assert_equal(int(dt.timestamp()), int(st1["atime"].timestamp()))
    assert_equal(int(dt.timestamp()), int(st1["ctime"].timestamp()))
    assert_equal(int(dt.timestamp()), int(st1["btime"].timestamp()))
    assert_equal(10, st1["size"])
    stonefs.unlink(b'file-setattrx')

@with_setup(setup_test)
def test_fsetattrx():
    fd = stonefs.open(b'file-fsetattrx', 'w', 0o655)
    stonefs.write(fd, b"1111", 0)
    st = stonefs.statx(b'file-fsetattrx', libstonefs.STONE_STATX_MODE, 0)
    mode = st["mode"] | stat.S_IXUSR
    assert_raises(TypeError, stonefs.fsetattrx, fd, "dict", 0, 0)

    time.sleep(1)
    statx_dict = dict()
    statx_dict["mode"] = mode
    statx_dict["uid"] = 9999
    statx_dict["gid"] = 9999
    dt = datetime.now()
    statx_dict["mtime"] = dt
    statx_dict["atime"] = dt
    statx_dict["ctime"] = dt
    statx_dict["size"] = 10
    statx_dict["btime"] = dt
    stonefs.fsetattrx(fd, statx_dict, libstonefs.STONE_SETATTR_MODE | libstonefs.STONE_SETATTR_UID |
                                                  libstonefs.STONE_SETATTR_GID | libstonefs.STONE_SETATTR_MTIME |
                                                  libstonefs.STONE_SETATTR_ATIME | libstonefs.STONE_SETATTR_CTIME |
                                                  libstonefs.STONE_SETATTR_SIZE | libstonefs.STONE_SETATTR_BTIME)
    st1 = stonefs.statx(b'file-fsetattrx', libstonefs.STONE_STATX_MODE | libstonefs.STONE_STATX_UID |
                                         libstonefs.STONE_STATX_GID | libstonefs.STONE_STATX_MTIME |
                                         libstonefs.STONE_STATX_ATIME | libstonefs.STONE_STATX_CTIME |
                                         libstonefs.STONE_STATX_SIZE | libstonefs.STONE_STATX_BTIME, 0)
    assert_equal(mode, st1["mode"])
    assert_equal(9999, st1["uid"])
    assert_equal(9999, st1["gid"])
    assert_equal(int(dt.timestamp()), int(st1["mtime"].timestamp()))
    assert_equal(int(dt.timestamp()), int(st1["atime"].timestamp()))
    assert_equal(int(dt.timestamp()), int(st1["ctime"].timestamp()))
    assert_equal(int(dt.timestamp()), int(st1["btime"].timestamp()))
    assert_equal(10, st1["size"])
    stonefs.close(fd)
    stonefs.unlink(b'file-fsetattrx')

@with_setup(setup_test)
def test_get_layout():
    fd = stonefs.open(b'file-get-layout', 'w', 0o755)
    stonefs.write(fd, b"1111", 0)
    assert_raises(TypeError, stonefs.get_layout, "fd")
    l_dict = stonefs.get_layout(fd)
    assert('stripe_unit' in l_dict.keys())
    assert('stripe_count' in l_dict.keys())
    assert('object_size' in l_dict.keys())
    assert('pool_id' in l_dict.keys())
    assert('pool_name' in l_dict.keys())

    stonefs.close(fd)
    stonefs.unlink(b'file-get-layout')

@with_setup(setup_test)
def test_get_default_pool():
    dp_dict = stonefs.get_default_pool()
    assert('pool_id' in dp_dict.keys())
    assert('pool_name' in dp_dict.keys())

@with_setup(setup_test)
def test_get_pool():
    dp_dict = stonefs.get_default_pool()
    assert('pool_id' in dp_dict.keys())
    assert('pool_name' in dp_dict.keys())
    assert_equal(stonefs.get_pool_id(dp_dict["pool_name"]), dp_dict["pool_id"])
    get_rep_cnt_cmd = "stone osd pool get " + dp_dict["pool_name"] + " size"
    s=os.popen(get_rep_cnt_cmd).read().strip('\n')
    size=int(s.split(" ")[-1])
    assert_equal(stonefs.get_pool_replication(dp_dict["pool_id"]), size)

@with_setup(setup_test)
def test_disk_quota_exceeeded_error():
    stonefs.mkdir("/dir-1", 0o755)
    stonefs.setxattr("/dir-1", "stone.quota.max_bytes", b"5", 0)
    fd = stonefs.open(b'/dir-1/file-1', 'w', 0o755)
    assert_raises(libstonefs.DiskQuotaExceeded, stonefs.write, fd, b"abcdeghiklmnopqrstuvwxyz", 0)
    stonefs.close(fd)
    stonefs.unlink(b"/dir-1/file-1")

@with_setup(setup_test)
def test_empty_snapshot_info():
    stonefs.mkdir("/dir-1", 0o755)

    # snap without metadata
    stonefs.mkdir("/dir-1/.snap/snap0", 0o755)
    snap_info = stonefs.snap_info("/dir-1/.snap/snap0")
    assert_equal(snap_info["metadata"], {})
    assert_greater(snap_info["id"], 0)
    stonefs.rmdir("/dir-1/.snap/snap0")

    # remove directory
    stonefs.rmdir("/dir-1")

@with_setup(setup_test)
def test_snapshot_info():
    stonefs.mkdir("/dir-1", 0o755)

    # snap with custom metadata
    md = {"foo": "bar", "zig": "zag", "abcdefg": "12345"}
    stonefs.mksnap("/dir-1", "snap0", 0o755, metadata=md)
    snap_info = stonefs.snap_info("/dir-1/.snap/snap0")
    assert_equal(snap_info["metadata"]["foo"], md["foo"])
    assert_equal(snap_info["metadata"]["zig"], md["zig"])
    assert_equal(snap_info["metadata"]["abcdefg"], md["abcdefg"])
    assert_greater(snap_info["id"], 0)
    stonefs.rmsnap("/dir-1", "snap0")

    # remove directory
    stonefs.rmdir("/dir-1")

@with_setup(setup_test)
def test_set_mount_timeout_post_mount():
    assert_raises(libstonefs.LibStoneFSStateError, stonefs.set_mount_timeout, 5)

@with_setup(setup_test)
def test_set_mount_timeout():
    stonefs.unmount()
    stonefs.set_mount_timeout(5)
    stonefs.mount()

@with_setup(setup_test)
def test_set_mount_timeout_lt0():
    stonefs.unmount()
    assert_raises(libstonefs.InvalidValue, stonefs.set_mount_timeout, -5)
    stonefs.mount()
