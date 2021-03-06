# -*-coding:utf8 -*-
import os
import re
import sys
import glob
import platform
from setuptools import setup, Extension
from Cython.Build import cythonize


# ---------------------------------------------
#  Prepare for source file and build options
# ---------------------------------------------
if platform.system() == 'Windows':
    HYDRUS_INCLUDES = ['3rd/prebuilt/include']
    HYDRUS_LIBRARIES = ['libuv', 'ws2_32', 'IPHLPAPI', 'Psapi', 'Userenv', 'advapi32']
    HYDRUS_LIBPATH = ['3rd/prebuilt/libs']
    HYDRUS_BUILD_FLAGS = ['/EHsc']
else:
    HYDRUS_INCLUDES = []
    HYDRUS_LIBRARIES = ['uv']
    HYDRUS_LIBPATH = []
    HYDRUS_BUILD_FLAGS = ['-std=c++11','-fno-strict-aliasing', '-fcommon', '-fPIC',
                          '-Wall', '-Wextra', '-Wno-unused-parameter','-Wno-missing-field-initializers', '-O2']

# Source files
HYDRUS_SRC_FILES = glob.glob('src/*.cc') + glob.glob('src/*.c') + glob.glob('hydrus/*.pyx')
# Define Macros
HYDRUS_MACROS = []

# ---------------------------------------------
#  Helpers
# ---------------------------------------------
def get_version():
    hydrus_file = os.path.abspath(os.path.dirname(__file__)) + '/hydrus/server.pyx'
    ver = '0.1.0'
    with open(hydrus_file, 'r') as f:
        text = f.read()
        matches = re.findall(r'__version__\s*=\s*\'(\S+)\'', text, re.I)
        if matches:
            ver = matches[0]
            print('================\nCurrent Version is: %s\n================' % matches[0])
    return ver


def create_extension(name):
    return Extension(
        name, HYDRUS_SRC_FILES,
        include_dirs=HYDRUS_INCLUDES,
        library_dirs=HYDRUS_LIBPATH,
        language='c++',
        libraries=HYDRUS_LIBRARIES,
        extra_compile_args=HYDRUS_BUILD_FLAGS,
        define_macros=HYDRUS_MACROS,
    )


# ---------------------------------------------
#   Setup start here
# ---------------------------------------------
setup(
    name='hydrus',
    packages=['hydrus'],
    version=get_version(),
    license='BSD License',
    description='A lightweight and fast enough WSGI Server for Python',
    author='drz',
    keywords=('wsgi', 'server', 'web'),
    url='',
    ext_modules=cythonize([
        create_extension('hydrus.server'),
    ])
)
