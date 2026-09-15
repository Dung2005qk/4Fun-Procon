@echo off
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /EHsc /MD /I artifacts/research/347/source/include artifacts/research/348/probe.cpp artifacts/research/348/horizon_pricing.cpp artifacts/research/347/build/udon_shield.lib /Foartifacts/research/348/ /Feartifacts/research/348/probe.exe
exit /b %errorlevel%
