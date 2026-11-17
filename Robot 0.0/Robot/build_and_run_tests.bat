@echo off
setlocal
echo ========================================================
echo Stopping Any Running Warehouse Processes
echo ========================================================
powershell -Command "Get-Process -Name WarehouseServer, WarehouseTests -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue" >nul 2>&1

echo ========================================================
echo Building Robot Warehouse System (C++ Backend ^& Tests)
echo ========================================================
if exist "D:\WinLibs\mingw64\bin" (
    set "PATH=D:\WinLibs\mingw64\bin;%PATH%"
) else if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin" (
    set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%"
)

set SCRIPT_DIR=%~dp0
if not exist "%SCRIPT_DIR%build" mkdir "%SCRIPT_DIR%build"
cd /d "%SCRIPT_DIR%build"
where cmake >nul 2>&1
if %ERRORLEVEL% equ 0 (
    cmake.exe --build .
) else (
    cd /d "%SCRIPT_DIR%"
    g++ -std=c++17 -O2 -Icore/include tests/test_regression.cpp core/src/*.cpp -lws2_32 -o build/WarehouseTests.exe
    cd /d "%SCRIPT_DIR%build"
)
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed with exit code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo Running C++ Regression ^& Cooperative Multi-Robot Tests
echo ========================================================
cd /d "%SCRIPT_DIR%"
build\WarehouseTests.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Tests failed with exit code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo All tests executed successfully!
echo ========================================================
endlocal
