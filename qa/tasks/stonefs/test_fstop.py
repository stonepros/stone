import logging

from tasks.stonefs.stonefs_test_case import StoneFSTestCase
from teuthology.exceptions import CommandFailedError

log = logging.getLogger(__name__)

class TestFSTop(StoneFSTestCase):
    def test_fstop_non_existent_cluster(self):
        self.mgr_cluster.mon_manager.raw_cluster_cmd("mgr", "module", "enable", "stats")
        try:
            self.mount_a.run_shell(['stonefs-top',
                                    '--cluster=hpec',
                                    '--id=admin',
                                    '--selftest'])
        except CommandFailedError:
            pass
        else:
            raise RuntimeError('expected stonefs-top command to fail.')
        self.mgr_cluster.mon_manager.raw_cluster_cmd("mgr", "module", "disable", "stats")

    def test_fstop(self):
        self.mgr_cluster.mon_manager.raw_cluster_cmd("mgr", "module", "enable", "stats")
        self.mount_a.run_shell(['stonefs-top',
                                '--id=admin',
                                '--selftest'])
        self.mgr_cluster.mon_manager.raw_cluster_cmd("mgr", "module", "disable", "stats")
