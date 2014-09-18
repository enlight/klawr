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
#include "SScriptTypeSelectionDialog.h"
#include "SScriptTypeTree.h"
#include "SScriptFileCreationDialog.h"
#include "KlawrGameProjectBuilder.h"
#include "KlawrScriptsReloader.h"

#define LOCTEXT_NAMESPACE "KlawrEditorPlugin.SScriptTypeSelectionDialog"

namespace Klawr {

void SScriptTypeSelectionDialog::Construct(const FArguments& InArgs)
{
	DefaultSelection = InArgs._DefaultSelection;

	bUserConfirmed = false;

	const float leftPad = 5;
	const float rightPad = leftPad;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
		[
			SNew(SVerticalBox)
			// title for tree widget
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(leftPad, 10, rightPad, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ScriptTypeTreeTitle", "Select Script Type"))
			]
			// tree widget
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(leftPad, 10, rightPad, 0)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SBox)
					// keep the height of the tree fixed, show scrollbar if needed
					.HeightOverride(160)
					[
						SAssignNew(ScriptTypeTreeWidget, SScriptTypeTree)
						.OnScriptTypeSelected(this, &SScriptTypeSelectionDialog::OnScriptTypeSelected)
					]
				]
			]
			// buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			.Padding(leftPad, 20)
			[
				// Create New Script Type button
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					.OnClicked(this, &SScriptTypeSelectionDialog::CreateNewScriptType_OnClicked)
					.Text(LOCTEXT("CreateNewScript", "Create New Script Type"))
				]
				// spacer
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				// OK & Cancel buttons
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SAssignNew(OKButton, SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SScriptTypeSelectionDialog::OK_OnClicked)
						.Text(LOCTEXT("OK", "OK"))
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SScriptTypeSelectionDialog::Cancel_OnClicked)
						.Text(LOCTEXT("Cancel", "Cancel"))
					]
				]
			]
		]
	];

	// OK button is disabled until the user selects a type from the tree
	OKButton->SetEnabled(!SelectedScriptType.IsEmpty());
}

FString SScriptTypeSelectionDialog::SelectScriptType(
	const FText& DialogTitle, const FString& InDefaultSelection
)
{
	TSharedRef<Klawr::SScriptTypeSelectionDialog> dialog = 
		SNew(Klawr::SScriptTypeSelectionDialog)
		.DefaultSelection(InDefaultSelection);

	if (dialog->ShowAsModalWindow(DialogTitle))
	{
		return dialog->SelectedScriptType;
	}
	return FString();
}

bool SScriptTypeSelectionDialog::ShowAsModalWindow(const FText& WindowTitle)
{
	TSharedRef<SWindow> window = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(500, 300))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			AsShared()
		];

	Dialog = window;
	GEditor->EditorAddModalWindow(window);
	return bUserConfirmed;
}

void SScriptTypeSelectionDialog::OnScriptTypeSelected(const FString& InScriptType)
{
	SelectedScriptType = InScriptType;
	OKButton->SetEnabled(!SelectedScriptType.IsEmpty());
}

FReply SScriptTypeSelectionDialog::CreateNewScriptType_OnClicked()
{
	FString scriptFilename = SScriptFileCreationDialog::CreateScriptFile(DefaultSelection);
	if (!scriptFilename.IsEmpty())
	{
		// update the game scripts .csproj
		if (FGameProjectBuilder::AddSourceFileToProject(scriptFilename))
		{
			// build the game scripts .csproj
			if (FGameProjectBuilder::BuildProject(GWarn))
			{
				if (FScriptsReloader::Get().ReloadScripts())
				{
					ScriptTypeTreeWidget->Reload();
					// TODO: select newly created type
					//ScriptTypeTreeWidget->SelectType(
					//	FString::Printf(TEXT("%s.%s"), 
					//		*FGameProjectBuilder::GetProjectRootNamespace(),
					//		*FPaths::GetBaseFilename(scriptFilename)
					//	)
					//);
				}
			}
		}
	}

	return FReply::Handled();
}

void SScriptTypeSelectionDialog::CloseDialog(bool bOKClicked)
{
	bUserConfirmed = bOKClicked;
	if (Dialog.IsValid())
	{
		Dialog.Pin()->RequestDestroyWindow();
	}
}

FReply SScriptTypeSelectionDialog::OK_OnClicked()
{
	CloseDialog(true);
	return FReply::Handled();
}

FReply SScriptTypeSelectionDialog::Cancel_OnClicked()
{
	CloseDialog();
	return FReply::Handled();
}

} // namespace Klawr

#undef LOCTEXT_NAMESPACE
