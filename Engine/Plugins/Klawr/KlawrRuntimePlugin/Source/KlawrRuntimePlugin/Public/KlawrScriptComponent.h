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

#include "KlawrScriptComponent.generated.h"

namespace Klawr
{
	struct ScriptComponentProxy;
} // namespace Klawr

/**
 * A component whose functionality is implemented in C# or any other CLI language.
 */
UCLASS(BlueprintType)
class KLAWRRUNTIMEPLUGIN_API UKlawrScriptComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public: // UActorComponent interface
	
	/** 
	 * Called after OnComponentCreated() has been called for all default (native) components 
	 * attached to the actor.
	 */
	virtual void OnRegister() override;
	
	/** Called before OnComponentDestroyed() if the component is registered. */
	virtual void OnUnregister() override;
	
	/**
	 * Begin gameplay.
	 * At this point OnRegister() has been called on all components attached to the actor.
	 */
	virtual void InitializeComponent() override;
	
	/** Update the state of the component. */
	virtual void TickComponent(
		float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction
	) override;

private:
	void CreateScriptComponentProxy();
	void DestroyScriptComponentProxy();

private:
	// a proxy that represents the managed counterpart of this script component
	Klawr::ScriptComponentProxy* Proxy;
};
