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
#include <mscoree.h>

namespace Klawr {

/*
 * Keeps track of app domain managers (created from managed code) on the unmanaged side.
 *
 * Unmanaged code can interact with these app domain managers via the IKlawrAppDomainManager
 * interface.
 */
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
	virtual HRESULT STDMETHODCALLTYPE GetHostManager(REFIID riid, void** ppObject) override;
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
	// the default app domain manager
	IKlawrAppDomainManagerPtr _appDomainManager;
	// TODO: keep track of any other app domain managers
};

} // namespace Klawr
