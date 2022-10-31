#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""The setup script."""

from setuptools import setup, find_packages

requirements = [ ]

setup_requirements = [ ]

setup(
    author="Willem Jan Withagen",
    author_email='wjw@digiware.nl',
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Environment :: Console',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'Operating System :: POSIX :: FreeBSD',
        'License :: OSI Approved :: BSD License',
        'Natural Language :: English',
        "Programming Language :: Python :: 2",
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
    ],
    description="Manage Stone OSDs on ZFS pool/volume/filesystem",
    install_requires=requirements,
    license="BSD license",
    include_package_data=True,
    keywords='stone-volume-zfs',
    name='stone-volume-zfs',
    packages=find_packages(include=['stone_volume_zfs']),
    setup_requires=setup_requirements,
    url='https://github.com/stone/stone/src/stone-volume/plugin/zfs',
    version='0.1.0',
    zip_safe=False,
    entry_points = dict(
        stone_volume_handlers = [
            'zfs = stone_volume_zfs.zfs:ZFS',
        ],
    ),
)
