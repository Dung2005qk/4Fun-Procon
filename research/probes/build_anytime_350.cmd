@echo off
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
"D:\VSBuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S artifacts/research/350/source -B artifacts/research/350/build -G Ninja -DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/ninja.exe -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%
"D:\VSBuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build artifacts/research/350/build --target udonshield_btc udonshield_tests udonshield_claim_cap_347 udonshield_pricing_contract_344 udonshield_strategy_bench udonshield_master_oracle -j 2
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /O2 /EHsc /MD /I artifacts/research/350/source/include artifacts/research/350/probe.cpp artifacts/research/350/build/udon_shield.lib /Foartifacts/research/350/ /Feartifacts/research/350/probe.exe
exit /b %errorlevel%
