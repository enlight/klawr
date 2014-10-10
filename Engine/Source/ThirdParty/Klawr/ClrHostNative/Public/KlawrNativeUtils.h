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
 * @brief Contains pointers to native UObject and UClass utility functions.
 *
 * These native functions will be called by managed code.
 *
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Managed,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr::Managed namespace (but it's hidden from clients of this library).
 */
struct ObjectUtilsProxy
{
	typedef class UClass* (*GetClassByNameFunc)(const TCHAR* nativeClassName);
	typedef const TCHAR* (*GetClassNameFunc)(class UClass* nativeClass);
	typedef unsigned char (*IsClassChildOfFunc)(class UClass* derivedClass, class UClass* baseClass);
	typedef void (*RemoveObjectRefAction)(class UObject* nativeObject);

	/** Get a UClass instance matching the given name (excluding U/A prefix). */
	GetClassByNameFunc GetClassByName;
	/** Get the name (excluding U/A prefix) of a UClass instance. */
	GetClassNameFunc GetClassName;
	/** Determine if one UClass is derived from another. */
	IsClassChildOfFunc IsClassChildOf;
	/** Called when a managed reference to a UObject instance is disposed. */
	RemoveObjectRefAction RemoveObjectRef;
};

/** 
 * @brief Contains pointers to native logging functions.
 *
 * These native functions will be called by managed code.
 *
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Managed,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr::Managed namespace (but it's hidden from clients of this library).
 */
struct LogUtilsProxy
{
	typedef void (*LogAction)(const TCHAR* text);

	/** Print an error to the UE4 console and log file, then crash (even if logging is disabled). */
	LogAction LogFatalError;
	/** Print an error to the UE4 console and log file. */
	LogAction LogError;
	/** Print a warning to the UE4 console and log file. */
	LogAction LogWarning;
	/** Print a message to the UE4 console and log file. */
	LogAction Display;
	/** Print a message to the log file, but not to the UE4 console. */
	LogAction Log;
	/** Print a verbose message to a log file (if Verbose logging is enabled). */
	LogAction LogVerbose;
	/** Print a verbose message to a log file (if VeryVerbose logging is enabled). */
	LogAction LogVeryVerbose;
};

// This class needs to be implemented by clients of the library.
class FArrayHelper;

/** 
 * @brief Contains pointers to native TArray manipulation functions.
 *
 * These native functions will be called by managed code.
 *
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Managed,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr::Managed namespace (but it's hidden from clients of this library).
 */
struct ArrayUtilsProxy
{
	int32 (*Num)(FArrayHelper* arrayHelper);
	void* (*GetRawPtr)(FArrayHelper* arrayHelper, int32 index);
	const TCHAR* (*GetString)(FArrayHelper* arrayHelper, int32 index);
	class UObject* (*GetObject)(FArrayHelper* arrayHelper, int32 index);
	void (*SetUInt8At)(FArrayHelper* arrayHelper, int32 index, uint8 item);
	void (*SetInt16At)(FArrayHelper* arrayHelper, int32 index, int16 item);
	void (*SetInt32At)(FArrayHelper* arrayHelper, int32 index, int32 item);
	void (*SetInt64At)(FArrayHelper* arrayHelper, int32 index, int64 item);
	void (*SetStringAt)(FArrayHelper* arrayHelper, int32 index, const TCHAR* item);
	void (*SetObjectAt)(FArrayHelper* arrayHelper, int32 index, class UObject* item);
	int32 (*Add)(FArrayHelper* arrayHelper);
	void (*Reset)(FArrayHelper* arrayHelper, int32 newCapacity);
	int32 (*Find)(FArrayHelper* arrayHelper, void* itemPtr);
	int32 (*FindUInt8)(FArrayHelper* arrayHelper, uint8 item);
	int32 (*FindInt16)(FArrayHelper* arrayHelper, int16 item);
	int32 (*FindInt32)(FArrayHelper* arrayHelper, int32 item);
	int32 (*FindInt64)(FArrayHelper* arrayHelper, int64 item);
	int32 (*FindString)(FArrayHelper* arrayHelper, const TCHAR* item);
	int32 (*FindObject)(FArrayHelper* arrayHelper, class UObject* item);
	void (*Insert)(FArrayHelper* arrayHelper, int32 index);
	void (*RemoveAt)(FArrayHelper* arrayHelper, int32 index);
	void (*Destroy)(FArrayHelper* arrayHelper);
};

/** Encapsulates native utility functions that are exported to managed code. */
struct NativeUtils
{
	ObjectUtilsProxy Object;
	LogUtilsProxy Log;
	ArrayUtilsProxy Array;
};

} // namespace Klawr
