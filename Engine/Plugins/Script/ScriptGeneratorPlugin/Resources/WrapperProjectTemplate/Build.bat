set BATCH_FILE_LOCATION=%~dp0
pushd %BATCH_FILE_LOCATION%

:: Call the batch file that figures out the VS2013 tools path for us
if not exist "%BATCH_FILE_LOCATION%..\..\..\Build\BatchFiles" goto Error_BatchFileInWrongLocation
pushd "%BATCH_FILE_LOCATION%..\..\..\Build\BatchFiles"
call GetVSComnToolsPath 12
popd

:: Setup the VS2013 environment for command-line builds (though all we really need is MSBuild)
if "%VsComnToolsPath%" == "" goto Error_NoVisualStudio2013Environment
call "%VsComnToolsPath%/../../VC/bin/x86_amd64/vcvarsx86_amd64.bat" >NUL
goto ReadyToCompile

:ReadyToCompile
msbuild Klawr.UnrealEngine.csproj /t:Rebuild /p:Platform=AnyCPU /p:Configuration=Release /nologo
if %ERRORLEVEL% == 0 goto Exit
goto Error_BuildFailed

:Error_BuildFailed
echo ERROR: Failed to build Klawr.UnrealEngine.csproj
pause
goto Exit

:Error_BatchFileInWrongLocation
echo ERROR: The batch file does not appear to be located in the Engine\Intermediate\ProjectFiles\Klawr directory.
echo        This batch file must be run from that directory.
pause
goto Exit

:Error_NoVisualStudio2013Environment
echo ERROR: A valid version of Visual Studio doesn't appear to be installed.
pause
goto Exit

:Exit
popd
