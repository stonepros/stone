import pytest

from stoneadm.services.osd import RemoveUtil, OSD
from tests import mock

from .fixtures import with_stoneadm_module


@pytest.fixture()
def stoneadm_module():
    with with_stoneadm_module({}) as m:
        yield m


@pytest.fixture()
def rm_util():
    with with_stoneadm_module({}) as m:
        r = RemoveUtil.__new__(RemoveUtil)
        r.__init__(m)
        yield r


@pytest.fixture()
def osd_obj():
    with mock.patch("stoneadm.services.osd.RemoveUtil"):
        o = OSD(0, mock.MagicMock())
        yield o
