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
#include "SScriptTypeTree.h"
#include "IKlawrRuntimePlugin.h"

namespace Klawr {

/**
 * A tree item in an SScriptTypeTree widget.
 *
 * Tree items of this type can represent either a namespace or a type.
 * Namespaces can contain one or more types, and/or one or more namespaces.
 */
class FScriptTypeTreeItem : public TSharedFromThis<FScriptTypeTreeItem>
{
public:
	// text that will be displayed for this item in the tree view
	FText Title;
	// parents store shared pointers to children, 
	// so children must only store weak pointers to their parents to ensure proper cleanup
	FScriptTypeTreeItemWeakPtr Parent;
	TArray<FScriptTypeTreeItemPtr> Namespaces;
	TArray<FScriptTypeTreeItemPtr> Types;
	// unqualified name of the type (e.g. MyClass) or namespace (e.g. Weapons) 
	FString Name;
	// fully qualified type name (e.g. GameScripts.MyClass) or namespace (e.g. GameScripts.Weapons)
	FString FullName;

	FScriptTypeTreeItem(const FString& InName, const FScriptTypeTreeItemWeakPtr& InParent)
		: Name(InName)
		, Parent(InParent)
	{
		Title = FText::FromString(Name);
		if (InParent.IsValid())
		{
			FullName = InParent.Pin()->FullName + TEXT(".") + Name;
		}
		else
		{
			FullName = Name;
		}
	}

	bool IsType() const
	{
		return (Namespaces.Num() == 0) && (Types.Num() == 0);
	}
};

void SScriptTypeTree::Construct(const FArguments& InArgs)
{
	OnScriptTypeSelected = InArgs._OnScriptTypeSelected;

	ChildSlot
	[
		SAssignNew(TreeView, STreeView<FScriptTypeTreeItemPtr>)
		.SelectionMode(ESelectionMode::Single)
		.TreeItemsSource(&RootNamespaces)
		// get child items for any given parent item
		.OnGetChildren(this, &SScriptTypeTree::TreeView_OnGetChildren)
		// generate a widget for each item
		.OnGenerateRow(this, &SScriptTypeTree::TreeView_OnGenerateRow)
		.OnSelectionChanged(this, &SScriptTypeTree::TreeView_OnSelectionChanged)
	];

	Populate();
}

/** 
 * Find the tree item matching the given namespace, or add the components of the given namespace 
 * to the tree and return the tree item matching the last component of the namespace. Each component
 * of the namespace is represent by a single tree item.
 */
FScriptTypeTreeItemPtr SScriptTypeTree::FindOrAddNamespaceItem(const FString& typeNamespace)
{
	TArray<FString> namespaceComponents;
	typeNamespace.ParseIntoArray(namespaceComponents, TEXT("."), true);
	FScriptTypeTreeItemPtr namespaceItem;
	FScriptTypeTreeItemWeakPtr namespaceParent;
	auto* namespaceItems = &RootNamespaces;

	for (const FString& namespaceComponent : namespaceComponents)
	{
		auto predicate = [namespaceComponent](const FScriptTypeTreeItemPtr& item)
		{
			return item->Name == namespaceComponent;
		};

		const FScriptTypeTreeItemPtr* item = namespaceItems->FindByPredicate(predicate);
		if (item)
		{
			namespaceItem = *item;
			namespaceItems = &(namespaceItem->Namespaces);
		}
		else
		{
			namespaceItem = MakeShareable(
				new FScriptTypeTreeItem(namespaceComponent, namespaceParent)
			);
			namespaceItems->Add(namespaceItem);
			namespaceItems = &namespaceItem->Namespaces;
		}
		namespaceParent = namespaceItem;
	}
	return namespaceItem;
}

/** 
 * Recursively merge namespace components with their parent if they have no types and only a single
 * child component.
 */
void SScriptTypeTree::MergeNamespaceItems(TArray<FScriptTypeTreeItemPtr>& namespaceItems)
{
	for (int32 i = 0; i < namespaceItems.Num(); ++i)
	{
		auto& namespaceItem = namespaceItems[i];
		if ((namespaceItem->Types.Num() == 0) && (namespaceItem->Namespaces.Num() == 1))
		{
			auto childItem = namespaceItems[i]->Namespaces[0];
			childItem->Title = FText::FromString(childItem->FullName);
			childItem->Parent = namespaceItem->Parent;
			namespaceItems[i] = childItem;
		}
		if (namespaceItem->Namespaces.Num() > 0)
		{
			MergeNamespaceItems(namespaceItem->Namespaces);
		}
	}
}

void SScriptTypeTree::Populate()
{
	TArray<FString> typeNames;
	IKlawrRuntimePlugin::Get().GetScriptComponentTypes(typeNames);

	FScriptTypeTreeItemPtr namespaceItem;
	for (const auto& fullTypeName : typeNames)
	{
		// split the fully qualified type name into a name and a namespace
		FString typeName;
		FString typeNamespace;
		int32 lastDotIndex;
		if (fullTypeName.FindLastChar(TEXT('.'), lastDotIndex))
		{
			typeName = fullTypeName.Mid(lastDotIndex + 1);
			typeNamespace = fullTypeName.Left(lastDotIndex);
		}
		else
		{
			typeName = fullTypeName;
		}

		if (!namespaceItem.IsValid() || (namespaceItem->FullName != typeNamespace))
		{
			namespaceItem = FindOrAddNamespaceItem(typeNamespace);
		}
		
		if (ensure(namespaceItem.IsValid()))
		{
			namespaceItem->Types.Add(
				MakeShareable(new FScriptTypeTreeItem(typeName, namespaceItem))
			);
		}
	}

	MergeNamespaceItems(RootNamespaces);

	TreeView->RequestTreeRefresh();
}

void SScriptTypeTree::Reload()
{
	RootNamespaces.Reset();
	Populate();
}

TSharedRef<ITableRow> SScriptTypeTree::TreeView_OnGenerateRow(
	FScriptTypeTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable
)
{
	return 
		SNew(STableRow<FScriptTypeTreeItemPtr>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(Item->Title)
		];
}

void SScriptTypeTree::TreeView_OnGetChildren(
	FScriptTypeTreeItemPtr Parent, TArray<FScriptTypeTreeItemPtr>& OutChildren
)
{
	if (Parent.IsValid())
	{
		OutChildren.Reset();
		OutChildren.Append(Parent->Namespaces);
		OutChildren.Append(Parent->Types);
	}
}

void SScriptTypeTree::TreeView_OnSelectionChanged(
	FScriptTypeTreeItemPtr Item, ESelectInfo::Type SelectInfo
)
{
	OnScriptTypeSelected.ExecuteIfBound(
		(Item.IsValid() && Item->IsType()) ? Item->FullName : FString()
	);
}

} // namespace Klawr
