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
#include "SScriptFileCreationDialog.h"
#include "DesktopPlatformModule.h"
#include "KlawrGameProjectBuilder.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"

#define LOCTEXT_NAMESPACE "KlawrEditorPlugin.SScriptFileCreationDialog"

namespace Klawr {

void SScriptFileCreationDialog::Construct(const FArguments& InArgs)
{
	bUserConfirmed = false;
	FString sourceFilename = GetSourceFileName();
	if (!sourceFilename.EndsWith(TEXT(".cs")))
	{
		sourceFilename.Append(FString(TEXT(".cs")));
	}
	SourceFilenameText = FText::FromString(sourceFilename);
	SourceLocationText = FText::FromString(TEXT("Scripts"));
	float UniformPadding = 5;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SGridPanel)
			.FillColumn(1, 1.0f)
			.FillRow(0, 1.0f)
			.FillRow(1, 1.0f)
			// source filename
			+ SGridPanel::Slot(0, 0)
			.Padding(UniformPadding)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SourceFilenameLabel", "Source Filename"))
				.ToolTipText(
					LOCTEXT(
						"SourceFilename_ToolTip", 
						"Name for new source file (including extension)"
					)
				)
			]
			+ SGridPanel::Slot(1, 0)
			.Padding(UniformPadding)
			[
				SNew(SEditableTextBox)
				.Text(this, &SScriptFileCreationDialog::SourceFilename_Text)
				.OnTextCommitted(
					this, &SScriptFileCreationDialog::SourceFilename_OnTextCommitted
				)
				.OnTextChanged(
					this, &SScriptFileCreationDialog::SourceFilename_OnTextCommitted, 
					ETextCommit::Default
				)
			]
			// source file location
			+ SGridPanel::Slot(0, 1)
			.Padding(UniformPadding)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SourceLocationLabel", "Source Location"))
				.ToolTipText(
					LOCTEXT(
						"SourceLocation_ToolTip",
						"Directory where new source file should be created"
					)
				)
			]
			+ SGridPanel::Slot(1, 1)
			.Padding(UniformPadding)
			[
				SNew(SEditableTextBox)
				.Text(this, &SScriptFileCreationDialog::SourceLocation_Text)
				.OnTextCommitted(
					this, &SScriptFileCreationDialog::SourceLocation_OnTextCommitted
				)
				.OnTextChanged(
					this, &SScriptFileCreationDialog::SourceLocation_OnTextCommitted, 
					ETextCommit::Default
				)
			]
			+ SGridPanel::Slot(2, 1)
			.Padding(UniformPadding)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("BrowseLabel", "Browse"))
				.OnClicked(this, &SScriptFileCreationDialog::SourceLocationBrowse_OnClicked)
			]
		]
		// OK & Cancel
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(5, 20)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
			.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
			.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SScriptFileCreationDialog::OK_OnClicked)
				.Text(LOCTEXT("OK", "OK"))
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SScriptFileCreationDialog::Cancel_OnClicked)
				.Text(LOCTEXT("Cancel", "Cancel"))
			]
		]
	];
}

bool SScriptFileCreationDialog::ShowAsModalWindow()
{
	TSharedRef<SWindow> window = SNew(SWindow)
		.Title(LOCTEXT("DialogTitle", "Create New Script File"))
		.ClientSize(FVector2D(500, 140))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			AsShared()
		];

	Dialog = window;
	GEditor->EditorAddModalWindow(window);
	return bUserConfirmed;
}

FString SScriptFileCreationDialog::CreateScriptFile(const FString& DefaultScriptName)
{
	TSharedRef<SScriptFileCreationDialog> dialog =
		SNew(SScriptFileCreationDialog);

	if (dialog->ShowAsModalWindow())
	{
		FString sourceFilename = dialog->GetSourceFilename();
		// convert the absolute path from the dialog to be relative to the project root
		FString absGameDir = FPaths::ConvertRelativePathToFull(FPaths::GameDir());
		FString sourceLocation = dialog->GetSourceLocation();
		FPaths::MakePathRelativeTo(sourceLocation, *absGameDir);

		if (GenerateScriptFile(sourceLocation, sourceFilename))
		{
			return FPaths::GameDir() / sourceLocation / sourceFilename;
		}
	}

	return FString();
}

FString SScriptFileCreationDialog::GetSourceFileName()
{
	return this->SourceFileName;
}

const FString& SScriptFileCreationDialog::GetSourceFilename() const
{
	return SourceFilenameText.ToString();
}

/** Get the absolute location of the source file set by the user. */
FString SScriptFileCreationDialog::GetSourceLocation() const
{
	FString sourceLocation = SourceLocationText.ToString();
	if (FPaths::IsRelative(sourceLocation))
	{
		return FPaths::ConvertRelativePathToFull(GetAbsoluteProjectDir(), sourceLocation);
	}
	return sourceLocation;
}

FText SScriptFileCreationDialog::SourceFilename_Text() const
{
	return SourceFilenameText;
}

void SScriptFileCreationDialog::SourceFilename_OnTextCommitted(
	const FText& InText, ETextCommit::Type InCommitType
)
{
	SourceFilenameText = InText;
}

FText SScriptFileCreationDialog::SourceLocation_Text() const
{
	return SourceLocationText;
}

void SScriptFileCreationDialog::SourceLocation_OnTextCommitted(
	const FText& InText, ETextCommit::Type InCommitType
)
{
	SourceLocationText = InText;
}

FString SScriptFileCreationDialog::GetAbsoluteProjectDir()
{
	FString projectDir = FPaths::GetPath(FPaths::GetProjectFilePath());
	projectDir = FPaths::ConvertRelativePathToFull(projectDir);
	FPaths::MakePlatformFilename(projectDir);
	if (!projectDir.EndsWith(FPlatformMisc::GetDefaultPathSeparator()))
	{
		projectDir.Append(FPlatformMisc::GetDefaultPathSeparator());
	}
	return projectDir;
}

bool SScriptFileCreationDialog::ValidateSourceLocation(const FString& InSourceLocation)
{
	// the source location must be within the project directory
	FString sourceLocation = InSourceLocation;
	if (!FPaths::IsRelative(sourceLocation))
	{
		FPaths::MakePlatformFilename(sourceLocation);
		if (!sourceLocation.StartsWith(GetAbsoluteProjectDir()))
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				LOCTEXT(
					"SelectLocationWithinProjectDir",
					"Please select a location within the project directory."
				)
			);
			return false;
		}
	}
	return true;
}

FReply SScriptFileCreationDialog::SourceLocationBrowse_OnClicked()
{
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	if (desktopPlatform)
	{
		FString projectDir = GetAbsoluteProjectDir();
		FString sourceLocation = SourceLocationText.ToString();

		if (sourceLocation.IsEmpty())
		{
			if (IFileManager::Get().DirectoryExists(*FPaths::Combine(*projectDir, TEXT("Scripts"))))
			{
				sourceLocation = FPaths::Combine(*projectDir, TEXT("Scripts"));
			}
			else if (IFileManager::Get().DirectoryExists(*FPaths::Combine(*projectDir, TEXT("Source"))))
			{
				sourceLocation = FPaths::Combine(*projectDir, TEXT("Source"));
			}
		}
		else if (FPaths::IsRelative(sourceLocation))
		{
			sourceLocation = FPaths::ConvertRelativePathToFull(projectDir, sourceLocation);
		}

		bool bDirSelected = desktopPlatform->OpenDirectoryDialog(
			nullptr,
			LOCTEXT("ChooseSourceLocationDialogTitle", "Choose location for new source file").ToString(),
			projectDir, sourceLocation
		);

		if (bDirSelected)
		{
			if (ValidateSourceLocation(sourceLocation))
			{
				if (!FPaths::IsRelative(sourceLocation))
				{
					FPaths::MakePathRelativeTo(sourceLocation, *projectDir);
				}
				SourceLocationText = FText::FromString(sourceLocation);
			}
		}
	}
	return FReply::Handled();
}

void SScriptFileCreationDialog::CloseDialog(bool bOKClicked)
{
	bUserConfirmed = bOKClicked;
	if (Dialog.IsValid())
	{
		Dialog.Pin()->RequestDestroyWindow();
	}
}

FReply SScriptFileCreationDialog::OK_OnClicked()
{
	if (ValidateSourceLocation(SourceLocationText.ToString()))
	{
		CloseDialog(true);
	}
	return FReply::Handled();
}

FReply SScriptFileCreationDialog::Cancel_OnClicked()
{
	CloseDialog();
	return FReply::Handled();
}

bool SScriptFileCreationDialog::GenerateScriptFile(
	const FString& InSourceLocation, const FString& InSourceFilename
)
{
	// copy a template from Resources folder to a location chosen by the user
	FString templatePath = FGameProjectBuilder::GetTemplatesDir();
	if (templatePath.IsEmpty())
	{
		// TODO: display error
		return false;
	}

	FString templateText;

	if (InSourceFilename.EndsWith(TEXT(".cs")))
	{
		templatePath = templatePath / TEXT("Template.cs");
		if (!FPaths::FileExists(templatePath))
		{
			// TODO: display error
			return false;
		}

		// read in the template and replace template variables
		if (FFileHelper::LoadFileToString(templateText, *templatePath))
		{
			templateText.ReplaceInline(
				TEXT("$RootNamespace$"), *FGameProjectBuilder::GetProjectRootNamespace(),
				ESearchCase::CaseSensitive
			);
			templateText.ReplaceInline(
				TEXT("$ScriptName$"), *FPaths::GetBaseFilename(InSourceFilename),
				ESearchCase::CaseSensitive
			);
		}
	}

	FString scriptPath = FPaths::GameDir() / InSourceLocation / InSourceFilename;
	return FFileHelper::SaveStringToFile(templateText, *scriptPath);
}


} // namespace Klawr

#undef LOCTEXT_NAMESPACE
