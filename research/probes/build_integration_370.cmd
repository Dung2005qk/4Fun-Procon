@echo off
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
"D:\VSBuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S artifacts/research/370/source -B artifacts/research/370/build -G Ninja -DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/ninja.exe -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%
"D:\VSBuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build artifacts/research/370/build --target udonshield_btc udonshield_tests udonshield_strategy_bench udonshield_master_oracle -j 2
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /EHsc /MD /I artifacts/research/370/source/include research/probes/resource_contract_357.cpp artifacts/research/370/build/udon_shield.lib /Foartifacts/research/370/ /Feartifacts/research/370/resource-contract.exe
exit /b %errorlevel%
