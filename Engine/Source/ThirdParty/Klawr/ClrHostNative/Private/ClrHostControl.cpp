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
#include "ClrHostControl.h"
#include "ClrHost.h"

namespace {

SAFEARRAY* CreateSafeArrayOfWrapperFunctions(void** wrapperFunctions, int numFunctions)
{
	SAFEARRAY* safeArray;
	//SAFEARRAYBOUND safeArrayBound[1];
	//safeArrayBound[0].cElements = numFunctions;
	//safeArrayBound[0].lLbound = 0;
	//safeArray = SafeArrayCreate(VT_VARIANT, 1, safeArrayBound);
	safeArray = SafeArrayCreateVector(VT_VARIANT, 0, numFunctions);
	if (safeArray)
	{
		VARIANT* safeArrayData = nullptr;
		HRESULT hr = SafeArrayAccessData(safeArray, (void**)&safeArrayData);
		if (SUCCEEDED(hr))
		{
			for (auto i = 0; i < numFunctions; ++i)
			{
				VARIANT* var = &safeArrayData[i];
				VariantInit(var);
				var->vt = VT_I8;
				var->llVal = 0;
				var->byref = wrapperFunctions[i];
			}
			SafeArrayUnaccessData(safeArray);
		}
	}
	return safeArray;
}

} // unnamed namespace

namespace Klawr {

HRESULT STDMETHODCALLTYPE ClrHostControl::GetHostManager(REFIID riid, void** ppObject)
{
	// no custom managers have been implemented yet
	*ppObject = nullptr;
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE ClrHostControl::SetAppDomainManager(
	DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager
)
{
	HRESULT hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&_appDomainManager));
	if (SUCCEEDED(hr))
	{
		auto clrHost = static_cast<ClrHost*>(IClrHost::Get());
		for (const auto& classWrapper : clrHost->ClassWrappers)
		{
			auto className = classWrapper.first.c_str();
			auto& wrapperInfo = classWrapper.second;
			auto safeArray = CreateSafeArrayOfWrapperFunctions(
				wrapperInfo.functionPointers, wrapperInfo.numFunctions
			);
			hr = _appDomainManager->SetNativeFunctionPointers(className, safeArray);
			assert(SUCCEEDED(hr));
		}
	}
	return hr;
}

} // namespace Klawr
