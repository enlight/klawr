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
#include "KlawrNativeUtils.h"
#include "KlawrClrHost.h"
#include "KlawrObjectReferencer.h"

namespace Klawr 
{
	/**
	 * Abstract base class for TArrayHelper.
	 * 
	 * It's impractical to wrap every instantiation of the TArrayHelper template (not by hand
	 * anyway) so it can be passed across the native/managed code boundary, but wrapping a simple
	 * interface like FArrayHelper is easy.
	 */
	class FArrayHelper
	{
	protected:
		// property type that corresponds to the element type of the TArray this helper acts on
		// e.g. for TArray<FString> this will be UStrProperty
		const UProperty* ElementProperty;
		int32 ElementSize;

	protected:
		void Construct(int32 index)
		{
			if (ElementProperty->HasAnyPropertyFlags(CPF_ZeroConstructor))
			{
				FMemory::Memzero(GetRawPtr(index), ElementSize);
			}
			else
			{
				ElementProperty->InitializeValue(GetRawPtr(index));
			}
		}

	public:
		FArrayHelper(const UProperty* elementProperty, int32 elementSize)
			: ElementProperty(elementProperty)
			, ElementSize()
		{
		}

		virtual ~FArrayHelper()
		{
		}

		const UProperty* GetElementProperty() const
		{
			return ElementProperty;
		}

		virtual int32 Num() const = 0;
		virtual uint8* GetRawPtr(int32 index) = 0;
		virtual int32 Add() = 0;
		virtual void Insert(int32 index) = 0;
		virtual void Remove(int32 index) = 0;
		virtual int32 Find(const void* item) const = 0;
		virtual void Reset(int32 newCapacity) = 0;
	};

	/**
	 * This class manipulates TArray directly.
	 * 
	 * This template class is used by the native code generator to expose native TArray(s) to
	 * managed code.
	 */
	template <typename T>
	class TArrayHelper : public FArrayHelper
	{
	private:
		typedef FArrayHelper Super;
		TArray<T>* Array;

	public:
		TArrayHelper(TArray<T>* array, const UArrayProperty* arrayProperty)
			: FArrayHelper(arrayProperty->Inner, arrayProperty->ElementSize)
			, Array(array)
		{
		}

		virtual ~TArrayHelper()
		{
		}

		int32 Num() const override
		{
			return Array->Num();
		}

		uint8* GetRawPtr(int32 index) override
		{
			return reinterpret_cast<uint8*>(&((*Array)[index]));
		}

		int32 Add() override
		{
			const int32 index = Array->AddUninitialized();
			Super::Construct(index);
			return index;
		}

		void Insert(int32 index) override
		{
			Array->InsertUninitialized(index);
			Super::Construct(index);
		}

		void Remove(int32 index) override
		{
			Array->RemoveAt(index);
		}

		int32 Find(const void* itemPtr) const override
		{
			return Array->Find(*static_cast<const T*>(itemPtr));
		}

		void Reset(int32 newCapacity) override
		{
			Array->Reset(newCapacity);
		}
	};

	namespace ArrayUtils 
	{
		int32 Num(FArrayHelper* arrayHelper)
		{
			return arrayHelper->Num();
		}

		void* GetRawPtr(FArrayHelper* arrayHelper, int32 index)
		{
			return arrayHelper->GetRawPtr(index);
		}

		const TCHAR* GetString(FArrayHelper* arrayHelper, int32 index)
		{
			auto prop = Cast<UStrProperty>(arrayHelper->GetElementProperty());
			if (prop)
			{
				auto& value = *reinterpret_cast<const FString*>(arrayHelper->GetRawPtr(index));
				return MakeStringCopyForCLR(*value);
			}
			// couldn't convert the string to the array element type
			check(false);
			return nullptr;
		}

		FScriptName GetName(FArrayHelper* arrayHelper, int32 index)
		{
			// FName is marshaled to FScriptName in managed code (because FScriptName is constant
			// size for all build configurations, FName is not)
			auto prop = Cast<UNameProperty>(arrayHelper->GetElementProperty());
			if (prop)
			{
				auto& value = *reinterpret_cast<const FName*>(arrayHelper->GetRawPtr(index));
				return NameToScriptName(value);
			}
			// couldn't convert the name to the array element type
			check(false);
			return NameToScriptName(NAME_None);
		}

		UObject* GetObject(FArrayHelper* arrayHelper, int32 index)
		{
			auto obj = reinterpret_cast<UObject*>(arrayHelper->GetRawPtr(index));
			if (obj)
			{
				// UObject* gets marshaled to UObjectHandle in managed code, 
				// UObjectHandle will release the reference to this UObject upon disposal.
				FObjectReferencer::AddObjectRef(obj);
			}
			return obj;
		}

		template <typename T>
		void SetValueAt(FArrayHelper* arrayHelper, int32 index, T item)
		{
			*(T*)arrayHelper->GetRawPtr(index) = item;
		}
		
		void SetStringAt(FArrayHelper* arrayHelper, int32 index, const TCHAR* item)
		{
			auto prop = Cast<UStrProperty>(arrayHelper->GetElementProperty());
			if (prop)
			{
				prop->SetPropertyValue(arrayHelper->GetRawPtr(index), FString(item));
				return;
			}
			// couldn't convert the string to the array element type
			check(false);
		}

		void SetNameAt(FArrayHelper* arrayHelper, int32 index, FScriptName item)
		{
			auto prop = Cast<UNameProperty>(arrayHelper->GetElementProperty());
			if (prop)
			{
				prop->SetPropertyValue(arrayHelper->GetRawPtr(index), ScriptNameToName(item));
				return;
			}
			// couldn't convert the name to the array element type
			check(false);
		}

		void SetObjectAt(FArrayHelper* arrayHelper, int32 index, UObject* item)
		{
			// UClass (need to check first since UClassProperty is derived from UObjectProperty)
			{
				auto prop = Cast<UClassProperty>(arrayHelper->GetElementProperty());
				if (prop)
				{
					prop->SetPropertyValue(
						arrayHelper->GetRawPtr(index), Cast<UClass>(item)
					);
					return;
				}
			}
			// UObject
			{
				auto prop = Cast<UObjectProperty>(arrayHelper->GetElementProperty());
				if (prop)
				{
					prop->SetPropertyValue(arrayHelper->GetRawPtr(index), item);
					return;
				}
			}
		}

		int32 Add(FArrayHelper* arrayHelper)
		{
			return arrayHelper->Add();
		}

		void Reset(FArrayHelper* arrayHelper, int32 newCapacity)
		{
			arrayHelper->Reset(newCapacity);
		}

		template <typename T>
		int32 FindByPtr(FArrayHelper* arrayHelper, T* itemPtr)
		{
			return arrayHelper->Find(itemPtr);
		}

		template <typename T>
		int32 FindByValue(FArrayHelper* arrayHelper, T item)
		{
			return arrayHelper->Find(&item);
		}
		
		int32 FindString(FArrayHelper* arrayHelper, const TCHAR* item)
		{
			// FString
			if (arrayHelper->GetElementProperty()->IsA<UStrProperty>())
			{
				FString strItem(item);
				return arrayHelper->Find(&strItem);
			}
			// couldn't convert the string to the array element type
			check(false);
			return INDEX_NONE;
		}

		int32 FindName(FArrayHelper* arrayHelper, FScriptName item)
		{
			if (arrayHelper->GetElementProperty()->IsA<UNameProperty>())
			{
				FName nameItem = ScriptNameToName(item);
				return arrayHelper->Find(&nameItem);
			}
			// couldn't convert the name to the array element type
			check(false);
			return INDEX_NONE;
		}

		void Insert(FArrayHelper* arrayHelper, int32 index)
		{
			arrayHelper->Insert(index);
		}

		void RemoveAt(FArrayHelper* arrayHelper, int32 index)
		{
			arrayHelper->Remove(index);
		}

		void Destroy(FArrayHelper* arrayHelper)
		{
			delete arrayHelper;
		}
	} // namespace ArrayUtils

	ArrayUtilsProxy FNativeUtils::Array =
	{
		ArrayUtils::Num,
		ArrayUtils::GetRawPtr,
		ArrayUtils::GetString,
		ArrayUtils::GetName,
		ArrayUtils::GetObject,
		ArrayUtils::SetValueAt<uint8>,
		ArrayUtils::SetValueAt<int16>,
		ArrayUtils::SetValueAt<int32>,
		ArrayUtils::SetValueAt<int64>,
		ArrayUtils::SetStringAt,
		ArrayUtils::SetNameAt,
		ArrayUtils::SetObjectAt,
		ArrayUtils::Add,
		ArrayUtils::Reset,
		ArrayUtils::FindByPtr<void>,
		ArrayUtils::FindByValue<uint8>,
		ArrayUtils::FindByValue<int16>,
		ArrayUtils::FindByValue<int32>,
		ArrayUtils::FindByValue<int64>,
		ArrayUtils::FindString,
		ArrayUtils::FindName,
		ArrayUtils::FindByPtr<UObject>,
		ArrayUtils::Insert,
		ArrayUtils::RemoveAt,
		ArrayUtils::Destroy,
	};

} // namespace Klawr
