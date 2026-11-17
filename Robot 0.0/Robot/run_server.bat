@echo off
setlocal
echo Starting Warehouse C++ Backend Server on port 8080...
if exist "D:\WinLibs\mingw64\bin" (
    set "PATH=D:\WinLibs\mingw64\bin;%PATH%"
) else if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin" (
    set "PATH=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT.LLVM_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%"
)
cd /d "%~dp0"
build\WarehouseServer.exe
endlocal
