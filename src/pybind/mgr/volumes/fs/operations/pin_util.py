import os
import errno

import stonefs

from ..exception import VolumeException
from distutils.util import strtobool

_pin_value = {
    "export": lambda x: int(x),
    "distributed": lambda x: int(strtobool(x)),
    "random": lambda x: float(x),
}
_pin_xattr = {
    "export": "stone.dir.pin",
    "distributed": "stone.dir.pin.distributed",
    "random": "stone.dir.pin.random",
}

def pin(fs, path, pin_type, pin_setting):
    """
    Set a pin on a directory.
    """
    assert pin_type in _pin_xattr

    try:
        pin_setting = _pin_value[pin_type](pin_setting)
    except ValueError as e:
        raise VolumeException(-errno.EINVAL, f"pin value wrong type: {pin_setting}")

    try:
        fs.setxattr(path, _pin_xattr[pin_type], str(pin_setting).encode('utf-8'), 0)
    except stonefs.Error as e:
        raise VolumeException(-e.args[0], e.args[1])
