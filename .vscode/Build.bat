@echo off
REM Enable delayed variable expansion for proper handling inside the loop
setlocal EnableDelayedExpansion

REM Ensure we are in the .vscode directory
cd /d "%~dp0"

REM Navigate to the parent directory to look for a .prj file
pushd ..

REM Check for any .prj files in the parent directory
set "project_name="
for %%f in (*.prj) do (
    set "project_name=%%~nf"
)

REM If a .prj file is found, continue with the build process
if defined project_name (
    echo Found project: %project_name%

    REM Navigate to the Build folder (relative to the parent directory)
    pushd Build

    REM Delete .o files in the Build directory
    if exist "*.o" del *.o

    REM Execute the imakew command with the .mak file in the Build folder
    REM C:\iccv712\bin\imakew -f "%project_name%.mak"
    C:\iccv712\bin\imakew -f "%project_name%.mak"

    REM Return to the .vscode directory
    popd
) else (
    echo No .prj file found in the parent directory!
    popd
    exit /b 1
)

REM Return to the original directory
popd
