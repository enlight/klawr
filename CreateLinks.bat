:: Engine source is in a git repo, Klawr source is in a different git repo, no simple way to put the Klawr checkout inside Engine checkout due to a shared directory structure,
:: so create a couple of directory junctions and a few symbolic links in the Engine source that link to the Klawr source.
@echo off
set BATCH_FILE_LOCATION=%~dp0
set ScriptGeneratorBaseLinkDir=C:\Projects\UE4\UnrealEngine\Engine\Plugins\Script\ScriptGeneratorPlugin
set ScriptGeneratorBaseTargetDir=%BATCH_FILE_LOCATION%Engine\Plugins\Script\ScriptGeneratorPlugin
set ScriptGeneratorCodeLinkDir=%ScriptGeneratorBaseLinkDir%\Source\ScriptGeneratorPlugin
set ScriptGeneratorCodeTargetDir=%ScriptGeneratorBaseTargetDir%\Source\ScriptGeneratorPlugin
set ScriptPluginCodeLinkDir=C:\Projects\UE4\UnrealEngine\Engine\Plugins\Script\ScriptPlugin\Source\ScriptPlugin
set ScriptPluginCodeTargetDir=%BATCH_FILE_LOCATION%Engine\Plugins\Script\ScriptPlugin\Source\ScriptPlugin
@echo on

mklink /J C:\Projects\UE4\UnrealEngine\Engine\Source\ThirdParty\Klawr %BATCH_FILE_LOCATION%Engine\Source\ThirdParty\Klawr
mklink /J %ScriptGeneratorBaseLinkDir%\Resources\WrapperProjectTemplate %ScriptGeneratorBaseTargetDir%\Resources\WrapperProjectTemplate

mklink %ScriptGeneratorCodeLinkDir%\Private\KlawrCodeFormatter.h %ScriptGeneratorCodeTargetDir%\Private\KlawrCodeFormatter.h
mklink %ScriptGeneratorCodeLinkDir%\Private\KlawrCodeGenerator.cpp %ScriptGeneratorCodeTargetDir%\Private\KlawrCodeGenerator.cpp
mklink %ScriptGeneratorCodeLinkDir%\Private\KlawrCodeGenerator.h %ScriptGeneratorCodeTargetDir%\Private\KlawrCodeGenerator.h
mklink %ScriptGeneratorCodeLinkDir%\Private\pugiconfig.hpp %ScriptGeneratorCodeTargetDir%\Private\pugiconfig.hpp
mklink %ScriptGeneratorCodeLinkDir%\Private\pugixml.cpp %ScriptGeneratorCodeTargetDir%\Private\pugixml.cpp
mklink %ScriptGeneratorCodeLinkDir%\Private\pugixml.hpp %ScriptGeneratorCodeTargetDir%\Private\pugixml.hpp

mklink %ScriptPluginCodeLinkDir%\Private\KlawrObjectUtils.cpp %ScriptPluginCodeTargetDir%\Private\KlawrObjectUtils.cpp
mklink %ScriptPluginCodeLinkDir%\Private\KlawrObjectUtils.h %ScriptPluginCodeTargetDir%\Private\KlawrObjectUtils.h
mklink %ScriptPluginCodeLinkDir%\Private\KlawrScriptContext.cpp %ScriptPluginCodeTargetDir%\Private\KlawrScriptContext.cpp
mklink %ScriptPluginCodeLinkDir%\Private\KlawrScriptContext.h %ScriptPluginCodeTargetDir%\Private\KlawrScriptContext.h
pause