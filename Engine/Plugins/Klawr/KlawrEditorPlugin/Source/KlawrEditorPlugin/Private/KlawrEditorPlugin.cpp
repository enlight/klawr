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
#include "KlawrBlueprint.h"
#include "KlawrBlueprintCompiler.h"
#include "KismetCompilerModule.h"
#include "AssetTypeActions_KlawrBlueprint.h"
#include "IKlawrRuntimePlugin.h"
#include "KlawrGameProjectBuilder.h"

DEFINE_LOG_CATEGORY(LogKlawrEditorPlugin);

namespace Klawr {

class FEditorPlugin : public IKlawrEditorPlugin
{
private:
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
	int PIEAppDomainID;

private:
	/** Called by the Blueprint compiler. */
	FReply CompileBlueprint(
		UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions,
		FCompilerResultsLog& Results, TArray<UObject*>* ObjLoaded
	)
	{
		if (auto KlawrBlueprint = Cast<UKlawrBlueprint>(Blueprint))
		{
			FBlueprintCompiler Compiler(KlawrBlueprint, Results, CompileOptions, ObjLoaded);
			Compiler.Compile();
			check(Compiler.NewClass);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

	void OnBeginPIE(const bool bIsSimulating)
	{
		UE_LOG(LogKlawrEditorPlugin, Display, TEXT("Creating a new app domain for PIE."));
		if (ensure(PIEAppDomainID == 0))
		{
			auto& Runtime = IKlawrRuntimePlugin::Get();
			if (Runtime.CreateAppDomain(PIEAppDomainID))
			{
				Runtime.SetPIEAppDomainID(PIEAppDomainID);
			}
			else
			{
				UE_LOG(LogKlawrEditorPlugin, Error, TEXT("Failed to create PIE app domain!"));
			}
		}
	}

	void OnEndPIE(const bool bIsSimulating)
	{
		UE_LOG(LogKlawrEditorPlugin, Display, TEXT("Unloading PIE app domain."));
		if (ensure(PIEAppDomainID != 0))
		{
			auto& Runtime = IKlawrRuntimePlugin::Get();
			if (!Runtime.DestroyAppDomain(PIEAppDomainID))
			{
				UE_LOG(
					LogKlawrEditorPlugin, Warning, TEXT("Failed to unload PIE app domain!")
				);
			}
			PIEAppDomainID = 0;
			Runtime.SetPIEAppDomainID(PIEAppDomainID);
		}
	}

public:
	FEditorPlugin()
		: PIEAppDomainID(0)
	{
	}

public: // IModuleInterface interface
	virtual void StartupModule() override
	{
		// check if game scripts assembly exists, if not build it
		if (!FPaths::FileExists(FGameProjectBuilder::GetProjectAssemblyFilename()))
		{
			// the .csproj may not exist yet, so generate it if need be
			if (!FPaths::FileExists(FGameProjectBuilder::GetProjectFilename()))
			{
				if (!FGameProjectBuilder::GenerateProject())
				{
					return;
				}
			}
			// FIXME: this shouldn't display any dialogs or anything, we're still loading modules
			// (though this seems to work fine for now)
			if (!FGameProjectBuilder::BuildProject(GWarn) || 
				!FPaths::FileExists(FGameProjectBuilder::GetProjectAssemblyFilename()))
			{
				UE_LOG(LogKlawrEditorPlugin, Error, TEXT("Failed to build scripts assembly."));
				return;
			}
		}

		IKlawrRuntimePlugin::Get().CreatePrimaryAppDomain();
		
		// register asset types
		auto& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_KlawrBlueprint));
		
		// register Blueprint compiler
		auto& CompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
		FBlueprintCompileDelegate CompileDelegate;
		CompileDelegate.BindRaw(this, &FEditorPlugin::CompileBlueprint);
		CompilerModule.GetCompilers().Add(CompileDelegate);

		FEditorDelegates::BeginPIE.AddRaw(this, &FEditorPlugin::OnBeginPIE);
		FEditorDelegates::EndPIE.AddRaw(this, &FEditorPlugin::OnEndPIE);
	}

	virtual void ShutdownModule() override
	{
		FEditorDelegates::BeginPIE.RemoveAll(this);
		FEditorDelegates::EndPIE.RemoveAll(this);

		// at this point the editor may have already unloaded the AssetTools module, 
		// in that case there's no need to unregister the previously registered asset types
		auto AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
		if (AssetToolsModule)
		{
			auto& AssetTools = AssetToolsModule->Get();
			for (auto Action : RegisteredAssetTypeActions)
			{
				AssetTools.UnregisterAssetTypeActions(Action);
			}
		}

		IKlawrRuntimePlugin::Get().DestroyPrimaryAppDomain();
	}
};

} // namespace Klawr

IMPLEMENT_MODULE(Klawr::FEditorPlugin, KlawrEditorPlugin)
