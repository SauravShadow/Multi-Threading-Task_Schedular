@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
echo.
echo === CMake Version ===
cmake --version
echo.
echo === Configuring project ===
if not exist build mkdir build
cmake -B build -S . -G "Visual Studio 16 2019" -A x64
echo.
echo === Building (Release) ===
cmake --build build --config Release
echo.
echo === Build Complete ===
echo Run: build\Release\task_schedular.exe
