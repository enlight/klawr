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
 * Dialog for creating a new script file.
 */
class SScriptFileCreationDialog : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SScriptFileCreationDialog) {}

		/** Initial value for Source Filename text-box. */
		SLATE_TEXT_ARGUMENT(SourceFilename)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
		
	/** 
	 * @brief Display the dialog so the user can create a new script file.
	 * @param DefaultScriptName The default name for the new script file, the user can overwrite it.
	 * @return The full name of the script file created by the user, or an empty string if no
	 *         script file was created (for whatever reason).
	 */
	static FString CreateScriptFile(const FString& DefaultScriptName);

private:
	bool ShowAsModalWindow();
	const FString& GetSourceFilename() const;
	FString GetSourceLocation() const;

	FText SourceFilename_Text() const;
	void SourceFilename_OnTextCommitted(
		const FText& InText, ETextCommit::Type InCommitType
	);

	FText SourceLocation_Text() const;
	void SourceLocation_OnTextCommitted(
		const FText& InText, ETextCommit::Type InCommitType
	);

	static FString GetAbsoluteProjectDir();

	static bool ValidateSourceLocation(const FString& InSourceLocation);
	FReply SourceLocationBrowse_OnClicked();

	void CloseDialog(bool bOKClicked = false);
	FReply OK_OnClicked();
	FReply Cancel_OnClicked();

	static bool GenerateScriptFile(const FString& InSourceLocation, const FString& InSourceFilename);

private:
	FText SourceFilenameText;
	FText SourceLocationText;
	bool bUserConfirmed;
	TWeakPtr<SWindow> Dialog;
};

} // namespace Klawr
