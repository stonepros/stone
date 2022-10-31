import fnmatch
from contextlib import contextmanager

from stone.deployment.service_spec import PlacementSpec, ServiceSpec
from stone.utils import datetime_to_str, datetime_now
from stoneadm.serve import StoneadmServe

try:
    from typing import Any, Iterator, List
except ImportError:
    pass

from stoneadm import StoneadmOrchestrator
from orchestrator import raise_if_exception, OrchResult, HostSpec, DaemonDescriptionStatus
from tests import mock


def get_stone_option(_, key):
    return __file__


def get_module_option_ex(_, module, key, default=None):
    if module == 'prometheus':
        if key == 'server_port':
            return 9283
    return None


def _run_stoneadm(ret):
    def foo(s, host, entity, cmd, e, **kwargs):
        if cmd == 'gather-facts':
            return '{}', '', 0
        return [ret], '', 0
    return foo


def match_glob(val, pat):
    ok = fnmatch.fnmatchcase(val, pat)
    if not ok:
        assert pat in val


@contextmanager
def with_stoneadm_module(module_options=None, store=None):
    """
    :param module_options: Set opts as if they were set before module.__init__ is called
    :param store: Set the store before module.__init__ is called
    """
    with mock.patch("stoneadm.module.StoneadmOrchestrator.get_stone_option", get_stone_option),\
            mock.patch("stoneadm.services.osd.RemoveUtil._run_mon_cmd"), \
            mock.patch('stoneadm.module.StoneadmOrchestrator.get_module_option_ex', get_module_option_ex),\
            mock.patch("stoneadm.module.StoneadmOrchestrator.get_osdmap"), \
            mock.patch("stoneadm.module.StoneadmOrchestrator.remote"), \
            mock.patch('stoneadm.offline_watcher.OfflineHostWatcher.run'):

        m = StoneadmOrchestrator.__new__(StoneadmOrchestrator)
        if module_options is not None:
            for k, v in module_options.items():
                m._stone_set_module_option('stoneadm', k, v)
        if store is None:
            store = {}
        if '_stone_get/mon_map' not in store:
            m.mock_store_set('_stone_get', 'mon_map', {
                'modified': datetime_to_str(datetime_now()),
                'fsid': 'foobar',
            })
        if '_stone_get/mgr_map' not in store:
            m.mock_store_set('_stone_get', 'mgr_map', {
                'services': {
                    'dashboard': 'http://[::1]:8080',
                    'prometheus': 'http://[::1]:8081'
                },
                'modules': ['dashboard', 'prometheus'],
            })
        for k, v in store.items():
            m._stone_set_store(k, v)

        m.__init__('stoneadm', 0, 0)
        m._cluster_fsid = "fsid"
        yield m


def wait(m, c):
    # type: (StoneadmOrchestrator, OrchResult) -> Any
    return raise_if_exception(c)


@contextmanager
def with_host(m: StoneadmOrchestrator, name, addr='1::4', refresh_hosts=True, rm_with_force=True):
    with mock.patch("stoneadm.utils.resolve_ip", return_value=addr):
        wait(m, m.add_host(HostSpec(hostname=name)))
        if refresh_hosts:
            StoneadmServe(m)._refresh_hosts_and_daemons()
        yield
        wait(m, m.remove_host(name, force=rm_with_force))


def assert_rm_service(stoneadm: StoneadmOrchestrator, srv_name):
    mon_or_mgr = stoneadm.spec_store[srv_name].spec.service_type in ('mon', 'mgr')
    if mon_or_mgr:
        assert 'Unable' in wait(stoneadm, stoneadm.remove_service(srv_name))
        return
    assert wait(stoneadm, stoneadm.remove_service(srv_name)) == f'Removed service {srv_name}'
    assert stoneadm.spec_store[srv_name].deleted is not None
    StoneadmServe(stoneadm)._check_daemons()
    StoneadmServe(stoneadm)._apply_all_services()
    assert stoneadm.spec_store[srv_name].deleted
    unmanaged = stoneadm.spec_store[srv_name].spec.unmanaged
    StoneadmServe(stoneadm)._purge_deleted_services()
    if not unmanaged:  # cause then we're not deleting daemons
        assert srv_name not in stoneadm.spec_store, f'{stoneadm.spec_store[srv_name]!r}'


@contextmanager
def with_service(stoneadm_module: StoneadmOrchestrator, spec: ServiceSpec, meth=None, host: str = '', status_running=False) -> Iterator[List[str]]:
    if spec.placement.is_empty() and host:
        spec.placement = PlacementSpec(hosts=[host], count=1)
    if meth is not None:
        c = meth(stoneadm_module, spec)
        assert wait(stoneadm_module, c) == f'Scheduled {spec.service_name()} update...'
    else:
        c = stoneadm_module.apply([spec])
        assert wait(stoneadm_module, c) == [f'Scheduled {spec.service_name()} update...']

    specs = [d.spec for d in wait(stoneadm_module, stoneadm_module.describe_service())]
    assert spec in specs

    StoneadmServe(stoneadm_module)._apply_all_services()

    if status_running:
        make_daemons_running(stoneadm_module, spec.service_name())

    dds = wait(stoneadm_module, stoneadm_module.list_daemons())
    own_dds = [dd for dd in dds if dd.service_name() == spec.service_name()]
    if host and spec.service_type != 'osd':
        assert own_dds

    yield [dd.name() for dd in own_dds]

    assert_rm_service(stoneadm_module, spec.service_name())


def make_daemons_running(stoneadm_module, service_name):
    own_dds = stoneadm_module.cache.get_daemons_by_service(service_name)
    for dd in own_dds:
        dd.status = DaemonDescriptionStatus.running  # We're changing the reference


def _deploy_stoneadm_binary(host):
    def foo(*args, **kwargs):
        return True
    return foo
