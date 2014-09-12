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
#include "KlawrScriptsReloader.h"
#include "KlawrGameProjectBuilder.h"
#include "DirectoryWatcherModule.h"
#include "IKlawrRuntimePlugin.h"

namespace Klawr {

FScriptsReloader* FScriptsReloader::Singleton = nullptr;

void FScriptsReloader::Startup()
{
	if (ensure(!Singleton))
	{
		Singleton = new FScriptsReloader();
	}
}

void FScriptsReloader::Shutdown()
{
	delete Singleton;
	Singleton = nullptr;
}

FScriptsReloader& FScriptsReloader::Get()
{
	check(Singleton);
	return *Singleton;
}

FScriptsReloader::~FScriptsReloader()
{
	if (bAutoReloadScripts)
	{
		Disable();
	}
}

void FScriptsReloader::OnSourceDirChanged(const TArray<FFileChangeData>& InFileChanges)
{
	// NOTE: for some reason InFileChanges sometimes contains duplicate entries
	for (auto& FileChange : InFileChanges)
	{
		const FString& Filename = FileChange.Filename;
		if (Filename.EndsWith(TEXT(".cs")))
		{
			switch (FileChange.Action)
			{
				case FFileChangeData::FCA_Added:
					NewScriptFiles.AddUnique(Filename);
					TimeOfLastModification = TimeSinceLastRefresh;
					break;

				case FFileChangeData::FCA_Modified:
					if (!NewScriptFiles.Contains(Filename))
					{
						// FIXME: there's actually no need to track the names of modified files, 
						// we only need to know that files were modified, the names are only
						// useful for debugging
						ModifiedScriptFiles.AddUnique(Filename);
						TimeOfLastModification = TimeSinceLastRefresh;
					}
					break;

				case FFileChangeData::FCA_Removed:
					NewScriptFiles.RemoveSingle(Filename);
					ModifiedScriptFiles.RemoveSingle(Filename);
					break;

				default:
					UE_LOG(
						LogKlawrEditorPlugin, Warning,
						TEXT("Ignoring unknown change to '%s'"), *Filename
					);
					break;
			}
		}
	}
}

void FScriptsReloader::OnBinaryDirChanged(const TArray<FFileChangeData>& FileChanges)
{
	for (auto& FileChange : FileChanges)
	{
		bool bActionRelevant = (FileChange.Action == FFileChangeData::FCA_Added)
			|| (FileChange.Action == FFileChangeData::FCA_Modified);

		const FString& Filename = FileChange.Filename;

		if (bActionRelevant && 
			Filename.EndsWith(TEXT(".dll")) &&
			(FPaths::GetBaseFilename(Filename) == FGameProjectBuilder::GetProjectAssemblyName()))
		{
			TimeOfLastModification = TimeSinceLastRefresh;
		}
	}
}

void FScriptsReloader::OnBeginPIE(const bool bIsSimulating)
{
	bIsPIEActive = true;
}

void FScriptsReloader::OnEndPIE(const bool bIsSimulating)
{
	bIsPIEActive = false;
}

void FScriptsReloader::Enable()
{
	FEditorDelegates::BeginPIE.AddRaw(this, &FScriptsReloader::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FScriptsReloader::OnEndPIE);

	LastScriptsAssemblyTimeStamp = IFileManager::Get().GetTimeStamp(
		*FGameProjectBuilder::GetProjectAssemblyFilename()
	);

	auto& DirWatcherModule = 
		FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");

	auto DirWatcher = DirWatcherModule.Get();

	if (!DirWatcher)
	{
		return;
	}

	TArray<FString> SourceDirs;
	FGameProjectBuilder::GetSourceDirs(SourceDirs);
	for (const FString& SourceDir : SourceDirs)
	{
		DirWatcher->RegisterDirectoryChangedCallback(
			SourceDir, 
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(
				this, &FScriptsReloader::OnSourceDirChanged
			)
		);
	}
	DirWatcher->RegisterDirectoryChangedCallback(
		FGameProjectBuilder::GetOutputDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FScriptsReloader::OnBinaryDirChanged)
	);

	bAutoReloadScripts = true;
}

void FScriptsReloader::Disable()
{
	bAutoReloadScripts = false;

	auto DirWatcherModule =
		FModuleManager::GetModulePtr<FDirectoryWatcherModule>("DirectoryWatcher");
		
	if (!DirWatcherModule)
	{
		return;
	}

	auto DirWatcher = DirWatcherModule->Get();
	if (DirWatcher)
	{
		return;
	}

	TArray<FString> SourceDirs;
	FGameProjectBuilder::GetSourceDirs(SourceDirs);
	for (const FString& SourceDir : SourceDirs)
	{
		DirWatcher->UnregisterDirectoryChangedCallback(
			SourceDir,
			IDirectoryWatcher::FDirectoryChanged::CreateRaw(
				this, &FScriptsReloader::OnSourceDirChanged
			)
		);
	}
	DirWatcher->UnregisterDirectoryChangedCallback(
		FGameProjectBuilder::GetOutputDir(),
		IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FScriptsReloader::OnBinaryDirChanged)
	);

	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
}

bool FScriptsReloader::ReloadScripts()
{
	// check the modification time of the game scripts assembly, reload immediately if it changed
	auto CurrentScriptsAssemblyTimeStamp = IFileManager::Get().GetTimeStamp(
		*FGameProjectBuilder::GetProjectAssemblyFilename()
	);

	if (CurrentScriptsAssemblyTimeStamp != LastScriptsAssemblyTimeStamp)
	{
		LastScriptsAssemblyTimeStamp = CurrentScriptsAssemblyTimeStamp;
		if (!IKlawrRuntimePlugin::Get().ReloadPrimaryAppDomain())
		{
			UE_LOG(
				LogKlawrEditorPlugin, Warning,
				TEXT("Failed to reload primary engine app domain!")
			);
			return false;
		}
	}
	return true;
}

void FScriptsReloader::Tick(float DeltaTime)
{
	TimeSinceLastRefresh += DeltaTime;
	bool bReload = false;

	// don't rebuild/reload while in PIE, delay until PIE exits
	if (bIsPIEActive)
	{
		return;
	}

	// this is how long we should wait (in seconds) after a user changes or adds a script file,
	// or the game scripts assembly is modified (possibly because the user rebuilt it)
	const double RefreshDelay = 3;
	if ((TimeSinceLastRefresh - TimeOfLastModification) < RefreshDelay)
	{
		return;
	}
	else
	{
		TimeSinceLastRefresh = 0;
		TimeOfLastModification = 0;
	}

	bool bProjectModified = false;
	bool bBuildFailed = false;
	
	if (NewScriptFiles.Num())
	{
		bProjectModified = FGameProjectBuilder::AddSourceFilesToProject(NewScriptFiles);
		NewScriptFiles.Reset();
	}

	if (bProjectModified || ModifiedScriptFiles.Num())
	{
		ModifiedScriptFiles.Reset();
		bBuildFailed = !FGameProjectBuilder::BuildProject(GWarn);
	}
	
	if (!bBuildFailed)
	{
		ReloadScripts();
	}
}

TStatId FScriptsReloader::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(IKlawrEditorPlugin, STATGROUP_Tickables);
}

} // namespace Klawr
