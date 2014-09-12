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
#include "KlawrCSharpProject.h"

namespace Klawr {

TSharedPtr<FCSharpProject> FCSharpProject::Load(const FString& InProjectFilename)
{
	if (FPaths::FileExists(InProjectFilename))
	{
		TSharedPtr<FCSharpProject> Project(new FCSharpProject());
		return Project->LoadFromFile(InProjectFilename) ? Project : nullptr;
	}
	return nullptr;
}

bool FCSharpProject::LoadFromFile(const FString& InProjectFilename)
{
	ProjectFilename = InProjectFilename;
	FPaths::ConvertRelativePathToFull(ProjectFilename);
	FPaths::MakePlatformFilename(ProjectFilename);

	pugi::xml_parse_result Result = XmlDoc.load_file(
		*ProjectFilename, pugi::parse_default | pugi::parse_declaration | pugi::parse_comments
	);

	if (!Result)
	{
		// TODO: log error
		return false;
	}

	return true;
}

bool FCSharpProject::Save()
{
	return SaveAs(ProjectFilename);
}

bool FCSharpProject::SaveAs(const FString& InProjectFilename)
{
	FString NewProjectFilename = InProjectFilename;
	FPaths::ConvertRelativePathToFull(NewProjectFilename);
	FPaths::MakePlatformFilename(NewProjectFilename);

	// preserve the BOM and line endings of the template .csproj when writing out the new file
	unsigned int OutputFlags =
		pugi::format_default | pugi::format_write_bom | pugi::format_save_file_text;

	if (!XmlDoc.save_file(*NewProjectFilename, TEXT("  "), OutputFlags))
	{
		// TODO: log error
		return false;
	}

	return true;
}

bool FCSharpProject::AddSourceFile(const FString& InSourceFilename, const FString& InLinkFilename)
{
	// look for an existing ItemGroup containing the source files
	if (!SourceNode)
	{
		SourceNode = XmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Compile")).parent();
	}

	// if the ItemGroup doesn't exist yet create it
	if (!SourceNode)
	{
		SourceNode = XmlDoc.child(TEXT("Project")).append_child(TEXT("ItemGroup"));
	}
	
	if (SourceNode)
	{
		FString SourceFilename = InSourceFilename;
		FPaths::MakePathRelativeTo(SourceFilename, *ProjectFilename);
		FPaths::MakePlatformFilename(SourceFilename);

		// only add the source file if it's not already in the project
		if (!SourceNode.find_child_by_attribute(TEXT("Compile"), TEXT("Include"), *SourceFilename))
		{
			auto CompileNode = SourceNode.append_child(TEXT("Compile"));
			CompileNode.append_attribute(TEXT("Include")) = *SourceFilename;

			if (!InLinkFilename.IsEmpty())
			{
				FString LinkFilename = InLinkFilename;
				FPaths::MakePlatformFilename(LinkFilename);
				CompileNode.append_child(TEXT("Link")).text() = *LinkFilename;
			}
			return true;
		}
	}
	return false;
}

void FCSharpProject::AddAssemblyReference(const FString& InAssemblyFilename, bool bCopyLocal)
{
	// look for an existing ItemGroup containing the assembly references
	if (!ReferencesNode)
	{
		ReferencesNode = XmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Reference")).parent();
	}

	// if the ItemGroup doesn't exist yet create it
	if (!ReferencesNode)
	{
		ReferencesNode = XmlDoc.child(TEXT("Project")).append_child(TEXT("ItemGroup"));
	}

	if (ReferencesNode)
	{
		FString AssemblyFilename = FPaths::ConvertRelativePathToFull(InAssemblyFilename);
		FPaths::MakePlatformFilename(AssemblyFilename);

		auto RefNode = ReferencesNode.append_child(TEXT("Reference"));
		RefNode.append_attribute(TEXT("Include")) = *FPaths::GetCleanFilename(InAssemblyFilename);
		RefNode.append_child(TEXT("HintPath")).text() = *AssemblyFilename;
		if (!bCopyLocal)
		{
			RefNode.append_child(TEXT("Private")).text() = TEXT("False");
		}
	}
}

void FCSharpProject::SetProjectGuid(const FGuid& Guid)
{
	XmlDoc.first_element_by_path(TEXT("/Project/PropertyGroup/ProjectGuid"))
		.text() = *Guid.ToString(EGuidFormats::DigitsWithHyphensInBraces);
}

void FCSharpProject::SetRootNamespace(const FString& RootNamespace)
{
	XmlDoc.first_element_by_path(TEXT("/Project/PropertyGroup/RootNamespace"))
		.text() = *RootNamespace;
}

void FCSharpProject::SetAssemblyName(const FString& AssemblyName)
{
	XmlDoc.first_element_by_path(TEXT("/Project/PropertyGroup/AssemblyName"))
		.text() = *AssemblyName;
}

void FCSharpProject::SetOutputPath(
	const FString& OutputPath
)
{
	// each Configuration|Platform in the .csproj has its own OutputPath, need to set all of them
	for (auto propertyGroupNode = XmlDoc.child(TEXT("Project")).child(TEXT("PropertyGroup"));
		propertyGroupNode;
		propertyGroupNode = propertyGroupNode.next_sibling(TEXT("PropertyGroup")))
	{
		propertyGroupNode.child(TEXT("OutputPath")).text() = *OutputPath;
	}
}

} // namespace Klawr
