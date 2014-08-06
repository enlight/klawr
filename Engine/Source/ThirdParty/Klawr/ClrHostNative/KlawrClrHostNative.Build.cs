using UnrealBuildTool;
using System.IO;

public class KlawrClrHostNative : ModuleRules
{
	public KlawrClrHostNative(TargetInfo Target)
	{
        // This module is built externally, just need to let UBT know the include and library paths
        // that should be passed through to any targets that depend on this module.
		Type = ModuleType.External;

        var moduleName = this.GetType().Name;
		// path to directory containing this Build.cs file
        var basePath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(moduleName));
        
        string architecture = null;
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            architecture = "x64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            architecture = "x86";
        }

        string configuration = null;
        switch (Target.Configuration)
        {
            case UnrealTargetConfiguration.Debug:
            case UnrealTargetConfiguration.DebugGame:
                configuration = BuildConfiguration.bDebugBuildsActuallyUseDebugCRT ? "Debug" : "Release";
                break;

            default:
                configuration = "Release";
                break;
        }

        if ((architecture != null) && (configuration != null))
        {
            PublicIncludePaths.Add(Path.Combine(basePath, "Public"));
			var libName = "Klawr.ClrHost.Native-" + architecture + "-" + configuration + ".lib";
			PublicLibraryPaths.Add(Path.Combine(basePath, "..", "Build"));
            PublicAdditionalLibraries.Add(libName);
        }
                
        // copy the host assemblies (assumed to have been built previously) to the engine binaries 
        // directory so that they can be found and loaded at runtime by the unmanaged CLR host

        string hostAssemblyFilename = "Klawr.ClrHost.Managed.dll";
        string hostAssemblySourceDir = Path.Combine(
            basePath, Path.Combine("..", "ClrHostManaged", "bin", configuration)
        );
        Utils.CollapseRelativeDirectories(ref hostAssemblySourceDir);
        string hostInterfacesAssemblyFilename = "Klawr.ClrHost.Interfaces.dll";
        string hostInterfacesAssemblySourceDir = Path.Combine(
            basePath, Path.Combine("..", "ClrHostInterfaces", "bin", configuration)
        );
        Utils.CollapseRelativeDirectories(ref hostInterfacesAssemblySourceDir);
        string binariesDir = Path.Combine(
            BuildConfiguration.RelativeEnginePath, "Binaries", Target.Platform.ToString()
        );

        bool bOverwrite = true;
        File.Copy(
            Path.Combine(hostAssemblySourceDir, hostAssemblyFilename),
            Path.Combine(binariesDir, hostAssemblyFilename), 
            bOverwrite
        );
        File.Copy(
            Path.Combine(hostInterfacesAssemblySourceDir, hostInterfacesAssemblyFilename),
            Path.Combine(binariesDir, hostInterfacesAssemblyFilename),
            bOverwrite
        );
	}
}
