"""
Test for Stone clusters with multiple FSs.
"""
import logging

from os.path import join as os_path_join

# CapsHelper is subclassed from StoneFSTestCase
from tasks.stonefs.caps_helper import CapsHelper

from teuthology.orchestra.run import CommandFailedError


log = logging.getLogger(__name__)


class TestMultiFS(CapsHelper):
    client_id = 'testuser'
    client_name = 'client.' + client_id
    # one dedicated for each FS
    MDSS_REQUIRED = 2
    CLIENTS_REQUIRED = 2

    def setUp(self):
        super(TestMultiFS, self).setUp()

        # we might have it - the client - if the same cluster was used for a
        # different vstart_runner.py run.
        self.run_cluster_cmd(f'auth rm {self.client_name}')

        self.fs1 = self.fs
        # After Octopus is EOL, we can remove this setting:
        self.fs1.set_allow_multifs()
        self.fs2 = self.mds_cluster.newfs(name='stonefs2', create=True)

        # we'll reassign caps to client.1 so that it can operate with stonefs2
        self.run_cluster_cmd(f'auth caps client.{self.mount_b.client_id} mon '
                             f'"allow r" osd "allow rw '
                             f'pool={self.fs2.get_data_pool_name()}" mds allow')
        self.mount_b.remount(stonefs_name=self.fs2.name)


class TestMONCaps(TestMultiFS):

    def test_moncap_with_one_fs_names(self):
        moncap = f'allow r fsname={self.fs1.name}'
        keyring = self.setup_test_env(moncap)

        self.run_mon_cap_tests(moncap, keyring)

    def test_moncap_with_multiple_fs_names(self):
        moncap = (f'allow r fsname={self.fs1.name}, '
                  f'allow r fsname={self.fs2.name}')
        keyring = self.setup_test_env(moncap)

        self.run_mon_cap_tests(moncap, keyring)

    def test_moncap_with_blanket_allow(self):
        moncap = 'allow r'
        keyring = self.setup_test_env(moncap)

        self.run_mon_cap_tests(moncap, keyring)

    def setup_test_env(self, moncap):
        return self.create_client(self.client_id, moncap)


#TODO: add tests for capsecs 'p' and 's'.
class TestMDSCaps(TestMultiFS):
    """
    0. Have 2 FSs on Stone cluster.
    1. Create new files on both FSs.
    2. Create a new client that has authorization for both FSs.
    3. Remount the current mounts with this new client.
    4. Test read and write on both FSs.
    """
    def test_rw_with_fsname_and_no_path_in_cap(self):
        perm = 'rw'
        filepaths, filedata, mounts = self.setup_test_env(perm, True)

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    def test_r_with_fsname_and_no_path_in_cap(self):
        perm = 'r'
        filepaths, filedata, mounts = self.setup_test_env(perm, True)

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    def test_rw_with_fsname_and_path_in_cap(self):
        perm = 'rw'
        filepaths, filedata, mounts = self.setup_test_env(perm, True,'dir1')

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    def test_r_with_fsname_and_path_in_cap(self):
        perm = 'r'
        filepaths, filedata, mounts = self.setup_test_env(perm, True, 'dir1')

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    # XXX: this tests the backward compatibility; "allow rw path=<dir1>" is
    # treated as "allow rw fsname=* path=<dir1>"
    def test_rw_with_no_fsname_and_path_in_cap(self):
        perm = 'rw'
        filepaths, filedata, mounts = self.setup_test_env(perm, False, 'dir1')

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    # XXX: this tests the backward compatibility; "allow r path=<dir1>" is
    # treated as "allow r fsname=* path=<dir1>"
    def test_r_with_no_fsname_and_path_in_cap(self):
        perm = 'r'
        filepaths, filedata, mounts = self.setup_test_env(perm, False, 'dir1')

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    def test_rw_with_no_fsname_and_no_path(self):
        perm = 'rw'
        filepaths, filedata, mounts = self.setup_test_env(perm)

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    def test_r_with_no_fsname_and_no_path(self):
        perm = 'r'
        filepaths, filedata, mounts = self.setup_test_env(perm)

        self.run_mds_cap_tests(filepaths, filedata, mounts, perm)

    def tearDown(self):
        self.mount_a.umount_wait()
        self.mount_b.umount_wait()

        super(type(self), self).tearDown()

    def setup_test_env(self, perm, fsname=False, stonefs_mntpt='/'):
        """
        Creates the cap string, files on both the FSs and then creates the
        new client with the cap and remounts both the FSs with newly created
        client.
        """
        filenames = ('file_on_fs1', 'file_on_fs2')
        filedata = ('some data on first fs', 'some data on second fs')
        mounts = (self.mount_a, self.mount_b)
        self.setup_fs_contents(stonefs_mntpt, filenames, filedata)

        keyring_paths = self.create_client_and_keyring_file(perm, fsname,
                                                            stonefs_mntpt)
        filepaths = self.remount_with_new_client(stonefs_mntpt, filenames,
                                                 keyring_paths)

        return filepaths, filedata, mounts

    def generate_caps(self, perm, fsname, stonefs_mntpt):
        moncap = 'allow r'
        osdcap = (f'allow {perm} tag stonefs data={self.fs1.name}, '
                  f'allow {perm} tag stonefs data={self.fs2.name}')

        if fsname:
            if stonefs_mntpt == '/':
                mdscap = (f'allow {perm} fsname={self.fs1.name}, '
                          f'allow {perm} fsname={self.fs2.name}')
            else:
                mdscap = (f'allow {perm} fsname={self.fs1.name} '
                          f'path=/{stonefs_mntpt}, '
                          f'allow {perm} fsname={self.fs2.name} '
                          f'path=/{stonefs_mntpt}')
        else:
            if stonefs_mntpt == '/':
                mdscap = f'allow {perm}'
            else:
                mdscap = f'allow {perm} path=/{stonefs_mntpt}'

        return moncap, osdcap, mdscap

    def create_client_and_keyring_file(self, perm, fsname, stonefs_mntpt):
        moncap, osdcap, mdscap = self.generate_caps(perm, fsname,
                                                    stonefs_mntpt)

        keyring = self.create_client(self.client_id, moncap, osdcap, mdscap)
        keyring_paths = []
        for mount_x in (self.mount_a, self.mount_b):
            keyring_paths.append(self.create_keyring_file(
                mount_x.client_remote, keyring))

        return keyring_paths

    def setup_fs_contents(self, stonefs_mntpt, filenames, filedata):
        filepaths = []
        iter_on = zip((self.mount_a, self.mount_b), filenames, filedata)

        for mount_x, filename, data in iter_on:
            if stonefs_mntpt != '/' :
                mount_x.run_shell(args=['mkdir', stonefs_mntpt])
                filepaths.append(os_path_join(mount_x.hostfs_mntpt,
                                              stonefs_mntpt, filename))
            else:
                filepaths.append(os_path_join(mount_x.hostfs_mntpt, filename))

            mount_x.write_file(filepaths[-1], data)

    def remount_with_new_client(self, stonefs_mntpt, filenames,
                                           keyring_paths):
        if isinstance(stonefs_mntpt, str) and stonefs_mntpt != '/' :
            stonefs_mntpt = '/' + stonefs_mntpt

        self.mount_a.remount(client_id=self.client_id,
                             client_keyring_path=keyring_paths[0],
                             client_remote=self.mount_a.client_remote,
                             stonefs_name=self.fs1.name,
                             stonefs_mntpt=stonefs_mntpt,
                             hostfs_mntpt=self.mount_a.hostfs_mntpt,
                             wait=True)
        self.mount_b.remount(client_id=self.client_id,
                             client_keyring_path=keyring_paths[1],
                             client_remote=self.mount_b.client_remote,
                             stonefs_name=self.fs2.name,
                             stonefs_mntpt=stonefs_mntpt,
                             hostfs_mntpt=self.mount_b.hostfs_mntpt,
                             wait=True)

        return (os_path_join(self.mount_a.hostfs_mntpt, filenames[0]),
                os_path_join(self.mount_b.hostfs_mntpt, filenames[1]))


class TestClientsWithoutAuth(TestMultiFS):

    def setUp(self):
        super(TestClientsWithoutAuth, self).setUp()

        # TODO: When MON and OSD caps for a Stone FS are assigned to a
        # client but MDS caps are not, mount.stone prints "permission
        # denied". But when MON caps are not assigned and MDS and OSD
        # caps are, mount.stone prints "no mds server or cluster laggy"
        # instead of "permission denied".
        #
        # Before uncommenting the following line a fix would be required
        # for latter case to change "no mds server is up or the cluster is
        #  laggy" to "permission denied".
        self.kernel_errmsgs = ('permission denied', 'no mds server is up or '
                               'the cluster is laggy', 'no such file or '
                               'directory',
                               'input/output error')

        # TODO: When MON and OSD caps are assigned for a Stone FS to a
        # client but MDS caps are not, stone-fuse prints "operation not
        # permitted". But when MON caps are not assigned and MDS and OSD
        # caps are, stone-fuse prints "no such file or directory" instead
        # of "operation not permitted".
        #
        # Before uncommenting the following line a fix would be required
        # for the latter case to change "no such file or directory" to
        # "operation not permitted".
        #self.assertIn('operation not permitted', retval[2].lower())
        self.fuse_errmsgs = ('operation not permitted', 'no such file or '
                             'directory')

        if 'kernel' in str(type(self.mount_a)).lower():
            self.errmsgs = self.kernel_errmsgs
        elif 'fuse' in str(type(self.mount_a)).lower():
            self.errmsgs = self.fuse_errmsgs
        else:
            raise RuntimeError('strange, the client was neither based on '
                               'kernel nor FUSE.')

    def check_that_mount_failed_for_right_reason(self, stderr):
        stderr = stderr.lower()
        for errmsg in self.errmsgs:
            if errmsg in stderr:
                break
        else:
            raise AssertionError('can\'t find expected set of words in the '
                                 f'stderr\nself.errmsgs - {self.errmsgs}\n'
                                 f'stderr - {stderr}')

    def test_mount_all_caps_absent(self):
        # setup part...
        keyring = self.fs1.authorize(self.client_id, ('/', 'rw'))
        keyring_path = self.create_keyring_file(self.mount_a.client_remote,
                                                keyring)

        # mount the FS for which client has no auth...
        retval = self.mount_a.remount(client_id=self.client_id,
                                      client_keyring_path=keyring_path,
                                      stonefs_name=self.fs2.name,
                                      check_status=False)

        # tests...
        self.assertIsInstance(retval, tuple)
        self.assertEqual(len(retval), 3)
        self.assertIsInstance(retval[0], CommandFailedError)
        self.check_that_mount_failed_for_right_reason(retval[2])

    def test_mount_mon_and_osd_caps_present_mds_caps_absent(self):
        # setup part...
        moncap = f'allow rw fsname={self.fs1.name}, allow rw fsname={self.fs2.name}'
        mdscap = f'allow rw fsname={self.fs1.name}'
        osdcap = (f'allow rw tag stonefs data={self.fs1.name}, allow rw tag '
                  f'stonefs data={self.fs2.name}')
        keyring = self.create_client(self.client_id, moncap, osdcap, mdscap)
        keyring_path = self.create_keyring_file(self.mount_a.client_remote,
                                                keyring)

        # mount the FS for which client has no auth...
        retval = self.mount_a.remount(client_id=self.client_id,
                                      client_keyring_path=keyring_path,
                                      stonefs_name=self.fs2.name,
                                      check_status=False)

        # tests...
        self.assertIsInstance(retval, tuple)
        self.assertEqual(len(retval), 3)
        self.assertIsInstance(retval[0], CommandFailedError)
        self.check_that_mount_failed_for_right_reason(retval[2])
