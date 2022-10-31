import json

from stone.deployment.service_spec import PlacementSpec, ServiceSpec, HostPlacementSpec
from stone.utils import datetime_to_str, datetime_now
from stoneadm import StoneadmOrchestrator
from stoneadm.inventory import SPEC_STORE_PREFIX
from stoneadm.migrations import LAST_MIGRATION
from stoneadm.tests.fixtures import _run_stoneadm, wait, with_host
from stoneadm.serve import StoneadmServe
from tests import mock


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_scheduler(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1', refresh_hosts=False):
        with with_host(stoneadm_module, 'host2', refresh_hosts=False):

            # emulate the old scheduler:
            c = stoneadm_module.apply_rgw(
                ServiceSpec('rgw', 'r.z', placement=PlacementSpec(host_pattern='*', count=2))
            )
            assert wait(stoneadm_module, c) == 'Scheduled rgw.r.z update...'

            # with pytest.raises(OrchestratorError, match="stoneadm migration still ongoing. Please wait, until the migration is complete."):
            StoneadmServe(stoneadm_module)._apply_all_services()

            stoneadm_module.migration_current = 0
            stoneadm_module.migration.migrate()
            # assert we need all daemons.
            assert stoneadm_module.migration_current == 0

            StoneadmServe(stoneadm_module)._refresh_hosts_and_daemons()
            stoneadm_module.migration.migrate()

            StoneadmServe(stoneadm_module)._apply_all_services()

            out = {o.hostname for o in wait(stoneadm_module, stoneadm_module.list_daemons())}
            assert out == {'host1', 'host2'}

            c = stoneadm_module.apply_rgw(
                ServiceSpec('rgw', 'r.z', placement=PlacementSpec(host_pattern='host1', count=2))
            )
            assert wait(stoneadm_module, c) == 'Scheduled rgw.r.z update...'

            # Sorry, for this hack, but I need to make sure, Migration thinks,
            # we have updated all daemons already.
            stoneadm_module.cache.last_daemon_update['host1'] = datetime_now()
            stoneadm_module.cache.last_daemon_update['host2'] = datetime_now()

            stoneadm_module.migration_current = 0
            stoneadm_module.migration.migrate()
            assert stoneadm_module.migration_current >= 2

            out = [o.spec.placement for o in wait(
                stoneadm_module, stoneadm_module.describe_service())]
            assert out == [PlacementSpec(count=2, hosts=[HostPlacementSpec(
                hostname='host1', network='', name=''), HostPlacementSpec(hostname='host2', network='', name='')])]


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_service_id_mon_one(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        stoneadm_module.set_store(SPEC_STORE_PREFIX + 'mon.wrong', json.dumps({
            'spec': {
                'service_type': 'mon',
                'service_id': 'wrong',
                'placement': {
                    'hosts': ['host1']
                }
            },
            'created': datetime_to_str(datetime_now()),
        }, sort_keys=True),
        )

        stoneadm_module.spec_store.load()

        assert len(stoneadm_module.spec_store.all_specs) == 1
        assert stoneadm_module.spec_store.all_specs['mon.wrong'].service_name() == 'mon'

        stoneadm_module.migration_current = 1
        stoneadm_module.migration.migrate()
        assert stoneadm_module.migration_current >= 2

        assert len(stoneadm_module.spec_store.all_specs) == 1
        assert stoneadm_module.spec_store.all_specs['mon'] == ServiceSpec(
            service_type='mon',
            unmanaged=True,
            placement=PlacementSpec(hosts=['host1'])
        )


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_service_id_mon_two(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        stoneadm_module.set_store(SPEC_STORE_PREFIX + 'mon', json.dumps({
            'spec': {
                'service_type': 'mon',
                'placement': {
                    'count': 5,
                }
            },
            'created': datetime_to_str(datetime_now()),
        }, sort_keys=True),
        )
        stoneadm_module.set_store(SPEC_STORE_PREFIX + 'mon.wrong', json.dumps({
            'spec': {
                'service_type': 'mon',
                'service_id': 'wrong',
                'placement': {
                    'hosts': ['host1']
                }
            },
            'created': datetime_to_str(datetime_now()),
        }, sort_keys=True),
        )

        stoneadm_module.spec_store.load()

        assert len(stoneadm_module.spec_store.all_specs) == 2
        assert stoneadm_module.spec_store.all_specs['mon.wrong'].service_name() == 'mon'
        assert stoneadm_module.spec_store.all_specs['mon'].service_name() == 'mon'

        stoneadm_module.migration_current = 1
        stoneadm_module.migration.migrate()
        assert stoneadm_module.migration_current >= 2

        assert len(stoneadm_module.spec_store.all_specs) == 1
        assert stoneadm_module.spec_store.all_specs['mon'] == ServiceSpec(
            service_type='mon',
            unmanaged=True,
            placement=PlacementSpec(count=5)
        )


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_service_id_mds_one(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        stoneadm_module.set_store(SPEC_STORE_PREFIX + 'mds', json.dumps({
            'spec': {
                'service_type': 'mds',
                'placement': {
                    'hosts': ['host1']
                }
            },
            'created': datetime_to_str(datetime_now()),
        }, sort_keys=True),
        )

        stoneadm_module.spec_store.load()

        # there is nothing to migrate, as the spec is gone now.
        assert len(stoneadm_module.spec_store.all_specs) == 0


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_nfs_initial(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        stoneadm_module.set_store(
            SPEC_STORE_PREFIX + 'mds',
            json.dumps({
                'spec': {
                    'service_type': 'nfs',
                    'service_id': 'foo',
                    'placement': {
                        'hosts': ['host1']
                    },
                    'spec': {
                        'pool': 'mypool',
                        'namespace': 'foons',
                    },
                },
                'created': datetime_to_str(datetime_now()),
            }, sort_keys=True),
        )
        stoneadm_module.migration_current = 1
        stoneadm_module.spec_store.load()

        ls = json.loads(stoneadm_module.get_store('nfs_migration_queue'))
        assert ls == [['foo', 'mypool', 'foons']]

        stoneadm_module.migration.migrate(True)
        assert stoneadm_module.migration_current == 2

        stoneadm_module.migration.migrate()
        assert stoneadm_module.migration_current == LAST_MIGRATION


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_nfs_initial_octopus(stoneadm_module: StoneadmOrchestrator):
    with with_host(stoneadm_module, 'host1'):
        stoneadm_module.set_store(
            SPEC_STORE_PREFIX + 'mds',
            json.dumps({
                'spec': {
                    'service_type': 'nfs',
                    'service_id': 'ganesha-foo',
                    'placement': {
                        'hosts': ['host1']
                    },
                    'spec': {
                        'pool': 'mypool',
                        'namespace': 'foons',
                    },
                },
                'created': datetime_to_str(datetime_now()),
            }, sort_keys=True),
        )
        stoneadm_module.migration_current = 1
        stoneadm_module.spec_store.load()

        ls = json.loads(stoneadm_module.get_store('nfs_migration_queue'))
        assert ls == [['ganesha-foo', 'mypool', 'foons']]

        stoneadm_module.migration.migrate(True)
        assert stoneadm_module.migration_current == 2

        stoneadm_module.migration.migrate()
        assert stoneadm_module.migration_current == LAST_MIGRATION


@mock.patch("stoneadm.serve.StoneadmServe._run_stoneadm", _run_stoneadm('[]'))
def test_migrate_admin_client_keyring(stoneadm_module: StoneadmOrchestrator):
    assert 'client.admin' not in stoneadm_module.keys.keys

    stoneadm_module.migration_current = 3
    stoneadm_module.migration.migrate()
    assert stoneadm_module.migration_current == LAST_MIGRATION

    assert stoneadm_module.keys.keys['client.admin'].placement.label == '_admin'
