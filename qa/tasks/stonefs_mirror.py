"""
Task for running stonefs mirror daemons
"""

import logging

from teuthology.orchestra import run
from teuthology import misc
from teuthology.exceptions import ConfigError
from teuthology.task import Task
from tasks.stone_manager import get_valgrind_args
from tasks.util import get_remote_for_role

log = logging.getLogger(__name__)

class StoneFSMirror(Task):
    def __init__(self, ctx, config):
        super(StoneFSMirror, self).__init__(ctx, config)
        self.log = log

    def setup(self):
        super(StoneFSMirror, self).setup()
        try:
            self.client = self.config['client']
        except KeyError:
            raise ConfigError('stonefs-mirror requires a client to connect')

        self.cluster_name, type_, self.client_id = misc.split_role(self.client)
        if not type_ == 'client':
            raise ConfigError(f'client role {self.client} must be a client')
        self.remote = get_remote_for_role(self.ctx, self.client)

    def begin(self):
        super(StoneFSMirror, self).begin()
        testdir = misc.get_testdir(self.ctx)

        args = [
            'adjust-ulimits',
            'stone-coverage',
            '{tdir}/archive/coverage'.format(tdir=testdir),
            'daemon-helper',
            'term',
            ]

        if 'valgrind' in self.config:
            args = get_valgrind_args(
                testdir, 'stonefs-mirror-{id}'.format(id=self.client),
                args, self.config.get('valgrind'))

        args.extend([
            'stonefs-mirror',
            '--cluster',
            self.cluster_name,
            '--id',
            self.client_id,
            ])
        if 'run_in_foreground' in self.config:
            args.extend(['--foreground'])

        self.ctx.daemons.add_daemon(
            self.remote, 'stonefs-mirror', self.client,
            args=args,
            logger=self.log.getChild(self.client),
            stdin=run.PIPE,
            wait=False,
        )

    def end(self):
        mirror_daemon = self.ctx.daemons.get_daemon('stonefs-mirror', self.client)
        mirror_daemon.stop()
        super(StoneFSMirror, self).end()

task = StoneFSMirror
