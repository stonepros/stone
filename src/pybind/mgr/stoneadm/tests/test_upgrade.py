import json
from unittest import mock

import pytest

from stone.deployment.service_spec import PlacementSpec, ServiceSpec
from stoneadm import StoneadmOrchestrator
from stoneadm.upgrade import StoneadmUpgrade
from stoneadm.serve import StoneadmServe
from orchestrator import OrchestratorError, DaemonDescription
from .fixtures import _run_stoneadm, wait, with_host, with_service


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('{}'))
def test_upgrade_start(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'test'):
        with with_host(stoneadm_module, 'test2'):
            with with_service(stoneadm_module, ServiceSpec('mgr', placement=PlacementSpec(count=2)), status_running=True):
                assert wait(stoneadm_module, stoneadm_module.upgrade_start(
                    'image_id', None)) == 'Initiating upgrade to image_id'

                assert wait(stoneadm_module, stoneadm_module.upgrade_status()
                            ).target_image == 'image_id'

                assert wait(stoneadm_module, stoneadm_module.upgrade_pause()
                            ) == 'Paused upgrade to image_id'

                assert wait(stoneadm_module, stoneadm_module.upgrade_resume()
                            ) == 'Resumed upgrade to image_id'

                assert wait(stoneadm_module, stoneadm_module.upgrade_stop()
                            ) == 'Stopped upgrade to image_id'


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('{}'))
@pytest.mark.parametrize("use_repo_digest",
                         [
                             False,
                             True
                         ])
def test_upgrade_run(use_repo_digest, stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        with with_host(stoneadm_module, 'host2'):
            stoneadm_module.set_container_image('global', 'from_image')
            stoneadm_module.use_repo_digest = use_repo_digest
            with with_service(stoneadm_module, ServiceSpec('mgr', placement=PlacementSpec(host_pattern='*', count=2)),
                              StoneadmOrchestrator.apply_mgr, '', status_running=True),\
                mock.patch("stoneadm.module.StoneadmOrchestrator.lookup_release_name",
                           return_value='foo'),\
                mock.patch("stoneadm.module.StoneadmOrchestrator.version",
                           new_callable=mock.PropertyMock) as version_mock,\
                mock.patch("stoneadm.module.StoneadmOrchestrator.get",
                           return_value={
                               # capture fields in both mon and osd maps
                               "require_osd_release": "pacific",
                               "min_mon_release": 16,
                           }):
                version_mock.return_value = 'stone version 18.2.1 (somehash)'
                assert wait(stoneadm_module, stoneadm_module.upgrade_start(
                    'to_image', None)) == 'Initiating upgrade to to_image'

                assert wait(stoneadm_module, stoneadm_module.upgrade_status()
                            ).target_image == 'to_image'

                def _versions_mock(cmd):
                    return json.dumps({
                        'mgr': {
                            'stone version 1.2.3 (asdf) blah': 1
                        }
                    })

                stoneadm_module._mon_command_mock_versions = _versions_mock

                with mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm(json.dumps({
                    'image_id': 'image_id',
                    'repo_digests': ['to_image@repo_digest'],
                    'stone_version': 'stone version 18.2.3 (hash)',
                }))):

                    stoneadm_module.upgrade._do_upgrade()

                assert stoneadm_module.upgrade_status is not None

                with mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm(
                    json.dumps([
                        dict(
                            name=list(stoneadm_module.cache.daemons['host1'].keys())[0],
                            style='stoneadm',
                            fsid='fsid',
                            container_id='container_id',
                            container_image_id='image_id',
                            container_image_digests=['to_image@repo_digest'],
                            deployed_by=['to_image@repo_digest'],
                            version='version',
                            state='running',
                        )
                    ])
                )):
                    StoneadmServe(stoneadm_module)._refresh_hosts_and_daemons()

                with mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm(json.dumps({
                    'image_id': 'image_id',
                    'repo_digests': ['to_image@repo_digest'],
                    'stone_version': 'stone version 18.2.3 (hash)',
                }))):
                    stoneadm_module.upgrade._do_upgrade()

                _, image, _ = stoneadm_module.check_mon_command({
                    'prefix': 'config get',
                    'who': 'global',
                    'key': 'container_image',
                })
                if use_repo_digest:
                    assert image == 'to_image@repo_digest'
                else:
                    assert image == 'to_image'


def test_upgrade_state_null(stoneadm_module: StoneadmOrchestrator):
    # This test validates https://tracker.stone.com/issues/47580
    stoneadm_module.set_store('upgrade_state', 'null')
    StoneadmUpgrade(stoneadm_module)
    assert StoneadmUpgrade(stoneadm_module).upgrade_state is None


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('{}'))
def test_not_enough_mgrs(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        with with_service(stoneadm_module, ServiceSpec('mgr', placement=PlacementSpec(count=1)), StoneadmOrchestrator.apply_mgr, ''):
            with pytest.raises(OrchestratorError):
                wait(stoneadm_module, stoneadm_module.upgrade_start('image_id', None))


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('{}'))
@mock.patch("stoneadm.StoneadmOrchestrator.check_mon_command")
def test_enough_mons_for_ok_to_stop(check_mon_command, stoneadm_module: StoneadmOrchestrator):
    # only 2 monitors, not enough for ok-to-stop to ever pass
    check_mon_command.return_value = (
        0, '{"monmap": {"mons": [{"name": "mon.1"}, {"name": "mon.2"}]}}', '')
    assert not stoneadm_module.upgrade._enough_mons_for_ok_to_stop()

    # 3 monitors, ok-to-stop should work fine
    check_mon_command.return_value = (
        0, '{"monmap": {"mons": [{"name": "mon.1"}, {"name": "mon.2"}, {"name": "mon.3"}]}}', '')
    assert stoneadm_module.upgrade._enough_mons_for_ok_to_stop()


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('{}'))
@mock.patch("stoneadm.module.HostCache.get_daemons_by_service")
@mock.patch("stoneadm.StoneadmOrchestrator.get")
def test_enough_mds_for_ok_to_stop(get, get_daemons_by_service, stoneadm_module: StoneadmOrchestrator):
    get.side_effect = [{'filesystems': [{'mdsmap': {'fs_name': 'test', 'max_mds': 1}}]}]
    get_daemons_by_service.side_effect = [[DaemonDescription()]]
    assert not stoneadm_module.upgrade._enough_mds_for_ok_to_stop(
        DaemonDescription(daemon_type='mds', daemon_id='test.host1.gfknd', service_name='mds.test'))

    get.side_effect = [{'filesystems': [{'mdsmap': {'fs_name': 'myfs.test', 'max_mds': 2}}]}]
    get_daemons_by_service.side_effect = [[DaemonDescription(), DaemonDescription()]]
    assert not stoneadm_module.upgrade._enough_mds_for_ok_to_stop(
        DaemonDescription(daemon_type='mds', daemon_id='myfs.test.host1.gfknd', service_name='mds.myfs.test'))

    get.side_effect = [{'filesystems': [{'mdsmap': {'fs_name': 'myfs.test', 'max_mds': 1}}]}]
    get_daemons_by_service.side_effect = [[DaemonDescription(), DaemonDescription()]]
    assert stoneadm_module.upgrade._enough_mds_for_ok_to_stop(
        DaemonDescription(daemon_type='mds', daemon_id='myfs.test.host1.gfknd', service_name='mds.myfs.test'))
