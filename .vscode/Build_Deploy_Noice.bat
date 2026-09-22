@echo off
REM Enable delayed variable expansion for proper handling inside the loop
setlocal EnableDelayedExpansion


echo Closing NoICE12 and PEMicro tools...
REM Path to NoICE12 executable
set "NOICE_EXE=C:\Program Files (x86)\NoICE\bin\NoICE12.exe"

for %%P in (NoICE12.exe) do (
    tasklist /FI "IMAGENAME eq %%P" 2>nul | find /I "%%P" >nul
    if !ERRORLEVEL! EQU 0 taskkill /F /IM %%P /T >nul 2>&1
)

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

    @REM REM Delete .s files 
    if exist "*.s" del *.s

    @REM REM Delete .lis files 
    if exist "*.lis" del *.lis

    REM Execute the imakew command with the .mak file in the Build folder
    REM C:\iccv712\bin\imakew -f "%project_name%.mak"
    C:\iccv712\bin\imakew -f "%project_name%.mak"

    REM Launch NoICE12 if the build succeeded and the DBG file exists
    if !errorlevel! EQU 0 (
        set "dbg_file=%cd%\Build\%project_name%.dbg"
        if exist "!dbg_file!" (
            if exist "!NOICE_EXE!" (
                echo Launching NoICE12 with !dbg_file!
                start "" "!NOICE_EXE!" "!dbg_file!"
            ) else (
                echo WARNING: NoICE12 not found at !NOICE_EXE!
            )
        ) else (
            echo WARNING: DBG file not found: !dbg_file!
        )
    ) else (
        echo Build failed -- skipping NoICE12 launch
    )

    REM Return to the .vscode directory
    popd
) else (
    echo No .prj file found in the parent directory!
    @REM popd
    exit /b 1
)

@REM REM Return to the original directory
popd
