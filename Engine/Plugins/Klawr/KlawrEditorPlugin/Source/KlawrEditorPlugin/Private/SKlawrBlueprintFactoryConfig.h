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

#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "KlawBlueprintFactory"

/**
 * Configuration dialog for Klaw Blueprint factory.
 */
class SKlawrBlueprintFactoryConfig : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SKlawrBlueprintFactoryConfig) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bUserConfirmed = false;
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
					.Text(LOCTEXT("SourceFilenameLabel", "Source Filename").ToString())
					.ToolTipText(
						LOCTEXT(
							"SourceFilename_ToolTip", 
							"Name for new source file (including extension)"
						).ToString()
					)
				]
				+ SGridPanel::Slot(1, 0)
				.Padding(UniformPadding)
				[
					SNew(SEditableTextBox)
					.Text(this, &SKlawrBlueprintFactoryConfig::SourceFilename_Text)
					.OnTextCommitted(
						this, &SKlawrBlueprintFactoryConfig::SourceFilename_OnTextCommitted
					)
					.OnTextChanged(
						this, &SKlawrBlueprintFactoryConfig::SourceFilename_OnTextCommitted, 
						ETextCommit::Default
					)
				]
				// source file location
				+ SGridPanel::Slot(0, 1)
				.Padding(UniformPadding)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SourceLocationLabel", "Source Location").ToString())
					.ToolTipText(
						LOCTEXT(
							"SourceLocation_ToolTip",
							"Directory where new source file should be created"
						).ToString()
					)
				]
				+ SGridPanel::Slot(1, 1)
				.Padding(UniformPadding)
				[
					SNew(SEditableTextBox)
					.Text(this, &SKlawrBlueprintFactoryConfig::SourceLocation_Text)
					.OnTextCommitted(
						this, &SKlawrBlueprintFactoryConfig::SourceLocation_OnTextCommitted
					)
					.OnTextChanged(
						this, &SKlawrBlueprintFactoryConfig::SourceLocation_OnTextCommitted, 
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
					.OnClicked(this, &SKlawrBlueprintFactoryConfig::SourceLocationBrowse_OnClicked)
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
					.OnClicked(this, &SKlawrBlueprintFactoryConfig::OK_OnClicked)
					.Text(LOCTEXT("OK", "OK"))
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					.OnClicked(this, &SKlawrBlueprintFactoryConfig::Cancel_OnClicked)
					.Text(LOCTEXT("Cancel", "Cancel"))
				]
			]
		];
	}

	bool ShowAsModalWindow()
	{
		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("ConfigurePropertiesTitle", "New Klawr Blueprint"))
			.ClientSize(FVector2D(500, 140))
			.SupportsMinimize(false)
			.SupportsMaximize(false)
			[
				AsShared()
			];
		
		Dialog = Window;
		GEditor->EditorAddModalWindow(Window);
		return bUserConfirmed;
	}

	const FString& GetSourceFilename() const
	{
		return SourceFilenameText.ToString();
	}

	/** Get the absolute location of the source file set by the user. */
	FString GetSourceLocation() const
	{
		FString SourceLocation = SourceLocationText.ToString();
		if (FPaths::IsRelative(SourceLocation))
		{
			return FPaths::ConvertRelativePathToFull(GetAbsoluteProjectDir(), SourceLocation);
		}
		return SourceLocation;
	}

private:

	FText SourceFilename_Text() const
	{
		return SourceFilenameText;
	}

	void SourceFilename_OnTextCommitted(
		const FText& InText, ETextCommit::Type InCommitType
	)
	{
		SourceFilenameText = InText;
	}

	FText SourceLocation_Text() const
	{
		return SourceLocationText;
	}

	void SourceLocation_OnTextCommitted(
		const FText& InText, ETextCommit::Type InCommitType
		)
	{
		SourceLocationText = InText;
	}

	static FString GetAbsoluteProjectDir()
	{
		FString ProjectDir = FPaths::GetPath(FPaths::GetProjectFilePath());
		ProjectDir = FPaths::ConvertRelativePathToFull(ProjectDir);
		FPaths::MakePlatformFilename(ProjectDir);
		if (!ProjectDir.EndsWith(FPlatformMisc::GetDefaultPathSeparator()))
		{
			ProjectDir.Append(FPlatformMisc::GetDefaultPathSeparator());
		}
		return ProjectDir;
	}

	static bool ValidateSourceLocation(const FString& InSourceLocation)
	{
		// the source location must be within the project directory
		FString SourceLocation = InSourceLocation;
		if (!FPaths::IsRelative(SourceLocation))
		{
			FPaths::MakePlatformFilename(SourceLocation);
			if (!SourceLocation.StartsWith(GetAbsoluteProjectDir()))
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

	FReply SourceLocationBrowse_OnClicked()
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			FString ProjectDir = GetAbsoluteProjectDir();
			FString SourceLocation = SourceLocationText.ToString();
				
			if (SourceLocation.IsEmpty())
			{
				if (IFileManager::Get().DirectoryExists(*FPaths::Combine(*ProjectDir, TEXT("Scripts"))))
				{
					SourceLocation = FPaths::Combine(*ProjectDir, TEXT("Scripts"));
				}
				else if (IFileManager::Get().DirectoryExists(*FPaths::Combine(*ProjectDir, TEXT("Source"))))
				{
					SourceLocation = FPaths::Combine(*ProjectDir, TEXT("Source"));
				}
			}
			else if (FPaths::IsRelative(SourceLocation))
			{
				SourceLocation = FPaths::ConvertRelativePathToFull(ProjectDir, SourceLocation);
			}

			bool bDirSelected = DesktopPlatform->OpenDirectoryDialog(
				nullptr,
				LOCTEXT("ChooseSourceLocationDialogTitle", "Choose location for new source file").ToString(),
				ProjectDir, SourceLocation
			);

			if (bDirSelected)
			{
				if (ValidateSourceLocation(SourceLocation))
				{
					if (!FPaths::IsRelative(SourceLocation))
					{
						FPaths::MakePathRelativeTo(SourceLocation, *ProjectDir);
					}
					SourceLocationText = FText::FromString(SourceLocation);
				}
			}
		}
		return FReply::Handled();
	}

	void CloseDialog(bool bOKClicked = false)
	{
		bUserConfirmed = bOKClicked;
		if (Dialog.IsValid())
		{
			Dialog.Pin()->RequestDestroyWindow();
		}
	}

	FReply OK_OnClicked()
	{
		if (ValidateSourceLocation(SourceLocationText.ToString()))
		{
			CloseDialog(true);
		}
		return FReply::Handled();
	}

	FReply Cancel_OnClicked()
	{
		CloseDialog();
		return FReply::Handled();
	}

private:
	FText SourceFilenameText;
	FText SourceLocationText;
	bool bUserConfirmed;
	TWeakPtr<SWindow> Dialog;
};

#undef LOCTEXT_NAMESPACE
