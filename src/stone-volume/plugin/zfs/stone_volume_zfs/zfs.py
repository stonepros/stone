# -*- coding: utf-8 -*-

from __future__ import print_function
import argparse
import os
import sys
import logging

from textwrap import dedent
from stone_volume import log, conf, configuration
from stone_volume import exceptions
from stone_volume import terminal

# The stone-volume-zfs specific code
import stone_volume_zfs.zfs
from stone_volume_zfs import devices
# from stone_volume_zfs.util import device
from stone_volume_zfs.devices import zfs

# the supported actions
from stone_volume_zfs.devices.zfs import inventory
from stone_volume_zfs.devices.zfs import prepare
from stone_volume_zfs.devices.zfs import zap


if __name__ == '__main__':
    zfs.ZFS()


class ZFS(object):

    # help info for subcommands
    help = "Use ZFS as the underlying technology for OSDs"

    # help info for the plugin
    help_menu = "Deploy OSDs with ZFS"
    _help = dedent("""
        Use ZFS as the underlying technology for OSDs

        {sub_zfshelp}
    """)
    name = 'zfs'

    def __init__(self, argv=None, parse=True):
        self.zfs_mapper = {
            'inventory': inventory.Inventory,
            'prepare': prepare.Prepare,
            'zap': zap.Zap,
        }
        if argv is None:
            self.argv = sys.argv
        else:
            self.argv = argv
        if parse:
            self.main(self.argv)

    def print_help(self, warning=False):
        return self._help.format(
            sub_zfshelp=terminal.subhelp(self.zfs_mapper)
        )

    def get_environ_vars(self):
        environ_vars = []
        for key, value in os.environ.items():
            if key.startswith('STONE_'):
                environ_vars.append("%s=%s" % (key, value))
        if not environ_vars:
            return ''
        else:
            environ_vars.insert(0, '\nEnviron Variables:')
            return '\n'.join(environ_vars)

    def load_stone_conf_path(self, cluster_name='stone'):
        abspath = '/etc/stone/%s.conf' % cluster_name
        conf.path = os.getenv('STONE_CONF', abspath)
        conf.cluster = cluster_name

    def stat_stone_conf(self):
        try:
            configuration.load(conf.path)
            return terminal.green(conf.path)
        except exceptions.ConfigurationError as error:
            return terminal.red(error)

    def load_log_path(self):
        conf.log_path = os.getenv('STONE_VOLUME_LOG_PATH', '/var/log/stone')

    def _get_split_args(self):
        subcommands = self.zfs_mapper.keys()
        slice_on_index = len(self.argv)
        pruned_args = self.argv
        for count, arg in enumerate(pruned_args):
            if arg in subcommands:
                slice_on_index = count
                break
        return pruned_args[:slice_on_index], pruned_args[slice_on_index:]

    def main(self, argv=None):
        if argv is None:
            return
        self.load_stone_conf_path()
        # these need to be available for the help, which gets parsed super
        # early
        self.load_stone_conf_path()
        self.load_log_path()
        main_args, subcommand_args = self._get_split_args()
        # no flags where passed in, return the help menu instead of waiting for
        # argparse which will end up complaning that there are no args
        if len(argv) < 1:
            print(self.print_help(warning=True))
            return
        parser = argparse.ArgumentParser(
            prog='stone-volume-zfs',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.print_help(),
        )
        parser.add_argument(
            '--cluster',
            default='stone',
            help='Cluster name (defaults to "stone")',
        )
        parser.add_argument(
            '--log-level',
            default='debug',
            help='Change the file log level (defaults to debug)',
        )
        parser.add_argument(
            '--log-path',
            default='/var/log/stone/',
            help='Change the log path (defaults to /var/log/stone)',
        )
        args = parser.parse_args(main_args)
        conf.log_path = args.log_path
        if os.path.isdir(conf.log_path):
            conf.log_path = os.path.join(args.log_path, 'stone-volume-zfs.log')
        log.setup()
        logger = logging.getLogger(__name__)
        logger.info("Running command: stone-volume-zfs %s %s",
                    " ".join(main_args), " ".join(subcommand_args))
        # set all variables from args and load everything needed according to
        # them
        self.load_stone_conf_path(cluster_name=args.cluster)
        try:
            conf.stone = configuration.load(conf.path)
        except exceptions.ConfigurationError as error:
            # we warn only here, because it is possible that the configuration
            # file is not needed, or that it will be loaded by some other means
            # (like reading from zfs tags)
            logger.exception('ignoring inability to load stone.conf')
            terminal.red(error)
        # dispatch to sub-commands
        terminal.dispatch(self.zfs_mapper, subcommand_args)
