from setuptools import setup, Distribution
from setuptools.command.install import install as InstallCommandBase
import pathlib


class BinaryDistribution(Distribution):
    def has_ext_modules(foo):
        return True


class InstallCommand(InstallCommandBase):
    """Override the dir where the headers go."""

    def finalize_options(self):
        ret = InstallCommandBase.finalize_options(self)
        self.install_lib = self.install_platlib
        return ret


long_description = """
# Embag: A really fast, simple bag file reader
.. image:: https://github.com/embarktrucks/embag/workflows/pypi_build/badge.svg)
    :target: https://github.com/embarktrucks/embag/actions

This library reads `ROS <https://wiki.ros.org>` `bag files <http://wiki.ros.org/Bags/Format/2.0>` quickly without their
`message descriptions`<http://wiki.ros.org/msg>` and without any dependencies.

See the `API README <https://github.com/embarktrucks/embag/tree/master/python>` for usage details.
"""

version = open('/tmp/embag/lib/version.bzl').readline().split('"')[1]

setup(
    name='embag',
    packages=['embag'],
    version=version,
    license='MIT',
    description='Fast ROS bag reader',
    long_description=long_description,
    author='Jason Snell',
    author_email='jason@embarktrucks.com',
    distclass=BinaryDistribution,
    include_package_data=True,
    package_data={
        'embag': ['libembag.so'],
    },
    cmdclass={
        'install': InstallCommand,
    },
    url='https://github.com/embarktrucks/embag',
    download_url='https://pypi.org/project/embag/',
    keywords=['ROS', 'bag', 'reader'],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
)
