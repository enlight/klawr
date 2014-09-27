@echo off

set BATCH_FILE_LOCATION=%~dp0
pushd %BATCH_FILE_LOCATION%

:: The tilde removes quotes from the passed in arguments
if "%~1"=="" goto Error_MissingArgs
if "%~2"=="" goto Error_MissingArgs

set VCVARS_FILENAME=%1
set PROJECT_FILENAME=%2

:: Setup the VS2013 environment for command-line builds (though all we really need is MSBuild)
if not exist %VCVARS_FILENAME% goto Error_NoVisualStudio2013Environment
call %VCVARS_FILENAME% >NUL
goto ReadyToCompile

:UsageInfo
echo Build.bat VCVARS_FILENAME PROJECT_FILENAME
echo VCVARS_FILENAME: Path to vcvars batch file that sets up a VS2013 build environment.
echo PROJECT_FILENAME: Path to the .csproj that should be built.
goto Exit

:ReadyToCompile
msbuild %PROJECT_FILENAME% /t:Rebuild /p:Platform=AnyCPU /p:Configuration=Release /nologo /verbosity:minimal
if %ERRORLEVEL% == 0 goto Exit
goto Error_BuildFailed

:Error_MissingArgs
echo ERROR: Missing arguments for command.
goto UsageInfo

:Error_BuildFailed
echo ERROR: Failed to build %PROJECT_FILENAME%
goto Exit

:Error_NoVisualStudio2013Environment
echo ERROR: A valid version of Visual Studio doesn't appear to be installed.
goto Exit

:Exit
popd
