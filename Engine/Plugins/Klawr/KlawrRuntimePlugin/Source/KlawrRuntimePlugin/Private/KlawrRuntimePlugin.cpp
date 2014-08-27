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
public: // IModuleInterface interface
	
	virtual void StartupModule() override
	{
		FObjectReferencer::Startup();
		IClrHost::Get()->Startup();
		NativeGlue::RegisterWrapperClasses();
		IClrHost::Get()->InitializeEngineAppDomain(FObjectUtils::Info);
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
