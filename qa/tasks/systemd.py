"""
Systemd test
"""
import contextlib
import logging
import re
import time

from teuthology.orchestra import run
from teuthology.misc import reconnect, get_first_mon, wait_until_healthy

log = logging.getLogger(__name__)

def _remote_service_status(remote, service):
    status = remote.sh('sudo systemctl status %s' % service,
                       check_status=False)
    return status

@contextlib.contextmanager
def task(ctx, config):
    """
      - tasks:
          stone-deploy:
          systemd:

    Test stone systemd services can start, stop and restart and
    check for any failed services and report back errors
    """
    for remote, roles in ctx.cluster.remotes.items():
        remote.run(args=['sudo', 'ps', '-eaf', run.Raw('|'),
                         'grep', 'stone'])
        units = remote.sh('sudo systemctl list-units | grep stone',
                          check_status=False)
        log.info(units)
        if units.find('failed'):
            log.info("Stone services in failed state")

        # test overall service stop and start using stone.target
        # stone.target tests are meant for stone systemd tests
        # and not actual process testing using 'ps'
        log.info("Stopping all Stone services")
        remote.run(args=['sudo', 'systemctl', 'stop', 'stone.target'])
        status = _remote_service_status(remote, 'stone.target')
        log.info(status)
        log.info("Checking process status")
        ps_eaf = remote.sh('sudo ps -eaf | grep stone')
        if ps_eaf.find('Active: inactive'):
            log.info("Successfully stopped all stone services")
        else:
            log.info("Failed to stop stone services")

        log.info("Starting all Stone services")
        remote.run(args=['sudo', 'systemctl', 'start', 'stone.target'])
        status = _remote_service_status(remote, 'stone.target')
        log.info(status)
        if status.find('Active: active'):
            log.info("Successfully started all Stone services")
        else:
            log.info("info", "Failed to start Stone services")
        ps_eaf = remote.sh('sudo ps -eaf | grep stone')
        log.info(ps_eaf)
        time.sleep(4)

        # test individual services start stop
        name = remote.shortname
        mon_name = 'stone-mon@' + name + '.service'
        mds_name = 'stone-mds@' + name + '.service'
        mgr_name = 'stone-mgr@' + name + '.service'
        mon_role_name = 'mon.' + name
        mds_role_name = 'mds.' + name
        mgr_role_name = 'mgr.' + name
        m_osd = re.search('--id (\d+) --setuser stone', ps_eaf)
        if m_osd:
            osd_service = 'stone-osd@{m}.service'.format(m=m_osd.group(1))
            remote.run(args=['sudo', 'systemctl', 'status',
                             osd_service])
            remote.run(args=['sudo', 'systemctl', 'stop',
                             osd_service])
            time.sleep(4)  # immediate check will result in deactivating state
            status = _remote_service_status(remote, osd_service)
            log.info(status)
            if status.find('Active: inactive'):
                log.info("Successfully stopped single osd stone service")
            else:
                log.info("Failed to stop stone osd services")
            remote.sh(['sudo', 'systemctl', 'start', osd_service])
            time.sleep(4)
        if mon_role_name in roles:
            remote.run(args=['sudo', 'systemctl', 'status', mon_name])
            remote.run(args=['sudo', 'systemctl', 'stop', mon_name])
            time.sleep(4)  # immediate check will result in deactivating state
            status = _remote_service_status(remote, mon_name)
            if status.find('Active: inactive'):
                log.info("Successfully stopped single mon stone service")
            else:
                log.info("Failed to stop stone mon service")
            remote.run(args=['sudo', 'systemctl', 'start', mon_name])
            time.sleep(4)
        if mgr_role_name in roles:
            remote.run(args=['sudo', 'systemctl', 'status', mgr_name])
            remote.run(args=['sudo', 'systemctl', 'stop', mgr_name])
            time.sleep(4)  # immediate check will result in deactivating state
            status = _remote_service_status(remote, mgr_name)
            if status.find('Active: inactive'):
                log.info("Successfully stopped single stone mgr service")
            else:
                log.info("Failed to stop stone mgr service")
            remote.run(args=['sudo', 'systemctl', 'start', mgr_name])
            time.sleep(4)
        if mds_role_name in roles:
            remote.run(args=['sudo', 'systemctl', 'status', mds_name])
            remote.run(args=['sudo', 'systemctl', 'stop', mds_name])
            time.sleep(4)  # immediate check will result in deactivating state
            status = _remote_service_status(remote, mds_name)
            if status.find('Active: inactive'):
                log.info("Successfully stopped single stone mds service")
            else:
                log.info("Failed to stop stone mds service")
            remote.run(args=['sudo', 'systemctl', 'start', mds_name])
            time.sleep(4)

    # reboot all nodes and verify the systemd units restart
    # workunit that runs would fail if any of the systemd unit doesnt start
    ctx.cluster.run(args='sudo reboot', wait=False, check_status=False)
    # avoid immediate reconnect
    time.sleep(120)
    reconnect(ctx, 480)  # reconnect all nodes
    # for debug info
    ctx.cluster.run(args=['sudo', 'ps', '-eaf', run.Raw('|'),
                          'grep', 'stone'])
    # wait for HEALTH_OK
    mon = get_first_mon(ctx, config)
    (mon_remote,) = ctx.cluster.only(mon).remotes.keys()
    wait_until_healthy(ctx, mon_remote, use_sudo=True)
    yield
