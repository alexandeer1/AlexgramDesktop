@echo OFF

set "FullScriptPath=%~dp0"

if "%VSCMD_ARG_TGT_ARCH%" neq "x64" (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
)

python "%FullScriptPath%prepare.py" %*
if %errorlevel% neq 0 goto error

exit /b

:error
echo FAILED
exit /b 1
