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

namespace Klawr {

/**
 * @brief Makes a copy of the given string, the resulting copy can be safely released by the CLR.
 *
 * The CLR will attempt to release any c-string that is returned to it from a native function after
 * it creates a corresponding managed string. However, it can only release the c-string if it was 
 * allocated on the correct heap. This function will allocate a copy of the passed in c-string on 
 * the same heap the CLR will attempt to release it from, this copy is what should be returned to 
 * the CLR instead of the original c-string.
 *
 * @return A copy of the c-string passed in.
 */
TCHAR* MakeStringCopyForCLR(const TCHAR* stringToCopy);

/** 
 * @brief Contains pointers to native UObject management functions.
 *
 * These functions will be called by managed code.
 *
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Interfaces,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr_ClrHost_Interfaces namespace.
 */
struct ObjectUtilsNativeInfo
{
	typedef void (*RemoveObjectRefAction)(void*);

	/** Native function to be called when a managed reference to a UObject instance is disposed. */
	RemoveObjectRefAction RemoveObjectRef;
};

/**
 * @brief Contains native/managed interop information for a ScriptObject instance.
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Interfaces,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr_ClrHost_Interfaces namespace.
 */
struct ScriptObjectInstanceInfo
{
	typedef void (*BeginPlayAction)();
	typedef void (*TickAction)(float);
	typedef void (*DestroyAction)();

	/** Unique ID of a managed ScriptObject instance. */
	__int64 InstanceID;
	/** Pointer to the managed BeginPlay() method of a ScriptObject instance. */
	BeginPlayAction BeginPlay;
	/** Pointer to the managed Tick() method of a ScriptObject instance. */
	TickAction Tick;
	/** Pointer to the managed Destroy() method of a ScriptObject instance. */
	DestroyAction Destroy;
};

/** This public interface can be used to pass native wrapper functions to the CLR host. */
class IClrHost
{
public:
	/** Startup the CLR. */
	virtual void Startup() = 0;
	/** Load the engine wrapper assembly. */
	virtual void InitializeEngineAppDomain(const ObjectUtilsNativeInfo&) = 0;
	/** Shutdown the CLR. */
	virtual void Shutdown() = 0;

	/** 
	 * @brief Store native wrapper functions for the given class.
	 * @param className The name of a scriptable C++ class (including prefix, e.g. AActor).
	 * @param wrapperFunctions Array of pointers to native wrapper functions for the given class.
	 * @param numFunctions Number of elements in the wrapperFunctions array.
	 */
	virtual void AddClass(const TCHAR* className, void** wrapperFunctions, int numFunctions) = 0;

	virtual bool CreateScriptObject(const TCHAR* className, void* owner, ScriptObjectInstanceInfo& info) = 0;
	virtual void DestroyScriptObject(__int64 instanceID) = 0;

public:
	/** Get the singleton instance. */
	static IClrHost* Get();
};

} // namespace Klawr