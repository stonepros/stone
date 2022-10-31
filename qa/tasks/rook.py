"""
Rook cluster task
"""
import argparse
import configobj
import contextlib
import json
import logging
import os
import yaml
from io import BytesIO

from tarfile import ReadError
from tasks.stone_manager import StoneManager
from teuthology import misc as teuthology
from teuthology.config import config as teuth_config
from teuthology.contextutil import safe_while
from teuthology.orchestra import run
from teuthology import contextutil
from tasks.stone import healthy
from tasks.stoneadm import update_archive_setting

log = logging.getLogger(__name__)


def _kubectl(ctx, config, args, **kwargs):
    cluster_name = config.get('cluster', 'stone')
    return ctx.rook[cluster_name].remote.run(
        args=['kubectl'] + args,
        **kwargs
    )


def shell(ctx, config):
    """
    Run command(s) inside the rook tools container.

      tasks:
      - kubeadm:
      - rook:
      - rook.shell:
          - stone -s

    or

      tasks:
      - kubeadm:
      - rook:
      - rook.shell:
          commands:
          - stone -s

    """
    if isinstance(config, list):
        config = {'commands': config}
    for cmd in config.get('commands', []):
        if isinstance(cmd, str):
            _shell(ctx, config, cmd.split(' '))
        else:
            _shell(ctx, config, cmd)


def _shell(ctx, config, args, **kwargs):
    cluster_name = config.get('cluster', 'stone')
    return _kubectl(
        ctx, config,
        [
            '-n', 'rook-stone',
            'exec',
            ctx.rook[cluster_name].toolbox, '--'
        ] + args,
        **kwargs
    )


@contextlib.contextmanager
def rook_operator(ctx, config):
    cluster_name = config['cluster']
    rook_branch = config.get('rook_branch', 'master')
    rook_git_url = config.get('rook_git_url', 'https://github.com/rook/rook')

    log.info(f'Cloning {rook_git_url} branch {rook_branch}')
    ctx.rook[cluster_name].remote.run(
        args=[
            'rm', '-rf', 'rook',
            run.Raw('&&'),
            'git',
            'clone',
            '--single-branch',
            '--branch', rook_branch,
            rook_git_url,
            'rook',
        ]
    )

    # operator.yaml
    operator_yaml = ctx.rook[cluster_name].remote.read_file(
        'rook/cluster/examples/kubernetes/stonepros/operator.yaml'
    )
    rook_image = config.get('rook_image')
    if rook_image:
        log.info(f'Patching operator to use image {rook_image}')
        crs = list(yaml.load_all(operator_yaml, Loader=yaml.FullLoader))
        assert len(crs) == 2
        crs[1]['spec']['template']['spec']['containers'][0]['image'] = rook_image
        operator_yaml = yaml.dump_all(crs)
    ctx.rook[cluster_name].remote.write_file('operator.yaml', operator_yaml)

    op_job = None
    try:
        log.info('Deploying operator')
        _kubectl(ctx, config, [
            'create',
            '-f', 'rook/cluster/examples/kubernetes/stonepros/crds.yaml',
            '-f', 'rook/cluster/examples/kubernetes/stonepros/common.yaml',
            '-f', 'operator.yaml',
        ])

        # on centos:
        if teuthology.get_distro(ctx) == 'centos':
            _kubectl(ctx, config, [
                '-n', 'rook-stone',
                'set', 'env', 'deploy/rook-stone-operator',
                'ROOK_HOSTPATH_REQUIRES_PRIVILEGED=true'
            ])

        # wait for operator
        op_name = None
        with safe_while(sleep=10, tries=90, action="wait for operator") as proceed:
            while not op_name and proceed():
                p = _kubectl(
                    ctx, config,
                    ['-n', 'rook-stone', 'get', 'pods', '-l', 'app=rook-stone-operator'],
                    stdout=BytesIO(),
                )
                for line in p.stdout.getvalue().decode('utf-8').strip().splitlines():
                    name, ready, status, _ = line.split(None, 3)
                    if status == 'Running':
                        op_name = name
                        break

        # log operator output
        op_job = _kubectl(
            ctx,
            config,
            ['-n', 'rook-stone', 'logs', '-f', op_name],
            wait=False,
            logger=log.getChild('operator'),
        )

        yield

    except Exception as e:
        log.exception(e)
        raise

    finally:
        log.info('Cleaning up rook operator')
        _kubectl(ctx, config, [
            'delete',
            '-f', 'operator.yaml',
        ])
        if False:
            # don't bother since we'll tear down k8s anyway (and this mysteriously
            # fails sometimes when deleting some of the CRDs... not sure why!)
            _kubectl(ctx, config, [
                'delete',
                '-f', 'rook/cluster/examples/kubernetes/stonepros/common.yaml',
            ])
            _kubectl(ctx, config, [
                'delete',
                '-f', 'rook/cluster/examples/kubernetes/stonepros/crds.yaml',
            ])
        ctx.rook[cluster_name].remote.run(args=['rm', '-rf', 'rook', 'operator.yaml'])
        if op_job:
            op_job.wait()
        run.wait(
            ctx.cluster.run(
                args=[
                    'sudo', 'rm', '-rf', '/var/lib/rook'
                ]
            )
        )


@contextlib.contextmanager
def stone_log(ctx, config):
    cluster_name = config['cluster']

    log_dir = '/var/lib/rook/rook-stone/log'
    update_archive_setting(ctx, 'log', log_dir)

    try:
        yield

    except Exception:
        # we need to know this below
        ctx.summary['success'] = False
        raise

    finally:
        log.info('Checking cluster log for badness...')
        def first_in_stone_log(pattern, excludes):
            """
            Find the first occurrence of the pattern specified in the Stone log,
            Returns None if none found.

            :param pattern: Pattern scanned for.
            :param excludes: Patterns to ignore.
            :return: First line of text (or None if not found)
            """
            args = [
                'sudo',
                'egrep', pattern,
                f'{log_dir}/stone.log',
            ]
            if excludes:
                for exclude in excludes:
                    args.extend([run.Raw('|'), 'egrep', '-v', exclude])
            args.extend([
                run.Raw('|'), 'head', '-n', '1',
            ])
            r = ctx.rook[cluster_name].remote.run(
                stdout=BytesIO(),
                args=args,
            )
            stdout = r.stdout.getvalue().decode()
            if stdout:
                return stdout
            return None

        if first_in_stone_log('\[ERR\]|\[WRN\]|\[SEC\]',
                             config.get('log-ignorelist')) is not None:
            log.warning('Found errors (ERR|WRN|SEC) in cluster log')
            ctx.summary['success'] = False
            # use the most severe problem as the failure reason
            if 'failure_reason' not in ctx.summary:
                for pattern in ['\[SEC\]', '\[ERR\]', '\[WRN\]']:
                    match = first_in_stone_log(pattern, config['log-ignorelist'])
                    if match is not None:
                        ctx.summary['failure_reason'] = \
                            '"{match}" in cluster log'.format(
                                match=match.rstrip('\n'),
                            )
                        break

        if ctx.archive is not None and \
                not (ctx.config.get('archive-on-error') and ctx.summary['success']):
            # and logs
            log.info('Compressing logs...')
            run.wait(
                ctx.cluster.run(
                    args=[
                        'sudo',
                        'find',
                        log_dir,
                        '-name',
                        '*.log',
                        '-print0',
                        run.Raw('|'),
                        'sudo',
                        'xargs',
                        '-0',
                        '--no-run-if-empty',
                        '--',
                        'gzip',
                        '--',
                    ],
                    wait=False,
                ),
            )

            log.info('Archiving logs...')
            path = os.path.join(ctx.archive, 'remote')
            try:
                os.makedirs(path)
            except OSError:
                pass
            for remote in ctx.cluster.remotes.keys():
                sub = os.path.join(path, remote.name)
                try:
                    os.makedirs(sub)
                except OSError:
                    pass
                try:
                    teuthology.pull_directory(remote, log_dir,
                                              os.path.join(sub, 'log'))
                except ReadError:
                    pass


def build_initial_config(ctx, config):
    path = os.path.join(os.path.dirname(__file__), 'rook-stone.conf')
    conf = configobj.ConfigObj(path, file_error=True)

    # overrides
    for section, keys in config.get('conf',{}).items():
        for key, value in keys.items():
            log.info(" override: [%s] %s = %s" % (section, key, value))
            if section not in conf:
                conf[section] = {}
            conf[section][key] = value

    return conf


@contextlib.contextmanager
def rook_cluster(ctx, config):
    cluster_name = config['cluster']

    # count how many OSDs we'll create
    num_devs = 0
    num_hosts = 0
    for remote in ctx.cluster.remotes.keys():
        ls = remote.read_file('/scratch_devs').decode('utf-8').strip().splitlines()
        num_devs += len(ls)
        num_hosts += 1
    ctx.rook[cluster_name].num_osds = num_devs

    # config
    config = build_initial_config(ctx, config)
    config_fp = BytesIO()
    config.write(config_fp)
    log.info(f'Config:\n{config_fp.getvalue()}')
    _kubectl(ctx, config, ['create', '-f', '-'], stdin=yaml.dump({
        'apiVersion': 'v1',
        'kind': 'ConfigMap',
        'metadata': {
            'name': 'rook-config-override',
            'namespace': 'rook-stone'},
        'data': {
            'config': config_fp.getvalue()
        }
    }))

    # cluster
    cluster = {
        'apiVersion': 'stone.rook.io/v1',
        'kind': 'StoneCluster',
        'metadata': {'name': 'rook-stone', 'namespace': 'rook-stone'},
        'spec': {
            'stoneVersion': {
                'image': ctx.rook[cluster_name].image,
                'allowUnsupported': True,
            },
            'dataDirHostPath': '/var/lib/rook',
            'skipUpgradeChecks': True,
            'mgr': {
                'count': 1,
                'modules': [
                    { 'name': 'rook', 'enabled': True },
                ],
            },
            'mon': {
                'count': num_hosts,
                'allowMultiplePerNode': True,
            },
            'storage': {
                'storageClassDeviceSets': [
                    {
                        'name': 'scratch',
                        'count': num_devs,
                        'portable': False,
                        'volumeClaimTemplates': [
                            {
                                'metadata': {'name': 'data'},
                                'spec': {
                                    'resources': {
                                        'requests': {
                                            'storage': '10Gi'  # <= (lte) the actual PV size
                                        }
                                    },
                                    'storageClassName': 'scratch',
                                    'volumeMode': 'Block',
                                    'accessModes': ['ReadWriteOnce'],
                                },
                            },
                        ],
                    }
                ],
            },
        }
    }
    teuthology.deep_merge(cluster['spec'], config.get('spec', {}))
    
    cluster_yaml = yaml.dump(cluster)
    log.info(f'Cluster:\n{cluster_yaml}')
    try:
        ctx.rook[cluster_name].remote.write_file('cluster.yaml', cluster_yaml)
        _kubectl(ctx, config, ['create', '-f', 'cluster.yaml'])
        yield

    except Exception as e:
        log.exception(e)
        raise

    finally:
        _kubectl(ctx, config, ['delete', '-f', 'cluster.yaml'], check_status=False)

        # wait for cluster to shut down
        log.info('Waiting for cluster to stop')
        running = True
        with safe_while(sleep=5, tries=100, action="wait for teardown") as proceed:
            while running and proceed():
                p = _kubectl(
                    ctx, config,
                    ['-n', 'rook-stone', 'get', 'pods'],
                    stdout=BytesIO(),
                )
                running = False
                for line in p.stdout.getvalue().decode('utf-8').strip().splitlines():
                    name, ready, status, _ = line.split(None, 3)
                    if (
                            name != 'NAME'
                            and not name.startswith('csi-')
                            and not name.startswith('rook-stone-operator-')
                            and not name.startswith('rook-stone-tools-')
                    ):
                        running = True
                        break

        _kubectl(
            ctx, config,
            ['-n', 'rook-stone', 'delete', 'configmap', 'rook-config-override'],
            check_status=False,
        )
        ctx.rook[cluster_name].remote.run(args=['rm', '-f', 'cluster.yaml'])


@contextlib.contextmanager
def rook_toolbox(ctx, config):
    cluster_name = config['cluster']
    try:
        _kubectl(ctx, config, [
            'create',
            '-f', 'rook/cluster/examples/kubernetes/stonepros/toolbox.yaml',
        ])

        log.info('Waiting for tools container to start')
        toolbox = None
        with safe_while(sleep=5, tries=100, action="wait for toolbox") as proceed:
            while not toolbox and proceed():
                p = _kubectl(
                    ctx, config,
                    ['-n', 'rook-stone', 'get', 'pods', '-l', 'app=rook-stone-tools'],
                    stdout=BytesIO(),
                )
                for line in p.stdout.getvalue().decode('utf-8').strip().splitlines():
                    name, ready, status, _ = line.split(None, 3)
                    if status == 'Running':
                        toolbox = name
                        break
        ctx.rook[cluster_name].toolbox = toolbox
        yield

    except Exception as e:
        log.exception(e)
        raise

    finally:
        _kubectl(ctx, config, [
            'delete',
            '-f', 'rook/cluster/examples/kubernetes/stonepros/toolbox.yaml',
        ], check_status=False)


@contextlib.contextmanager
def wait_for_osds(ctx, config):
    cluster_name = config.get('cluster', 'stone')

    want = ctx.rook[cluster_name].num_osds
    log.info(f'Waiting for {want} OSDs')
    with safe_while(sleep=10, tries=90, action="check osd count") as proceed:
        while proceed():
            p = _shell(ctx, config, ['stone', 'osd', 'stat', '-f', 'json'],
                       stdout=BytesIO(),
                       check_status=False)
            if p.exitstatus == 0:
                r = json.loads(p.stdout.getvalue().decode('utf-8'))
                have = r.get('num_up_osds', 0)
                if have == want:
                    break
                log.info(f' have {have}/{want} OSDs')

    yield


@contextlib.contextmanager
def stone_config_keyring(ctx, config):
    # get config and push to hosts
    log.info('Distributing stone config and client.admin keyring')
    p = _shell(ctx, config, ['cat', '/etc/stonepros/stone.conf'], stdout=BytesIO())
    conf = p.stdout.getvalue()
    p = _shell(ctx, config, ['cat', '/etc/stonepros/keyring'], stdout=BytesIO())
    keyring = p.stdout.getvalue()
    ctx.cluster.run(args=['sudo', 'mkdir', '-p', '/etc/stone'])
    for remote in ctx.cluster.remotes.keys():
        remote.write_file(
            '/etc/stonepros/stone.conf',
            conf,
            sudo=True,
        )
        remote.write_file(
            '/etc/stonepros/keyring',
            keyring,
            sudo=True,
        )

    try:
        yield

    except Exception as e:
        log.exception(e)
        raise

    finally:
        log.info('Cleaning up config and client.admin keyring')
        ctx.cluster.run(args=[
            'sudo', 'rm', '-f',
            '/etc/stonepros/stone.conf',
            '/etc/stonepros/stone.client.admin.keyring'
        ])


@contextlib.contextmanager
def stone_clients(ctx, config):
    cluster_name = config['cluster']

    log.info('Setting up client nodes...')
    clients = ctx.cluster.only(teuthology.is_type('client', cluster_name))
    for remote, roles_for_host in clients.remotes.items():
        for role in teuthology.cluster_roles_of_type(roles_for_host, 'client',
                                                     cluster_name):
            name = teuthology.stone_role(role)
            client_keyring = '/etc/stonepros/{0}.{1}.keyring'.format(cluster_name,
                                                                name)
            r = _shell(ctx, config,
                args=[
                    'stone', 'auth',
                    'get-or-create', name,
                    'mon', 'allow *',
                    'osd', 'allow *',
                    'mds', 'allow *',
                    'mgr', 'allow *',
                ],
                stdout=BytesIO(),
            )
            keyring = r.stdout.getvalue()
            remote.write_file(client_keyring, keyring, sudo=True, mode='0644')
    yield


@contextlib.contextmanager
def task(ctx, config):
    """
    Deploy rook-stone cluster

      tasks:
      - kubeadm:
      - rook:
          branch: wip-foo
          spec:
            mon:
              count: 1

    The spec item is deep-merged against the cluster.yaml.  The branch, sha1, or
    image items are used to determine the Stone container image.
    """
    if not config:
        config = {}
    assert isinstance(config, dict), \
        "task only supports a dictionary for configuration"

    log.info('Rook start')

    overrides = ctx.config.get('overrides', {})
    teuthology.deep_merge(config, overrides.get('stone', {}))
    teuthology.deep_merge(config, overrides.get('rook', {}))
    log.info('Config: ' + str(config))

    # set up cluster context
    if not hasattr(ctx, 'rook'):
        ctx.rook = {}
    if 'cluster' not in config:
        config['cluster'] = 'stone'
    cluster_name = config['cluster']
    if cluster_name not in ctx.rook:
        ctx.rook[cluster_name] = argparse.Namespace()

    ctx.rook[cluster_name].remote = list(ctx.cluster.remotes.keys())[0]

    # image
    teuth_defaults = teuth_config.get('defaults', {})
    stoneadm_defaults = teuth_defaults.get('stoneadm', {})
    containers_defaults = stoneadm_defaults.get('containers', {})
    container_image_name = containers_defaults.get('image', None)
    if 'image' in config:
        ctx.rook[cluster_name].image = config.get('image')
    else:
        sha1 = config.get('sha1')
        flavor = config.get('flavor', 'default')
        if sha1:
            if flavor == "crimson":
                ctx.rook[cluster_name].image = container_image_name + ':' + sha1 + '-' + flavor
            else:
                ctx.rook[cluster_name].image = container_image_name + ':' + sha1
        else:
            # hmm, fall back to branch?
            branch = config.get('branch', 'master')
            ctx.rook[cluster_name].image = container_image_name + ':' + branch
    log.info('Stone image is %s' % ctx.rook[cluster_name].image)
    
    with contextutil.nested(
            lambda: rook_operator(ctx, config),
            lambda: stone_log(ctx, config),
            lambda: rook_cluster(ctx, config),
            lambda: rook_toolbox(ctx, config),
            lambda: wait_for_osds(ctx, config),
            lambda: stone_config_keyring(ctx, config),
            lambda: stone_clients(ctx, config),
    ):
        if not hasattr(ctx, 'managers'):
            ctx.managers = {}
        ctx.managers[cluster_name] = StoneManager(
            ctx.rook[cluster_name].remote,
            ctx=ctx,
            logger=log.getChild('stone_manager.' + cluster_name),
            cluster=cluster_name,
            rook=True,
        )
        try:
            if config.get('wait-for-healthy', True):
                healthy(ctx=ctx, config=config)
            log.info('Rook complete, yielding')
            yield

        finally:
            log.info('Tearing down rook')
