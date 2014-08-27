:: Engine source is in a git repo, Klawr source is in a different git repo, no simple way to put the Klawr checkout inside Engine checkout due to a shared directory structure,
:: so create a couple of directory junctions and a few symbolic links in the Engine source that link to the Klawr source.
@echo off
set BATCH_FILE_LOCATION=%~dp0
set ENGINE_SOURCE_LOCATION=C:\Projects\UE4\UnrealEngine\
@echo on

mklink /J %ENGINE_SOURCE_LOCATION%Engine\Source\ThirdParty\Klawr %BATCH_FILE_LOCATION%Engine\Source\ThirdParty\Klawr
mklink /J %ENGINE_SOURCE_LOCATION%Engine\Plugins\Klawr\KlawrCodeGeneratorPlugin %BATCH_FILE_LOCATION%Engine\Plugins\Klawr\KlawrCodeGeneratorPlugin
mklink /J %ENGINE_SOURCE_LOCATION%Engine\Plugins\Klawr\KlawrRuntimePlugin %BATCH_FILE_LOCATION%Engine\Plugins\Klawr\KlawrRuntimePlugin
mklink /J %ENGINE_SOURCE_LOCATION%Engine\Plugins\Klawr\KlawrEditorPlugin %BATCH_FILE_LOCATION%Engine\Plugins\Klawr\KlawrEditorPlugin

pause