#!/usr/bin/env python
from setuptools import setup, Extension, find_packages
from distutils import sysconfig
import subprocess
import glob
import sys
try:
    from numpy.distutils.misc_util import get_info
    from os.path import dirname
    WITHNUMPY = True
except:
    WITHNUMPY = False

srcs = [x for x in 
    glob.glob("libbm/*.c")]
srcs.append("pybmtools.c")

libs=["m", "z"]

if 'dynamic_lookup' not in (sysconfig.get_config_var('LDSHARED') or ''):
    if sysconfig.get_config_vars('BLDLIBRARY') is not None:
        #Note the "-l" prefix!
        for e in sysconfig.get_config_vars('BLDLIBRARY')[0].split():
            if e[0:2] == "-l":
                libs.append(e[2:])
    elif sys.version_info[0] >= 3 and sys.version_info[1] >= 3:
        libs.append("python%i.%im" % (sys.version_info[0], sys.version_info[1]))
    else:
        libs.append("python%i.%i" % (sys.version_info[0], sys.version_info[1]))

additional_libs = [sysconfig.get_config_var("LIBDIR"), sysconfig.get_config_var("LIBPL")]

defines = []
try:
    foo, _ = subprocess.Popen(['curl-config', '--libs'], stdout=subprocess.PIPE).communicate()
    libs.append("curl")
    foo = foo.decode().strip().split()
except:
    foo = []
    defines.append(('NOCURL', None))
    sys.stderr.write("Either libcurl isn't installed, it didn't come with curl-config, or curl-config isn't in your $PATH. pyBM will be installed without support for remote files.\n")

for v in foo:
    if v[0:2] == '-L':
        additional_libs.append(v[2:])

include_dirs = ['libbm', sysconfig.get_config_var("INCLUDEPY")]
if WITHNUMPY is True:
    defines.extend([('WITHNUMPY', None), ('NPY_NO_DEPRECATED_API', 'NPY_1_7_API_VERSION')])
    extra_info = get_info('npymath')
    include_dirs.extend(extra_info['include_dirs'])
    libs.extend(extra_info['libraries'])
    extra_info['library_dirs'].extend(additional_libs)
    additional_libs = extra_info['library_dirs']

module1 = Extension('pybmtools',
                    sources = srcs,
                    libraries = libs,
                    library_dirs = additional_libs, 
                    define_macros = defines,
                    include_dirs = include_dirs)

setup(name = 'pybmtools',
       version = '0.1.1',
       description = 'A package for accessing binaMeth/bm files using libBMtools',
       author = 'momocoding',
       author_email = 'mail',
       url = "https://github.com/ZhouQiangwei/pybmtools.git",
       download_url = "",
       keywords = ["bioinformatics", "binaMeth", "BM", "bmtools"],
       classifier = ["Development Status :: 5 - Production/Stable",
                     "Intended Audience :: Developers",
                     "License :: OSI Approved",
                     "Programming Language :: C",
                     "Programming Language :: Python",
                     "Programming Language :: Python :: 2",
                     "Programming Language :: Python :: 2.7",
                     "Programming Language :: Python :: 3",
                     "Programming Language :: Python :: 3.5",
                     "Programming Language :: Python :: 3.6",
                     "Programming Language :: Python :: 3.7",
                     "Programming Language :: Python :: Implementation :: CPython",
                     "Operating System :: POSIX",
                     "Operating System :: Unix",
                     "Operating System :: MacOS"],
       packages = find_packages(),
       include_package_data = True,
       extras_require = {'numpy input': ["numpy"]},
       ext_modules = [module1])
