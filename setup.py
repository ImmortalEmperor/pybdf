#! /usr/bin/env python
# -*- coding: utf-8 -*-

from numpy.distutils.core import setup
from numpy.distutils.core import Extension
from sysconfig import get_paths
import pybind11, platform

extentions = []

if 'Win' not in platform.platform():
	extentions.append( Extension(   name = 'libforbdf',
		            		sources = ['libforbdf.f95']))

extentions.append( Extension(   name = 'libcppbdf',
                    sources = ['libcppbdf.cpp'],
                    include_dirs = [pybind11.get_include(), pybind11.get_include(True), '/home/aidan/.local/include/eigen/'],
                    language = 'c++'))

setup(name="pybdf",    
    version="0.2.5",
      py_modules=["pybdf"],
      ext_modules = extentions,
      author="Samuele Carcagno",
      author_email="sam.carcagno@gmail.com;",
      description="pybdf is a python library for reading BIOSEMI bdf files.",
      long_description=\
      """
      pybdf provides python functions to read BIOSEMI 24-bit BDF files (used for storing electroencephalographic recordings)
      The software is currently in **ALPHA** status. 
      """,
      license="GPL v3",
      url="https://github.com/sam81/pybdf",
      requires=['numpy (>=1.6.1)', 'pybind11 (>=2.2)'],
      classifiers=[
          'Development Status :: 3 - Alpha',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
          'Operating System :: MacOS :: MacOS X',
          'Operating System :: Microsoft :: Windows',
          'Operating System :: POSIX',
          'Programming Language :: Python :: 3',
          'Topic :: Scientific/Engineering :: Bio-Informatics'
          ]
      )
