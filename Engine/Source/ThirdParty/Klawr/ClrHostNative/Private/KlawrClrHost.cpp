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
#include "ClrHost.h"
#include "ClrHostControl.h"
#include <metahost.h>
#include <comdef.h> // for _COM_SMARTPTR_TYPEDEF
#include <memory> // for unique_ptr
#include "KlawrClrHostInterfaces.h"

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

IClrHost* IClrHost::Get()
{
	static auto singleton = std::make_unique<ClrHost>();
	return singleton.get();
}

} // namespace Klawr
