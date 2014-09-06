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

#include "pugixml.hpp"

namespace Klawr {

/**
 * Allows for simple manipulation of C# project files (.csproj).
 */
class FCSharpProject
{
public:
	static TSharedPtr<FCSharpProject> Load(const FString& InProjectFilename);
	bool Save();
	bool SaveAs(const FString& InProjectFilename);

	/**
	 * Add a source file to the project.
	 * @param SourceFilename Path to source file on disk.
	 * @param LinkFilename Path under which the source file should be displayed in the project,
	 *                     may be empty.
	 * @note SourceFilename and LinkFilename don't have to match, e.g. SourceFilename could be 
	 *       "../../Scripts/MyClass.cs" while LinkFilename is "MyScripts/MyClass.cs".
	 */
	void AddSourceFile(const FString& SourceFilename, const FString& LinkFilename);

	void AddAssemblyReference(const FString& AssemblyFilename);

	void SetProjectGuid(const FGuid& Guid);
	void SetRootNamespace(const FString& RootNamespace);
	void SetAssemblyName(const FString& AssemblyName);

private:
	// users should call the static Load() to obtain new instances
	FCSharpProject() { }

	bool LoadFromFile(const FString& InProjectFilename);

private:
	// absolute path to .csproj file
	FString ProjectFilename;
	pugi::xml_document XmlDoc;
	pugi::xml_node SourceNode;
	pugi::xml_node ReferencesNode;
};

} // namespace Klawr