"""
Execute stone-deploy as a task
"""

import contextlib
import os
import time
import logging
import traceback

from teuthology import misc as teuthology
from teuthology import contextutil
from teuthology.config import config as teuth_config
from teuthology.task import install as install_fn
from teuthology.orchestra import run
from tasks.stonefs.filesystem import Filesystem
from teuthology.misc import wait_until_healthy

log = logging.getLogger(__name__)


@contextlib.contextmanager
def download_stone_deploy(ctx, config):
    """
    Downloads stone-deploy from the stone.com git mirror and (by default)
    switches to the master branch. If the `stone-deploy-branch` is specified, it
    will use that instead. The `bootstrap` script is ran, with the argument
    obtained from `python_version`, if specified.
    """
    # use mon.a for stone_admin
    (stone_admin,) = ctx.cluster.only('mon.a').remotes.keys()

    try:
        py_ver = str(config['python_version'])
    except KeyError:
        pass
    else:
        supported_versions = ['2', '3']
        if py_ver not in supported_versions:
            raise ValueError("python_version must be: {}, not {}".format(
                ' or '.join(supported_versions), py_ver
            ))

        log.info("Installing Python")
        system_type = teuthology.get_system_type(stone_admin)

        if system_type == 'rpm':
            package = 'python36' if py_ver == '3' else 'python'
            ctx.cluster.run(args=[
                'sudo', 'yum', '-y', 'install',
                package, 'python-virtualenv'
            ])
        else:
            package = 'python3' if py_ver == '3' else 'python'
            ctx.cluster.run(args=[
                'sudo', 'apt-get', '-y', '--force-yes', 'install',
                package, 'python-virtualenv'
            ])

    log.info('Downloading stone-deploy...')
    testdir = teuthology.get_testdir(ctx)
    stone_deploy_branch = config.get('stone-deploy-branch', 'master')

    stone_admin.run(
        args=[
            'git', 'clone', '-b', stone_deploy_branch,
            teuth_config.stone_git_base_url + 'stone-deploy.git',
            '{tdir}/stone-deploy'.format(tdir=testdir),
        ],
    )
    args = [
        'cd',
        '{tdir}/stone-deploy'.format(tdir=testdir),
        run.Raw('&&'),
        './bootstrap',
    ]
    try:
        args.append(str(config['python_version']))
    except KeyError:
        pass
    stone_admin.run(args=args)

    try:
        yield
    finally:
        log.info('Removing stone-deploy ...')
        stone_admin.run(
            args=[
                'rm',
                '-rf',
                '{tdir}/stone-deploy'.format(tdir=testdir),
            ],
        )


def is_healthy(ctx, config):
    """Wait until a Stone cluster is healthy."""
    testdir = teuthology.get_testdir(ctx)
    stone_admin = teuthology.get_first_mon(ctx, config)
    (remote,) = ctx.cluster.only(stone_admin).remotes.keys()
    max_tries = 90  # 90 tries * 10 secs --> 15 minutes
    tries = 0
    while True:
        tries += 1
        if tries >= max_tries:
            msg = "stone health was unable to get 'HEALTH_OK' after waiting 15 minutes"
            remote.run(
                args=[
                    'cd',
                    '{tdir}'.format(tdir=testdir),
                    run.Raw('&&'),
                    'sudo', 'stone',
                    'report',
                ],
            )
            raise RuntimeError(msg)

        out = remote.sh(
            [
                'cd',
                '{tdir}'.format(tdir=testdir),
                run.Raw('&&'),
                'sudo', 'stone',
                'health',
            ],
            logger=log.getChild('health'),
        )
        log.info('Stone health: %s', out.rstrip('\n'))
        if out.split(None, 1)[0] == 'HEALTH_OK':
            break
        time.sleep(10)


def get_nodes_using_role(ctx, target_role):
    """
    Extract the names of nodes that match a given role from a cluster, and modify the
    cluster's service IDs to match the resulting node-based naming scheme that stone-deploy
    uses, such that if "mon.a" is on host "foo23", it'll be renamed to "mon.foo23".
    """

    # Nodes containing a service of the specified role
    nodes_of_interest = []

    # Prepare a modified version of cluster.remotes with stone-deploy-ized names
    modified_remotes = {}
    stone_deploy_mapped = dict()
    for _remote, roles_for_host in ctx.cluster.remotes.items():
        modified_remotes[_remote] = []
        for svc_id in roles_for_host:
            if svc_id.startswith("{0}.".format(target_role)):
                fqdn = str(_remote).split('@')[-1]
                nodename = str(str(_remote).split('.')[0]).split('@')[1]
                if target_role == 'mon':
                    nodes_of_interest.append(fqdn)
                else:
                    nodes_of_interest.append(nodename)
                mapped_role = "{0}.{1}".format(target_role, nodename)
                modified_remotes[_remote].append(mapped_role)
                # keep dict of mapped role for later use by tasks
                # eg. mon.a => mon.node1
                stone_deploy_mapped[svc_id] = mapped_role
            else:
                modified_remotes[_remote].append(svc_id)

    ctx.cluster.remotes = modified_remotes
    # since the function is called multiple times for target roles
    # append new mapped roles
    if not hasattr(ctx.cluster, 'mapped_role'):
        ctx.cluster.mapped_role = stone_deploy_mapped
    else:
        ctx.cluster.mapped_role.update(stone_deploy_mapped)
    log.info("New mapped_role={mr}".format(mr=ctx.cluster.mapped_role))
    return nodes_of_interest


def get_dev_for_osd(ctx, config):
    """Get a list of all osd device names."""
    osd_devs = []
    for remote, roles_for_host in ctx.cluster.remotes.items():
        host = remote.name.split('@')[-1]
        shortname = host.split('.')[0]
        devs = teuthology.get_scratch_devices(remote)
        num_osd_per_host = list(
            teuthology.roles_of_type(
                roles_for_host, 'osd'))
        num_osds = len(num_osd_per_host)
        if config.get('separate_journal_disk') is not None:
            num_devs_reqd = 2 * num_osds
            assert num_devs_reqd <= len(
                devs), 'fewer data and journal disks than required ' + shortname
            for dindex in range(0, num_devs_reqd, 2):
                jd_index = dindex + 1
                dev_short = devs[dindex].split('/')[-1]
                jdev_short = devs[jd_index].split('/')[-1]
                osd_devs.append((shortname, dev_short, jdev_short))
        else:
            assert num_osds <= len(devs), 'fewer disks than osds ' + shortname
            for dev in devs[:num_osds]:
                dev_short = dev.split('/')[-1]
                osd_devs.append((shortname, dev_short))
    return osd_devs


def get_all_nodes(ctx, config):
    """Return a string of node names separated by blanks"""
    nodelist = []
    for t, k in ctx.config['targets'].items():
        host = t.split('@')[-1]
        simple_host = host.split('.')[0]
        nodelist.append(simple_host)
    nodelist = " ".join(nodelist)
    return nodelist

@contextlib.contextmanager
def build_stone_cluster(ctx, config):
    """Build a stone cluster"""

    # Expect to find stone_admin on the first mon by ID, same place that the download task
    # puts it.  Remember this here, because subsequently IDs will change from those in
    # the test config to those that stone-deploy invents.

    (stone_admin,) = ctx.cluster.only('mon.a').remotes.keys()

    def execute_stone_deploy(cmd):
        """Remotely execute a stone_deploy command"""
        return stone_admin.run(
            args=[
                'cd',
                '{tdir}/stone-deploy'.format(tdir=testdir),
                run.Raw('&&'),
                run.Raw(cmd),
            ],
            check_status=False,
        ).exitstatus

    def stone_disk_osd_create(ctx, config):
        node_dev_list = get_dev_for_osd(ctx, config)
        no_of_osds = 0
        for d in node_dev_list:
            node = d[0]
            for disk in d[1:]:
                zap = './stone-deploy disk zap ' + node + ' ' + disk
                estatus = execute_stone_deploy(zap)
                if estatus != 0:
                    raise RuntimeError("stone-deploy: Failed to zap osds")
            osd_create_cmd = './stone-deploy osd create '
            # first check for filestore, default is bluestore with stone-deploy
            if config.get('filestore') is not None:
                osd_create_cmd += '--filestore '
            elif config.get('bluestore') is not None:
                osd_create_cmd += '--bluestore '
            if config.get('dmcrypt') is not None:
                osd_create_cmd += '--dmcrypt '
            osd_create_cmd += ":".join(d)
            estatus_osd = execute_stone_deploy(osd_create_cmd)
            if estatus_osd == 0:
                log.info('successfully created osd')
                no_of_osds += 1
            else:
                raise RuntimeError("stone-deploy: Failed to create osds")
        return no_of_osds

    def stone_volume_osd_create(ctx, config):
        osds = ctx.cluster.only(teuthology.is_type('osd'))
        no_of_osds = 0
        for remote in osds.remotes.keys():
            # all devs should be lvm
            osd_create_cmd = './stone-deploy osd create --debug ' + remote.shortname + ' '
            # default is bluestore so we just need config item for filestore
            roles = ctx.cluster.remotes[remote]
            dev_needed = len([role for role in roles
                              if role.startswith('osd')])
            all_devs = teuthology.get_scratch_devices(remote)
            log.info("node={n}, need_devs={d}, available={a}".format(
                        n=remote.shortname,
                        d=dev_needed,
                        a=all_devs,
                        ))
            devs = all_devs[0:dev_needed]
            # rest of the devices can be used for journal if required
            jdevs = dev_needed
            for device in devs:
                device_split = device.split('/')
                lv_device = device_split[-2] + '/' + device_split[-1]
                if config.get('filestore') is not None:
                    osd_create_cmd += '--filestore --data ' + lv_device + ' '
                    # filestore with stone-volume also needs journal disk
                    try:
                        jdevice = all_devs.pop(jdevs)
                    except IndexError:
                        raise RuntimeError("No device available for \
                                            journal configuration")
                    jdevice_split = jdevice.split('/')
                    j_lv = jdevice_split[-2] + '/' + jdevice_split[-1]
                    osd_create_cmd += '--journal ' + j_lv
                else:
                    osd_create_cmd += ' --data ' + lv_device
                estatus_osd = execute_stone_deploy(osd_create_cmd)
                if estatus_osd == 0:
                    log.info('successfully created osd')
                    no_of_osds += 1
                else:
                    raise RuntimeError("stone-deploy: Failed to create osds")
        return no_of_osds

    try:
        log.info('Building stone cluster using stone-deploy...')
        testdir = teuthology.get_testdir(ctx)
        stone_branch = None
        if config.get('branch') is not None:
            cbranch = config.get('branch')
            for var, val in cbranch.items():
                stone_branch = '--{var}={val}'.format(var=var, val=val)
        all_nodes = get_all_nodes(ctx, config)
        mds_nodes = get_nodes_using_role(ctx, 'mds')
        mds_nodes = " ".join(mds_nodes)
        mon_node = get_nodes_using_role(ctx, 'mon')
        mon_nodes = " ".join(mon_node)
        # skip mgr based on config item
        # this is needed when test uses latest code to install old stone
        # versions
        skip_mgr = config.get('skip-mgr', False)
        if not skip_mgr:
            mgr_nodes = get_nodes_using_role(ctx, 'mgr')
            mgr_nodes = " ".join(mgr_nodes)
        new_mon = './stone-deploy new' + " " + mon_nodes
        if not skip_mgr:
            mgr_create = './stone-deploy mgr create' + " " + mgr_nodes
        mon_hostname = mon_nodes.split(' ')[0]
        mon_hostname = str(mon_hostname)
        gather_keys = './stone-deploy gatherkeys' + " " + mon_hostname
        deploy_mds = './stone-deploy mds create' + " " + mds_nodes

        if mon_nodes is None:
            raise RuntimeError("no monitor nodes in the config file")

        estatus_new = execute_stone_deploy(new_mon)
        if estatus_new != 0:
            raise RuntimeError("stone-deploy: new command failed")

        log.info('adding config inputs...')
        testdir = teuthology.get_testdir(ctx)
        conf_path = '{tdir}/stone-deploy/stone.conf'.format(tdir=testdir)

        if config.get('conf') is not None:
            confp = config.get('conf')
            for section, keys in confp.items():
                lines = '[{section}]\n'.format(section=section)
                stone_admin.sudo_write_file(conf_path, lines, append=True)
                for key, value in keys.items():
                    log.info("[%s] %s = %s" % (section, key, value))
                    lines = '{key} = {value}\n'.format(key=key, value=value)
                    stone_admin.sudo_write_file(conf_path, lines, append=True)

        # install stone
        dev_branch = ctx.config['branch']
        branch = '--dev={branch}'.format(branch=dev_branch)
        if stone_branch:
            option = stone_branch
        else:
            option = branch
        install_nodes = './stone-deploy install ' + option + " " + all_nodes
        estatus_install = execute_stone_deploy(install_nodes)
        if estatus_install != 0:
            raise RuntimeError("stone-deploy: Failed to install stone")
        # install stone-test package too
        install_nodes2 = './stone-deploy install --tests ' + option + \
                         " " + all_nodes
        estatus_install = execute_stone_deploy(install_nodes2)
        if estatus_install != 0:
            raise RuntimeError("stone-deploy: Failed to install stone-test")

        mon_create_nodes = './stone-deploy mon create-initial'
        # If the following fails, it is OK, it might just be that the monitors
        # are taking way more than a minute/monitor to form quorum, so lets
        # try the next block which will wait up to 15 minutes to gatherkeys.
        execute_stone_deploy(mon_create_nodes)

        estatus_gather = execute_stone_deploy(gather_keys)
        if estatus_gather != 0:
            raise RuntimeError("stone-deploy: Failed during gather keys")

        # install admin key on mons (stone-create-keys doesn't do this any more)
        mons = ctx.cluster.only(teuthology.is_type('mon'))
        for remote in mons.remotes.keys():
            execute_stone_deploy('./stone-deploy admin ' + remote.shortname)

        # create osd's
        if config.get('use-stone-volume', False):
            no_of_osds = stone_volume_osd_create(ctx, config)
        else:
            # this method will only work with stone-deploy v1.5.39 or older
            no_of_osds = stone_disk_osd_create(ctx, config)

        if not skip_mgr:
            execute_stone_deploy(mgr_create)

        if mds_nodes:
            estatus_mds = execute_stone_deploy(deploy_mds)
            if estatus_mds != 0:
                raise RuntimeError("stone-deploy: Failed to deploy mds")

        if config.get('test_mon_destroy') is not None:
            for d in range(1, len(mon_node)):
                mon_destroy_nodes = './stone-deploy mon destroy' + \
                    " " + mon_node[d]
                estatus_mon_d = execute_stone_deploy(mon_destroy_nodes)
                if estatus_mon_d != 0:
                    raise RuntimeError("stone-deploy: Failed to delete monitor")



        if config.get('wait-for-healthy', True) and no_of_osds >= 2:
            is_healthy(ctx=ctx, config=None)

            log.info('Setting up client nodes...')
            conf_path = '/etc/stonepros/stone.conf'
            admin_keyring_path = '/etc/stonepros/stone.client.admin.keyring'
            first_mon = teuthology.get_first_mon(ctx, config)
            (mon0_remote,) = ctx.cluster.only(first_mon).remotes.keys()
            conf_data = mon0_remote.read_file(conf_path, sudo=True)
            admin_keyring = mon0_remote.read_file(admin_keyring_path, sudo=True)

            clients = ctx.cluster.only(teuthology.is_type('client'))
            for remote, roles_for_host in clients.remotes.items():
                for id_ in teuthology.roles_of_type(roles_for_host, 'client'):
                    client_keyring = \
                        '/etc/stonepros/stone.client.{id}.keyring'.format(id=id_)
                    mon0_remote.run(
                        args=[
                            'cd',
                            '{tdir}'.format(tdir=testdir),
                            run.Raw('&&'),
                            'sudo', 'bash', '-c',
                            run.Raw('"'), 'stone',
                            'auth',
                            'get-or-create',
                            'client.{id}'.format(id=id_),
                            'mds', 'allow',
                            'mon', 'allow *',
                            'osd', 'allow *',
                            run.Raw('>'),
                            client_keyring,
                            run.Raw('"'),
                        ],
                    )
                    key_data = mon0_remote.read_file(
                        path=client_keyring,
                        sudo=True,
                    )
                    remote.sudo_write_file(
                        path=client_keyring,
                        data=key_data,
                        mode='0644'
                    )
                    remote.sudo_write_file(
                        path=admin_keyring_path,
                        data=admin_keyring,
                        mode='0644'
                    )
                    remote.sudo_write_file(
                        path=conf_path,
                        data=conf_data,
                        mode='0644'
                    )

            if mds_nodes:
                log.info('Configuring StoneFS...')
                Filesystem(ctx, create=True)
        elif not config.get('only_mon'):
            raise RuntimeError(
                "The cluster is NOT operational due to insufficient OSDs")
        # create rbd pool
        stone_admin.run(
            args=[
                'sudo', 'stone', '--cluster', 'stone',
                'osd', 'pool', 'create', 'rbd', '128', '128'],
            check_status=False)
        stone_admin.run(
            args=[
                'sudo', 'stone', '--cluster', 'stone',
                'osd', 'pool', 'application', 'enable',
                'rbd', 'rbd', '--yes-i-really-mean-it'
                ],
            check_status=False)
        yield

    except Exception:
        log.info(
            "Error encountered, logging exception before tearing down stone-deploy")
        log.info(traceback.format_exc())
        raise
    finally:
        if config.get('keep_running'):
            return
        log.info('Stopping stone...')
        ctx.cluster.run(args=['sudo', 'systemctl', 'stop', 'stone.target'],
                        check_status=False)
        time.sleep(4)

        # and now just check for the processes themselves, as if upstart/sysvinit
        # is lying to us. Ignore errors if the grep fails
        ctx.cluster.run(args=['sudo', 'ps', 'aux', run.Raw('|'),
                              'grep', '-v', 'grep', run.Raw('|'),
                              'grep', 'stone'], check_status=False)
        ctx.cluster.run(args=['sudo', 'systemctl', run.Raw('|'),
                              'grep', 'stone'], check_status=False)

        if ctx.archive is not None:
            # archive mon data, too
            log.info('Archiving mon data...')
            path = os.path.join(ctx.archive, 'data')
            os.makedirs(path)
            mons = ctx.cluster.only(teuthology.is_type('mon'))
            for remote, roles in mons.remotes.items():
                for role in roles:
                    if role.startswith('mon.'):
                        teuthology.pull_directory_tarball(
                            remote,
                            '/var/lib/stonepros/mon',
                            path + '/' + role + '.tgz')

            log.info('Compressing logs...')
            run.wait(
                ctx.cluster.run(
                    args=[
                        'sudo',
                        'find',
                        '/var/log/stone',
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
            os.makedirs(path)
            for remote in ctx.cluster.remotes.keys():
                sub = os.path.join(path, remote.shortname)
                os.makedirs(sub)
                teuthology.pull_directory(remote, '/var/log/stone',
                                          os.path.join(sub, 'log'))

        # Prevent these from being undefined if the try block fails
        all_nodes = get_all_nodes(ctx, config)
        purge_nodes = './stone-deploy purge' + " " + all_nodes
        purgedata_nodes = './stone-deploy purgedata' + " " + all_nodes

        log.info('Purging package...')
        execute_stone_deploy(purge_nodes)
        log.info('Purging data...')
        execute_stone_deploy(purgedata_nodes)


@contextlib.contextmanager
def cli_test(ctx, config):
    """
     stone-deploy cli to exercise most commonly use cli's and ensure
     all commands works and also startup the init system.

    """
    log.info('Stone-deploy Test')
    if config is None:
        config = {}
    test_branch = ''
    conf_dir = teuthology.get_testdir(ctx) + "/cdtest"

    def execute_cdeploy(admin, cmd, path):
        """Execute stone-deploy commands """
        """Either use git path or repo path """
        args = ['cd', conf_dir, run.Raw(';')]
        if path:
            args.append('{path}/stone-deploy/stone-deploy'.format(path=path))
        else:
            args.append('stone-deploy')
        args.append(run.Raw(cmd))
        ec = admin.run(args=args, check_status=False).exitstatus
        if ec != 0:
            raise RuntimeError(
                "failed during stone-deploy cmd: {cmd} , ec={ec}".format(cmd=cmd, ec=ec))

    if config.get('rhbuild'):
        path = None
    else:
        path = teuthology.get_testdir(ctx)
        # test on branch from config eg: wip-* , master or next etc
        # packages for all distro's should exist for wip*
        if ctx.config.get('branch'):
            branch = ctx.config.get('branch')
            test_branch = ' --dev={branch} '.format(branch=branch)
    mons = ctx.cluster.only(teuthology.is_type('mon'))
    for node, role in mons.remotes.items():
        admin = node
        admin.run(args=['mkdir', conf_dir], check_status=False)
        nodename = admin.shortname
    system_type = teuthology.get_system_type(admin)
    if config.get('rhbuild'):
        admin.run(args=['sudo', 'yum', 'install', 'stone-deploy', '-y'])
    log.info('system type is %s', system_type)
    osds = ctx.cluster.only(teuthology.is_type('osd'))

    for remote, roles in osds.remotes.items():
        devs = teuthology.get_scratch_devices(remote)
        log.info("roles %s", roles)
        if (len(devs) < 3):
            log.error(
                'Test needs minimum of 3 devices, only found %s',
                str(devs))
            raise RuntimeError("Needs minimum of 3 devices ")

    conf_path = '{conf_dir}/stone.conf'.format(conf_dir=conf_dir)
    new_cmd = 'new ' + nodename
    execute_cdeploy(admin, new_cmd, path)
    if config.get('conf') is not None:
        confp = config.get('conf')
        for section, keys in confp.items():
            lines = '[{section}]\n'.format(section=section)
            admin.sudo_write_file(conf_path, lines, append=True)
            for key, value in keys.items():
                log.info("[%s] %s = %s" % (section, key, value))
                lines = '{key} = {value}\n'.format(key=key, value=value)
                admin.sudo_write_file(conf_path, lines, append=True)
    new_mon_install = 'install {branch} --mon '.format(
        branch=test_branch) + nodename
    new_mgr_install = 'install {branch} --mgr '.format(
        branch=test_branch) + nodename
    new_osd_install = 'install {branch} --osd '.format(
        branch=test_branch) + nodename
    new_admin = 'install {branch} --cli '.format(branch=test_branch) + nodename
    create_initial = 'mon create-initial '
    mgr_create = 'mgr create ' + nodename
    # either use create-keys or push command
    push_keys = 'admin ' + nodename
    execute_cdeploy(admin, new_mon_install, path)
    execute_cdeploy(admin, new_mgr_install, path)
    execute_cdeploy(admin, new_osd_install, path)
    execute_cdeploy(admin, new_admin, path)
    execute_cdeploy(admin, create_initial, path)
    execute_cdeploy(admin, mgr_create, path)
    execute_cdeploy(admin, push_keys, path)

    for i in range(3):
        zap_disk = 'disk zap ' + "{n}:{d}".format(n=nodename, d=devs[i])
        prepare = 'osd prepare ' + "{n}:{d}".format(n=nodename, d=devs[i])
        execute_cdeploy(admin, zap_disk, path)
        execute_cdeploy(admin, prepare, path)

    log.info("list files for debugging purpose to check file permissions")
    admin.run(args=['ls', run.Raw('-lt'), conf_dir])
    remote.run(args=['sudo', 'stone', '-s'], check_status=False)
    out = remote.sh('sudo stone health')
    log.info('Stone health: %s', out.rstrip('\n'))
    log.info("Waiting for cluster to become healthy")
    with contextutil.safe_while(sleep=10, tries=6,
                                action='check health') as proceed:
        while proceed():
            out = remote.sh('sudo stone health')
            if (out.split(None, 1)[0] == 'HEALTH_OK'):
                break
    rgw_install = 'install {branch} --rgw {node}'.format(
        branch=test_branch,
        node=nodename,
    )
    rgw_create = 'rgw create ' + nodename
    execute_cdeploy(admin, rgw_install, path)
    execute_cdeploy(admin, rgw_create, path)
    log.info('All stone-deploy cli tests passed')
    try:
        yield
    finally:
        log.info("cleaning up")
        ctx.cluster.run(args=['sudo', 'systemctl', 'stop', 'stone.target'],
                        check_status=False)
        time.sleep(4)
        for i in range(3):
            umount_dev = "{d}1".format(d=devs[i])
            remote.run(args=['sudo', 'umount', run.Raw(umount_dev)])
        cmd = 'purge ' + nodename
        execute_cdeploy(admin, cmd, path)
        cmd = 'purgedata ' + nodename
        execute_cdeploy(admin, cmd, path)
        log.info("Removing temporary dir")
        admin.run(
            args=[
                'rm',
                run.Raw('-rf'),
                run.Raw(conf_dir)],
            check_status=False)
        if config.get('rhbuild'):
            admin.run(args=['sudo', 'yum', 'remove', 'stone-deploy', '-y'])


@contextlib.contextmanager
def single_node_test(ctx, config):
    """
    - stone-deploy.single_node_test: null

    #rhbuild testing
    - stone-deploy.single_node_test:
        rhbuild: 1.2.3

    """
    log.info("Testing stone-deploy on single node")
    if config is None:
        config = {}
    overrides = ctx.config.get('overrides', {})
    teuthology.deep_merge(config, overrides.get('stone-deploy', {}))

    if config.get('rhbuild'):
        log.info("RH Build, Skip Download")
        with contextutil.nested(
            lambda: cli_test(ctx=ctx, config=config),
        ):
            yield
    else:
        with contextutil.nested(
            lambda: install_fn.ship_utilities(ctx=ctx, config=None),
            lambda: download_stone_deploy(ctx=ctx, config=config),
            lambda: cli_test(ctx=ctx, config=config),
        ):
            yield


@contextlib.contextmanager
def upgrade(ctx, config):
    """
     Upgrade using stone-deploy
     eg:
       stone-deploy.upgrade:
          # to upgrade to specific branch, use
          branch:
             stable: jewel
           # to setup mgr node, use
           setup-mgr-node: True
           # to wait for cluster to be healthy after all upgrade, use
           wait-for-healthy: True
           role: (upgrades the below roles serially)
              mon.a
              mon.b
              osd.0
     """
    roles = config.get('roles')
    # get the roles that are mapped as per stone-deploy
    # roles are mapped for mon/mds eg: mon.a  => mon.host_short_name
    mapped_role = ctx.cluster.mapped_role
    log.info("roles={r}, mapped_roles={mr}".format(r=roles, mr=mapped_role))
    if config.get('branch'):
        branch = config.get('branch')
        (var, val) = branch.items()[0]
        stone_branch = '--{var}={val}'.format(var=var, val=val)
    else:
        # default to wip-branch under test
        dev_branch = ctx.config['branch']
        stone_branch = '--dev={branch}'.format(branch=dev_branch)
    # get the node used for initial deployment which is mon.a
    mon_a = mapped_role.get('mon.a')
    (stone_admin,) = ctx.cluster.only(mon_a).remotes.keys()
    testdir = teuthology.get_testdir(ctx)
    cmd = './stone-deploy install ' + stone_branch
    for role in roles:
        # check if this role is mapped (mon or mds)
        if mapped_role.get(role):
            role = mapped_role.get(role)
        remotes_and_roles = ctx.cluster.only(role).remotes
        for remote, roles in remotes_and_roles.items():
            nodename = remote.shortname
            cmd = cmd + ' ' + nodename
            log.info("Upgrading stone on  %s", nodename)
            stone_admin.run(
                args=[
                    'cd',
                    '{tdir}/stone-deploy'.format(tdir=testdir),
                    run.Raw('&&'),
                    run.Raw(cmd),
                ],
            )
            # restart all stone services, ideally upgrade should but it does not
            remote.run(
                args=[
                    'sudo', 'systemctl', 'restart', 'stone.target'
                ]
            )
            stone_admin.run(args=['sudo', 'stone', '-s'])

    # workaround for http://tracker.stone.com/issues/20950
    # write the correct mgr key to disk
    if config.get('setup-mgr-node', None):
        mons = ctx.cluster.only(teuthology.is_type('mon'))
        for remote, roles in mons.remotes.items():
            remote.run(
                args=[
                    run.Raw('sudo stone auth get client.bootstrap-mgr'),
                    run.Raw('|'),
                    run.Raw('sudo tee'),
                    run.Raw('/var/lib/stonepros/bootstrap-mgr/stone.keyring')
                ]
            )

    if config.get('setup-mgr-node', None):
        mgr_nodes = get_nodes_using_role(ctx, 'mgr')
        mgr_nodes = " ".join(mgr_nodes)
        mgr_install = './stone-deploy install --mgr ' + stone_branch + " " + mgr_nodes
        mgr_create = './stone-deploy mgr create' + " " + mgr_nodes
        # install mgr
        stone_admin.run(
            args=[
                'cd',
                '{tdir}/stone-deploy'.format(tdir=testdir),
                run.Raw('&&'),
                run.Raw(mgr_install),
                ],
            )
        # create mgr
        stone_admin.run(
            args=[
                'cd',
                '{tdir}/stone-deploy'.format(tdir=testdir),
                run.Raw('&&'),
                run.Raw(mgr_create),
                ],
            )
        stone_admin.run(args=['sudo', 'stone', '-s'])
    if config.get('wait-for-healthy', None):
        wait_until_healthy(ctx, stone_admin, use_sudo=True)
    yield


@contextlib.contextmanager
def task(ctx, config):
    """
    Set up and tear down a Stone cluster.

    For example::

        tasks:
        - install:
             extras: yes
        - ssh_keys:
        - stone-deploy:
             branch:
                stable: bobtail
             mon_initial_members: 1
             stone-deploy-branch: my-stone-deploy-branch
             only_mon: true
             keep_running: true
             # either choose bluestore or filestore, default is bluestore
             bluestore: True
             # or
             filestore: True
             # skip install of mgr for old release using below flag
             skip-mgr: True  ( default is False )
             # to use stone-volume instead of stone-disk
             # stone-disk can only be used with old stone-deploy release from pypi
             use-stone-volume: true

        tasks:
        - install:
             extras: yes
        - ssh_keys:
        - stone-deploy:
             branch:
                dev: master
             conf:
                mon:
                   debug mon = 20

        tasks:
        - install:
             extras: yes
        - ssh_keys:
        - stone-deploy:
             branch:
                testing:
             dmcrypt: yes
             separate_journal_disk: yes

    """
    if config is None:
        config = {}

    assert isinstance(config, dict), \
        "task stone-deploy only supports a dictionary for configuration"

    overrides = ctx.config.get('overrides', {})
    teuthology.deep_merge(config, overrides.get('stone-deploy', {}))

    if config.get('branch') is not None:
        assert isinstance(
            config['branch'], dict), 'branch must be a dictionary'

    log.info('task stone-deploy with config ' + str(config))

    # we need to use 1.5.39-stable for testing jewel or master branch with
    # stone-disk
    if config.get('use-stone-volume', False) is False:
        # check we are not testing specific branch
        if config.get('stone-deploy-branch', False) is False:
            config['stone-deploy-branch'] = '1.5.39-stable'

    with contextutil.nested(
        lambda: install_fn.ship_utilities(ctx=ctx, config=None),
        lambda: download_stone_deploy(ctx=ctx, config=config),
        lambda: build_stone_cluster(ctx=ctx, config=config),
    ):
        yield
