#!/usr/bin/env python
from importlib import import_module, util
import glob
import subprocess
import sys
import sysconfig
from os.path import dirname

from setuptools import Extension, find_packages, setup

if util.find_spec("numpy") is not None:
    np = import_module("numpy")
    WITHNUMPY = True
else:
    np = None
    WITHNUMPY = False

srcs = [x for x in 
    glob.glob("libdm/*.c")]
srcs.append("pydmtools.c")

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
    sys.stderr.write("Either libcurl isn't installed, it didn't come with curl-config, or curl-config isn't in your $PATH. pyDM will be installed without support for remote files.\n")

for v in foo:
    if v[0:2] == '-L':
        additional_libs.append(v[2:])

include_dirs = ['libdm', sysconfig.get_config_var("INCLUDEPY")]
if WITHNUMPY is True:
    defines.extend([('WITHNUMPY', None), ('NPY_NO_DEPRECATED_API', 'NPY_1_7_API_VERSION')])
    include_dirs.append(np.get_include())

module1 = Extension('pydmtools.pydmtools',
                    sources = srcs,
                    libraries = libs,
                    library_dirs = additional_libs, 
                    define_macros = defines,
                    include_dirs = include_dirs)
print(module1)

setup(name = 'pydmtools',
       version = '0.1.4',
       description = 'A Software Package for Accessing and Manipulating DM Files',
       author = "momocoding",
       author_email = "",
       url = "https://github.com/ZhouQiangwei/pydmtools.git",
       download_url = "",
       keywords = ["bioinformatics", "DNA methylation", "DM", "dmtools"],
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
       python_requires=">=3.8",
       ext_modules = [module1])
