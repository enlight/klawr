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

#include "KlawrRuntimePluginPrivatePCH.h"
#include "KlawrObjectUtils.h"
#include "KlawrClrHost.h"
#include "KlawrObjectReferencer.h"

namespace Klawr {
	namespace ObjectUtils {
		static UClass* GetClassByName(const TCHAR* nativeClassName)
		{
			return Cast<UClass>(StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, nativeClassName, true));
		}

		static const TCHAR* GetClassName(UClass* nativeClass)
		{
			FString className;
			static_cast<UClass*>(nativeClass)->GetName(className);
			return Klawr::MakeStringCopyForCLR(*className);
		}

		static uint8 IsClassChildOf(UClass* derivedClass, UClass* baseClass)
		{
			return static_cast<UClass*>(derivedClass)->IsChildOf(static_cast<UClass*>(baseClass));
		}

		static void RemoveObjectRef(UObject* obj)
		{
			// NOTE: currently UClass instances aren't reference counted, under the assumption they 
			// won't be garbage collected... it's probably a bad assumption!
			if (!obj->IsA<UClass>())
			{
				Klawr::FObjectReferencer::RemoveObjectRef(obj);
			}
		}
	} // namespace ObjectUtils

ObjectUtilsNativeInfo FObjectUtils::Info =
{
	ObjectUtils::GetClassByName,
	ObjectUtils::GetClassName,
	ObjectUtils::IsClassChildOf,
	ObjectUtils::RemoveObjectRef
};

} // namespace Klawr