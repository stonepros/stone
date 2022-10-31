from setuptools import setup, find_packages


with open("README.rst", "r") as fh:
    long_description = fh.read()


setup(
    name='stone',
    version='1.0.0',
    packages=find_packages(),
    author='',
    author_email='dev@stone.io',
    description='Stone common library',
    long_description=long_description,
    license='LGPLv2+',
    keywords='stone',
    url="https://github.com/stone/stone",
    zip_safe = False,
    install_requires=(	
        'pyyaml',	
    ),
    classifiers = [
        'Intended Audience :: Developer',
        'Operating System :: POSIX :: Linux',
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
    ]
)
