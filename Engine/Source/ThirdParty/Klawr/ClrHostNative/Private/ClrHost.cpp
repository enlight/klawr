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

#include "KlawrClrHostPCH.h"
#include "ClrHost.h"
#include "ClrHostControl.h"
#include <metahost.h>
#include "KlawrClrHostInterfaces.h"

#pragma comment(lib, "mscoree.lib")

namespace {

SAFEARRAY* CreateSafeArrayOfWrapperFunctions(void** wrapperFunctions, int numFunctions)
{
	SAFEARRAY* safeArray = SafeArrayCreateVector(VT_I8, 0, numFunctions);
	if (safeArray)
	{
		LONGLONG* safeArrayData = nullptr;
		HRESULT hr = SafeArrayAccessData(safeArray, (void**)&safeArrayData);
		if (SUCCEEDED(hr))
		{
			for (auto i = 0; i < numFunctions; ++i)
			{
				safeArrayData[i] = reinterpret_cast<LONGLONG>(wrapperFunctions[i]);
			}
			hr = SafeArrayUnaccessData(safeArray);
			assert(SUCCEEDED(hr));
		}
	}
	return safeArray;
}

} // unnamed namespace

namespace Klawr {

void ClrHost::Startup()
{
	_COM_SMARTPTR_TYPEDEF(ICLRMetaHost, IID_ICLRMetaHost);
	_COM_SMARTPTR_TYPEDEF(ICLRRuntimeInfo, IID_ICLRRuntimeInfo);
	_COM_SMARTPTR_TYPEDEF(ICLRControl, IID_ICLRControl);

	// bootstrap the CLR
	
	ICLRMetaHostPtr metaHost;
	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&metaHost));
	assert(SUCCEEDED(hr));

	// specify which version of the CLR should be used
	ICLRRuntimeInfoPtr runtimeInfo;
	hr = metaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&runtimeInfo));
	assert(SUCCEEDED(hr));

	// load the CLR (it won't be initialized just yet)
	hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&_runtimeHost));
	assert(SUCCEEDED(hr));

	// hook up our unmanaged host to the runtime host
	assert(!_hostControl);
	_hostControl = new ClrHostControl();
	hr = _runtimeHost->SetHostControl(_hostControl);
	assert(SUCCEEDED(hr));

	ICLRControlPtr clrControl;
	hr = _runtimeHost->GetCLRControl(&clrControl);
	assert(SUCCEEDED(hr));

	// by default the CLR runtime will look for the app domain manager assembly in the same 
	// directory as the application, which in this case will be 
	// C:\Program Files\Unreal Engine\4.X\Engine\Binaries\Win64 (or Win32)
	hr = clrControl->SetAppDomainManagerType(L"Klawr.ClrHost.Managed", L"Klawr.ClrHost.Managed.DefaultAppDomainManager");
	assert(SUCCEEDED(hr));

	// initialize the CLR (not strictly necessary because the runtime can initialize itself)
	hr = _runtimeHost->Start();
	assert(SUCCEEDED(hr));
}

void ClrHost::Shutdown()
{
	auto defaultAppDomainManager = _hostControl->GetDefaultAppDomainManager();
	if (defaultAppDomainManager)
	{
		defaultAppDomainManager->DestroyEngineAppDomain();
	}

	// NOTE: There's a crash here while debugging with the Mixed mode debugger, but everything works
	// fine when using the Auto mode debugger (which probably ends up using the Native debugger 
	// since this project is native). Everything also works fine if you detach the Mixed debugger 
	// before getting here.
	HRESULT hr = _runtimeHost->Stop();
	assert(SUCCEEDED(hr));

	if (_hostControl)
	{
		_hostControl->Release();
		_hostControl = nullptr;
	}
}

void ClrHost::InitializeEngineAppDomain()
{
	_hostControl->GetDefaultAppDomainManager()->CreateEngineAppDomain();
	auto engineAppDomainManager = _hostControl->GetEngineAppDomainManager();
	if (engineAppDomainManager)
	{
		for (const auto& classWrapper : _classWrappers)
		{
			auto className = classWrapper.first.c_str();
			auto& wrapperInfo = classWrapper.second;
			auto safeArray = CreateSafeArrayOfWrapperFunctions(
				wrapperInfo.functionPointers, wrapperInfo.numFunctions
			);
			HRESULT hr = engineAppDomainManager->SetNativeFunctionPointers(className, safeArray);
			assert(SUCCEEDED(hr));
		}
		engineAppDomainManager->LoadUnrealEngineWrapperAssembly();
	}
}

bool ClrHost::CreateScriptObject(const TCHAR* className, ScriptObjectInstanceInfo& info)
{
	Klawr_ClrHost_Interfaces::ScriptObjectInstanceInfo srcInfo;
	bool created = !!_hostControl->GetEngineAppDomainManager()->CreateScriptObject(className, &srcInfo);
	if (created)
	{
		info.InstanceID = srcInfo.InstanceID;
		info.BeginPlay = reinterpret_cast<ScriptObjectInstanceInfo::BeginPlayAction>(srcInfo.BeginPlay);
		info.Tick = reinterpret_cast<ScriptObjectInstanceInfo::TickAction>(srcInfo.Tick);
		info.Destroy = reinterpret_cast<ScriptObjectInstanceInfo::DestroyAction>(srcInfo.Destroy);
	}
	return created;
}

void ClrHost::DestroyScriptObject(__int64 instanceID)
{
	_hostControl->GetEngineAppDomainManager()->DestroyScriptObject(instanceID);
}

} // namespace Klawr
