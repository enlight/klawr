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
#pragma once

#include "KlawrClrHostInterfaces.h"
#include <map>

namespace Klawr {

using Managed::IDefaultAppDomainManager;
using Managed::IDefaultAppDomainManagerPtr;
using Managed::IEngineAppDomainManager;
using Managed::IEngineAppDomainManagerPtr;

/**
 * Keeps track of app domain managers (created from managed code) on the unmanaged side.
 *
 * There is only one default app domain manager, and multiple engine app domain managers. The
 */
class ClrHostControl : public IHostControl
{
public:
	ClrHostControl()
		: _refCount(1)
	{
	}

	IDefaultAppDomainManager* GetDefaultAppDomainManager()
	{
		return _defaultAppDomainManager.GetInterfacePtr();
	}

	IEngineAppDomainManager* GetEngineAppDomainManager(int appDomainID);

	/**
	 * Unload an engine app domain and release the internal reference to its app domain manager. 
	 * @return true if the app domain was successfully unloaded, false otherwise
	 */
	bool DestroyEngineAppDomain(int appDomainID);

	/**
	 * Unload all engine app domains and release all internal references to any app domain managers.
	 * @note This should be called only before the CLR is stopped.
	 */
	void Shutdown();

public: // IHostControl interface
	virtual HRESULT STDMETHODCALLTYPE GetHostManager(REFIID riid, void** ppObject) override;

	/**
	 * Called from managed code every time a new app domain is created.
	 * @param dwAppDomainID The ID of the new app domain.
	 * @param pUnkAppDomainManager The manager for the new app domain.
	 */
	virtual HRESULT STDMETHODCALLTYPE SetAppDomainManager(
		DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager
	) override;

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
	// the app domain manager for the default app domain (that can't be unloaded while the CLR is running)
	IDefaultAppDomainManagerPtr _defaultAppDomainManager;
	// app domain managers for engine app domains (that can be unloaded while the CLR is running)
	std::map<DWORD /* App Domain ID */, IEngineAppDomainManagerPtr> _engineAppDomainManagers;
};

} // namespace Klawr
