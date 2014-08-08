:: Engine source is in a git repo, Klawr source is in a different git repo, no simple way to put the Klawr checkout inside Engine checkout due to a shared directory structure,
:: so create a couple of directory junctions and a few symbolic links in the Engine source that link to the Klawr source.
set ScriptGeneratorLinkDir=C:\Projects\UE4\UnrealEngine\Engine\Plugins\Script\ScriptGeneratorPlugin
set ScriptGeneratorTargetDir=C:\Projects\UE4\klawr\Engine\Plugins\Script\ScriptGeneratorPlugin
mklink /J C:\Projects\UE4\UnrealEngine\Engine\Source\ThirdParty\Klawr C:\Projects\UE4\klawr\Engine\Source\ThirdParty\Klawr
mklink /J %ScriptGeneratorLinkDir%\Resources\WrapperProjectTemplate %ScriptGeneratorTargetDir%\Resources\WrapperProjectTemplate
mklink %ScriptGeneratorLinkDir%\Source\ScriptGeneratorPlugin\Private\KlawrCodeFormatter.h %ScriptGeneratorTargetDir%\Source\ScriptGeneratorPlugin\Private\KlawrCodeFormatter.h
mklink %ScriptGeneratorLinkDir%\Source\ScriptGeneratorPlugin\Private\KlawrCodeGenerator.cpp %ScriptGeneratorTargetDir%\Source\ScriptGeneratorPlugin\Private\KlawrCodeGenerator.cpp
mklink %ScriptGeneratorLinkDir%\Source\ScriptGeneratorPlugin\Private\KlawrCodeGenerator.h %ScriptGeneratorTargetDir%\Source\ScriptGeneratorPlugin\Private\KlawrCodeGenerator.h
mklink %ScriptGeneratorLinkDir%\Source\ScriptGeneratorPlugin\Private\pugiconfig.hpp %ScriptGeneratorTargetDir%\Source\ScriptGeneratorPlugin\Private\pugiconfig.hpp
mklink %ScriptGeneratorLinkDir%\Source\ScriptGeneratorPlugin\Private\pugixml.cpp %ScriptGeneratorTargetDir%\Source\ScriptGeneratorPlugin\Private\pugixml.cpp
mklink %ScriptGeneratorLinkDir%\Source\ScriptGeneratorPlugin\Private\pugixml.hpp %ScriptGeneratorTargetDir%\Source\ScriptGeneratorPlugin\Private\pugixml.hpp
pause