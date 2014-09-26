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

/** 
 * @brief Convert a one-dimensional COM SAFEARRAY to a std::vector.
 * This only works if TSafeArrayElement can be implicitly converted to TVectorElement.
 */
template<typename TSafeArrayElement, typename TVectorElement>
void SafeArrayToVector(SAFEARRAY* input, std::vector<TVectorElement>& output)
{
	LONG lowerBound, upperBound;
	HRESULT hr = SafeArrayGetLBound(input, 1, &lowerBound);
	if (FAILED(hr))
	{
		return;
	}
	hr = SafeArrayGetUBound(input, 1, &upperBound);
	if (FAILED(hr))
	{
		return;
	}
	LONG numElements = upperBound - lowerBound + 1;

	output.reserve(numElements);
	TSafeArrayElement* safeArrayData = nullptr;
	hr = SafeArrayAccessData(input, (void**)&safeArrayData);
	if (SUCCEEDED(hr))
	{
		for (auto i = 0; i < numElements; ++i)
		{
			output.push_back(safeArrayData[i]);
		}
		hr = SafeArrayUnaccessData(input);
		assert(SUCCEEDED(hr));
	}
}

} // unnamed namespace

namespace Klawr {

struct ProxySizeChecks
{
	static_assert(
		sizeof(Klawr_ClrHost_Interfaces::ObjectUtilsProxy) == sizeof(ObjectUtilsProxy),
		"ObjectUtilsProxy doesn't have the same size in native and managed code!"
	);

	static_assert(
		sizeof(Klawr_ClrHost_Interfaces::LogUtilsProxy) == sizeof(LogUtilsProxy),
		"LogUtilsProxy doesn't have the same size in native and managed code!"
	);

	static_assert(
		sizeof(Klawr_ClrHost_Interfaces::ScriptComponentProxy) == sizeof(ScriptComponentProxy),
		"ScriptComponentProxy doesn't have the same size in native and managed code!"
	);

};

bool ClrHost::Startup(const TCHAR* engineAppDomainAppBase, const TCHAR* gameScriptsAssemblyName)
{
	_COM_SMARTPTR_TYPEDEF(ICLRMetaHost, IID_ICLRMetaHost);
	_COM_SMARTPTR_TYPEDEF(ICLRRuntimeInfo, IID_ICLRRuntimeInfo);
	_COM_SMARTPTR_TYPEDEF(ICLRControl, IID_ICLRControl);

	_engineAppDomainAppBase = engineAppDomainAppBase;
	_gameScriptsAssemblyName = gameScriptsAssemblyName;

	// bootstrap the CLR
	
	ICLRMetaHostPtr metaHost;
	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&metaHost));
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// specify which version of the CLR should be used
	ICLRRuntimeInfoPtr runtimeInfo;
	hr = metaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&runtimeInfo));
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// load the CLR (it won't be initialized just yet)
	hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&_runtimeHost));
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// hook up our unmanaged host to the runtime host
	assert(!_hostControl);
	_hostControl = new ClrHostControl();
	hr = _runtimeHost->SetHostControl(_hostControl);
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	ICLRControlPtr clrControl;
	hr = _runtimeHost->GetCLRControl(&clrControl);
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// by default the CLR runtime will look for the app domain manager assembly in the same 
	// directory as the application, which in this case will be 
	// C:\Program Files\Unreal Engine\4.X\Engine\Binaries\Win64 (or Win32)
	hr = clrControl->SetAppDomainManagerType(
		L"Klawr.ClrHost.Managed", L"Klawr.ClrHost.Managed.DefaultAppDomainManager"
	);

	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// initialize the CLR (not strictly necessary because the runtime can initialize itself)
	hr = _runtimeHost->Start();
	return SUCCEEDED(hr);
}

void ClrHost::Shutdown()
{
	_hostControl->Shutdown();
	
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

bool ClrHost::CreateEngineAppDomain(int& outAppDomainID)
{
	outAppDomainID = _hostControl->GetDefaultAppDomainManager()->CreateEngineAppDomain(
		_engineAppDomainAppBase.c_str()
	);
	return _hostControl->GetEngineAppDomainManager(outAppDomainID) != nullptr;
}

bool ClrHost::InitEngineAppDomain(
	int appDomainID, const ObjectUtilsProxy& objectUtils, const LogUtilsProxy& logUtils
)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		// pass all the native wrapper functions to the managed side of the CLR host so that they 
		// can be hooked up to properties and methods of the generated C# wrapper classes (though 
		// that will happen a bit later)
		for (const auto& classWrapper : _classWrappers)
		{
			auto className = classWrapper.first.c_str();
			auto& wrapperInfo = classWrapper.second;
			auto safeArray = CreateSafeArrayOfWrapperFunctions(
				wrapperInfo.functionPointers, wrapperInfo.numFunctions
			);
			HRESULT hr = appDomainManager->SetNativeFunctionPointers(className, safeArray);
			assert(SUCCEEDED(hr));
		}

		// pass a few utility functions to the managed side to deal with native UObject instances
		appDomainManager->BindObjectUtils(
			reinterpret_cast<Klawr_ClrHost_Interfaces::ObjectUtilsProxy*>(
				const_cast<ObjectUtilsProxy*>(&objectUtils)
			)
		);

		appDomainManager->BindLogUtils(
			reinterpret_cast<Klawr_ClrHost_Interfaces::LogUtilsProxy*>(
				const_cast<LogUtilsProxy*>(&logUtils)
			)
		);

		// now that everything the engine wrapper assembly needs is in place it can be loaded
		appDomainManager->LoadUnrealEngineWrapperAssembly();
		appDomainManager->LoadAssembly(_gameScriptsAssemblyName.c_str());
	}
	return appDomainManager != nullptr;
}

bool ClrHost::DestroyEngineAppDomain(int appDomainID)
{
	return _hostControl->DestroyEngineAppDomain(appDomainID);
}

bool ClrHost::CreateScriptObject(
	int appDomainID, const TCHAR* className, class UObject* owner, ScriptObjectInstanceInfo& info
)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (!appDomainManager)
	{
		return false;
	}

	Klawr_ClrHost_Interfaces::ScriptObjectInstanceInfo srcInfo;
	bool created = !!appDomainManager->CreateScriptObject(
		className, reinterpret_cast<INT_PTR>(owner), &srcInfo
	);
	if (created)
	{
		info.InstanceID = srcInfo.InstanceID;
		info.BeginPlay = reinterpret_cast<ScriptObjectInstanceInfo::BeginPlayAction>(srcInfo.BeginPlay);
		info.Tick = reinterpret_cast<ScriptObjectInstanceInfo::TickAction>(srcInfo.Tick);
		info.Destroy = reinterpret_cast<ScriptObjectInstanceInfo::DestroyAction>(srcInfo.Destroy);
	}
	return created;
}

void ClrHost::DestroyScriptObject(int appDomainID, __int64 instanceID)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->DestroyScriptObject(instanceID);
	}
}

bool ClrHost::CreateScriptComponent(
	int appDomainID, const TCHAR* className, class UObject* nativeComponent, ScriptComponentProxy& proxy
)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (!appDomainManager)
	{
		return false;
	}

	return !!appDomainManager->CreateScriptComponent(
		className, reinterpret_cast<INT_PTR>(nativeComponent),
		reinterpret_cast<Klawr_ClrHost_Interfaces::ScriptComponentProxy*>(&proxy)
	);
}

void ClrHost::DestroyScriptComponent(int appDomainID, __int64 instanceID)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->DestroyScriptComponent(instanceID);
	}
}

void ClrHost::GetScriptComponentTypes(int appDomainID, std::vector<tstring>& types) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* safeArray = appDomainManager->GetScriptComponentTypes();
		if (safeArray)
		{
			SafeArrayToVector<BSTR>(safeArray, types);
		}
	}
}

} // namespace Klawr
