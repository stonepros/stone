import os
import errno

import stonefs

from ..exception import VolumeException

def mksnap(fs, snappath):
    """
    Create a snapshot, or do nothing if it already exists.
    """
    try:
        # snap create does not accept mode -- use default
        fs.mkdir(snappath, 0o755)
    except stonefs.ObjectExists:
        return
    except stonefs.Error as e:
        raise VolumeException(-e.args[0], e.args[1])

def rmsnap(fs, snappath):
    """
    Remove a snapshot
    """
    try:
        fs.stat(snappath)
        fs.rmdir(snappath)
    except stonefs.ObjectNotFound:
        raise VolumeException(-errno.ENOENT, "snapshot '{0}' does not exist".format(os.path.basename(snappath)))
    except stonefs.Error as e:
        raise VolumeException(-e.args[0], e.args[1])
