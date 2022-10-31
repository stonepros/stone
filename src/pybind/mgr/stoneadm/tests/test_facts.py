from ..import StoneadmOrchestrator

from .fixtures import wait


def test_facts(stoneadm_module: StoneadmOrchestrator):
    facts = {'node-1.stone.com': {'bios_version': 'F2', 'cpu_cores': 16}}
    stoneadm_module.cache.facts = facts
    ret_facts = stoneadm_module.get_facts('node-1.stone.com')
    assert wait(stoneadm_module, ret_facts) == [{'bios_version': 'F2', 'cpu_cores': 16}]
