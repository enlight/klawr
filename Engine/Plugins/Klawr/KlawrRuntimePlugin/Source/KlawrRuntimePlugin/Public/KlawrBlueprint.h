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

#include "Engine/Blueprint.h"
#include "KlawrBlueprint.generated.h"

/**
 * Generates UObject classes for script objects written in C# or any other CLI language.
 */
UCLASS(BlueprintType)
class KLAWRRUNTIMEPLUGIN_API UKlawrBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	static bool ValidateGeneratedClass(const UClass* Class);
#endif

public: // UBlueprint interface
#if WITH_EDITOR
	/** Get the class generated when this blueprint is compiled. */
	virtual UClass* GetBlueprintClass() const override;
	/** Check if the generic blueprint factory works with this blueprint (hint: it doesn't). */
	virtual bool SupportedByDefaultBlueprintFactory() const override { return true; }
#endif // WITH_EDITOR
};
