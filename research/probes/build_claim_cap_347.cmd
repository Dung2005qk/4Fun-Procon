@echo off
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
"D:\VSBuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S artifacts/research/347/source -B artifacts/research/347/build -G Ninja -DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/ninja.exe -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%
"D:\VSBuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build artifacts/research/347/build --target udonshield_btc udonshield_tests udonshield_claim_cap_347 udonshield_pricing_contract_344 udonshield_strategy_bench udonshield_master_oracle -j 2
exit /b %errorlevel%
