import logging

from io import BytesIO
from tasks.stonefs.xfstests_dev import XFSTestsDev

log = logging.getLogger(__name__)

class TestACLs(XFSTestsDev):

    def test_acls(self):
        from tasks.stonefs.fuse_mount import FuseMount
        from tasks.stonefs.kernel_mount import KernelMount

        # TODO: make xfstests-dev compatible with stone-fuse. xfstests-dev
        # remounts StoneFS before running tests using kernel, so stone-fuse
        # mounts are never actually testsed.
        if isinstance(self.mount_a, FuseMount):
            log.info('client is fuse mounted')
            self.skipTest('Requires kernel client; xfstests-dev not '\
                          'compatible with stone-fuse ATM.')
        elif isinstance(self.mount_a, KernelMount):
            log.info('client is kernel mounted')

        self.mount_a.client_remote.run(args=['sudo', './check',
            'generic/099'], cwd=self.repo_path, stdout=BytesIO(),
            stderr=BytesIO(), timeout=30, check_status=True,
            label='running tests for ACLs from xfstests-dev')
