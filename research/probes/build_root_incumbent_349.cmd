@echo off
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /EHsc /MD /I artifacts/research/347/source/include artifacts/research/349/probe.cpp artifacts/research/349/horizon_pricing.cpp artifacts/research/347/build/udon_shield.lib /Foartifacts/research/349/ /Feartifacts/research/349/probe.exe
exit /b %errorlevel%
