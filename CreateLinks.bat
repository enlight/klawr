:: Engine source is in a git repo, Klawr source is in a different git repo, no simple way to put the Klawr checkout inside Engine checkout due to a shared directory structure,
:: so create a couple of directory junctions and a few symbolic links in the Engine source that link to the Klawr source.

@echo off
color F

set BATCH_FILE_LOCATION=%~dp0

:GetDirectory
set /p ENGINE_SOURCE_LOCATION="Specify Unreal Engine 4 Source Directory: " % = %

if (%ENGINE_SOURCE_LOCATION%) == () (goto GetDirectory)

:CreateJunctions
echo Creating Engine\Plugins\Klawr Directory...
mkdir %ENGINE_SOURCE_LOCATION%\Engine\Plugins\Klawr\

echo Creating Engine\Source\ThirdParty\Klawr Junction...
mklink /J "%ENGINE_SOURCE_LOCATION%\Engine\Source\ThirdParty\Klawr" "%BATCH_FILE_LOCATION%\Engine\Source\ThirdParty\Klawr"

echo Creating Engine\Plugins\Klawr\KlawrCodeGeneratorPlugin Junction...
mklink /J "%ENGINE_SOURCE_LOCATION%\Engine\Plugins\Klawr\KlawrCodeGeneratorPlugin" "%BATCH_FILE_LOCATION%\Engine\Plugins\Klawr\KlawrCodeGeneratorPlugin"

echo Creating Engine\Plugins\Klawr\KlawrRuntimePlugin Junction...
mklink /J "%ENGINE_SOURCE_LOCATION%\Engine\Plugins\Klawr\KlawrRuntimePlugin" "%BATCH_FILE_LOCATION%\Engine\Plugins\Klawr\KlawrRuntimePlugin"

echo Creating Engine\Plugins\Klawr\KlawrEditorPlugin Junction...
mklink /J "%ENGINE_SOURCE_LOCATION%\Engine\Plugins\Klawr\KlawrEditorPlugin" "%BATCH_FILE_LOCATION%\Engine\Plugins\Klawr\KlawrEditorPlugin"

echo Done!
pause

