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
#include "KlawrScriptComponent.h"
#include "KlawrClrHost.h"
#include "KlawrBlueprintGeneratedClass.h"

UKlawrScriptComponent::UKlawrScriptComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Proxy(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
}

void UKlawrScriptComponent::CreateScriptComponentProxy()
{
	check(!Proxy);

	auto GeneratedClass = UKlawrBlueprintGeneratedClass::GetBlueprintGeneratedClass(GetClass());
	if (GeneratedClass)
	{
		Proxy = new Klawr::ScriptComponentProxy();
		bool bCreated = Klawr::IClrHost::Get()->CreateScriptComponent(
			IKlawrRuntimePlugin::Get().GetObjectAppDomainID(this),
			*GeneratedClass->ScriptDefinedType, this, *Proxy
		);

		if (!bCreated)
		{
			delete Proxy;
			Proxy = nullptr;
		}
		else
		{
			check(Proxy->InstanceID != 0);
		}
	}
}

void UKlawrScriptComponent::DestroyScriptComponentProxy()
{
	check(Proxy);

	if (Proxy->InstanceID != 0)
	{
		Klawr::IClrHost::Get()->DestroyScriptComponent(
			IKlawrRuntimePlugin::Get().GetObjectAppDomainID(this), Proxy->InstanceID
		);
	}

	delete Proxy;
	Proxy = nullptr;
}

void UKlawrScriptComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
		
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		CreateScriptComponentProxy();
		if (Proxy && Proxy->OnComponentCreated)
		{
			Proxy->OnComponentCreated();
		}
	}
}

void UKlawrScriptComponent::OnComponentDestroyed()
{
	if (Proxy)
	{
		if (Proxy->OnComponentDestroyed)
		{
			Proxy->OnComponentDestroyed();
		}

		DestroyScriptComponentProxy();
	}

	Super::OnComponentDestroyed();
}

void UKlawrScriptComponent::OnRegister()
{
	Super::OnRegister();

	if (!Proxy && !HasAnyFlags(RF_ClassDefaultObject))
	{
		CreateScriptComponentProxy();
	}

	if (Proxy && Proxy->OnRegister)
	{
		Proxy->OnRegister();
	}
}

void UKlawrScriptComponent::OnUnregister()
{
	if (Proxy)
	{
		if (Proxy->OnUnregister)
		{
			Proxy->OnUnregister();
		}
	
		// bHasBeenCreated should only be true if OnComponentCreated() was called, in which case
		// the proxy should only be destroyed in OnComponentDestroyed()
		if (!Super::bHasBeenCreated)
		{
			DestroyScriptComponentProxy();
		}
	}

	Super::OnUnregister();
}

void UKlawrScriptComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (Proxy && Proxy->InitializeComponent)
	{
		Proxy->InitializeComponent();
	}
}

void UKlawrScriptComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Proxy && Proxy->TickComponent)
	{
		Proxy->TickComponent(DeltaTime);
	}
}
