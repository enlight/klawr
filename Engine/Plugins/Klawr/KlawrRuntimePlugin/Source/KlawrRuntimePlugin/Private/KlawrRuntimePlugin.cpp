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
#include "KlawrNativeUtils.h"
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
	int PrimaryEngineAppDomainID;

#if WITH_EDITOR
	int PIEAppDomainID;
#endif // WITH_EDITOR

public:

	FRuntimePlugin()
		: PrimaryEngineAppDomainID(0)
	{
#if WITH_EDITOR
		PIEAppDomainID = 0;
#endif // WITH_EDITOR
	}

#if WITH_EDITOR
	virtual void SetPIEAppDomainID(int AppDomainID) override
	{
		PIEAppDomainID = AppDomainID;
	}

	virtual bool ReloadPrimaryAppDomain() override
	{
		// to ensure that we don't end up without a primary engine app domain because any of the
		// assemblies couldn't be reloaded for whatever reason we create a new app domain before
		// getting rid of the current one
		bool bReloaded = false;
		int NewAppDomainID = 0;
		if (CreateAppDomain(NewAppDomainID))
		{
			if (DestroyPrimaryAppDomain())
			{
				PrimaryEngineAppDomainID = NewAppDomainID;
				bReloaded = true;
			}
			else
			{
				DestroyAppDomain(NewAppDomainID);
			}
		}
		else
		{
			UE_LOG(
				LogKlawrRuntimePlugin, Error,
				TEXT("Failed to create a new primary engine app domain!")
			);
		}
		return bReloaded;
	}

	virtual void GetScriptComponentTypes(TArray<FString>& Types) override
	{
		std::vector<tstring> scriptTypes;
		IClrHost::Get()->GetScriptComponentTypes(PrimaryEngineAppDomainID, scriptTypes);
		Types.Reserve(scriptTypes.size());
		for (const auto& scriptType : scriptTypes)
		{
			Types.Add(FString(scriptType.c_str()));
		}
	}
#endif // WITH_EDITOR

	virtual int GetObjectAppDomainID(const UObject* Object) const override
	{
#if WITH_EDITOR
		return Object->GetOutermost()->HasAnyPackageFlags(PKG_PlayInEditor) ?
			PIEAppDomainID : PrimaryEngineAppDomainID;
#else
		return PrimaryEngineAppDomainID;
#endif // WITH_EDITOR
	}

	virtual bool CreatePrimaryAppDomain() override
	{
		bool bCreated = false;

		if (ensure(PrimaryEngineAppDomainID == 0))
		{
			bCreated = CreateAppDomain(PrimaryEngineAppDomainID);

			if (!bCreated)
			{
				UE_LOG(
					LogKlawrRuntimePlugin, Error, 
					TEXT("Failed to create primary engine app domain!")
				);
			}
		}
		
		return bCreated;
	}

	virtual bool DestroyPrimaryAppDomain() override
	{
		bool bDestroyed = DestroyAppDomain(PrimaryEngineAppDomainID);
		if (bDestroyed)
		{
			PrimaryEngineAppDomainID = 0;
		}
		else
		{
			UE_LOG(
				LogKlawrRuntimePlugin, Error, 
				TEXT("Failed to destroy primary engine app domain!")
			);
		}
		return bDestroyed;
	}

	virtual bool CreateAppDomain(int& outAppDomainID) override
	{
		IClrHost* clrHost = IClrHost::Get();
		if (clrHost->CreateEngineAppDomain(outAppDomainID))
		{
			NativeUtils nativeUtils =
			{
				FNativeUtils::Object,
				FNativeUtils::Log,
				FNativeUtils::Array
			};
			return clrHost->InitEngineAppDomain(outAppDomainID, nativeUtils);
		}
		return false;
	}

	virtual bool DestroyAppDomain(int AppDomainID) override
	{
		// nothing to destroy if it doesn't exist
		if (AppDomainID == 0)
		{
			return true;
		}

		bool bDestroyed = IClrHost::Get()->DestroyEngineAppDomain(AppDomainID);

#if WITH_EDITOR
		// FIXME: This isn't very robust, need to improve!
		// hopefully the app domain was unloaded successfully and all references to native 
		// objects within that app domain were released, but in case something went wrong the
		// remaining references need to be cleared out so that UnrealEd can garbage collect the
		// native objects that were referenced in the app domain
		int32 NumReleased = FObjectReferencer::RemoveAllObjectRefsInAppDomain(AppDomainID);
		if (NumReleased > 0)
		{
			UE_LOG(
				LogKlawrRuntimePlugin, Warning,
				TEXT("%d object(s) still referenced in the engine app domain #%d."), 
				NumReleased, AppDomainID
			);
		}
#endif // WITH_EDITOR

		return bDestroyed;
	}

public: // IModuleInterface interface
	
	virtual void StartupModule() override
	{
		FObjectReferencer::Startup();
		FString GameAssembliesDir = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(
				*FPaths::GameDir(), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory(),
				TEXT("Klawr")
			)
		);
		
		if (IClrHost::Get()->Startup(*GameAssembliesDir, TEXT("GameScripts")))
		{
			NativeGlue::RegisterWrapperClasses();
#if !WITH_EDITOR
			// When running in the editor the primary app domain will be created when the Klawr 
			// editor plugin starts up, which will be after the runtime plugin, this is done so that
			// the scripts assembly can be built (if it is missing) before the primary engine app
			// domain attempts to load it.
			CreatePrimaryAppDomain();
#endif // !WITH_EDITOR
		}
		else
		{
			UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("Failed to start CLR!"));
		}
	}
	
	virtual void ShutdownModule() override
	{
		// the host will destroy all app domains on shutdown, there is no need to explicitly
		// destroy the primary app domain
		IClrHost::Get()->Shutdown();
		FObjectReferencer::Shutdown();
	}

	virtual bool SupportsDynamicReloading() override
	{
		// once the CLR has been stopped in a particular process it cannot be started again,
		// so this module cannot be reloaded
		return false;
	}
};

} // namespace Klawr

IMPLEMENT_MODULE(Klawr::FRuntimePlugin, KlawrRuntimePlugin)

#include "KlawrGeneratedNativeWrappers.inl"
