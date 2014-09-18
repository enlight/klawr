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

typedef TSharedPtr<class FScriptTypeTreeItem> FScriptTypeTreeItemPtr;
typedef TWeakPtr<class FScriptTypeTreeItem> FScriptTypeTreeItemWeakPtr;

/** 
 * @brief A widget that displays a tree of relevant types defined in the game scripts assembly or 
 *        any other assemblies loaded into the primary app domain.
 * 
 * Currently the relevant types consist of those derived from UKlawrScriptComponent.
 */
class SScriptTypeTree : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnScriptTypeSelected, const FString& /*ScriptType*/);

	SLATE_BEGIN_ARGS(SScriptTypeTree) {}
		/** Callback to be fired when a type is selected in the tree. */
		SLATE_EVENT(FOnScriptTypeSelected, OnScriptTypeSelected)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	/** Rebuild the tree from scratch. */
	void Reload();

private:
	FScriptTypeTreeItemPtr FindOrAddNamespaceItem(const FString& typeNamespace);
	void MergeNamespaceItems(TArray<FScriptTypeTreeItemPtr>& namespaceItems);
	void Populate();

	/** Called by TreeView to generate a table row for the given item. */
	TSharedRef<ITableRow> TreeView_OnGenerateRow(
		FScriptTypeTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable
	);

	/** Called by TreeView to obtain child items for the given parent item. */
	void TreeView_OnGetChildren(
		FScriptTypeTreeItemPtr Parent, TArray<FScriptTypeTreeItemPtr>& OutChildren
	);

	/** Called by TreeView when an item is selected in the tree. */
	void TreeView_OnSelectionChanged(FScriptTypeTreeItemPtr Item, ESelectInfo::Type SelectInfo);

private:
	TSharedPtr<STreeView<FScriptTypeTreeItemPtr>> TreeView;

	/** Namespaces that are the top-level items in the tree. */
	TArray<FScriptTypeTreeItemPtr> RootNamespaces;

	FOnScriptTypeSelected OnScriptTypeSelected;
};

} // namespace Klawr
