# type: ignore

import errno
import json
import mock
import os
import pytest
import socket
import sys
import time
import threading
import unittest

from http.server import HTTPServer
from textwrap import dedent
from urllib.request import Request, urlopen
from urllib.error import HTTPError

from typing import List, Optional

from .fixtures import (
    stoneadm_fs,
    exporter,
    mock_docker,
    mock_podman,
    with_stoneadm_ctx,
    mock_bad_firewalld,
)

with mock.patch('builtins.open', create=True):
    from importlib.machinery import SourceFileLoader
    cd = SourceFileLoader('stoneadm', 'stoneadm').load_module()


def get_stone_conf(
        fsid='00000000-0000-0000-0000-0000deadbeef',
        mon_host='[v2:192.168.1.1:3300/0,v1:192.168.1.1:6789/0]'):
    return f'''
# minimal stone.conf for {fsid}
[global]
        fsid = {fsid}
        mon_host = {mon_host}
'''

class TestStoneAdm(object):

    def test_docker_unit_file(self):
        ctx = cd.StoneadmContext()
        ctx.container_engine = mock_docker()
        r = cd.get_unit_file(ctx, '9b9d7609-f4d5-4aba-94c8-effa764d96c9')
        assert 'Requires=docker.service' in r
        ctx.container_engine = mock_podman()
        r = cd.get_unit_file(ctx, '9b9d7609-f4d5-4aba-94c8-effa764d96c9')
        assert 'Requires=docker.service' not in r

    @mock.patch('stoneadm.logger')
    def test_attempt_bind(self, logger):
        ctx = None
        address = None
        port = 0

        def os_error(errno):
            _os_error = OSError()
            _os_error.errno = errno
            return _os_error

        for side_effect, expected_exception in (
            (os_error(errno.EADDRINUSE), cd.PortOccupiedError),
            (os_error(errno.EAFNOSUPPORT), cd.Error),
            (os_error(errno.EADDRNOTAVAIL), cd.Error),
            (None, None),
        ):
            _socket = mock.Mock()
            _socket.bind.side_effect = side_effect
            try:
                cd.attempt_bind(ctx, _socket, address, port)
            except Exception as e:
                assert isinstance(e, expected_exception)
            else:
                if expected_exception is not None:
                    assert False

    @mock.patch('stoneadm.attempt_bind')
    @mock.patch('stoneadm.logger')
    def test_port_in_use(self, logger, attempt_bind):
        empty_ctx = None

        assert cd.port_in_use(empty_ctx, 9100) == False

        attempt_bind.side_effect = cd.PortOccupiedError('msg')
        assert cd.port_in_use(empty_ctx, 9100) == True

        os_error = OSError()
        os_error.errno = errno.EADDRNOTAVAIL
        attempt_bind.side_effect = os_error
        assert cd.port_in_use(empty_ctx, 9100) == False

        os_error = OSError()
        os_error.errno = errno.EAFNOSUPPORT
        attempt_bind.side_effect = os_error
        assert cd.port_in_use(empty_ctx, 9100) == False

    @mock.patch('socket.socket')
    @mock.patch('stoneadm.logger')
    def test_check_ip_port_success(self, logger, _socket):
        ctx = cd.StoneadmContext()
        ctx.skip_ping_check = False  # enables executing port check with `check_ip_port`

        for address, address_family in (
            ('0.0.0.0', socket.AF_INET),
            ('::', socket.AF_INET6),
        ):
            try:
                cd.check_ip_port(ctx, address, 9100)
            except:
                assert False
            else:
                assert _socket.call_args == mock.call(address_family, socket.SOCK_STREAM)

    @mock.patch('socket.socket')
    @mock.patch('stoneadm.logger')
    def test_check_ip_port_failure(self, logger, _socket):
        ctx = cd.StoneadmContext()
        ctx.skip_ping_check = False  # enables executing port check with `check_ip_port`

        def os_error(errno):
            _os_error = OSError()
            _os_error.errno = errno
            return _os_error

        for address, address_family in (
            ('0.0.0.0', socket.AF_INET),
            ('::', socket.AF_INET6),
        ):
            for side_effect, expected_exception in (
                (os_error(errno.EADDRINUSE), cd.PortOccupiedError),
                (os_error(errno.EADDRNOTAVAIL), cd.Error),
                (os_error(errno.EAFNOSUPPORT), cd.Error),
                (None, None),
            ):
                mock_socket_obj = mock.Mock()
                mock_socket_obj.bind.side_effect = side_effect
                _socket.return_value = mock_socket_obj
                try:
                    cd.check_ip_port(ctx, address, 9100)
                except Exception as e:
                    assert isinstance(e, expected_exception)
                else:
                    if side_effect is not None:
                        assert False


    def test_is_not_fsid(self):
        assert not cd.is_fsid('no-uuid')

    def test_is_fsid(self):
        assert cd.is_fsid('e863154d-33c7-4350-bca5-921e0467e55b')

    def test__get_parser_image(self):
        args = cd._parse_args(['--image', 'foo', 'version'])
        assert args.image == 'foo'

    def test_parse_mem_usage(self):
        cd.logger = mock.Mock()
        len, summary = cd._parse_mem_usage(0, 'c6290e3f1489,-- / --')
        assert summary == {}

    def test_CustomValidation(self):
        assert cd._parse_args(['deploy', '--name', 'mon.a', '--fsid', 'fsid'])

        with pytest.raises(SystemExit):
            cd._parse_args(['deploy', '--name', 'wrong', '--fsid', 'fsid'])

    @pytest.mark.parametrize("test_input, expected", [
        ("1.6.2", (1,6,2)),
        ("1.6.2-stable2", (1,6,2)),
    ])
    def test_parse_podman_version(self, test_input, expected):
        assert cd._parse_podman_version(test_input) == expected

    def test_parse_podman_version_invalid(self):
        with pytest.raises(ValueError) as res:
            cd._parse_podman_version('inval.id')
        assert 'inval' in str(res.value)

    def test_is_ipv6(self):
        cd.logger = mock.Mock()
        for good in ("[::1]", "::1",
                     "fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"):
            assert cd.is_ipv6(good)
        for bad in ("127.0.0.1",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffg",
                    "1:2:3:4:5:6:7:8:9", "fd00::1::1", "[fg::1]"):
            assert not cd.is_ipv6(bad)

    def test_unwrap_ipv6(self):
        def unwrap_test(address, expected):
            assert cd.unwrap_ipv6(address) == expected

        tests = [
            ('::1', '::1'), ('[::1]', '::1'),
            ('[fde4:8dba:82e1:0:5054:ff:fe6a:357]', 'fde4:8dba:82e1:0:5054:ff:fe6a:357'),
            ('can actually be any string', 'can actually be any string'),
            ('[but needs to be stripped] ', '[but needs to be stripped] ')]
        for address, expected in tests:
            unwrap_test(address, expected)

    def test_wrap_ipv6(self):
        def wrap_test(address, expected):
            assert cd.wrap_ipv6(address) == expected

        tests = [
            ('::1', '[::1]'), ('[::1]', '[::1]'),
            ('fde4:8dba:82e1:0:5054:ff:fe6a:357',
             '[fde4:8dba:82e1:0:5054:ff:fe6a:357]'),
            ('myhost.example.com', 'myhost.example.com'),
            ('192.168.0.1', '192.168.0.1'),
            ('', ''), ('fd00::1::1', 'fd00::1::1')]
        for address, expected in tests:
            wrap_test(address, expected)

    @mock.patch('stoneadm.Firewalld', mock_bad_firewalld)
    @mock.patch('stoneadm.logger')
    def test_skip_firewalld(self, logger, stoneadm_fs):
        """
        test --skip-firewalld actually skips changing firewall
        """

        ctx = cd.StoneadmContext()
        with pytest.raises(Exception):
            cd.update_firewalld(ctx, 'mon')

        ctx.skip_firewalld = True
        cd.update_firewalld(ctx, 'mon')

        ctx.skip_firewalld = False
        with pytest.raises(Exception):
            cd.update_firewalld(ctx, 'mon')

        ctx = cd.StoneadmContext()
        ctx.ssl_dashboard_port = 8888
        ctx.dashboard_key = None
        ctx.dashboard_password_noupdate = True
        ctx.initial_dashboard_password = 'password'
        ctx.initial_dashboard_user = 'User'
        with pytest.raises(Exception):
            cd.prepare_dashboard(ctx, 0, 0, lambda _, extra_mounts=None, ___=None : '5', lambda : None)

        ctx.skip_firewalld = True
        cd.prepare_dashboard(ctx, 0, 0, lambda _, extra_mounts=None, ___=None : '5', lambda : None)

        ctx.skip_firewalld = False
        with pytest.raises(Exception):
            cd.prepare_dashboard(ctx, 0, 0, lambda _, extra_mounts=None, ___=None : '5', lambda : None)

    @mock.patch('stoneadm.call_throws')
    @mock.patch('stoneadm.get_parm')
    def test_registry_login(self, get_parm, call_throws):
        # test normal valid login with url, username and password specified
        call_throws.return_value = '', '', 0
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['registry-login', '--registry-url', 'sample-url',
            '--registry-username', 'sample-user', '--registry-password',
            'sample-pass'])
        ctx.container_engine = mock_docker()
        retval = cd.command_registry_login(ctx)
        assert retval == 0

        # test bad login attempt with invalid arguments given
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['registry-login', '--registry-url', 'bad-args-url'])
        with pytest.raises(Exception) as e:
            assert cd.command_registry_login(ctx)
        assert str(e.value) == ('Invalid custom registry arguments received. To login to a custom registry include '
                                '--registry-url, --registry-username and --registry-password options or --registry-json option')

        # test normal valid login with json file
        get_parm.return_value = {"url": "sample-url", "username": "sample-username", "password": "sample-password"}
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['registry-login', '--registry-json', 'sample-json'])
        ctx.container_engine = mock_docker()
        retval = cd.command_registry_login(ctx)
        assert retval == 0

        # test bad login attempt with bad json file
        get_parm.return_value = {"bad-json": "bad-json"}
        ctx: cd.StoneadmContext =  cd.stoneadm_init_ctx(
            ['registry-login', '--registry-json', 'sample-json'])
        with pytest.raises(Exception) as e:
            assert cd.command_registry_login(ctx)
        assert str(e.value) == ("json provided for custom registry login did not include all necessary fields. "
                        "Please setup json file as\n"
                        "{\n"
                          " \"url\": \"REGISTRY_URL\",\n"
                          " \"username\": \"REGISTRY_USERNAME\",\n"
                          " \"password\": \"REGISTRY_PASSWORD\"\n"
                        "}\n")

        # test login attempt with valid arguments where login command fails
        call_throws.side_effect = Exception
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['registry-login', '--registry-url', 'sample-url',
            '--registry-username', 'sample-user', '--registry-password',
            'sample-pass'])
        with pytest.raises(Exception) as e:
            cd.command_registry_login(ctx)
        assert str(e.value) == "Failed to login to custom registry @ sample-url as sample-user with given password"

    def test_get_image_info_from_inspect(self):
        # podman
        out = """204a01f9b0b6710dd0c0af7f37ce7139c47ff0f0105d778d7104c69282dfbbf1,[docker.io/stone/stone@sha256:1cc9b824e1b076cdff52a9aa3f0cc8557d879fb2fbbba0cafed970aca59a3992]"""
        r = cd.get_image_info_from_inspect(out, 'registry/stone/stone:latest')
        print(r)
        assert r == {
            'image_id': '204a01f9b0b6710dd0c0af7f37ce7139c47ff0f0105d778d7104c69282dfbbf1',
            'repo_digests': ['docker.io/stone/stone@sha256:1cc9b824e1b076cdff52a9aa3f0cc8557d879fb2fbbba0cafed970aca59a3992']
        }

        # docker
        out = """sha256:16f4549cf7a8f112bbebf7946749e961fbbd1b0838627fe619aab16bc17ce552,[quay.stone.io/stone-ci/stone@sha256:4e13da36c1bd6780b312a985410ae678984c37e6a9493a74c87e4a50b9bda41f]"""
        r = cd.get_image_info_from_inspect(out, 'registry/stone/stone:latest')
        assert r == {
            'image_id': '16f4549cf7a8f112bbebf7946749e961fbbd1b0838627fe619aab16bc17ce552',
            'repo_digests': ['quay.stone.io/stone-ci/stone@sha256:4e13da36c1bd6780b312a985410ae678984c37e6a9493a74c87e4a50b9bda41f']
        }

        # multiple digests (podman)
        out = """e935122ab143a64d92ed1fbb27d030cf6e2f0258207be1baf1b509c466aeeb42,[docker.io/prom/prometheus@sha256:e4ca62c0d62f3e886e684806dfe9d4e0cda60d54986898173c1083856cfda0f4 docker.io/prom/prometheus@sha256:efd99a6be65885c07c559679a0df4ec709604bcdd8cd83f0d00a1a683b28fb6a]"""
        r = cd.get_image_info_from_inspect(out, 'registry/prom/prometheus:latest')
        assert r == {
            'image_id': 'e935122ab143a64d92ed1fbb27d030cf6e2f0258207be1baf1b509c466aeeb42',
            'repo_digests': [
                'docker.io/prom/prometheus@sha256:e4ca62c0d62f3e886e684806dfe9d4e0cda60d54986898173c1083856cfda0f4',
                'docker.io/prom/prometheus@sha256:efd99a6be65885c07c559679a0df4ec709604bcdd8cd83f0d00a1a683b28fb6a',
            ]
        }


    def test_dict_get(self):
        result = cd.dict_get({'a': 1}, 'a', require=True)
        assert result == 1
        result = cd.dict_get({'a': 1}, 'b')
        assert result is None
        result = cd.dict_get({'a': 1}, 'b', default=2)
        assert result == 2

    def test_dict_get_error(self):
        with pytest.raises(cd.Error):
            cd.dict_get({'a': 1}, 'b', require=True)

    def test_dict_get_join(self):
        result = cd.dict_get_join({'foo': ['a', 'b']}, 'foo')
        assert result == 'a\nb'
        result = cd.dict_get_join({'foo': [1, 2]}, 'foo')
        assert result == '1\n2'
        result = cd.dict_get_join({'bar': 'a'}, 'bar')
        assert result == 'a'
        result = cd.dict_get_join({'a': 1}, 'a')
        assert result == 1

    def test_last_local_images(self):
        out = '''
docker.io/stone/daemon-base@
docker.io/stone/stone:v15.2.5
docker.io/stone/daemon-base:octopus
        '''
        image = cd._filter_last_local_stone_image(out)
        assert image == 'docker.io/stone/stone:v15.2.5'

    def test_normalize_image_digest(self):
        s = 'myhostname:5000/stone/stone@sha256:753886ad9049004395ae990fbb9b096923b5a518b819283141ee8716ddf55ad1'
        assert cd.normalize_image_digest(s) == s

        s = 'stone/stone:latest'
        assert cd.normalize_image_digest(s) == f'{cd.DEFAULT_REGISTRY}/{s}'

    @pytest.mark.parametrize('fsid, stone_conf, list_daemons, result, err, ',
        [
            (
                None,
                None,
                [],
                None,
                None,
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                [],
                '00000000-0000-0000-0000-0000deadbeef',
                None,
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                [
                    {'fsid': '10000000-0000-0000-0000-0000deadbeef'},
                    {'fsid': '20000000-0000-0000-0000-0000deadbeef'},
                ],
                '00000000-0000-0000-0000-0000deadbeef',
                None,
            ),
            (
                None,
                None,
                [
                    {'fsid': '00000000-0000-0000-0000-0000deadbeef'},
                ],
                '00000000-0000-0000-0000-0000deadbeef',
                None,
            ),
            (
                None,
                None,
                [
                    {'fsid': '10000000-0000-0000-0000-0000deadbeef'},
                    {'fsid': '20000000-0000-0000-0000-0000deadbeef'},
                ],
                None,
                r'Cannot infer an fsid',
            ),
            (
                None,
                get_stone_conf(fsid='00000000-0000-0000-0000-0000deadbeef'),
                [],
                '00000000-0000-0000-0000-0000deadbeef',
                None,
            ),
            (
                None,
                get_stone_conf(fsid='00000000-0000-0000-0000-0000deadbeef'),
                [
                    {'fsid': '00000000-0000-0000-0000-0000deadbeef'},
                ],
                '00000000-0000-0000-0000-0000deadbeef',
                None,
            ),
            (
                None,
                get_stone_conf(fsid='00000000-0000-0000-0000-0000deadbeef'),
                [
                    {'fsid': '10000000-0000-0000-0000-0000deadbeef'},
                    {'fsid': '20000000-0000-0000-0000-0000deadbeef'},
                ],
                None,
                r'Cannot infer an fsid',
            ),
        ])
    @mock.patch('stoneadm.call')
    def test_infer_fsid(self, _call, fsid, stone_conf, list_daemons, result, err, stoneadm_fs):
        # build the context
        ctx = cd.StoneadmContext()
        ctx.fsid = fsid

        # mock the decorator
        mock_fn = mock.Mock()
        mock_fn.return_value = 0
        infer_fsid = cd.infer_fsid(mock_fn)

        # mock the stone.conf file content
        if stone_conf:
            f = stoneadm_fs.create_file('stone.conf', contents=stone_conf)
            ctx.config = f.path

        # test
        with mock.patch('stoneadm.list_daemons', return_value=list_daemons):
            if err:
                with pytest.raises(cd.Error, match=err):
                    infer_fsid(ctx)
            else:
                infer_fsid(ctx)
            assert ctx.fsid == result

    @pytest.mark.parametrize('fsid, config, name, list_daemons, result, ',
        [
            (
                None,
                '/foo/bar.conf',
                None,
                [],
                '/foo/bar.conf',
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                None,
                [],
                cd.SHELL_DEFAULT_CONF,
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                None,
                [{'name': 'mon.a', 'fsid': '00000000-0000-0000-0000-0000deadbeef', 'style': 'stoneadm:v1'}],
                '/var/lib/stone/00000000-0000-0000-0000-0000deadbeef/mon.a/config',
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                None,
                [{'name': 'mon.a', 'fsid': 'aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa', 'style': 'stoneadm:v1'}],
                cd.SHELL_DEFAULT_CONF,
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                None,
                [{'name': 'mon.a', 'fsid': '00000000-0000-0000-0000-0000deadbeef', 'style': 'legacy'}],
                cd.SHELL_DEFAULT_CONF,
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                None,
                [{'name': 'osd.0'}],
                cd.SHELL_DEFAULT_CONF,
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                '/foo/bar.conf',
                'mon.a',
                [{'name': 'mon.a', 'style': 'stoneadm:v1'}],
                '/foo/bar.conf',
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                'mon.a',
                [],
                '/var/lib/stone/00000000-0000-0000-0000-0000deadbeef/mon.a/config',
            ),
            (
                '00000000-0000-0000-0000-0000deadbeef',
                None,
                'osd.0',
                [],
                '/var/lib/stone/00000000-0000-0000-0000-0000deadbeef/osd.0/config',
            ),
            (
                None,
                None,
                None,
                [],
                cd.SHELL_DEFAULT_CONF,
            ),
        ])
    @mock.patch('stoneadm.call')
    @mock.patch('stoneadm.logger')
    def test_infer_config(self, logger, _call, fsid, config, name, list_daemons, result, stoneadm_fs):
        # build the context
        ctx = cd.StoneadmContext()
        ctx.fsid = fsid
        ctx.config = config
        ctx.name = name

        # mock the decorator
        mock_fn = mock.Mock()
        mock_fn.return_value = 0
        infer_config = cd.infer_config(mock_fn)

        # mock the config file
        stoneadm_fs.create_file(result)

        # test
        with mock.patch('stoneadm.list_daemons', return_value=list_daemons):
            infer_config(ctx)
            assert ctx.config == result

    @mock.patch('stoneadm.call')
    def test_extract_uid_gid_fail(self, _call):
        err = """Error: container_linux.go:370: starting container process caused: process_linux.go:459: container init caused: process_linux.go:422: setting cgroup config for procHooks process caused: Unit libpod-056038e1126191fba41d8a037275136f2d7aeec9710b9ee
ff792c06d8544b983.scope not found.: OCI runtime error"""
        _call.return_value = ('', err, 127)
        ctx = cd.StoneadmContext()
        ctx.container_engine = mock_podman()
        with pytest.raises(cd.Error, match='OCI'):
            cd.extract_uid_gid(ctx)

    @pytest.mark.parametrize('test_input, expected', [
        ([cd.make_fsid(), cd.make_fsid(), cd.make_fsid()], 3),
        ([cd.make_fsid(), 'invalid-fsid', cd.make_fsid(), '0b87e50c-8e77-11ec-b890-'], 2),
        (['f6860ec2-8e76-11ec-', '0b87e50c-8e77-11ec-b890-', ''], 0),
        ([], 0),
    ])
    def test_get_stone_cluster_count(self, test_input, expected):
        ctx = cd.StoneadmContext()
        with mock.patch('os.listdir', return_value=test_input):
            assert cd.get_stone_cluster_count(ctx) == expected

    def test_set_image_minimize_config(self):
        def throw_cmd(cmd):
            raise cd.Error(' '.join(cmd))
        ctx = cd.StoneadmContext()
        ctx.image = 'test_image'
        ctx.no_minimize_config = True
        fake_cli = lambda cmd, __=None, ___=None: throw_cmd(cmd)
        with pytest.raises(cd.Error, match='config set global container_image test_image'):
            cd.finish_bootstrap_config(
                ctx=ctx,
                fsid=cd.make_fsid(),
                config='',
                mon_id='a', mon_dir='mon_dir',
                mon_network=None, ipv6=False,
                cli=fake_cli,
                cluster_network=None,
                ipv6_cluster_network=False
            )


class TestCustomContainer(unittest.TestCase):
    cc: cd.CustomContainer

    def setUp(self):
        self.cc = cd.CustomContainer(
            'e863154d-33c7-4350-bca5-921e0467e55b',
            'container',
            config_json={
                'entrypoint': 'bash',
                'gid': 1000,
                'args': [
                    '--no-healthcheck',
                    '-p 6800:6800'
                ],
                'envs': ['SECRET=password'],
                'ports': [8080, 8443],
                'volume_mounts': {
                    '/CONFIG_DIR': '/foo/conf',
                    'bar/config': '/bar:ro'
                },
                'bind_mounts': [
                    [
                        'type=bind',
                        'source=/CONFIG_DIR',
                        'destination=/foo/conf',
                        ''
                    ],
                    [
                        'type=bind',
                        'source=bar/config',
                        'destination=/bar:ro',
                        'ro=true'
                    ]
                ]
            },
            image='docker.io/library/hello-world:latest'
        )

    def test_entrypoint(self):
        self.assertEqual(self.cc.entrypoint, 'bash')

    def test_uid_gid(self):
        self.assertEqual(self.cc.uid, 65534)
        self.assertEqual(self.cc.gid, 1000)

    def test_ports(self):
        self.assertEqual(self.cc.ports, [8080, 8443])

    def test_get_container_args(self):
        result = self.cc.get_container_args()
        self.assertEqual(result, [
            '--no-healthcheck',
            '-p 6800:6800'
        ])

    def test_get_container_envs(self):
        result = self.cc.get_container_envs()
        self.assertEqual(result, ['SECRET=password'])

    def test_get_container_mounts(self):
        result = self.cc.get_container_mounts('/xyz')
        self.assertDictEqual(result, {
            '/CONFIG_DIR': '/foo/conf',
            '/xyz/bar/config': '/bar:ro'
        })

    def test_get_container_binds(self):
        result = self.cc.get_container_binds('/xyz')
        self.assertEqual(result, [
            [
                'type=bind',
                'source=/CONFIG_DIR',
                'destination=/foo/conf',
                ''
            ],
            [
                'type=bind',
                'source=/xyz/bar/config',
                'destination=/bar:ro',
                'ro=true'
            ]
        ])


class TestStoneadmExporter(object):
    exporter: cd.StoneadmDaemon
    files_created: List[str] = []
    crt = """-----BEGIN CERTIFICATE-----
MIIC1zCCAb8CEFHoZE2MfUVzo53fzzBKAT0wDQYJKoZIhvcNAQENBQAwKjENMAsG
A1UECgwEQ2VwaDEZMBcGA1UECwwQY2VwaGFkbS1leHBvcnRlcjAeFw0yMDExMjUy
MzEwNTVaFw0zMDExMjMyMzEwNTVaMCoxDTALBgNVBAoMBENlcGgxGTAXBgNVBAsM
EGNlcGhhZG0tZXhwb3J0ZXIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB
AQCsTfcJcXbREqfx1zTUuEmK+lJn9WWjk0URRF1Z+QgPkascNdkX16PnvhbGwXmF
BTdAcNl7V0U+z4EsGJ7hJsB7qTq6Rb6wNl7r0OxjeWOmB9xbF4Q/KR5yrbM1DA9A
B5fNswrUXViku5Y2jlOAz+ZMBhYxMx0edqhxSn297j04Z6RF4Mvkc43v0FH7Ju7k
O5+0VbdzcOdu37DFpoE4Ll2MZ/GuAHcJ8SD06sEdzFEjRCraav976743XcUlhZGX
ZTTG/Zf/a+wuCjtMG3od7vRFfuRrM5oTE133DuQ5deR7ybcZNDyopDjHF8xB1bAk
IOz4SbP6Q25K99Czm1K+3kMLAgMBAAEwDQYJKoZIhvcNAQENBQADggEBACmtvZb8
dJGHx/WC0/JHxnEJCJM2qnn87ELzbbIQL1w1Yb/I6JQYPgq+WiQPaHaLL9eYsm0l
dFwvrh+WC0JpXDfADnUnkTSB/WpZ2nC+2JxBptrQEuIcqNXpcJd0bKDiHunv04JI
uEVpTAK05dBV38qNmIlu4HyB4OEnuQpyOr9xpIhdxuJ95O9K0j5BIw98ZaEwYNUP
Rm3YlQwfS6R5xaBvL9kyfxyAD2joNj44q6w/5zj4egXVIA5VpkQm8DmMtu0Pd2NG
dzfYRmqrDolh+rty8HiyIxzeDJQ5bj6LKbUkmABvX50nDySVyMfHmt461/n7W65R
CHFLoOmfJJik+Uc=\n-----END CERTIFICATE-----
"""
    key = """-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCsTfcJcXbREqfx
1zTUuEmK+lJn9WWjk0URRF1Z+QgPkascNdkX16PnvhbGwXmFBTdAcNl7V0U+z4Es
GJ7hJsB7qTq6Rb6wNl7r0OxjeWOmB9xbF4Q/KR5yrbM1DA9AB5fNswrUXViku5Y2
jlOAz+ZMBhYxMx0edqhxSn297j04Z6RF4Mvkc43v0FH7Ju7kO5+0VbdzcOdu37DF
poE4Ll2MZ/GuAHcJ8SD06sEdzFEjRCraav976743XcUlhZGXZTTG/Zf/a+wuCjtM
G3od7vRFfuRrM5oTE133DuQ5deR7ybcZNDyopDjHF8xB1bAkIOz4SbP6Q25K99Cz
m1K+3kMLAgMBAAECggEASnAwToMXWsGdjqxzpYasNv9oBIOO0nk4OHp5ffpJUjiT
XM+ip1tA80g7HMjPD/mt4gge3NtaDgWlf4Bve0O7mnEE7x5cgFIs9eG/jkYOF9eD
ilMBjivcfJywNDWujPH60iIMhqyBNEHaZl1ck+S9UJC8m6rCZLvMj40n/5riFfBy
1sjf2uOwcfWrjSj9Ju4wlMI6khSSz2aYC7glQQ/fo2+YArbEUcy60iloPQ6wEgZK
okoVWZA9AehwLcnRjkwd9EVmMMtRGPE/AcP4s/kKA0tRDRicPLN727Ke/yxv+Ppo
hbIZIcOn7soOFAENcodJ4YRSCd++QfCNaVAi7vwWWQKBgQDeBY4vvr+H0brbSjQg
O7Fpqub/fxZY3UoHWDqWs2X4o3qhDqaTQODpuYtCm8YQE//55JoLWKAD0evq5dLS
YLrtC1Vyxf+TA7opCUjWBe+liyndbJdB5q0zF7qdWUtQKGVSWyUWhK8gHa6M64fP
oi83DD7F0OGusTWGtfbceErk/wKBgQDGrJLRo/5xnAH5VmPfNu+S6h0M2qM6CYwe
Y5wHFG2uQQct73adf53SkhvZVmOzJsWQbVnlDOKMhqazcs+7VWRgO5X3naWVcctE
Hggw9MgpbXAWFOI5sNYsCYE58E+fTHjE6O4A3MhMCsze+CIC3sKuPQBBiL9bWSOX
8POswqfl9QKBgDe/nVxPwTgRaaH2l/AgDQRDbY1qE+psZlJBzTRaB5jPM9ONIjaH
a/JELLuk8a7H1tagmC2RK1zKMTriSnWY5FbxKZuQLAR2QyBavHdBNlOTBggbZD+f
9I2Hv8wSx95wxkBPsphc6Lxft5ya55czWjewU3LIaGK9DHuu5TWm3udxAoGBAJGP
PsJ59KIoOwoDUYjpJv3sqPwR9CVBeXeKY3aMcQ+KdUgiejVKmsb8ZYsG0GUhsv3u
ID7BAfsTbG9tXuVR2wjmnymcRwUHKnXtyvKTZVN06vpCsryx4zjAff2FI9ECpjke
r8HSAK41+4QhKEoSC3C9IMLi/dBfrsRTtTSOKZVBAoGBAI2dl5HEIFpufaI4toWM
LO5HFrlXgRDGoc/+Byr5/8ZZpYpU115Ol/q6M+l0koV2ygJ9jeJJEllFWykIDS6F
XxazFI74swAqobHb2ZS/SLhoVxE82DdSeXrjkTvUjNtrW5zs1gIMKBR4nD6H8AqL
iMN28C2bKGao5UHvdER1rGy7
-----END PRIVATE KEY-----
"""
    token = "MyAccessToken"

    @classmethod
    def setup_class(cls):
        # create the ssl files
        fname = os.path.join(os.getcwd(), 'crt')
        with open(fname, 'w') as crt:
            crt.write(cls.crt)
            cls.files_created.append(fname)
        fname = os.path.join(os.getcwd(), 'key')
        with open(fname, 'w') as crt:
            crt.write(cls.key)
            cls.files_created.append(fname)
        fname = os.path.join(os.getcwd(), 'token')
        with open(fname, 'w') as crt:
            crt.write(cls.token)
            cls.files_created.append(fname)
         # start a simple http instance to test the requesthandler
        cls.server = HTTPServer(('0.0.0.0', 9443), cd.StoneadmDaemonHandler)
        cls.server.stoneadm_cache = cd.StoneadmCache()
        cls.server.token = cls.token
        t = threading.Thread(target=cls.server.serve_forever)
        t.daemon = True
        t.start() 

    @classmethod
    def teardown_class(cls):
        cls.server.shutdown()
        assert len(cls.files_created) > 0
        for f in cls.files_created:
            os.remove(f)      
    
    def setup_method(self):
        # re-init the cache for every test
        TestStoneadmExporter.server.stoneadm_cache = cd.StoneadmCache()
    
    def teardown_method(self):
        pass

    def test_files_ready(self):
        assert os.path.exists(os.path.join(os.getcwd(), 'crt'))
        assert os.path.exists(os.path.join(os.getcwd(), 'key'))
        assert os.path.exists(os.path.join(os.getcwd(), 'token'))

    def test_can_run(self, exporter):
        assert exporter.can_run
    
    def test_token_valid(self, exporter):
        assert exporter.token == self.token

    def test_unit_name(self,exporter):
        assert exporter.unit_name
        assert exporter.unit_name == "stone-foobar-stoneadm-exporter.test.service"

    def test_unit_run(self,exporter):
        assert exporter.unit_run
        lines = exporter.unit_run.split('\n')
        assert len(lines) == 2
        assert "stoneadm exporter --fsid foobar --id test --port 9443 &" in lines[1]

    def test_binary_path(self, exporter):
        assert os.path.isfile(exporter.binary_path)

    def test_systemd_unit(self, exporter):
        assert exporter.unit_file

    def test_validate_passes(self, exporter):
        config = {
            "crt": self.crt,
            "key": self.key,
            "token": self.token,
        }
        cd.StoneadmDaemon.validate_config(config)

    def test_validate_fails(self, exporter):
        config = {
            "key": self.key,
            "token": self.token,
        }
        with pytest.raises(cd.Error):
            cd.StoneadmDaemon.validate_config(config)

    def test_port_active(self, exporter):
        assert exporter.port_active == True

    def test_rqst_health_200(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs)
        r = urlopen(req)
        assert r.status == 200

    def test_rqst_all_inactive_500(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata",headers=hdrs)
        try:
            r = urlopen(req)
        except HTTPError as e:
            assert e.code == 500

    def test_rqst_no_auth_401(self):
        req=Request("http://localhost:9443/v1/metadata")
        try:
            urlopen(req)
        except HTTPError as e:
            assert e.code == 401
 
    def test_rqst_bad_auth_401(self):
        hdrs={"Authorization":f"Bearer BogusAuthToken"}
        req=Request("http://localhost:9443/v1/metadata",headers=hdrs)
        try:
            urlopen(req)
        except HTTPError as e:
            assert e.code == 401

    def test_rqst_badURL_404(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metazoic",headers=hdrs)
        try:
            urlopen(req)
        except HTTPError as e:
            assert e.code == 404

    def test_rqst_inactive_task_204(self):
        # all tasks initialise as inactive, and then 'go' active as their thread starts
        # so we can pick any task to check for an inactive response (no content)
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/disks",headers=hdrs)
        r = urlopen(req)
        assert r.status == 204

    def test_rqst_active_task_200(self):
        TestStoneadmExporter.server.stoneadm_cache.tasks['host'] = 'active'
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/host",headers=hdrs)
        r = urlopen(req)
        assert r.status == 200

    def test_rqst_all_206(self):
        TestStoneadmExporter.server.stoneadm_cache.tasks['disks'] = 'active'
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata",headers=hdrs)
        r = urlopen(req)
        assert r.status == 206

    def test_rqst_disks_200(self):
        TestStoneadmExporter.server.stoneadm_cache.tasks['disks'] = 'active'
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/disks",headers=hdrs)
        r = urlopen(req)
        assert r.status == 200

    def test_thread_exception(self, exporter):
        # run is patched to invoke a mocked scrape_host thread that will raise so 
        # we check here that the exception handler updates the cache object as we'd
        # expect with the error
        exporter.run()
        assert exporter.stoneadm_cache.host['scrape_errors']
        assert exporter.stoneadm_cache.host['scrape_errors'] == ['ValueError exception: wah']
        assert exporter.stoneadm_cache.errors == ['host thread stopped']

    # Test the requesthandler does the right thing with invalid methods...
    # ie. return a "501" - Not Implemented / Unsupported Method
    def test_invalid_method_HEAD(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="HEAD")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_DELETE(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="DELETE")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_POST(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="POST")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_PUT(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="PUT")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_CONNECT(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="CONNECT")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_TRACE(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="TRACE")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_OPTIONS(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="OPTIONS")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_invalid_method_PATCH(self):
        hdrs={"Authorization":f"Bearer {TestStoneadmExporter.token}"}
        req=Request("http://localhost:9443/v1/metadata/health",headers=hdrs, method="PATCH")
        with pytest.raises(HTTPError, match=r"HTTP Error 501: .*") as e:
            urlopen(req)

    def test_ipv4_subnet(self):
        rc, v, msg = cd.check_subnet('192.168.1.0/24')
        assert rc == 0 and v[0] == 4
    
    def test_ipv4_subnet_list(self):
        rc, v, msg = cd.check_subnet('192.168.1.0/24,10.90.90.0/24')
        assert rc == 0 and not msg
    
    def test_ipv4_subnet_badlist(self):
        rc, v, msg = cd.check_subnet('192.168.1.0/24,192.168.1.1')
        assert rc == 1 and msg

    def test_ipv4_subnet_mixed(self):
        rc, v, msg = cd.check_subnet('192.168.100.0/24,fe80::/64')
        assert rc == 0 and v == [4,6]

    def test_ipv6_subnet(self):
        rc, v, msg = cd.check_subnet('fe80::/64')
        assert rc == 0 and v[0] == 6
    
    def test_subnet_mask_missing(self):
        rc, v, msg = cd.check_subnet('192.168.1.58')
        assert rc == 1 and msg
    
    def test_subnet_mask_junk(self):
        rc, v, msg = cd.check_subnet('wah')
        assert rc == 1 and msg


class TestMaintenance:
    systemd_target = "stone.00000000-0000-0000-0000-000000c0ffee.target"
    fsid = '0ea8cdd0-1bbf-11ec-a9c7-5254002763fa'

    def test_systemd_target_OK(self, tmp_path):
        base = tmp_path
        wants = base / "stone.target.wants"
        wants.mkdir()
        target = wants / TestMaintenance.systemd_target
        target.touch()
        ctx = cd.StoneadmContext()
        ctx.unit_dir = str(base)

        assert cd.systemd_target_state(ctx, target.name)

    def test_systemd_target_NOTOK(self, tmp_path):
        base = tmp_path
        ctx = cd.StoneadmContext()
        ctx.unit_dir = str(base)
        assert not cd.systemd_target_state(ctx, TestMaintenance.systemd_target)

    def test_parser_OK(self):
        args = cd._parse_args(['host-maintenance', 'enter'])
        assert args.maintenance_action == 'enter'

    def test_parser_BAD(self):
        with pytest.raises(SystemExit):
            cd._parse_args(['host-maintenance', 'wah'])

    @mock.patch('os.listdir', return_value=[])
    @mock.patch('stoneadm.call')
    @mock.patch('stoneadm.systemd_target_state')
    def test_enter_failure_1(self, _target_state, _call, _listdir):
        _call.return_value = '', '', 999
        _target_state.return_value = True
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['host-maintenance', 'enter', '--fsid', TestMaintenance.fsid])
        ctx.container_engine = mock_podman()
        retval = cd.command_maintenance(ctx)
        assert retval.startswith('failed')

    @mock.patch('os.listdir', return_value=[])
    @mock.patch('stoneadm.call')
    @mock.patch('stoneadm.systemd_target_state')
    def test_enter_failure_2(self, _target_state, _call, _listdir):
        _call.side_effect = [('', '', 0), ('', '', 999)]
        _target_state.return_value = True
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['host-maintenance', 'enter', '--fsid', TestMaintenance.fsid])
        ctx.container_engine = mock_podman()
        retval = cd.command_maintenance(ctx)
        assert retval.startswith('failed')

    @mock.patch('os.listdir', return_value=[])
    @mock.patch('stoneadm.call')
    @mock.patch('stoneadm.systemd_target_state')
    @mock.patch('stoneadm.target_exists')
    def test_exit_failure_1(self, _target_exists, _target_state, _call, _listdir):
        _call.return_value = '', '', 999
        _target_state.return_value = False
        _target_exists.return_value = True
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['host-maintenance', 'exit', '--fsid', TestMaintenance.fsid])
        ctx.container_engine = mock_podman()
        retval = cd.command_maintenance(ctx)
        assert retval.startswith('failed')

    @mock.patch('os.listdir', return_value=[])
    @mock.patch('stoneadm.call')
    @mock.patch('stoneadm.systemd_target_state')
    @mock.patch('stoneadm.target_exists')
    def test_exit_failure_2(self, _target_exists, _target_state, _call, _listdir):
        _call.side_effect = [('', '', 0), ('', '', 999)]
        _target_state.return_value = False
        _target_exists.return_value = True
        ctx: cd.StoneadmContext = cd.stoneadm_init_ctx(
            ['host-maintenance', 'exit', '--fsid', TestMaintenance.fsid])
        ctx.container_engine = mock_podman()
        retval = cd.command_maintenance(ctx)
        assert retval.startswith('failed')


class TestMonitoring(object):
    @mock.patch('stoneadm.call')
    def test_get_version_alertmanager(self, _call):
        ctx = cd.StoneadmContext()
        ctx.container_engine = mock_podman()
        daemon_type = 'alertmanager'

        # binary `prometheus`
        _call.return_value = '', '{}, version 0.16.1'.format(daemon_type), 0
        version = cd.Monitoring.get_version(ctx, 'container_id', daemon_type)
        assert version == '0.16.1'

        # binary `prometheus-alertmanager`
        _call.side_effect = (
            ('', '', 1),
            ('', '{}, version 0.16.1'.format(daemon_type), 0),
        )
        version = cd.Monitoring.get_version(ctx, 'container_id', daemon_type)
        assert version == '0.16.1'

    @mock.patch('stoneadm.call')
    def test_get_version_prometheus(self, _call):
        ctx = cd.StoneadmContext()
        ctx.container_engine = mock_podman()
        daemon_type = 'prometheus'
        _call.return_value = '', '{}, version 0.16.1'.format(daemon_type), 0
        version = cd.Monitoring.get_version(ctx, 'container_id', daemon_type)
        assert version == '0.16.1'

    @mock.patch('stoneadm.call')
    def test_get_version_node_exporter(self, _call):
        ctx = cd.StoneadmContext()
        ctx.container_engine = mock_podman()
        daemon_type = 'node-exporter'
        _call.return_value = '', '{}, version 0.16.1'.format(daemon_type.replace('-', '_')), 0
        version = cd.Monitoring.get_version(ctx, 'container_id', daemon_type)
        assert version == '0.16.1'

    def test_create_daemon_dirs_prometheus(self, stoneadm_fs):
        """
        Ensures the required and optional files given in the configuration are
        created and mapped correctly inside the container. Tests absolute and
        relative file paths given in the configuration.
        """

        fsid = 'aaf5a720-13fe-4a3b-82b9-2d99b7fd9704'
        daemon_type = 'prometheus'
        uid, gid = 50, 50
        daemon_id = 'home'
        ctx = cd.StoneadmContext()
        ctx.data_dir = '/somedir'
        ctx.config_json = json.dumps({
            'files': {
                'prometheus.yml': 'foo',
                '/etc/prometheus/alerting/stone_alerts.yml': 'bar'
            }
        })

        cd.create_daemon_dirs(ctx,
                              fsid,
                              daemon_type,
                              daemon_id,
                              uid,
                              gid,
                              config=None,
                              keyring=None)

        prefix = '{data_dir}/{fsid}/{daemon_type}.{daemon_id}'.format(
            data_dir=ctx.data_dir,
            fsid=fsid,
            daemon_type=daemon_type,
            daemon_id=daemon_id
        )

        expected = {
            'etc/prometheus/prometheus.yml': 'foo',
            'etc/prometheus/alerting/stone_alerts.yml': 'bar',
        }

        for file,content in expected.items():
            file = os.path.join(prefix, file)
            assert os.path.exists(file)
            with open(file) as f:
                assert f.read() == content

        # assert uid/gid after redeploy
        new_uid = uid+1
        new_gid = gid+1
        cd.create_daemon_dirs(ctx,
                              fsid,
                              daemon_type,
                              daemon_id,
                              new_uid,
                              new_gid,
                              config=None,
                              keyring=None)
        for file,content in expected.items():
            file = os.path.join(prefix, file)
            assert os.stat(file).st_uid == new_uid
            assert os.stat(file).st_gid == new_gid


class TestBootstrap(object):

    @staticmethod
    def _get_cmd(*args):
        return [
            'bootstrap',
            '--allow-mismatched-release',
            '--skip-prepare-host',
            '--skip-dashboard',
            *args,
        ]


###############################################3

    def test_config(self, stoneadm_fs):
        conf_file = 'foo'
        cmd = self._get_cmd(
            '--mon-ip', '192.168.1.1',
            '--skip-mon-network',
            '--config', conf_file,
        )

        with with_stoneadm_ctx(cmd) as ctx:
            msg = r'No such file or directory'
            with pytest.raises(cd.Error, match=msg):
                cd.command_bootstrap(ctx)

        stoneadm_fs.create_file(conf_file)
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_bootstrap(ctx)
            assert retval == 0

    def test_no_mon_addr(self, stoneadm_fs):
        cmd = self._get_cmd()
        with with_stoneadm_ctx(cmd) as ctx:
            msg = r'must specify --mon-ip or --mon-addrv'
            with pytest.raises(cd.Error, match=msg):
                cd.command_bootstrap(ctx)

    def test_skip_mon_network(self, stoneadm_fs):
        cmd = self._get_cmd('--mon-ip', '192.168.1.1')

        with with_stoneadm_ctx(cmd, list_networks={}) as ctx:
            msg = r'--skip-mon-network'
            with pytest.raises(cd.Error, match=msg):
                cd.command_bootstrap(ctx)

        cmd += ['--skip-mon-network']
        with with_stoneadm_ctx(cmd, list_networks={}) as ctx:
            retval = cd.command_bootstrap(ctx)
            assert retval == 0

    @pytest.mark.parametrize('mon_ip, list_networks, result',
        [
            # IPv4
            (
                'eth0',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                False,
            ),
            (
                '0.0.0.0',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                False,
            ),
            (
                '192.168.1.0',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                False,
            ),
            (
                '192.168.1.1',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                True,
            ),
            (
                '192.168.1.1:1234',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                True,
            ),
            (
                '192.168.1.1:0123',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                True,
            ),
            # IPv6
            (
                '::',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                False,
            ),
            (
                '::ffff:192.168.1.0',
                {"ffff::/64": {"eth0": ["::ffff:c0a8:101"]}},
                False,
            ),
            (
                '::ffff:192.168.1.1',
                {"ffff::/64": {"eth0": ["::ffff:c0a8:101"]}},
                True,
            ),
            (
                '::ffff:c0a8:101',
                {"ffff::/64": {"eth0": ["::ffff:c0a8:101"]}},
                True,
            ),
            (
                '[::ffff:c0a8:101]:1234',
                {"ffff::/64": {"eth0": ["::ffff:c0a8:101"]}},
                True,
            ),
            (
                '[::ffff:c0a8:101]:0123',
                {"ffff::/64": {"eth0": ["::ffff:c0a8:101"]}},
                True,
            ),
            (
                '0000:0000:0000:0000:0000:FFFF:C0A8:0101',
                {"ffff::/64": {"eth0": ["::ffff:c0a8:101"]}},
                True,
            ),
        ])
    def test_mon_ip(self, mon_ip, list_networks, result, stoneadm_fs):
        cmd = self._get_cmd('--mon-ip', mon_ip)
        if not result:
            with with_stoneadm_ctx(cmd, list_networks=list_networks) as ctx:
                msg = r'--skip-mon-network'
                with pytest.raises(cd.Error, match=msg):
                    cd.command_bootstrap(ctx)
        else:
            with with_stoneadm_ctx(cmd, list_networks=list_networks) as ctx:
                retval = cd.command_bootstrap(ctx)
                assert retval == 0

    @pytest.mark.parametrize('mon_addrv, list_networks, err',
        [
            # IPv4
            (
                '192.168.1.1',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                r'must use square backets',
            ),
            (
                '[192.168.1.1]',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                r'must include port number',
            ),
            (
                '[192.168.1.1:1234]',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                None,
            ),
            (
                '[192.168.1.1:0123]',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                None,
            ),
            (
                '[v2:192.168.1.1:3300,v1:192.168.1.1:6789]',
                {'192.168.1.0/24': {'eth0': ['192.168.1.1']}},
                None,
            ),
            # IPv6
            (
                '[::ffff:192.168.1.1:1234]',
                {'ffff::/64': {'eth0': ['::ffff:c0a8:101']}},
                None,
            ),
            (
                '[::ffff:192.168.1.1:0123]',
                {'ffff::/64': {'eth0': ['::ffff:c0a8:101']}},
                None,
            ),
            (
                '[0000:0000:0000:0000:0000:FFFF:C0A8:0101:1234]',
                {'ffff::/64': {'eth0': ['::ffff:c0a8:101']}},
                None,
            ),
            (
                '[v2:0000:0000:0000:0000:0000:FFFF:C0A8:0101:3300,v1:0000:0000:0000:0000:0000:FFFF:C0A8:0101:6789]',
                {'ffff::/64': {'eth0': ['::ffff:c0a8:101']}},
                None,
            ),
        ])
    def test_mon_addrv(self, mon_addrv, list_networks, err, stoneadm_fs):
        cmd = self._get_cmd('--mon-addrv', mon_addrv)
        if err:
            with with_stoneadm_ctx(cmd, list_networks=list_networks) as ctx:
                with pytest.raises(cd.Error, match=err):
                    cd.command_bootstrap(ctx)
        else:
            with with_stoneadm_ctx(cmd, list_networks=list_networks) as ctx:
                retval = cd.command_bootstrap(ctx)
                assert retval == 0

    def test_allow_fqdn_hostname(self, stoneadm_fs):
        hostname = 'foo.bar'
        cmd = self._get_cmd(
            '--mon-ip', '192.168.1.1',
            '--skip-mon-network',
        )

        with with_stoneadm_ctx(cmd, hostname=hostname) as ctx:
            msg = r'--allow-fqdn-hostname'
            with pytest.raises(cd.Error, match=msg):
                cd.command_bootstrap(ctx)

        cmd += ['--allow-fqdn-hostname']
        with with_stoneadm_ctx(cmd, hostname=hostname) as ctx:
            retval = cd.command_bootstrap(ctx)
            assert retval == 0

    @pytest.mark.parametrize('fsid, err',
        [
            ('', None),
            ('00000000-0000-0000-0000-0000deadbeef', None),
            ('00000000-0000-0000-0000-0000deadbeez', 'not an fsid'),
        ])
    def test_fsid(self, fsid, err, stoneadm_fs):
        cmd = self._get_cmd(
            '--mon-ip', '192.168.1.1',
            '--skip-mon-network',
            '--fsid', fsid,
        )

        with with_stoneadm_ctx(cmd) as ctx:
            if err:
                with pytest.raises(cd.Error, match=err):
                    cd.command_bootstrap(ctx)
            else:
                retval = cd.command_bootstrap(ctx)
                assert retval == 0


class TestShell(object):

    def test_fsid(self, stoneadm_fs):
        fsid = '00000000-0000-0000-0000-0000deadbeef'

        cmd = ['shell', '--fsid', fsid]
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.fsid == fsid

        cmd = ['shell', '--fsid', '00000000-0000-0000-0000-0000deadbeez']
        with with_stoneadm_ctx(cmd) as ctx:
            err = 'not an fsid'
            with pytest.raises(cd.Error, match=err):
                retval = cd.command_shell(ctx)
                assert retval == 1
                assert ctx.fsid == None

        s = get_stone_conf(fsid=fsid)
        f = stoneadm_fs.create_file('stone.conf', contents=s)

        cmd = ['shell', '--fsid', fsid, '--config', f.path]
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.fsid == fsid

        cmd = ['shell', '--fsid', '10000000-0000-0000-0000-0000deadbeef', '--config', f.path]
        with with_stoneadm_ctx(cmd) as ctx:
            err = 'fsid does not match stone.conf'
            with pytest.raises(cd.Error, match=err):
                retval = cd.command_shell(ctx)
                assert retval == 1
                assert ctx.fsid == None

    def test_name(self, stoneadm_fs):
        cmd = ['shell', '--name', 'foo']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0

        cmd = ['shell', '--name', 'foo.bar']
        with with_stoneadm_ctx(cmd) as ctx:
            err = r'must pass --fsid'
            with pytest.raises(cd.Error, match=err):
                retval = cd.command_shell(ctx)
                assert retval == 1

        fsid = '00000000-0000-0000-0000-0000deadbeef'
        cmd = ['shell', '--name', 'foo.bar', '--fsid', fsid]
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0

    def test_config(self, stoneadm_fs):
        cmd = ['shell']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.config == None

        stoneadm_fs.create_file(cd.SHELL_DEFAULT_CONF)
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.config == cd.SHELL_DEFAULT_CONF

        cmd = ['shell', '--config', 'foo']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.config == 'foo'

    def test_keyring(self, stoneadm_fs):
        cmd = ['shell']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.keyring == None

        stoneadm_fs.create_file(cd.SHELL_DEFAULT_KEYRING)
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.keyring == cd.SHELL_DEFAULT_KEYRING

        cmd = ['shell', '--keyring', 'foo']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert ctx.keyring == 'foo'

    @mock.patch('stoneadm.StoneContainer')
    def test_mount_no_dst(self, m_stone_container, stoneadm_fs):
        cmd = ['shell', '--mount', '/etc/foo']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert m_stone_container.call_args.kwargs['volume_mounts']['/etc/foo'] == '/mnt/foo'

    @mock.patch('stoneadm.StoneContainer')
    def test_mount_with_dst_no_opt(self, m_stone_container, stoneadm_fs):
        cmd = ['shell', '--mount', '/etc/foo:/opt/foo/bar']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert m_stone_container.call_args.kwargs['volume_mounts']['/etc/foo'] == '/opt/foo/bar'

    @mock.patch('stoneadm.StoneContainer')
    def test_mount_with_dst_and_opt(self, m_stone_container, stoneadm_fs):
        cmd = ['shell', '--mount', '/etc/foo:/opt/foo/bar:Z']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_shell(ctx)
            assert retval == 0
            assert m_stone_container.call_args.kwargs['volume_mounts']['/etc/foo'] == '/opt/foo/bar:Z'

class TestStoneVolume(object):

    @staticmethod
    def _get_cmd(*args):
        return [
            'stone-volume',
            *args,
            '--', 'inventory', '--format', 'json'
        ]

    def test_noop(self, stoneadm_fs):
        cmd = self._get_cmd()
        with with_stoneadm_ctx(cmd) as ctx:
            cd.command_stone_volume(ctx)
            assert ctx.fsid == None
            assert ctx.config == None
            assert ctx.keyring == None
            assert ctx.config_json == None

    def test_fsid(self, stoneadm_fs):
        fsid = '00000000-0000-0000-0000-0000deadbeef'

        cmd = self._get_cmd('--fsid', fsid)
        with with_stoneadm_ctx(cmd) as ctx:
            cd.command_stone_volume(ctx)
            assert ctx.fsid == fsid

        cmd = self._get_cmd('--fsid', '00000000-0000-0000-0000-0000deadbeez')
        with with_stoneadm_ctx(cmd) as ctx:
            err = 'not an fsid'
            with pytest.raises(cd.Error, match=err):
                retval = cd.command_shell(ctx)
                assert retval == 1
                assert ctx.fsid == None

        s = get_stone_conf(fsid=fsid)
        f = stoneadm_fs.create_file('stone.conf', contents=s)

        cmd = self._get_cmd('--fsid', fsid, '--config', f.path)
        with with_stoneadm_ctx(cmd) as ctx:
            cd.command_stone_volume(ctx)
            assert ctx.fsid == fsid

        cmd = self._get_cmd('--fsid', '10000000-0000-0000-0000-0000deadbeef', '--config', f.path)
        with with_stoneadm_ctx(cmd) as ctx:
            err = 'fsid does not match stone.conf'
            with pytest.raises(cd.Error, match=err):
                cd.command_stone_volume(ctx)
                assert ctx.fsid == None

    def test_config(self, stoneadm_fs):
        cmd = self._get_cmd('--config', 'foo')
        with with_stoneadm_ctx(cmd) as ctx:
            err = r'No such file or directory'
            with pytest.raises(cd.Error, match=err):
                cd.command_stone_volume(ctx)

        stoneadm_fs.create_file('bar')
        cmd = self._get_cmd('--config', 'bar')
        with with_stoneadm_ctx(cmd) as ctx:
            cd.command_stone_volume(ctx)
            assert ctx.config == 'bar'

    def test_keyring(self, stoneadm_fs):
        cmd = self._get_cmd('--keyring', 'foo')
        with with_stoneadm_ctx(cmd) as ctx:
            err = r'No such file or directory'
            with pytest.raises(cd.Error, match=err):
                cd.command_stone_volume(ctx)

        stoneadm_fs.create_file('bar')
        cmd = self._get_cmd('--keyring', 'bar')
        with with_stoneadm_ctx(cmd) as ctx:
            cd.command_stone_volume(ctx)
            assert ctx.keyring == 'bar'


class TestIscsi:
    def test_unit_run(self, stoneadm_fs):
        fsid = '9b9d7609-f4d5-4aba-94c8-effa764d96c9'
        config_json = {
                'files': {'iscsi-gateway.cfg': ''}
            }
        with with_stoneadm_ctx(['--image=stone/stone'], list_networks={}) as ctx:
            import json
            ctx.config_json = json.dumps(config_json)
            ctx.fsid = fsid
            cd.get_parm.return_value = config_json
            c = cd.get_container(ctx, fsid, 'iscsi', 'daemon_id')

            cd.make_data_dir(ctx, fsid, 'iscsi', 'daemon_id')
            cd.deploy_daemon_units(
                ctx,
                fsid,
                0, 0,
                'iscsi',
                'daemon_id',
                c,
                True, True
            )

            with open('/var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/unit.run') as f:
                assert f.read() == """set -e
if ! grep -qs /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/configfs /proc/mounts; then mount -t configfs none /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/configfs; fi
# iscsi tcmu-runner container
! /usr/bin/podman rm -f stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi.daemon_id-tcmu 2> /dev/null
! /usr/bin/podman rm -f stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi-daemon_id-tcmu 2> /dev/null
/usr/bin/podman run --rm --ipc=host --stop-signal=SIGTERM --net=host --entrypoint /usr/bin/tcmu-runner --privileged --group-add=disk --init --name stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi-daemon_id-tcmu -e CONTAINER_IMAGE=stone/stone -e NODE_NAME=host1 -e STONE_USE_RANDOM_NONCE=1 -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/config:/etc/stone/stone.conf:z -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/keyring:/etc/stone/keyring:z -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/iscsi-gateway.cfg:/etc/stone/iscsi-gateway.cfg:z -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/configfs:/sys/kernel/config -v /var/log/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9:/var/log:z -v /dev:/dev --mount type=bind,source=/lib/modules,destination=/lib/modules,ro=true stone/stone &
# iscsi.daemon_id
! /usr/bin/podman rm -f stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi.daemon_id 2> /dev/null
! /usr/bin/podman rm -f stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi-daemon_id 2> /dev/null
/usr/bin/podman run --rm --ipc=host --stop-signal=SIGTERM --net=host --entrypoint /usr/bin/rbd-target-api --privileged --group-add=disk --init --name stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi-daemon_id -e CONTAINER_IMAGE=stone/stone -e NODE_NAME=host1 -e STONE_USE_RANDOM_NONCE=1 -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/config:/etc/stone/stone.conf:z -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/keyring:/etc/stone/keyring:z -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/iscsi-gateway.cfg:/etc/stone/iscsi-gateway.cfg:z -v /var/lib/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9/iscsi.daemon_id/configfs:/sys/kernel/config -v /var/log/stone/9b9d7609-f4d5-4aba-94c8-effa764d96c9:/var/log:z -v /dev:/dev --mount type=bind,source=/lib/modules,destination=/lib/modules,ro=true stone/stone
"""

    def test_get_container(self):
        """
        Due to a combination of socket.getfqdn() and podman's behavior to
        add the container name into the /etc/hosts file, we cannot use periods
        in container names. But we need to be able to detect old existing containers.
        Assert this behaviour. I think we can remove this in Stone R
        """
        fsid = '9b9d7609-f4d5-4aba-94c8-effa764d96c9'
        with with_stoneadm_ctx(['--image=stone/stone'], list_networks={}) as ctx:
            ctx.fsid = fsid
            c = cd.get_container(ctx, fsid, 'iscsi', 'something')
            assert c.cname == 'stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi-something'
            assert c.old_cname == 'stone-9b9d7609-f4d5-4aba-94c8-effa764d96c9-iscsi.something'


class TestCheckHost:

    @mock.patch('stoneadm.find_executable', return_value='foo')
    @mock.patch('stoneadm.check_time_sync', return_value=True)
    def test_container_engine(self, find_executable, check_time_sync):
        ctx = cd.StoneadmContext()

        ctx.container_engine = None
        err = r'No container engine binary found'
        with pytest.raises(cd.Error, match=err):
            cd.command_check_host(ctx)

        ctx.container_engine = mock_podman()
        cd.command_check_host(ctx)

        ctx.container_engine = mock_docker()
        cd.command_check_host(ctx)


class TestRmRepo:

    @pytest.mark.parametrize('os_release',
        [
            # Apt
            dedent("""
            NAME="Ubuntu"
            VERSION="20.04 LTS (Focal Fossa)"
            ID=ubuntu
            ID_LIKE=debian
            PRETTY_NAME="Ubuntu 20.04 LTS"
            VERSION_ID="20.04"
            HOME_URL="https://www.ubuntu.com/"
            SUPPORT_URL="https://help.ubuntu.com/"
            BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
            PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
            VERSION_CODENAME=focal
            UBUNTU_CODENAME=focal
            """),

            # YumDnf
            dedent("""
            NAME="CentOS Linux"
            VERSION="8 (Core)"
            ID="centos"
            ID_LIKE="rhel fedora"
            VERSION_ID="8"
            PLATFORM_ID="platform:el8"
            PRETTY_NAME="CentOS Linux 8 (Core)"
            ANSI_COLOR="0;31"
            CPE_NAME="cpe:/o:centos:centos:8"
            HOME_URL="https://www.centos.org/"
            BUG_REPORT_URL="https://bugs.centos.org/"

            CENTOS_MANTISBT_PROJECT="CentOS-8"
            CENTOS_MANTISBT_PROJECT_VERSION="8"
            REDHAT_SUPPORT_PRODUCT="centos"
            REDHAT_SUPPORT_PRODUCT_VERSION="8"
            """),

            # Zypper
            dedent("""
            NAME="openSUSE Tumbleweed"
            # VERSION="20210810"
            ID="opensuse-tumbleweed"
            ID_LIKE="opensuse suse"
            VERSION_ID="20210810"
            PRETTY_NAME="openSUSE Tumbleweed"
            ANSI_COLOR="0;32"
            CPE_NAME="cpe:/o:opensuse:tumbleweed:20210810"
            BUG_REPORT_URL="https://bugs.opensuse.org"
            HOME_URL="https://www.opensuse.org/"
            DOCUMENTATION_URL="https://en.opensuse.org/Portal:Tumbleweed"
            LOGO="distributor-logo"
            """),
        ])
    @mock.patch('stoneadm.find_executable', return_value='foo')
    def test_container_engine(self, find_executable, os_release, stoneadm_fs):
        stoneadm_fs.create_file('/etc/os-release', contents=os_release)
        ctx = cd.StoneadmContext()

        ctx.container_engine = None
        cd.command_rm_repo(ctx)

        ctx.container_engine = mock_podman()
        cd.command_rm_repo(ctx)

        ctx.container_engine = mock_docker()
        cd.command_rm_repo(ctx)


class TestValidateRepo:

    @pytest.mark.parametrize('values',
        [
            # Apt - no checks
            dict(
            version="",
            release="pacific",
            err_text="",
            os_release=dedent("""
            NAME="Ubuntu"
            VERSION="20.04 LTS (Focal Fossa)"
            ID=ubuntu
            ID_LIKE=debian
            PRETTY_NAME="Ubuntu 20.04 LTS"
            VERSION_ID="20.04"
            HOME_URL="https://www.ubuntu.com/"
            SUPPORT_URL="https://help.ubuntu.com/"
            BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
            PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
            VERSION_CODENAME=focal
            UBUNTU_CODENAME=focal
            """)),

            # YumDnf on Centos8 - OK
            dict(
            version="",
            release="pacific",
            err_text="",
            os_release=dedent("""
            NAME="CentOS Linux"
            VERSION="8 (Core)"
            ID="centos"
            ID_LIKE="rhel fedora"
            VERSION_ID="8"
            PLATFORM_ID="platform:el8"
            PRETTY_NAME="CentOS Linux 8 (Core)"
            ANSI_COLOR="0;31"
            CPE_NAME="cpe:/o:centos:centos:8"
            HOME_URL="https://www.centos.org/"
            BUG_REPORT_URL="https://bugs.centos.org/"

            CENTOS_MANTISBT_PROJECT="CentOS-8"
            CENTOS_MANTISBT_PROJECT_VERSION="8"
            REDHAT_SUPPORT_PRODUCT="centos"
            REDHAT_SUPPORT_PRODUCT_VERSION="8"
            """)),

            # YumDnf on Fedora - Fedora not supported
            dict(
            version="",
            release="pacific",
            err_text="does not build Fedora",
            os_release=dedent("""
            NAME="Fedora Linux"
            VERSION="35 (Cloud Edition)"
            ID=fedora
            VERSION_ID=35
            VERSION_CODENAME=""
            PLATFORM_ID="platform:f35"
            PRETTY_NAME="Fedora Linux 35 (Cloud Edition)"
            ANSI_COLOR="0;38;2;60;110;180"
            LOGO=fedora-logo-icon
            CPE_NAME="cpe:/o:fedoraproject:fedora:35"
            HOME_URL="https://fedoraproject.org/"
            DOCUMENTATION_URL="https://docs.fedoraproject.org/en-US/fedora/f35/system-administrators-guide/"
            SUPPORT_URL="https://ask.fedoraproject.org/"
            BUG_REPORT_URL="https://bugzilla.redhat.com/"
            REDHAT_BUGZILLA_PRODUCT="Fedora"
            REDHAT_BUGZILLA_PRODUCT_VERSION=35
            REDHAT_SUPPORT_PRODUCT="Fedora"
            REDHAT_SUPPORT_PRODUCT_VERSION=35
            PRIVACY_POLICY_URL="https://fedoraproject.org/wiki/Legal:PrivacyPolicy"
            VARIANT="Cloud Edition"
            VARIANT_ID=cloud
            """)),

            # YumDnf on Centos 7 - no pacific
            dict(
            version="",
            release="pacific",
            err_text="does not support pacific",
            os_release=dedent("""
            NAME="CentOS Linux"
            VERSION="7 (Core)"
            ID="centos"
            ID_LIKE="rhel fedora"
            VERSION_ID="7"
            PRETTY_NAME="CentOS Linux 7 (Core)"
            ANSI_COLOR="0;31"
            CPE_NAME="cpe:/o:centos:centos:7"
            HOME_URL="https://www.centos.org/"
            BUG_REPORT_URL="https://bugs.centos.org/"

            CENTOS_MANTISBT_PROJECT="CentOS-7"
            CENTOS_MANTISBT_PROJECT_VERSION="7"
            REDHAT_SUPPORT_PRODUCT="centos"
            REDHAT_SUPPORT_PRODUCT_VERSION="7"
            """)),

            # YumDnf on Centos 7 - nothing after pacific
            dict(
            version="",
            release="zillions",
            err_text="does not support pacific",
            os_release=dedent("""
            NAME="CentOS Linux"
            VERSION="7 (Core)"
            ID="centos"
            ID_LIKE="rhel fedora"
            VERSION_ID="7"
            PRETTY_NAME="CentOS Linux 7 (Core)"
            ANSI_COLOR="0;31"
            CPE_NAME="cpe:/o:centos:centos:7"
            HOME_URL="https://www.centos.org/"
            BUG_REPORT_URL="https://bugs.centos.org/"

            CENTOS_MANTISBT_PROJECT="CentOS-7"
            CENTOS_MANTISBT_PROJECT_VERSION="7"
            REDHAT_SUPPORT_PRODUCT="centos"
            REDHAT_SUPPORT_PRODUCT_VERSION="7"
            """)),

            # YumDnf on Centos 7 - nothing v16 or higher
            dict(
            version="v16.1.3",
            release="",
            err_text="does not support",
            os_release=dedent("""
            NAME="CentOS Linux"
            VERSION="7 (Core)"
            ID="centos"
            ID_LIKE="rhel fedora"
            VERSION_ID="7"
            PRETTY_NAME="CentOS Linux 7 (Core)"
            ANSI_COLOR="0;31"
            CPE_NAME="cpe:/o:centos:centos:7"
            HOME_URL="https://www.centos.org/"
            BUG_REPORT_URL="https://bugs.centos.org/"

            CENTOS_MANTISBT_PROJECT="CentOS-7"
            CENTOS_MANTISBT_PROJECT_VERSION="7"
            REDHAT_SUPPORT_PRODUCT="centos"
            REDHAT_SUPPORT_PRODUCT_VERSION="7"
            """)),
        ])
    @mock.patch('stoneadm.find_executable', return_value='foo')
    def test_distro_validation(self, find_executable, values, stoneadm_fs):
        os_release = values['os_release']
        release = values['release']
        version = values['version']
        err_text = values['err_text']

        stoneadm_fs.create_file('/etc/os-release', contents=os_release)
        ctx = cd.StoneadmContext()
        ctx.repo_url = 'http://localhost'
        pkg = cd.create_packager(ctx, stable=release, version=version)

        if err_text:
            with pytest.raises(cd.Error, match=err_text):
                pkg.validate()
        else:
            with mock.patch('stoneadm.urlopen', return_value=None):
                pkg.validate()

    @pytest.mark.parametrize('values',
        [
            # Apt - not checked
            dict(
            version="",
            release="pacific",
            err_text="",
            os_release=dedent("""
            NAME="Ubuntu"
            VERSION="20.04 LTS (Focal Fossa)"
            ID=ubuntu
            ID_LIKE=debian
            PRETTY_NAME="Ubuntu 20.04 LTS"
            VERSION_ID="20.04"
            HOME_URL="https://www.ubuntu.com/"
            SUPPORT_URL="https://help.ubuntu.com/"
            BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
            PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
            VERSION_CODENAME=focal
            UBUNTU_CODENAME=focal
            """)),

            # YumDnf on Centos8 - force failure
            dict(
            version="",
            release="foobar",
            err_text="failed to fetch repository metadata",
            os_release=dedent("""
            NAME="CentOS Linux"
            VERSION="8 (Core)"
            ID="centos"
            ID_LIKE="rhel fedora"
            VERSION_ID="8"
            PLATFORM_ID="platform:el8"
            PRETTY_NAME="CentOS Linux 8 (Core)"
            ANSI_COLOR="0;31"
            CPE_NAME="cpe:/o:centos:centos:8"
            HOME_URL="https://www.centos.org/"
            BUG_REPORT_URL="https://bugs.centos.org/"

            CENTOS_MANTISBT_PROJECT="CentOS-8"
            CENTOS_MANTISBT_PROJECT_VERSION="8"
            REDHAT_SUPPORT_PRODUCT="centos"
            REDHAT_SUPPORT_PRODUCT_VERSION="8"
            """)),
        ])
    @mock.patch('stoneadm.find_executable', return_value='foo')
    def test_http_validation(self, find_executable, values, stoneadm_fs):
        from urllib.error import HTTPError

        os_release = values['os_release']
        release = values['release']
        version = values['version']
        err_text = values['err_text']

        stoneadm_fs.create_file('/etc/os-release', contents=os_release)
        ctx = cd.StoneadmContext()
        ctx.repo_url = 'http://localhost'
        pkg = cd.create_packager(ctx, stable=release, version=version)

        with mock.patch('stoneadm.urlopen') as _urlopen:
            _urlopen.side_effect = HTTPError(ctx.repo_url, 404, "not found", None, fp=None)
            if err_text:
                with pytest.raises(cd.Error, match=err_text):
                    pkg.validate()
            else:
                pkg.validate()


class TestPull:

    @mock.patch('time.sleep')
    @mock.patch('stoneadm.call', return_value=('', '', 0))
    @mock.patch('stoneadm.get_image_info_from_inspect', return_value={})
    def test_error(self, get_image_info_from_inspect, call, sleep):
        ctx = cd.StoneadmContext()
        ctx.container_engine = mock_podman()
        ctx.insecure = False

        call.return_value = ('', '', 0)
        retval = cd.command_pull(ctx)
        assert retval == 0

        err = 'maximum retries reached'

        call.return_value = ('', 'foobar', 1)
        with pytest.raises(cd.Error) as e:
            cd.command_pull(ctx)
        assert err not in str(e.value)

        call.return_value = ('', 'net/http: TLS handshake timeout', 1)
        with pytest.raises(cd.Error) as e:
            cd.command_pull(ctx)
        assert err in str(e.value)

    @mock.patch('stoneadm.logger')
    @mock.patch('stoneadm.get_image_info_from_inspect', return_value={})
    @mock.patch('stoneadm.get_last_local_stone_image', return_value='last_local_stone_image')
    def test_image(self, get_last_local_stone_image, get_image_info_from_inspect, logger):
        cmd = ['pull']
        with with_stoneadm_ctx(cmd) as ctx:
            retval = cd.command_pull(ctx)
            assert retval == 0
            assert ctx.image == cd.DEFAULT_IMAGE

        with mock.patch.dict(os.environ, {"STONEADM_IMAGE": 'stoneadm_image_environ'}):
            cmd = ['pull']
            with with_stoneadm_ctx(cmd) as ctx:
                retval = cd.command_pull(ctx)
                assert retval == 0
                assert ctx.image == 'stoneadm_image_environ'

            cmd = ['--image',  'stoneadm_image_param', 'pull']
            with with_stoneadm_ctx(cmd) as ctx:
                retval = cd.command_pull(ctx)
                assert retval == 0
                assert ctx.image == 'stoneadm_image_param'


class TestApplySpec:

    def test_parse_yaml(self, stoneadm_fs):
        yaml = '''service_type: host
hostname: vm-00
addr: 192.168.122.44
labels:
 - example1
 - example2
---
service_type: host
hostname: vm-01
addr: 192.168.122.247
labels:
 - grafana
---
service_type: host
hostname: vm-02
addr: 192.168.122.165'''

        stoneadm_fs.create_file('spec.yml', contents=yaml)

        retdic = [{'service_type': 'host', 'hostname': 'vm-00', 'addr': '192.168.122.44', 'labels': '- example1- example2'},
                  {'service_type': 'host', 'hostname': 'vm-01', 'addr': '192.168.122.247', 'labels': '- grafana'},
                  {'service_type': 'host', 'hostname': 'vm-02', 'addr': '192.168.122.165'}]

        with open('spec.yml') as f:
            dic = cd.parse_yaml_objs(f)
            assert dic == retdic

    @mock.patch('stoneadm.call', return_value=('', '', 0))
    def test_distribute_ssh_keys(self, call):
        ctx = cd.StoneadmContext()
        ctx.ssh_public_key = None
        ctx.ssh_user = 'root'

        host_spec = {'service_type': 'host', 'hostname': 'vm-02', 'addr': '192.168.122.165'}

        retval = cd._distribute_ssh_keys(ctx, host_spec, 'bootstrap_hostname')

        assert retval == 0

        call.return_value = ('', '', 1)

        retval = cd._distribute_ssh_keys(ctx, host_spec, 'bootstrap_hostname')

        assert retval == 1


class TestSNMPGateway:
    V2c_config = {
        'snmp_community': 'public',
        'destination': '192.168.1.10:162',
        'snmp_version': 'V2c',
    }
    V3_no_priv_config = {
        'destination': '192.168.1.10:162',
        'snmp_version': 'V3',
        'snmp_v3_auth_username': 'myuser',
        'snmp_v3_auth_password': 'mypassword',
        'snmp_v3_auth_protocol': 'SHA',
        'snmp_v3_engine_id': '8000C53F00000000',
    }
    V3_priv_config = {
        'destination': '192.168.1.10:162',
        'snmp_version': 'V3',
        'snmp_v3_auth_username': 'myuser',
        'snmp_v3_auth_password': 'mypassword',
        'snmp_v3_auth_protocol': 'SHA',
        'snmp_v3_priv_protocol': 'DES',
        'snmp_v3_priv_password': 'mysecret',
        'snmp_v3_engine_id': '8000C53F00000000',
    }
    no_destination_config = {
        'snmp_version': 'V3',
        'snmp_v3_auth_username': 'myuser',
        'snmp_v3_auth_password': 'mypassword',
        'snmp_v3_auth_protocol': 'SHA',
        'snmp_v3_priv_protocol': 'DES',
        'snmp_v3_priv_password': 'mysecret',
        'snmp_v3_engine_id': '8000C53F00000000',
    }
    bad_version_config = {
        'snmp_community': 'public',
        'destination': '192.168.1.10:162',
        'snmp_version': 'V1',
    }

    def test_unit_run_V2c(self, stoneadm_fs):
        fsid = 'ca734440-3dc6-11ec-9b98-5254002537a6'
        with with_stoneadm_ctx(['--image=docker.io/maxwo/snmp-notifier:v1.2.1'], list_networks={}) as ctx:
            import json
            ctx.config_json = json.dumps(self.V2c_config)
            ctx.fsid = fsid
            ctx.tcp_ports = '9464'
            cd.get_parm.return_value = self.V2c_config
            c = cd.get_container(ctx, fsid, 'snmp-gateway', 'daemon_id')

            cd.make_data_dir(ctx, fsid, 'snmp-gateway', 'daemon_id')

            cd.create_daemon_dirs(ctx, fsid, 'snmp-gateway', 'daemon_id', 0, 0)
            with open(f'/var/lib/stone/{fsid}/snmp-gateway.daemon_id/snmp-gateway.conf', 'r') as f:
                conf = f.read().rstrip()
                assert conf == 'SNMP_NOTIFIER_COMMUNITY=public'

            cd.deploy_daemon_units(
                ctx,
                fsid,
                0, 0,
                'snmp-gateway',
                'daemon_id',
                c,
                True, True
            )
            with open(f'/var/lib/stone/{fsid}/snmp-gateway.daemon_id/unit.run', 'r') as f:
                run_cmd = f.readlines()[-1].rstrip()
                assert run_cmd.endswith('docker.io/maxwo/snmp-notifier:v1.2.1 --web.listen-address=:9464 --snmp.destination=192.168.1.10:162 --snmp.version=V2c --log.level=info --snmp.trap-description-template=/etc/snmp_notifier/description-template.tpl')

    def test_unit_run_V3_noPriv(self, stoneadm_fs):
        fsid = 'ca734440-3dc6-11ec-9b98-5254002537a6'
        with with_stoneadm_ctx(['--image=docker.io/maxwo/snmp-notifier:v1.2.1'], list_networks={}) as ctx:
            import json
            ctx.config_json = json.dumps(self.V3_no_priv_config)
            ctx.fsid = fsid
            ctx.tcp_ports = '9465'
            cd.get_parm.return_value = self.V3_no_priv_config
            c = cd.get_container(ctx, fsid, 'snmp-gateway', 'daemon_id')

            cd.make_data_dir(ctx, fsid, 'snmp-gateway', 'daemon_id')

            cd.create_daemon_dirs(ctx, fsid, 'snmp-gateway', 'daemon_id', 0, 0)
            with open(f'/var/lib/stone/{fsid}/snmp-gateway.daemon_id/snmp-gateway.conf', 'r') as f:
                conf = f.read()
                assert conf == 'SNMP_NOTIFIER_AUTH_USERNAME=myuser\nSNMP_NOTIFIER_AUTH_PASSWORD=mypassword\n'

            cd.deploy_daemon_units(
                ctx,
                fsid,
                0, 0,
                'snmp-gateway',
                'daemon_id',
                c,
                True, True
            )
            with open(f'/var/lib/stone/{fsid}/snmp-gateway.daemon_id/unit.run', 'r') as f:
                run_cmd = f.readlines()[-1].rstrip()
                assert run_cmd.endswith('docker.io/maxwo/snmp-notifier:v1.2.1 --web.listen-address=:9465 --snmp.destination=192.168.1.10:162 --snmp.version=V3 --log.level=info --snmp.trap-description-template=/etc/snmp_notifier/description-template.tpl --snmp.authentication-enabled --snmp.authentication-protocol=SHA --snmp.security-engine-id=8000C53F00000000')

    def test_unit_run_V3_Priv(self, stoneadm_fs):
        fsid = 'ca734440-3dc6-11ec-9b98-5254002537a6'
        with with_stoneadm_ctx(['--image=docker.io/maxwo/snmp-notifier:v1.2.1'], list_networks={}) as ctx:
            import json
            ctx.config_json = json.dumps(self.V3_priv_config)
            ctx.fsid = fsid
            ctx.tcp_ports = '9464'
            cd.get_parm.return_value = self.V3_priv_config
            c = cd.get_container(ctx, fsid, 'snmp-gateway', 'daemon_id')

            cd.make_data_dir(ctx, fsid, 'snmp-gateway', 'daemon_id')

            cd.create_daemon_dirs(ctx, fsid, 'snmp-gateway', 'daemon_id', 0, 0)
            with open(f'/var/lib/stone/{fsid}/snmp-gateway.daemon_id/snmp-gateway.conf', 'r') as f:
                conf = f.read()
                assert conf == 'SNMP_NOTIFIER_AUTH_USERNAME=myuser\nSNMP_NOTIFIER_AUTH_PASSWORD=mypassword\nSNMP_NOTIFIER_PRIV_PASSWORD=mysecret\n'

            cd.deploy_daemon_units(
                ctx,
                fsid,
                0, 0,
                'snmp-gateway',
                'daemon_id',
                c,
                True, True
            )
            with open(f'/var/lib/stone/{fsid}/snmp-gateway.daemon_id/unit.run', 'r') as f:
                run_cmd = f.readlines()[-1].rstrip()
                assert run_cmd.endswith('docker.io/maxwo/snmp-notifier:v1.2.1 --web.listen-address=:9464 --snmp.destination=192.168.1.10:162 --snmp.version=V3 --log.level=info --snmp.trap-description-template=/etc/snmp_notifier/description-template.tpl --snmp.authentication-enabled --snmp.authentication-protocol=SHA --snmp.security-engine-id=8000C53F00000000 --snmp.private-enabled --snmp.private-protocol=DES')

    def test_unit_run_no_dest(self, stoneadm_fs):
        fsid = 'ca734440-3dc6-11ec-9b98-5254002537a6'
        with with_stoneadm_ctx(['--image=docker.io/maxwo/snmp-notifier:v1.2.1'], list_networks={}) as ctx:
            import json
            ctx.config_json = json.dumps(self.no_destination_config)
            ctx.fsid = fsid
            ctx.tcp_ports = '9464'
            cd.get_parm.return_value = self.no_destination_config

            with pytest.raises(Exception) as e:
                c = cd.get_container(ctx, fsid, 'snmp-gateway', 'daemon_id')
            assert str(e.value) == "config is missing destination attribute(<ip>:<port>) of the target SNMP listener"

    def test_unit_run_bad_version(self, stoneadm_fs):
        fsid = 'ca734440-3dc6-11ec-9b98-5254002537a6'
        with with_stoneadm_ctx(['--image=docker.io/maxwo/snmp-notifier:v1.2.1'], list_networks={}) as ctx:
            import json
            ctx.config_json = json.dumps(self.bad_version_config)
            ctx.fsid = fsid
            ctx.tcp_ports = '9464'
            cd.get_parm.return_value = self.bad_version_config

            with pytest.raises(Exception) as e:
                c = cd.get_container(ctx, fsid, 'snmp-gateway', 'daemon_id')
            assert str(e.value) == 'not a valid snmp version: V1'
