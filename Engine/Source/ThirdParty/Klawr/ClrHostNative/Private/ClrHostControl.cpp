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

namespace Klawr {

IEngineAppDomainManager* ClrHostControl::GetEngineAppDomainManager(int appDomainID)
{
	auto it = _engineAppDomainManagers.find(appDomainID);
	return (it != _engineAppDomainManagers.end()) ? it->second.GetInterfacePtr() : nullptr;
}

bool ClrHostControl::DestroyEngineAppDomain(int appDomainID)
{
	auto it = _engineAppDomainManagers.find(appDomainID);
	if (it != _engineAppDomainManagers.end())
	{
		_engineAppDomainManagers.erase(it);
	}

	if (_defaultAppDomainManager)
	{
		return _defaultAppDomainManager->DestroyEngineAppDomain(appDomainID) != VARIANT_FALSE;
	}

	return false;
}

void ClrHostControl::Shutdown()
{
	_engineAppDomainManagers.clear();

	if (_defaultAppDomainManager)
	{
		_defaultAppDomainManager->DestroyAllEngineAppDomains();
		_defaultAppDomainManager = nullptr;
	}
}

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
	HRESULT hr;
	if (!_defaultAppDomainManager)
	{
		hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&_defaultAppDomainManager));
	}
	else
	{
		IEngineAppDomainManagerPtr engineAppDomainManager;
		hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&engineAppDomainManager));
		if (SUCCEEDED(hr))
		{
			_engineAppDomainManagers[dwAppDomainID] = engineAppDomainManager;
		}
	}
	assert(SUCCEEDED(hr));
	return hr;
}

} // namespace Klawr
