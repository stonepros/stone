from collections import namedtuple


sys_info = namedtuple('sys_info', ['devices'])
sys_info.devices = dict()


class UnloadedConfig(object):
    """
    This class is used as the default value for conf.stone so that if
    a configuration file is not successfully loaded then it will give
    a nice error message when values from the config are used.
    """
    def __getattr__(self, *a):
        raise RuntimeError("No valid stone configuration file was loaded.")

conf = namedtuple('config', ['stone', 'cluster', 'verbosity', 'path', 'log_path'])
conf.stone = UnloadedConfig()

__version__ = "1.0.0"

__release__ = "pacific"
