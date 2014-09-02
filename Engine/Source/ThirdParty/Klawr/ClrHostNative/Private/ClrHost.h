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

#include "KlawrClrHostPCH.h"
#include "KlawrClrHost.h"
#include <comdef.h> // for _COM_SMARTPTR_TYPEDEF
#include <map>

_COM_SMARTPTR_TYPEDEF(ICLRRuntimeHost, IID_ICLRRuntimeHost); // for ICLRRuntimeHostPtr

namespace Klawr {

/**
 * @brief The implementation of the public IClrHost interface.
 */
class ClrHost : public IClrHost
{
public: // IClrHost interface
	virtual void Startup() override;
	virtual bool CreateEngineAppDomain(const ObjectUtilsNativeInfo& objectUtils, int& outAppDomainID) override;
	virtual bool DestroyEngineAppDomain(int appDomainID);
	virtual void Shutdown() override;

	virtual void AddClass(const TCHAR* className, void** wrapperFunctions, int numFunctions) override
	{
		_classWrappers[className] = { wrapperFunctions, numFunctions };
	}

	virtual bool CreateScriptObject(
		int appDomainID, const TCHAR* className, class UObject* owner, ScriptObjectInstanceInfo& info
	) override;

	virtual void DestroyScriptObject(int appDomainID, __int64 instanceID) override;

	virtual bool CreateScriptComponent(
		int appDomainID, const TCHAR* className, class UObject* nativeComponent, ScriptComponentProxy& proxy
	) override;

	virtual void DestroyScriptComponent(int appDomainID, __int64 instanceID) override;

public:
	ClrHost() : _hostControl(nullptr) {}

private:
	class ClrHostControl* _hostControl;
	ICLRRuntimeHostPtr _runtimeHost;

	struct ClassWrapperInfo
	{
		void** functionPointers;
		int numFunctions;
	};
	std::map<tstring, ClassWrapperInfo> _classWrappers;
};

} // namespace Klawr
