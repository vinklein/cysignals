set WKDIR=%cd%
cd %PYTHON%\\Lib\\distutils\\
git apply --stat %WKDIR%\\appveyorutil\\mingw64.python2.7.14.patch
git apply %WKDIR%\\appveyorutil\\mingw64.python2.7.14.patch
cd %WKDIR%
