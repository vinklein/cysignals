set WKDIR=%cd%
cd %PYTHON%\\Lib\\distutils\\
git apply --stat %WKDIR%\\appveyorutil\\%1
git apply %WKDIR%\\appveyorutil\\%1
cd %WKDIR%
