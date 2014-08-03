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
#include "KlawrClrHost.h"
#include <metahost.h>
#include <comdef.h> // for _COM_SMARTPTR_TYPEDEF

#ifdef _DEBUG
#import "../../ClrHostInterfaces/bin/Debug/Klawr.ClrHost.Interfaces.tlb"
#else
#import "../../ClrHostInterfaces/bin/Release/Klawr.ClrHost.Interfaces.tlb"
#endif // _DEBUG

// so we can use the types from the tlb
using namespace Klawr_ClrHost_Interfaces;

#pragma comment(lib, "mscoree.lib")

namespace Klawr {

TCHAR* MakeStringCopyForCLR(const TCHAR* stringToCopy)
{
	const size_t bufferSize = (_tcslen(stringToCopy) + 1) * sizeof(TCHAR);
	TCHAR* buffer = (TCHAR*)CoTaskMemAlloc(bufferSize);
	if (buffer)
	{
		memcpy(buffer, stringToCopy, bufferSize);
	}
	return buffer;
}

// keeps track of app domain managers (created from managed code) on the unmanaged side,
// unmanaged code can interact with these app domain managers via the IKlawrAppDomainManager
// interface
class ClrHostControl : public IHostControl
{
public:
	ClrHostControl()
		: _refCount(0)
	{
	}

	IKlawrAppDomainManager* GetDefaultDomainManager()
	{
		return _appDomainManager.GetInterfacePtr();
	}

public: // IHostControl interface
	virtual HRESULT STDMETHODCALLTYPE GetHostManager(REFIID riid, void** ppObject) override
	{
		// no custom managers have been implemented yet
		*ppObject = nullptr;
		return E_NOINTERFACE;
	}

	virtual HRESULT STDMETHODCALLTYPE SetAppDomainManager(
		DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager
	) override
	{
		HRESULT hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&_appDomainManager));
		if (SUCCEEDED(hr))
		{
			//hr = _appDomainManager->SetNativeFunctionPointers(nativeClassName, functionPointers);
		}
		return hr;
	}

public: // IUnknown interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (!ppvObject)
		{
			return E_POINTER;
		}
		if ((riid == IID_IUnknown) || (riid == IID_IHostControl))
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_POINTER;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef() override
	{
		return InterlockedIncrement(&_refCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release() override
	{
		ULONG refCount = InterlockedDecrement(&_refCount);
		if (refCount == 0)
		{
			delete this;
		}
		return refCount;
	}

private:
	volatile ULONG _refCount;
	// the default app domain manager
	IKlawrAppDomainManagerPtr _appDomainManager;
	// TODO: keep track of any other app domain managers
};

// bootstrap the CLR runtime and load a simple test assembly
void TestClrHost()
{
	_COM_SMARTPTR_TYPEDEF(ICLRMetaHost, IID_ICLRMetaHost);
	ICLRMetaHostPtr metaHost;
	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&metaHost));
	assert(SUCCEEDED(hr));

	// specify which version of the CLR should be used
	_COM_SMARTPTR_TYPEDEF(ICLRRuntimeInfo, IID_ICLRRuntimeInfo);
	ICLRRuntimeInfoPtr runtimeInfo;
	hr = metaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&runtimeInfo));
	assert(SUCCEEDED(hr));

	// load the CLR (it won't be initialized just yet)
	_COM_SMARTPTR_TYPEDEF(ICLRRuntimeHost, IID_ICLRRuntimeHost);
	ICLRRuntimeHostPtr runtimeHost;
	hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&runtimeHost));
	assert(SUCCEEDED(hr));

	// TODO: figure out if the host control we create gets cleaned up automatically
	// hook up our unmanaged host to the runtime host
	auto clrHostControl = new ClrHostControl();
	hr = runtimeHost->SetHostControl(clrHostControl);
	assert(SUCCEEDED(hr));

	_COM_SMARTPTR_TYPEDEF(ICLRControl, IID_ICLRControl);
	ICLRControlPtr clrControl;
	hr = runtimeHost->GetCLRControl(&clrControl);
	assert(SUCCEEDED(hr));
	
	// by default the CLR runtime will look for the app domain manager assembly in the same 
	// directory as the application, which in this case will be 
	// C:\Program Files\Unreal Engine\4.X\Engine\Binaries\Win64 (or Win32)
	hr = clrControl->SetAppDomainManagerType(L"Klawr.ClrHost.Managed", L"Klawr.ClrHost.Managed.KlawrAppDomainManager");
	assert(SUCCEEDED(hr));

	// initialize the CLR (not strictly necessary because the runtime can initialize itself)
	hr = runtimeHost->Start();
	assert(SUCCEEDED(hr));

	//clrHostControl->GetDefaultDomainManager()->RunTest();

	// NOTE: There's a crash here while debugging with the Mixed mode debugger, but everything works
	// fine when using the Auto mode debugger (which probably ends up using the Native debugger 
	// since this project is native).
	hr = runtimeHost->Stop();
	assert(SUCCEEDED(hr));
}

} // namespace Klawr
