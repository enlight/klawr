//-------------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2014 Vadim Macagon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-------------------------------------------------------------------------------

#include "KlawrEditorPluginPrivatePCH.h"
#include "KlawrGameProjectBuilder.h"
#include "KlawrCSharpProject.h"
#include "FeedbackContextMarkup.h"

namespace Klawr {

namespace FGameProjectBuilderInternal 
{
	/** @return true if the .csproj was modified, false otherwise */
	bool AddSourceFileToProject(
		const TSharedRef<FCSharpProject>& Project, const FString& SourceFilename, 
		const TArray<FString>& SourceDirs
	)
	{
		const FString GameDir = FPaths::GameDir();
		FString LinkFilename = SourceFilename;
		FPaths::MakePathRelativeTo(LinkFilename, *GameDir);
		LinkFilename = GameDir / LinkFilename;
		for (const FString& SourceDir : SourceDirs)
		{
			if (LinkFilename.RemoveFromStart(SourceDir))
			{
				return Project->AddSourceFile(SourceFilename, LinkFilename);
			}
		}
		// source file is outside any of the source directories
		return false;
	}
} // FGameProjectBuilderInternal

const FString& FGameProjectBuilder::GetProjectFilename()
{
	static const FString ProjectFilename = FPaths::Combine(
		*FPaths::GameIntermediateDir(), TEXT("ProjectFiles"),
		*(FPaths::GetBaseFilename(FPaths::GetProjectFilePath()).Append(FString(TEXT("Scripts")))),
		*(FPaths::GetBaseFilename(FPaths::GetProjectFilePath()).Append(FString(TEXT("Scripts"))))
	).Append(FString(TEXT(".csproj")));

	return ProjectFilename;
}

bool FGameProjectBuilder::GenerateProject()
{
	const FString ProjectFilename = GetProjectFilename();
	const FString ProjectTemplateDir = GetTemplatesDir() / TEXT("GameProjectTemplate");
	const FString ProjectBasePath = FPaths::GetPath(ProjectFilename);

	
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	// CopyDirectoryTree() may fail if the destination directory doesn't exist
	if (!PlatformFile.CreateDirectoryTree(*ProjectBasePath))
	{
		// TODO: log error
		return false;
	}

	// copy the entire project template folder
	if (!PlatformFile.CopyDirectoryTree(*ProjectBasePath, *ProjectTemplateDir, true))
	{
		// TODO: log error
		return false;
	}

	// rename the template .csproj
	if (!IFileManager::Get().Move(*ProjectFilename, *(ProjectBasePath / TEXT("Game.csproj"))))
	{
		// TODO: log error
		return false;
	}

	// TODO: run a search/replace on AssemblyInfo.cs

	auto Project = FCSharpProject::Load(ProjectFilename);
	if (!Project.IsValid())
	{
		return false;
	}

	FGuid ProjectGuid;
	FPlatformMisc::CreateGuid(ProjectGuid);
	Project->SetProjectGuid(ProjectGuid);
	Project->SetRootNamespace(GetProjectRootNamespace());
	Project->SetAssemblyName(GetProjectAssemblyName());

	// set the directory to which the game scripts assembly and pdb files will be output to
	FString OutputPath = GetOutputDir();
	FPaths::MakePathRelativeTo(OutputPath, *ProjectFilename);
	FPaths::MakePlatformFilename(OutputPath);
	Project->SetOutputPath(OutputPath);

	Project->AddAssemblyReference(
		FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Klawr.ClrHost.Interfaces.dll")),
		false
	);
	Project->AddAssemblyReference(
		FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Klawr.ClrHost.Managed.dll")),
		false
	);
	Project->AddAssemblyReference(
		FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Klawr.UnrealEngine.dll")),
		false
	);
		
	// TODO: scan for assemblies in the script source directories and add them to project references,
	// the tricky part is figuring out if it's a native dll or an assembly

	// scan the script source directories for any source files and add them to the project
	TArray<FString> SourceDirs;
	GetSourceDirs(SourceDirs);
	TArray<FString> SourceFiles;

	for (const FString& SourceDir : SourceDirs)
	{
		IFileManager::Get().FindFilesRecursive(
			SourceFiles, *SourceDir, TEXT("*.cs"), true /* Files */, false /* Dirs */, false
		);
	}

	for (const FString& SourceFilename : SourceFiles)
	{
		FGameProjectBuilderInternal::AddSourceFileToProject(
			Project.ToSharedRef(), SourceFilename, SourceDirs
		);
	}

	return Project->SaveAs(ProjectFilename);
}

bool FGameProjectBuilder::AddSourceFileToProject(const FString& SourceFilename)
{
	auto Project = FCSharpProject::Load(GetProjectFilename());
	if (Project.IsValid())
	{
		TArray<FString> SourceDirs;
		GetSourceDirs(SourceDirs);

		bool bProjectModified = FGameProjectBuilderInternal::AddSourceFileToProject(
			Project.ToSharedRef(), SourceFilename, SourceDirs
		);
		
		return bProjectModified ? Project->Save() : false;
	}
	return GenerateProject();
}

bool FGameProjectBuilder::AddSourceFilesToProject(const TArray<FString>& SourceFiles)
{
	auto Project = FCSharpProject::Load(GetProjectFilename());
	if (Project.IsValid())
	{
		TArray<FString> SourceDirs;
		GetSourceDirs(SourceDirs);

		bool bProjectModified = false;
		for (const FString& SourceFilename : SourceFiles)
		{
			bProjectModified |= FGameProjectBuilderInternal::AddSourceFileToProject(
				Project.ToSharedRef(), SourceFilename, SourceDirs
			);
		}
		
		return bProjectModified ? Project->Save() : false;
	}
	return GenerateProject();
}

FString FGameProjectBuilder::GetTemplatesDir()
{
	FString BaseDir = FPaths::EnginePluginsDir();
	if (!BaseDir.IsEmpty())
	{
		FString CandidateDir = BaseDir / TEXT("Klawr/KlawrEditorPlugin/Resources/Templates");
		if (IFileManager::Get().DirectoryExists(*CandidateDir))
		{
			return CandidateDir;
		}
		else
		{
			CandidateDir = BaseDir / TEXT("KlawrEditorPlugin/Resources/Templates");
			if (IFileManager::Get().DirectoryExists(*CandidateDir))
			{
				return CandidateDir;
			}
		}
	}

	BaseDir = FPaths::GamePluginsDir();
	if (!BaseDir.IsEmpty())
	{
		FString CandidateDir = BaseDir / TEXT("Klawr/KlawrEditorPlugin/Resources/Templates");
		if (!IFileManager::Get().DirectoryExists(*CandidateDir))
		{
			return CandidateDir;
		}
		else
		{
			CandidateDir = BaseDir / TEXT("KlawrEditorPlugin/Resources/Templates");
			if (IFileManager::Get().DirectoryExists(*CandidateDir))
			{
				return CandidateDir;
			}
		}
	}

	return FString();
}

void FGameProjectBuilder::GetSourceDirs(TArray<FString>& OutSourceDirs)
{
	static FString SourceDirs[] =
	{
		FPaths::GameDir() / TEXT("Scripts/"),
		FPaths::GameDir() / TEXT("Source") / TEXT("Scripts/"),
	};
	
	OutSourceDirs.Reset();
	for (const FString& SourceDir : SourceDirs)
	{
		if (IFileManager::Get().DirectoryExists(*SourceDir))
		{
			OutSourceDirs.Add(SourceDir);
		}
	}
}

const FString& FGameProjectBuilder::GetProjectRootNamespace()
{
	static const FString RootNamespace = FPaths::GetBaseFilename(FPaths::GetProjectFilePath())
		.Replace(TEXT(" "), TEXT("")); // remove any spaces in the name

	return RootNamespace;
}

const FString& FGameProjectBuilder::GetProjectAssemblyName()
{
	static const FString AssemblyName = "GameScripts";
	
	return AssemblyName;
}

const FString& FGameProjectBuilder::GetProjectAssemblyFilename()
{
	static const FString AssemblyFilename = (GetOutputDir() / GetProjectAssemblyName())
		.Append(FString(TEXT(".dll")));

	return AssemblyFilename;
}

const FString& FGameProjectBuilder::GetOutputDir()
{
	static const FString OutputDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			*FPaths::GameDir(), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory(),
			TEXT("Klawr/ShadowCopy")
		)
	);
	return OutputDir;
}

bool FGameProjectBuilder::BuildProject(FFeedbackContext* Warn)
{
	// make sure the game scripts .csproj exists before attempting to build it
	if (!FPaths::FileExists(GetProjectFilename()))
	{
		if (!GenerateProject())
		{
			// TODO: log error
			return false;
		}
	}

	FString EnvironmentSetupFilename;
	if (!FPlatformMisc::GetVSComnTools(12 /* VS 2013 */, EnvironmentSetupFilename))
	{
		// TODO: log error
		return false;
	}
	EnvironmentSetupFilename /= TEXT("../../VC/bin/x86_amd64/vcvarsx86_amd64.bat");
	FPaths::CollapseRelativeDirectories(EnvironmentSetupFilename);
	FPaths::MakePlatformFilename(EnvironmentSetupFilename);

	if (!FPaths::FileExists(EnvironmentSetupFilename))
	{
		// TODO: log error
		return false;
	}

	FString BuildFilename = FPaths::GetPath(GetProjectFilename()) / TEXT("Build.bat");
	FPaths::CollapseRelativeDirectories(BuildFilename);
	FPaths::MakePlatformFilename(BuildFilename);
	FString Args = FString::Printf(
		TEXT("\"%s\" %s"), 
		*EnvironmentSetupFilename, *FPaths::GetCleanFilename(GetProjectFilename())
	);

	Warn->Log(TEXT("Compiling Scripts..."));
	Warn->Logf(TEXT("Running %s %s"), *BuildFilename, *Args);

	int32 ReturnCode = -1; // zero will indicate success
	bool bBuildExecuted = FFeedbackContextMarkup::PipeProcessOutput(
		NSLOCTEXT("KlawrGameProjectBuilder", "CompilingScripts", "Compiling Scripts..."), 
		BuildFilename, Args, Warn, &ReturnCode
	);
	bool bBuildSucceeded = bBuildExecuted && (ReturnCode == 0);

	if (bBuildSucceeded)
	{
		CopyPrivateReferencedAssemblies();
	}
	return bBuildSucceeded;
}

void FGameProjectBuilder::CopyPrivateReferencedAssemblies()
{
	static const FString DestDir = FPaths::Combine(
		*FPaths::GameDir(), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory(),
		TEXT("Klawr/Assemblies")
	);

	auto& FileManager = IFileManager::Get();

	// these assemblies don't need to be shadow copied, so they go into Klawr/Assemblies

	FileManager.Copy(
		*(DestDir / TEXT("Klawr.ClrHost.Interfaces.dll")),
		*FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Klawr.ClrHost.Interfaces.dll"))
	);

	FileManager.Copy(
		*(DestDir / TEXT("Klawr.ClrHost.Managed.dll")),
		*FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Klawr.ClrHost.Managed.dll"))
	);

	FileManager.Copy(
		*(DestDir / TEXT("Klawr.UnrealEngine.dll")),
		*FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Klawr.UnrealEngine.dll"))
	);
}

} // namespace Klawr
