#!/usr/bin/env python

from setuptools import setup
from setuptools.extension import Extension
from distutils.command.build import build as _build

# Option "-D_hypot=hypot" is mandatory for mingw64
extensions = [Extension('cysignals_example',
                        ['cysignals_example.pyx'],
                        extra_compile_args=['-D_hypot=hypot'])]

class build(_build):
    def run(self):
        dist = self.distribution
        ext_modules = dist.ext_modules
        if ext_modules:
            dist.ext_modules[:] = self.cythonize(ext_modules)
        _build.run(self)

    def cythonize(self, extensions):
        from Cython.Build.Dependencies import cythonize
        return cythonize(extensions)


setup(
    name="cysignals_example",
    version='1.0',
    license='Public Domain',
    setup_requires=["Cython>=0.28"],
    ext_modules=extensions,
    cmdclass=dict(build=build),
)
