from setuptools import setup, find_packages
import os


setup(
    name='stone-volume',
    version='1.0.0',
    packages=find_packages(),

    author='',
    author_email='contact@redhat.com',
    description='Deploy Stone OSDs using different device technologies like lvm or physical disks',
    license='LGPLv2+',
    keywords='stone volume disk devices lvm',
    url="https://github.com/stone/stone",
    zip_safe = False,
    install_requires='stone',
    dependency_links=[''.join(['file://', os.path.join(os.getcwd(), '../',
                                                       'python-common#egg=stone-1.0.0')])],
    tests_require=[
        'pytest >=2.1.3',
        'tox',
        'stone',
    ],
    entry_points = dict(
        console_scripts = [
            'stone-volume = stone_volume.main:Volume',
            'stone-volume-systemd = stone_volume.systemd:main',
        ],
    ),
    classifiers = [
        'Environment :: Console',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'Operating System :: POSIX :: Linux',
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
    ]
)
