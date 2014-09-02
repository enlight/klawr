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
#include "KlawrRuntimePluginPrivatePCH.h"
#include "KlawrClrHost.h"
#include "KlawrObjectUtils.h"
#include "KlawrObjectReferencer.h"

DEFINE_LOG_CATEGORY(LogKlawrRuntimePlugin);

namespace Klawr {

UProperty* FindScriptPropertyHelper(const UClass* Class, FName PropertyName)
{
	TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper);
	for ( ; PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		if (Property->GetFName() == PropertyName)
		{
			return Property;
		}
	}
	return nullptr;
}

namespace NativeGlue {

// defined in KlawrGeneratedNativeWrappers.inl (included down below)
void RegisterWrapperClasses();

} // namespace NativeGlue

class FRuntimePlugin : public IKlawrRuntimePlugin
{
	int _primaryEngineAppDomain;

#if WITH_EDITOR

	int _PIEAppDomain;

public:

	FRuntimePlugin()
		: _primaryEngineAppDomain(0)
		, _PIEAppDomain(0)
	{
	}

	virtual int GetObjectAppDomainID(const UObject* Object) const override
	{
		return (Object->GetOutermost()->PackageFlags & PKG_PlayInEditor) ?
			_PIEAppDomain : _primaryEngineAppDomain;
	}

	virtual void OnBeginPIE(bool bIsSimulating) override
	{
		UE_LOG(LogKlawrRuntimePlugin, Display, TEXT("Creating a new app domain for PIE."));
		if (ensure(_PIEAppDomain == 0))
		{
			if (!IClrHost::Get()->CreateEngineAppDomain(FObjectUtils::Info, _PIEAppDomain))
			{
				UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("Failed to create PIE app domain!"));
			}
		}
	}

	virtual void OnEndPIE(bool bIsSimulating) override
	{
		UE_LOG(LogKlawrRuntimePlugin, Display, TEXT("Unloading PIE app domain."));
		if (ensure(_PIEAppDomain != 0))
		{
			if (!IClrHost::Get()->DestroyEngineAppDomain(_PIEAppDomain))
			{
				UE_LOG(
					LogKlawrRuntimePlugin, Warning,	TEXT("Failed to unload PIE app domain!")
				);
			}
			// hopefully the PIE app domain was unloaded successfully and all references to native 
			// objects within that app domain were released, but in case something went wrong the
			// remaining references need to be cleared out so that UnrealEd can get rid of all the
			// objects it created for PIE
			int32 NumReleased = FObjectReferencer::RemoveAllObjectRefsInAppDomain(_PIEAppDomain);
			if (NumReleased > 0)
			{
				UE_LOG(
					LogKlawrRuntimePlugin, Warning, 
					TEXT("%d object(s) still referenced in PIE app domain."), NumReleased
				);
			}
			_PIEAppDomain = 0;
		}
	}

#elif // standalone build

	FRuntimePlugin()
		: _primaryEngineAppDomain(0)
	{
	}

	virtual int GetObjectAppDomainID(const UObject* Object) const override
	{
		return _primaryEngineAppDomain;
	}

#endif // WITH_EDITOR

public: // IModuleInterface interface
	
	virtual void StartupModule() override
	{
		FObjectReferencer::Startup();
		IClrHost::Get()->Startup();
		NativeGlue::RegisterWrapperClasses();
		if (!IClrHost::Get()->CreateEngineAppDomain(FObjectUtils::Info, _primaryEngineAppDomain))
		{
			UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("Failed to create primary engine app domain!"));
		}
	}
	
	virtual void ShutdownModule() override
	{
		IClrHost::Get()->Shutdown();
		FObjectReferencer::Shutdown();
	}
};

} // namespace Klawr

IMPLEMENT_MODULE(Klawr::FRuntimePlugin, KlawrRuntimePlugin)

#include "KlawrGeneratedNativeWrappers.inl"
