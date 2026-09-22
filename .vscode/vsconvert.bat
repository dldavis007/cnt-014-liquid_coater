@REM echo Files moved and .SRC and .prj files modified successfully!
@echo off
REM Enable delayed variable expansion for proper handling inside the loop
setlocal EnableDelayedExpansion

REM Get the full absolute path of the parent directory of the .vscode folder
set "parent_dir=%~dp0.."
for %%a in ("%parent_dir%") do set "parent_dir=%%~fpa"

REM Ensure the parent directory path ends with a backslash
set "parent_dir=%parent_dir%\"

REM Rename any .src files to .SRC in the parent directory
for %%f in ("%parent_dir%*.src") do ren "%%f" "%%~nf.SRC"

REM Create the necessary directories in the parent folder
mkdir "%parent_dir%Build"
mkdir "%parent_dir%Document Files"
REM Note: No space in "SourceFiles" to avoid issues with ImageCraft Compiler (doesn't like spaces in paths)
mkdir "%parent_dir%SourceFiles" 

REM Move .txt files in the parent folder to "Document Files"
move "%parent_dir%*.txt" "%parent_dir%Document Files"

REM Move .c and .h files in the parent folder to "SourceFiles"
move "%parent_dir%*.c" "%parent_dir%SourceFiles"
move "%parent_dir%*.h" "%parent_dir%SourceFiles"

REM Move all remaining files in the parent folder except .SRC, .prj, and this batch file to "Build"
for %%f in ("%parent_dir%*.*") do (
    if NOT "%%~xf"==".SRC" if NOT "%%~xf"==".prj" if NOT "%%~nxf"=="%batch_file%" (
        move "%%f" "%parent_dir%Build"
    )
)

REM Process the .SRC file based on sections [Files], [Headers], and [Documents]
for %%f in ("%parent_dir%*.SRC") do (
    set "src_file=%%f"
)

REM Initialize section flag variables
set "in_files_section=0"
set "in_headers_section=0"
set "in_documents_section=0"

REM Read and process the .SRC file
(for /f "usebackq delims=" %%a in ("%src_file%") do (
    set "line=%%a"
    
    REM Detect section headers and set flags
    if "%%a"=="[Files]" (
        set "in_files_section=1"
        set "in_headers_section=0"
        set "in_documents_section=0"
        echo %%a
    ) else if "%%a"=="[Headers]" (
        set "in_files_section=0"
        set "in_headers_section=1"
        set "in_documents_section=0"
        echo %%a
    ) else if "%%a"=="[Documents]" (
        set "in_files_section=0"
        set "in_headers_section=0"
        set "in_documents_section=1"
        echo %%a
    ) else (
        REM Process lines in each section
        if !in_files_section!==1 (
            REM Remove any existing "Source Files\" prefix (with space) and add "SourceFiles\" (without space)
            set "modified_line=!line!"
            
            REM Check if line starts with "Source Files\" and remove it
            echo !line! | findstr /b /c:"Source Files\\" >nul
            if !errorlevel!==0 (
                REM Remove "Source Files\" prefix (11 characters + backslash = 12 chars)
                set "modified_line=!line:~12!"
            )
            
            REM Also check for "SourceFiles\" to avoid double-prefixing
            echo !modified_line! | findstr /b /c:"SourceFiles\\" >nul
            if !errorlevel!==0 (
                REM Line already has "SourceFiles\", output as-is
                echo !modified_line!
            ) else (
                REM Add "SourceFiles\" prefix
                echo SourceFiles\!modified_line!
            )
            
        ) else if !in_headers_section!==1 (
            REM Remove any existing "Source Files\" prefix (with space) and add "SourceFiles\" (without space)
            set "modified_line=!line!"
            
            REM Check if line starts with "Source Files\" and remove it
            echo !line! | findstr /b /c:"Source Files\\" >nul
            if !errorlevel!==0 (
                REM Remove "Source Files\" prefix
                set "modified_line=!line:~12!"
            )
            
            REM Also check for "SourceFiles\" to avoid double-prefixing
            echo !modified_line! | findstr /b /c:"SourceFiles\\" >nul
            if !errorlevel!==0 (
                REM Line already has "SourceFiles\", output as-is
                echo !modified_line!
            ) else (
                REM Add "SourceFiles\" prefix
                echo SourceFiles\!modified_line!
            )
            
        ) else if !in_documents_section!==1 (
            REM For Documents section, keep "Document Files\" with space
            set "modified_line=!line!"
            
            REM Check if line already starts with "Document Files\" to avoid double-prefixing
            echo !line! | findstr /b /c:"Document Files\\" >nul
            if !errorlevel!==0 (
                REM Line already has "Document Files\", output as-is
                echo !line!
            ) else (
                REM Add "Document Files\" prefix
                echo Document Files\!line!
            )
            
        ) else (
            REM Default case: Output the line unchanged
            echo %%a
        )
    )
)) > "%parent_dir%temp_file.txt"

REM Overwrite the original .SRC file with the modified contents
move /y "%parent_dir%temp_file.txt" "%src_file%"

REM Modify the line "Edit8=" in the .prj file to "Edit8=[parent dir]\Build\"
for %%f in ("%parent_dir%*.prj") do (
    set "prj_file=%%f"
)

(for /f "usebackq delims=" %%a in ("%prj_file%") do (
    echo %%a | findstr /r "^Edit8=" >nul
    if !errorlevel!==0 (
        echo Edit8=%parent_dir%Build\
    ) else (
        echo %%a
    )
)) > "%parent_dir%temp_prj.txt"

REM Overwrite the original .prj file with the modified contents
move /y "%parent_dir%temp_prj.txt" "%prj_file%"

echo Files moved and .SRC and .prj files modified successfully!