:: Engine source is in a git repo, Klawr source is in a different git repo, no simple way to put the Klawr checkout inside Engine checkout due to a shared directory structure,
:: so create a couple of directory junctions in the Engine source that link to the Klawr source.

@echo off
color F

set BATCH_FILE_LOCATION=%~dp0

:GetDirectory
set /p ENGINE_SOURCE_LOCATION="Specify Unreal Engine 4 Source Directory: " % = %

if (%ENGINE_SOURCE_LOCATION%) == () (goto GetDirectory)

:CreateJunctions
echo Creating Engine\Source\ThirdParty\Klawr Junction...
mklink /J "%ENGINE_SOURCE_LOCATION%\Engine\Source\ThirdParty\Klawr" "%BATCH_FILE_LOCATION%\Engine\Source\ThirdParty\Klawr"

echo Creating Engine\Plugins\Klawr Junction...
mklink /J "%ENGINE_SOURCE_LOCATION%\Engine\Plugins\Klawr" "%BATCH_FILE_LOCATION%\Engine\Plugins\Klawr"

echo Done!
pause

