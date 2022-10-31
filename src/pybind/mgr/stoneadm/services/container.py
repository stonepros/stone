import logging
from typing import List, Any, Tuple, Dict, cast

from stone.deployment.service_spec import CustomContainerSpec

from .stoneadmservice import StoneadmService, StoneadmDaemonDeploySpec

logger = logging.getLogger(__name__)


class CustomContainerService(StoneadmService):
    TYPE = 'container'

    def prepare_create(self, daemon_spec: StoneadmDaemonDeploySpec) \
            -> StoneadmDaemonDeploySpec:
        assert self.TYPE == daemon_spec.daemon_type
        daemon_spec.final_config, daemon_spec.deps = self.generate_config(daemon_spec)
        return daemon_spec

    def generate_config(self, daemon_spec: StoneadmDaemonDeploySpec) \
            -> Tuple[Dict[str, Any], List[str]]:
        assert self.TYPE == daemon_spec.daemon_type
        deps: List[str] = []
        spec = cast(CustomContainerSpec, self.mgr.spec_store[daemon_spec.service_name].spec)
        config: Dict[str, Any] = spec.config_json()
        logger.debug(
            'Generated configuration for \'%s\' service: config-json=%s, dependencies=%s' %
            (self.TYPE, config, deps))
        return config, deps
