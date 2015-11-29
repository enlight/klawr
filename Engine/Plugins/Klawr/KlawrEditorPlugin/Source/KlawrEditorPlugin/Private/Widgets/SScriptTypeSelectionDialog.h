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
 * A dialog that lets the user select a type defined in a script.
 */
class SScriptTypeSelectionDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScriptTypeSelectionDialog) {}
		//SLATE_TEXT_ARGUMENT(DefaultSelection)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/** 
	 * @brief Display the dialog so the user can select a script type.
	 * @param DialogTitle The title the displayed dialog should be given.
	 * @param InDefaultSelection The name of the type that should be selected by default.
	 * @return The full name of the script type selected by the user, or an empty string if the
	 *         the user didn't select any type (for whatever reason).
	 */
	static FString SelectScriptType(const FText& DialogTitle, const FString& InDefaultSelection);

	FString GetDefaultSelection();
	void	SetDefaultSelection(FString& selection);

private:
	bool ShowAsModalWindow(const FText& WindowTitle);
	void OnScriptTypeSelected(const FString& InScriptType);
	FReply CreateNewScriptType_OnClicked();
	void CloseDialog(bool bOKClicked = false);
	FReply OK_OnClicked();
	FReply Cancel_OnClicked();

private:
	bool bUserConfirmed;
	TWeakPtr<SWindow> Dialog;
	TSharedPtr<class SScriptTypeTree> ScriptTypeTreeWidget;
	TSharedPtr<class SButton> OKButton;
	FString SelectedScriptType;
	FString DefaultSelection;
};

} // namespace Klawr
