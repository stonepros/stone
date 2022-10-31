from __future__ import print_function
import json
import logging
import os
from textwrap import dedent
from stone_volume.util import prepare as prepare_utils
from stone_volume.util import encryption as encryption_utils
from stone_volume.util import disk
from stone_volume.util import system
from stone_volume import decorators, terminal
from stone_volume.devices.lvm.common import rollback_osd
from .common import create_parser

logger = logging.getLogger(__name__)

def prepare_dmcrypt(key, device, device_type, fsid):
    """
    Helper for devices that are encrypted. The operations needed for
    block, db, wal, or data/journal devices are all the same
    """
    if not device:
        return ''
    kname = disk.lsblk(device)['KNAME']
    mapping = 'stone-{}-{}-{}-dmcrypt'.format(fsid, kname, device_type)
    # format data device
    encryption_utils.luks_format(
        key,
        device
    )
    encryption_utils.luks_open(
        key,
        device,
        mapping
    )

    return '/dev/mapper/{}'.format(mapping)

def prepare_bluestore(block, wal, db, secrets, osd_id, fsid, tmpfs):
    """
    :param block: The name of the logical volume for the bluestore data
    :param wal: a regular/plain disk or logical volume, to be used for block.wal
    :param db: a regular/plain disk or logical volume, to be used for block.db
    :param secrets: A dict with the secrets needed to create the osd (e.g. stonex)
    :param id_: The OSD id
    :param fsid: The OSD fsid, also known as the OSD UUID
    """
    stonex_secret = secrets.get('stonex_secret', prepare_utils.create_key())

    if secrets.get('dmcrypt_key'):
        key = secrets['dmcrypt_key']
        block = prepare_dmcrypt(key, block, 'block', fsid)
        wal = prepare_dmcrypt(key, wal, 'wal', fsid)
        db = prepare_dmcrypt(key, db, 'db', fsid)

    # create the directory
    prepare_utils.create_osd_path(osd_id, tmpfs=tmpfs)
    # symlink the block
    prepare_utils.link_block(block, osd_id)
    # get the latest monmap
    prepare_utils.get_monmap(osd_id)
    # write the OSD keyring if it doesn't exist already
    prepare_utils.write_keyring(osd_id, stonex_secret)
    # prepare the osd filesystem
    prepare_utils.osd_mkfs_bluestore(
        osd_id, fsid,
        keyring=stonex_secret,
        wal=wal,
        db=db
    )


class Prepare(object):

    help = 'Format a raw device and associate it with a (BlueStore) OSD'

    def __init__(self, argv):
        self.argv = argv
        self.osd_id = None

    def safe_prepare(self, args=None):
        """
        An intermediate step between `main()` and `prepare()` so that we can
        capture the `self.osd_id` in case we need to rollback

        :param args: Injected args, usually from `raw create` which compounds
                     both `prepare` and `create`
        """
        if args is not None:
            self.args = args
        try:
            self.prepare()
        except Exception:
            logger.exception('raw prepare was unable to complete')
            logger.info('will rollback OSD ID creation')
            rollback_osd(self.args, self.osd_id)
            raise
        dmcrypt_log = 'dmcrypt' if args.dmcrypt else 'clear'
        terminal.success("stone-volume raw {} prepare successful for: {}".format(dmcrypt_log, self.args.data))


    @decorators.needs_root
    def prepare(self):
        secrets = {'stonex_secret': prepare_utils.create_key()}
        encrypted = 1 if self.args.dmcrypt else 0
        stonex_lockbox_secret = '' if not encrypted else prepare_utils.create_key()

        if encrypted:
            secrets['dmcrypt_key'] = os.getenv('STONE_VOLUME_DMCRYPT_SECRET')
            secrets['stonex_lockbox_secret'] = stonex_lockbox_secret # dummy value to make `stone osd new` not complaining

        osd_fsid = system.generate_uuid()
        crush_device_class = self.args.crush_device_class
        if crush_device_class:
            secrets['crush_device_class'] = crush_device_class
        tmpfs = not self.args.no_tmpfs
        wal = ""
        db = ""
        if self.args.block_wal:
            wal = self.args.block_wal
        if self.args.block_db:
            db = self.args.block_db

        # reuse a given ID if it exists, otherwise create a new ID
        self.osd_id = prepare_utils.create_id(
            osd_fsid, json.dumps(secrets))

        prepare_bluestore(
            self.args.data,
            wal,
            db,
            secrets,
            self.osd_id,
            osd_fsid,
            tmpfs,
        )

    def main(self):
        sub_command_help = dedent("""
        Prepare an OSD by assigning an ID and FSID, registering them with the
        cluster with an ID and FSID, formatting the volume.

        Once the OSD is ready, an ad-hoc systemd unit will be enabled so that
        it can later get activated and the OSD daemon can get started.

            stone-volume raw prepare --bluestore --data {device}

        DB and WAL devices are supported.

            stone-volume raw prepare --bluestore --data {device} --block.db {device} --block.wal {device}

        """)
        parser = create_parser(
            prog='stone-volume raw prepare',
            description=sub_command_help,
        )
        if not self.argv:
            print(sub_command_help)
            return
        self.args = parser.parse_args(self.argv)
        if not self.args.bluestore:
            terminal.error('must specify --bluestore (currently the only supported backend)')
            raise SystemExit(1)
        if self.args.dmcrypt and not os.getenv('STONE_VOLUME_DMCRYPT_SECRET'):
            terminal.error('encryption was requested (--dmcrypt) but environment variable ' \
                           'STONE_VOLUME_DMCRYPT_SECRET is not set, you must set ' \
                           'this variable to provide a dmcrypt secret.')
            raise SystemExit(1)

        self.safe_prepare(self.args)
