# -*- coding: utf-8 -*-

import argparse
import json
import logging
import sys

from stone.deployment.drive_group import DriveGroupSpec
from stone.deployment.drive_selection.selector import DriveSelection
from stone.deployment.translate import to_stone_volume
from stone.deployment.inventory import Device
from stone_volume.inventory import Inventory
from stone_volume.devices.lvm.batch import Batch

logger = logging.getLogger(__name__)

class Deploy(object):

    help = '''
    Deploy OSDs according to a drive groups specification.

    The DriveGroup specification must be passed in json.
    It can either be (preference in this order)
      - in a file, path passed as a positional argument
      - read from stdin, pass "-" as a positional argument
      - a json string passed via the --spec argument

    Either the path postional argument or --spec must be specifed.
    '''

    def __init__(self, argv):
        self.argv = argv

    def main(self):
        parser = argparse.ArgumentParser(
            prog='stone-volume drive-group',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.help,
        )
        parser.add_argument(
            'path',
            nargs='?',
            default=None,
            help=('Path to file containing drive group spec or "-" to read from stdin'),
        )
        parser.add_argument(
            '--spec',
            default='',
            nargs='?',
            help=('drive-group json string')
        )
        parser.add_argument(
            '--dry-run',
            default=False,
            action='store_true',
            help=('dry run, only print the batch command that would be run'),
        )
        self.args = parser.parse_args(self.argv)
        if self.args.path:
            if self.args.path == "-":
                commands = self.from_json(sys.stdin)
            else:
                with open(self.args.path, 'r') as f:
                    commands = self.from_json(f)
        elif self.args.spec:
            dg = json.loads(self.args.spec)
            commands = self.get_dg_spec(dg)
        else:
            # either --spec or path arg must be specified
            parser.print_help(sys.stderr)
            sys.exit(0)
        cmd = commands.run()
        if not cmd:
            logger.error('DriveGroup didn\'t produce any commands')
            return
        if self.args.dry_run:
            logger.info('Returning stone-volume command (--dry-run was passed): {}'.format(cmd))
            print(cmd)
        else:
            logger.info('Running stone-volume command: {}'.format(cmd))
            batch_args = cmd.split(' ')[2:]
            b = Batch(batch_args)
            b.main()

    def from_json(self, file_):
        dg = {}
        dg = json.load(file_)
        return self.get_dg_spec(dg)

    def get_dg_spec(self, dg):
        dg_spec = DriveGroupSpec._from_json_impl(dg)
        dg_spec.validate()
        i = Inventory(['--filter-for-batch'])
        i.main()
        inventory = i.get_report()
        devices = [Device.from_json(i) for i in inventory]
        selection = DriveSelection(dg_spec, devices)
        return to_stone_volume(selection)
