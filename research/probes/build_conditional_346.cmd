@echo off
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /DNDEBUG /EHsc /MD /I include research\probes\http_prefix_oracle_321.cpp build-release\udon_shield.lib /Foartifacts\research\346\probe.obj /Feartifacts\research\346\probe.exe
exit /b %errorlevel%
