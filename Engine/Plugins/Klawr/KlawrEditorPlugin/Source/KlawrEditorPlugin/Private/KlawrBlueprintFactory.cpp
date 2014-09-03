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

bool UKlawrBlueprintFactory::ConfigureProperties()
{
	TSharedRef<SKlawrBlueprintFactoryConfig> Widget = SNew(SKlawrBlueprintFactoryConfig);
	if (Widget->ShowAsModalWindow())
	{
		ScriptName = Widget->GetSourceFilename();
		ScriptLocation = Widget->GetSourceLocation();
		return true;
	}
	return false;
}

UObject* UKlawrBlueprintFactory::FactoryCreateNew(
	UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context,
	FFeedbackContext* Warn
)
{
	GenerateScriptFile();
	// TODO: store path to new file in the Blueprint
	// TODO: store name of C# class in the Blueprint

	auto NewBlueprint = CastChecked<UKlawrBlueprint>(
		FKismetEditorUtilities::CreateBlueprint(
			UKlawrScriptComponent::StaticClass(), InParent, InName, BPTYPE_Normal, 
			UKlawrBlueprint::StaticClass(),	UKlawrBlueprintGeneratedClass::StaticClass(), 
			"UKlawrBlueprintFactory"
		)
	);

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
	}

	FString ScriptPath = FPaths::Combine(*ScriptLocation, *ScriptName);
	if (ScriptName.EndsWith(TEXT(".cs")))
	{
		TemplatePath = FPaths::Combine(*TemplatePath, TEXT("NewScript.cs"));
		// TODO: read in the template and replace the class name, then write it out to the new file
	}
}
