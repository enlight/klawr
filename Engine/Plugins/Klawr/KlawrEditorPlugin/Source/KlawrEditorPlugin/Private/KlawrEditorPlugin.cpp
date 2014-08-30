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

DEFINE_LOG_CATEGORY(LogKlawrEditorPlugin);

namespace Klawr {

class FEditorPlugin : public IKlawrEditorPlugin
{
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

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

public: // IModuleInterface interface
	virtual void StartupModule() override
	{
		// register asset types
		auto& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_KlawrBlueprint));
		
		// register Blueprint compiler
		auto& CompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
		FBlueprintCompileDelegate CompileDelegate;
		CompileDelegate.BindRaw(this, &FEditorPlugin::CompileBlueprint);
		CompilerModule.GetCompilers().Add(CompileDelegate);
	}

	virtual void ShutdownModule() override
	{
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
	}
};

} // namespace Klawr

IMPLEMENT_MODULE(Klawr::FEditorPlugin, KlawrEditorPlugin)
