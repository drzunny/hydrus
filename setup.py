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
    HYDRUS_INCLUDES = ['3rd/libuv/include']
    HYDRUS_LIBRARIES = ['libuv']
    HYDRUS_BUILD_FLAGS = []
else:
    HYDRUS_INCLUDES = []
    HYDRUS_LIBRARIES = ['uv']
    HYDRUS_BUILD_FLAGS = ['-std=c++11','-fno-strict-aliasing', '-fcommon', '-fPIC',
                          '-Wall', '-Wextra', '-Wno-unused-parameter','Wno-missing-field-initializers', '-O2']

# Source files
HYDRUS_SRC_FILES = glob.glob('hydrus/*.cc') + ['hydrus/_hydrus.pyx']
# Define Macros
HYDRUS_MACROS = []

# ---------------------------------------------
#  Helpers
# ---------------------------------------------
def get_version():
    hydrus_file = os.path.abspath(os.path.dirname(__file__) + '/hydrus/_hydrus.pyx')
    ver = '0.1.0'
    with open(hydrus_file, 'r') as f:
        text = f.read()
        matches = re.findall(r'__version__\s*=\s*(\S+)', text, re.I)
        if matches:
            ver = matches[0]
    return ver


# ---------------------------------------------
#   Setup start here
# ---------------------------------------------
setup(
    name='hydrus',
    package=['hydrus'],
    version=get_version(),
    license='BSD License',
    description='A lightweight and fast enough WSGI Server for Python',
    author='drz',
    keywords=('wsgi', 'server', 'web'),
    url='',
    setup_requires=['setuptools_cython', 'Cython'],
    ext_modules=cythonize([
        Extension(
            'hydrus._hydrus', HYDRUS_SRC_FILES,
            include_dirs=HYDRUS_INCLUDES,
            language='c++',
            libraries=HYDRUS_LIBRARIES,
            extra_compile_args=HYDRUS_BUILD_FLAGS,
            define_macros=HYDRUS_MACROS
        )
    ])
)
