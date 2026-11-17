@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   Building Robot Warehouse Management ^& Automation System
echo ========================================================

if exist "D:\WinLibs\mingw64\bin" (
    set "PATH=D:\WinLibs\mingw64\bin;%PATH%"
) else if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin" (
    set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%"
)

if not exist "D:\Temp" mkdir "D:\Temp"
set "TEMP=D:\Temp"
set "TMP=D:\Temp"

echo [*] Build Environment Diagnostics:
echo     TEMP=%TEMP%
echo     TMP=%TMP%
echo [*] Resolved Toolchain Paths:
where g++
where cmake
where ninja

if not exist build mkdir build
if not exist data mkdir data

where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    goto compile_with_gpp
)

echo [*] CMake detected. Configuring build with CMake...
cd build
cmake -G "Ninja" ..
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b 1
)

echo [*] Building targets with Ninja...
ninja
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Ninja compilation encountered errors.
    cd ..
    exit /b 1
)

echo.
echo [*] Executing Regression Test Suite...
echo.
if exist WarehouseTests.exe (
    WarehouseTests.exe
) else if exist Release\WarehouseTests.exe (
    Release\WarehouseTests.exe
) else (
    echo [ERROR] Test binary WarehouseTests.exe not found after build!
    cd ..
    exit /b 1
)

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Tests failed! One or more regression tests encountered failures.
    cd ..
    exit /b 1
)

echo.
echo [*] Executing WarehouseCore CLI Demo...
echo.
if exist WarehouseCore.exe (
    WarehouseCore.exe
) else if exist Release\WarehouseCore.exe (
    Release\WarehouseCore.exe
)

cd ..
echo.
echo ========================================================
echo [SUCCESS] Build, tests, and CLI demonstration succeeded!
echo ========================================================
exit /b 0

:compile_with_gpp
where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Neither CMake nor g++ was found in PATH.
    echo Please ensure D:\WinLibs\mingw64\bin is accessible.
    exit /b 1
)

echo [*] Compiling via MinGW g++...
g++ -std=c++17 -O2 -Icore/include core/main.cpp core/src/*.cpp -o build/WarehouseCore.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Failed to compile WarehouseCore.
    exit /b 1
)

g++ -std=c++17 -O2 -Icore/include server/server.cpp core/src/*.cpp -lws2_32 -o build/WarehouseServer.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Failed to compile WarehouseServer.
    exit /b 1
)

g++ -std=c++17 -O2 -Icore/include tests/test_regression.cpp core/src/*.cpp -lws2_32 -o build/WarehouseTests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Failed to compile WarehouseTests.
    exit /b 1
)

echo.
echo [*] Executing Regression Tests...
build\WarehouseTests.exe
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Tests failed! Regression test suite reported failures.
    exit /b 1
)

echo.
echo [*] Executing WarehouseCore CLI Demo...
build\WarehouseCore.exe

echo.
echo ========================================================
echo [SUCCESS] Direct compilation and regression tests succeeded!
echo ========================================================
exit /b 0
