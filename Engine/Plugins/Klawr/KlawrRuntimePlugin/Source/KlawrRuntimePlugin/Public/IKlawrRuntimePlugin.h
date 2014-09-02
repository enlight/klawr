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

#include "ModuleManager.h"

/**
 * Hosts the MS CLR, which is used to execute managed code in UnrealEd and in-game.
 */
class IKlawrRuntimePlugin : public IModuleInterface
{
public:
	/**
	 * Singleton-like access to this module's interface.
	 * @warning Don't call this during the shutdown phase, the module might have been unloaded already.
	 * @warning It is only valid to call Get() if IsAvailable() returns true.
	 * @return The singleton instance, loading the module on demand if needed.
	 */
	static inline IKlawrRuntimePlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<IKlawrRuntimePlugin>("KlawrRuntimePlugin");
	}

	/**
	 * Checks to see if this module is loaded and ready. 
	 * @return True if the module is loaded and ready to use.
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("KlawrRuntimePlugin");
	}

	/**
	 * Get the ID of the app domain in which the given object is referenced.
	 */
	virtual int GetObjectAppDomainID(const UObject* Object) const = 0;

#if WITH_EDITOR

	virtual void OnBeginPIE(bool bIsSimulating) = 0;
	virtual void OnEndPIE(bool bIsSimulating) = 0;

#endif // WITH_EDITOR
};
