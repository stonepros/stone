# -*- coding: utf-8 -*-

from setuptools import setup

__version__ = '0.0.1'

setup(
    name='stonefs-shell',
    version=__version__,
    description='Interactive shell for Stone file system',
    keywords='stonefs, shell',
    scripts=['stonefs-shell'],
    install_requires=[
        'stonefs',
        'cmd2',
        'colorama',
    ],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Console',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3'
    ],
    license='LGPLv2+',
)
