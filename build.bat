@echo off
setlocal enabledelayedexpansion

REM ==============================================
REM VOID Horror FPS — Windows Build Script (MSVC)
REM ==============================================

echo === VOID Horror FPS - Windows Build ===

REM Check for compiler
cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC 'cl' compiler not found.
    echo Please run this script from "x64 Native Tools Command Prompt for VS".
    exit /b 1
)

if not exist build mkdir build

echo [1/2] Compiling src files...
set CFLAGS=/nologo /std:c++17 /O2 /W3 /MD /I src
set LFLAGS=user32.lib gdi32.lib opengl32.lib winmm.lib

cl %CFLAGS% /Fe"build\void_game.exe" /Fo"build\\" src\main.cpp src\platform.cpp src\renderer.cpp src\audio.cpp src\world.cpp src\entity.cpp src\game.cpp src\vmath.cpp /link %LFLAGS%
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo [2/2] Build successful: build\void_game.exe

REM Optional UPX compression (if installed)
where upx >nul 2>&1
if %errorlevel% equ 0 (
    echo Compressing with UPX...
    upx --best --lzma build\void_game.exe
) else (
    echo [INFO] UPX not found, skipping compression.
)

echo.
echo You can run the game from build\void_game.exe
exit /b 0
