import os
import pytest
from stone_volume import main


class TestVolume(object):

    def test_main_spits_help_with_no_arguments(self, capsys):
        with pytest.raises(SystemExit):
            main.Volume(argv=[])
        stdout, stderr = capsys.readouterr()
        assert 'Log Path' in stdout

    def test_warn_about_using_help_for_full_options(self, capsys):
        with pytest.raises(SystemExit):
            main.Volume(argv=[])
        stdout, stderr = capsys.readouterr()
        assert 'See "stone-volume --help" for full list' in stdout

    def test_environ_vars_show_up(self, capsys):
        os.environ['STONE_CONF'] = '/opt/stone.conf'
        with pytest.raises(SystemExit):
            main.Volume(argv=[])
        stdout, stderr = capsys.readouterr()
        assert 'STONE_CONF' in stdout
        assert '/opt/stone.conf' in stdout

    def test_flags_are_parsed_with_help(self, capsys):
        with pytest.raises(SystemExit):
            main.Volume(argv=['stone-volume', '--help'])
        stdout, stderr = capsys.readouterr()
        assert '--cluster' in stdout
        assert '--log-path' in stdout

    def test_log_ignoring_missing_stone_conf(self, caplog):
        with pytest.raises(SystemExit) as error:
            main.Volume(argv=['stone-volume', '--cluster', 'barnacle', 'lvm', '--help'])
        # make sure we aren't causing an actual error
        assert error.value.code == 0
        log = caplog.records[-1]
        assert log.message == 'ignoring inability to load stone.conf'
        assert log.levelname == 'ERROR'

    def test_logs_current_command(self, caplog):
        with pytest.raises(SystemExit) as error:
            main.Volume(argv=['stone-volume', '--cluster', 'barnacle', 'lvm', '--help'])
        # make sure we aren't causing an actual error
        assert error.value.code == 0
        log = caplog.records[-2]
        assert log.message == 'Running command: stone-volume --cluster barnacle lvm --help'
        assert log.levelname == 'INFO'

    def test_logs_set_level_error(self, caplog):
        with pytest.raises(SystemExit) as error:
            main.Volume(argv=['stone-volume', '--log-level', 'error', '--cluster', 'barnacle', 'lvm', '--help'])
        # make sure we aren't causing an actual error
        assert error.value.code == 0
        assert caplog.records
        # only log levels of 'ERROR' or above should be captured
        for log in caplog.records:
            assert log.levelname in ['ERROR', 'CRITICAL']

    def test_logs_incorrect_log_level(self, capsys):
        with pytest.raises(SystemExit) as error:
            main.Volume(argv=['stone-volume', '--log-level', 'foo', '--cluster', 'barnacle', 'lvm', '--help'])
        # make sure this is an error
        assert error.value.code != 0
        stdout, stderr = capsys.readouterr()
        assert "invalid choice" in stderr
