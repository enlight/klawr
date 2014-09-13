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

#include "AssetTypeActions_Base.h"
#include "KlawrBlueprint.h"
#include "KlawrBlueprintEditor.h"

/** 
 * Stores UKlawrBlueprint related bits and pieces used by the asset registry, content browser, etc. 
 */
class FAssetTypeActions_KlawrBlueprint : public FAssetTypeActions_Base
{
public: // IAssetTypeActions interface
	virtual FText GetName() const override
	{
		return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_KlawrBlueprint", "Klawr Blueprint");
	}

	virtual UClass* GetSupportedClass() const override
	{
		return UKlawrBlueprint::StaticClass();
	}

	virtual FColor GetTypeColor() const override
	{
		return FColor(80, 123, 72);
	}

	virtual void OpenAssetEditor(
		const TArray<UObject*>& InObjects,
		TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()
	) override
	{
		const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ?
			EToolkitMode::WorldCentric : EToolkitMode::Standalone;

		for (auto Object : InObjects)
		{
			auto Blueprint = Cast<UBlueprint>(Object);
			if (Blueprint && Blueprint->SkeletonGeneratedClass && Blueprint->GeneratedClass)
			{
				TArray<UBlueprint*> Blueprints;
				Blueprints.Add(Blueprint);

				TSharedRef<FKlawrBlueprintEditor> Editor(new FKlawrBlueprintEditor());
				Editor->InitBlueprintEditor(Mode, EditWithinLevelEditor, Blueprints);
			}
		}
	}

	virtual uint32 GetCategories() override
	{
		return EAssetTypeCategories::Blueprint;
	}
};
