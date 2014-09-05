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
#include "KlawrBlueprintFactory.h"
#include "KlawrBlueprint.h"
#include "KlawrBlueprintGeneratedClass.h"
#include "KlawrScriptComponent.h"
#include "SKlawrBlueprintFactoryConfig.h"

UKlawrBlueprintFactory::UKlawrBlueprintFactory(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Super::SupportedClass = UKlawrBlueprint::StaticClass();
	Super::bCreateNew = true;
	Super::bEditAfterNew = false;
	Super::bEditorImport = false;
	Super::bText = false;
	
	Super::Formats.Add(TEXT("cs;C# Source"));
}

bool UKlawrBlueprintFactory::DoesSupportClass(UClass* Class)
{
	return Class == UKlawrBlueprint::StaticClass();
}

UObject* UKlawrBlueprintFactory::FactoryCreateNew(
	UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context,
	FFeedbackContext* Warn
)
{
	TSharedRef<SKlawrBlueprintFactoryConfig> Widget = 
		SNew(SKlawrBlueprintFactoryConfig)
		.SourceFilename(InName.ToString());

	if (!Widget->ShowAsModalWindow())
	{
		return nullptr;
	}
	
	SourceFilename = Widget->GetSourceFilename();
	// convert the absolute path from the config dialog to be relative to the project root
	FString AbsGameDir = FPaths::ConvertRelativePathToFull(FPaths::GameDir());
	SourceLocation = Widget->GetSourceLocation();
	FPaths::MakePathRelativeTo(SourceLocation, *AbsGameDir);
	
	GenerateScriptFile();
	
	auto NewBlueprint = CastChecked<UKlawrBlueprint>(
		FKismetEditorUtilities::CreateBlueprint(
			UKlawrScriptComponent::StaticClass(), InParent, InName, BPTYPE_Normal, 
			UKlawrBlueprint::StaticClass(),	UKlawrBlueprintGeneratedClass::StaticClass(), 
			"UKlawrBlueprintFactory"
		)
	);
	
	NewBlueprint->ScriptDefinedType = FString::Printf(
		TEXT("%s.%s"), 
		*FPaths::GetBaseFilename(FPaths::GetProjectFilePath()), 
		*FPaths::GetBaseFilename(SourceFilename)
	);
	NewBlueprint->SourceFilePath = FPaths::Combine(*SourceLocation, *SourceFilename);
	FString ResolvedSourceFilePath = 
		FPaths::Combine(*FPaths::GameDir(), *NewBlueprint->SourceFilePath);
	NewBlueprint->SourceTimeStamp = 
		IFileManager::Get().GetTimeStamp(*ResolvedSourceFilePath).ToString();

	FKismetEditorUtilities::CompileBlueprint(NewBlueprint);
	FEditorDelegates::OnAssetPostImport.Broadcast(this, NewBlueprint);
	return NewBlueprint;
}

FString UKlawrBlueprintFactory::GetTemplatesDir()
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

void UKlawrBlueprintFactory::GenerateScriptFile()
{
	// copy a template from Resources folder to a location chosen by the user
	FString TemplatePath = GetTemplatesDir();
	if (TemplatePath.IsEmpty())
	{
		// TODO: display error
		return;
	}

	FString ProjectName = FPaths::GetBaseFilename(FPaths::GetProjectFilePath());
	// remove any spaces in the name
	ProjectName.ReplaceInline(TEXT(" "), TEXT(""));

	FString TemplateText;

	if (SourceFilename.EndsWith(TEXT(".cs")))
	{
		TemplatePath = FPaths::Combine(*TemplatePath, TEXT("Template.cs"));
		if (!FPaths::FileExists(TemplatePath))
		{
			// TODO: display error
			return;
		}

		// read in the template and replace template variables
		if (FFileHelper::LoadFileToString(TemplateText, *TemplatePath))
		{
			TemplateText.ReplaceInline(
				TEXT("$ProjectName$"), *ProjectName, ESearchCase::CaseSensitive
			);
			TemplateText.ReplaceInline(
				TEXT("$ScriptName$"), *FPaths::GetBaseFilename(SourceFilename), 
				ESearchCase::CaseSensitive
			);
		}
	}

	FString ScriptPath = FPaths::Combine(*FPaths::GameDir(), *SourceLocation, *SourceFilename);
	FFileHelper::SaveStringToFile(TemplateText, *ScriptPath);
}
