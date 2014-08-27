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

#include "KlawrCodeGeneratorPluginPrivatePCH.h"
#include "KlawrCodeGenerator.h"

DEFINE_LOG_CATEGORY(LogKlawrCodeGenerator);

namespace Klawr {

class FCodeGeneratorPlugin : public ICodeGeneratorPlugin
{
private:
	TAutoPtr<FCodeGenerator> CodeGenerator;

public: // IModuleInterface interface
	virtual void StartupModule() override
	{
	}
	
	virtual void ShutdownModule() override
	{
		CodeGenerator.Reset();
	}

public:	// IScriptGeneratorPlugin interface
	virtual FString GetGeneratedCodeModuleName() const override
	{
		return TEXT("ScriptPlugin"); 
	}
	
	virtual bool ShouldExportClassesForModule(
		const FString& ModuleName, EBuildModuleType::Type ModuleType
	) const
	{
		bool bCanExport = (ModuleType == EBuildModuleType::Runtime || ModuleType == EBuildModuleType::Game);
		if (bCanExport)
		{
			// only export functions from selected modules
			static struct FConfig
			{
				TArray<FString> SupportedModules;
				TArray<FString> ExcludedModules;
				FConfig()
				{
					GConfig->GetArray(
						TEXT("Plugins"), TEXT("ScriptSupportedModules"), SupportedModules, 
						GEngineIni
					);
					GConfig->GetArray(
						TEXT("Plugins"), TEXT("ScriptExcludedModules"), ExcludedModules,
						GEngineIni
					);
				}
			} Config;
			bCanExport = !Config.ExcludedModules.Contains(ModuleName) 
				&& ((Config.SupportedModules.Num() == 0) || Config.SupportedModules.Contains(ModuleName));
		}
		return bCanExport;
	}
	
	virtual bool SupportsTarget(const FString& TargetName) const override
	{
		return true;
	}
	
	virtual void Initialize(
		const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory, 
		const FString& IncludeBase
	) override
	{
		UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Using Klawr Code Generator."));
		CodeGenerator = new FCodeGenerator(RootLocalPath, RootBuildPath, OutputDirectory, IncludeBase);
		UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Output directory: %s"), *OutputDirectory);
	}
	
	virtual void ExportClass(
		UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, 
		bool bHasChanged
	) override
	{
		CodeGenerator->ExportClass(Class, SourceHeaderFilename, GeneratedHeaderFilename, bHasChanged);
	}
	
	virtual void FinishExport() override
	{
		CodeGenerator->FinishExport();
	}
};

} // namespace Klawr

IMPLEMENT_MODULE(Klawr::FCodeGeneratorPlugin, KlawrCodeGeneratorPlugin)
