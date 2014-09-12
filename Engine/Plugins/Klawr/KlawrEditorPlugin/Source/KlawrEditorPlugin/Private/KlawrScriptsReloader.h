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
#pragma once

namespace Klawr {

/**
 * @brief Automatically rebuilds and/or reloads the game scripts assembly.
 * This class will automatically rebuild the game scripts whenever the user modifies or adds 
 * source files in the script source directories, the rebuilt assembly is then reloaded.
 * The game scripts assembly will also be automatically reloaded if the user explicitly rebuilds
 * it in Visual Studio.
 */
class FScriptsReloader : public FTickableEditorObject
{
public:
	/** Create the singleton. */
	static void Startup();
	/** Destroy the singleton. */
	static void Shutdown();
	/** Get the singleton instance, can only be called after Startup(). */
	static FScriptsReloader& Get();

	/** Enable game scripts assembly rebuilding/reloading. */
	void Enable();
	/** Disable game scripts assembly rebuilding/reloading. */
	void Disable();
	/** 
	 * Reload the game scripts assembly if it was modified on disk since it was loaded. 
	 * @return false if the reload failed (but only if it was actually attempted), true otherwise
	 */
	bool ReloadScripts();

public: // FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bAutoReloadScripts; }
	virtual TStatId GetStatId() const override;

private:
	FScriptsReloader()
		: bAutoReloadScripts(false)
		, TimeSinceLastRefresh(0)
		, TimeOfLastModification(0)
		, bIsPIEActive(false)
	{
	}

	virtual ~FScriptsReloader();

	/** Called by the directory watcher when script files are added, modified, or removed. */
	void OnSourceDirChanged(const TArray<struct FFileChangeData>& InFileChanges);
	/** Called by the directory watcher when assemblies are added, modified, or removed. */
	void OnBinaryDirChanged(const TArray<struct FFileChangeData>& InFileChanges);
	/** Called when UnrealEd enters Play In Editor mode. */
	void OnBeginPIE(const bool bIsSimulating);
	/** Called when UnrealEd exits Play In Editor mode. */
	void OnEndPIE(const bool bIsSimulating);


private:
	// true when automatic rebuild and reload of the scripts assembly is enabled
	bool bAutoReloadScripts;
	double TimeSinceLastRefresh;
	double TimeOfLastModification;
	// true when UnrealEd is in Play In Editor mode
	bool bIsPIEActive;
	// source files user has added to the script source directories since last refresh
	TArray<FString> NewScriptFiles;
	// source files user has modified in the script source directories since last refresh
	TArray<FString> ModifiedScriptFiles;
	// time stamp of the game scripts assembly at the time it was last loaded
	FDateTime LastScriptsAssemblyTimeStamp;

	static FScriptsReloader* Singleton;
};

} // namespace Klawr
