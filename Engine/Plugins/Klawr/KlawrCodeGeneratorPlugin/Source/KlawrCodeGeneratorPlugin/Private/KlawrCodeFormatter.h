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

/** Automatically indents and terminates lines of code. */
class FCodeFormatter
{
public:
	/** Keeps track of the indentation level in generated code. */
	class FIndent
	{
	public:
		/** String that should be prefixed to a line of text to ensure it is indented properly. */
		FString Text;

		FIndent(TCHAR InSpace, int32 InTabSize)
			: Level(0)
			, Space(InSpace)
			, TabSize(InTabSize)
		{
		}

		/** Increment the indent by one level. */
		FIndent& operator++() // prefix increment
		{
			++Level;
			Text = FString::ChrN(Level * TabSize, Space);
			return *this;
		}

		/** Decrement the indent by one level. */
		FIndent& operator--() // prefix decrement
		{
			check(Level > 0);
			--Level;
			Text = FString::ChrN(Level * TabSize, Space);
			return *this;
		}
	private:
		int32 Level;
		// character used for indentation
		TCHAR Space;
		// number of characters that make up a single tab
		int32 TabSize;
	};

	struct OpenBrace {};
	struct CloseBrace {};
	struct LineTerminator {};

	/** Current indent. */
	FIndent Indent;
	/** The formatted code. */
	FString Content;

	FCodeFormatter(TCHAR InSpace, int32 InTabSize)
		: Indent(InSpace, InTabSize)
	{
	}

	/** 
	 * Append the given line of code.
	 * The text will be indented according to the current indentation level, and line terminated.
	 */
	FCodeFormatter& operator<<(const FString& Text)
	{
		if (!Text.IsEmpty())
		{
			Content += Indent.Text + Text + LINE_TERMINATOR;
		}
		return *this;
	}

	/** 
	 * Append the given line of code.
	 * The text will be indented according to the current indentation level, and line terminated.
	 */
	FCodeFormatter& operator<<(const TCHAR* Text)
	{
		Content += Indent.Text + Text + LINE_TERMINATOR;
		return *this;
	}

	/** 
	 * Append an opening brace.
	 * A line terminator will be appended after the brace, and the indentation level will be 
	 * increased by one.
	 */
	FCodeFormatter& operator<<(const OpenBrace&)
	{
		Content += Indent.Text + TEXT("{") LINE_TERMINATOR;
		++Indent;
		return *this;
	}

	/** 
	 * Append a closing brace.
	 * The indentation level will be decreased by one, and a line terminator will be appended after
	 * the brace.
	 */
	FCodeFormatter& operator<<(const CloseBrace&)
	{
		--Indent;
		Content += Indent.Text + TEXT("}") LINE_TERMINATOR;
		return *this;
	}

	/** Append a line terminator. */
	FCodeFormatter& operator<<(const LineTerminator&)
	{
		Content += LINE_TERMINATOR;
		return *this;
	}
};

} // namespace
