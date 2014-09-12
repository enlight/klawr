namespace UnrealBuildTool.Rules
{
	public class KlawrEditorPlugin : ModuleRules
	{
		public KlawrEditorPlugin(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add other public include paths required here ...
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add other public dependencies that you statically link with here ...
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"UnrealEd",
					"AssetTools",
					"KlawrRuntimePlugin",
					"ClassViewer",
					"KismetCompiler",
					"Kismet",
					"BlueprintGraph",
                    "Slate",
                    "SlateCore",
                    "EditorStyle",
                    "DesktopPlatform",
                    "DirectoryWatcher",
					// ... add private dependencies that you statically link with here ...
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
			);
		}
	}
}