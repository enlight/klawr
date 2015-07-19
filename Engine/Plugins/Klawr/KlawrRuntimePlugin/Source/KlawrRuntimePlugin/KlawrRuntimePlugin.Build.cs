using System.IO;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class KlawrRuntimePlugin : ModuleRules
	{
		public KlawrRuntimePlugin(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add other public include paths required here ...
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
                    "KlawrRuntimePlugin/Private"
					// ... add other private include paths required here ...
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
                    // ... add other public dependencies that you statically link with here ...
				}
			);

			if (UEBuildConfiguration.bBuildEditor == true)
			{

				PublicDependencyModuleNames.AddRange(
					new string[] 
					{
						"UnrealEd", 
					}
				);
			}

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
			);

			var KlawrPath = Path.Combine(UEBuildConfiguration.UEThirdPartySourceDirectory, "Klawr");
			if (Directory.Exists(KlawrPath))
			{
				Definitions.Add("WITH_KLAWR=1");
				PrivateDependencyModuleNames.Add("KlawrClrHostNative");
			}
		}
	}
}