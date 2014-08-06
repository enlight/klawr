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

#include "ScriptGeneratorPluginPrivatePCH.h"
#include "KlawrCodeGenerator.h"
#include "KlawrCodeFormatter.h"
#include "pugixml.hpp"

// Names of structs that can be used for interop (they have a corresponding struct type in managed code)
const FName FKlawrCodeGenerator::Name_Vector2D("Vector2D");
const FName FKlawrCodeGenerator::Name_Vector("Vector");
const FName FKlawrCodeGenerator::Name_Vector4("Vector4");
const FName FKlawrCodeGenerator::Name_Quat("Quat");
const FName FKlawrCodeGenerator::Name_Transform("Transform");
const FName FKlawrCodeGenerator::Name_LinearColor("LinearColor");
const FName FKlawrCodeGenerator::Name_Color("Color");

const FString FKlawrCodeGenerator::UnmanagedFunctionPointerAttribute = 
	TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl)]");

const FString FKlawrCodeGenerator::MarshalReturnedBoolAsUint8Attribute =
	TEXT("[return: MarshalAs(UnmanagedType.U1)]");

const FString FKlawrCodeGenerator::MarshalBoolParameterAsUint8Attribute =
	TEXT("[MarshalAs(UnmanagedType.U1)]");

const FString FKlawrCodeGenerator::ClrHostInterfacesAssemblyName = "Klawr.ClrHost.Interfaces";

FKlawrCodeGenerator::FKlawrCodeGenerator(
	const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory, 
	const FString& InIncludeBase
) : FScriptCodeGeneratorBase(RootLocalPath, RootBuildPath, OutputDirectory, InIncludeBase)
{
}

FString FKlawrCodeGenerator::InitializeFunctionDispatchParam(UFunction* Function, UProperty* Param, int32 ParamIndex)
{	
	if (!(Param->GetPropertyFlags() & CPF_ReturnParm))
	{
		FString ParamName = Param->GetName();
		FString Initializer;

		if (Param->IsA(UClassProperty::StaticClass()))
		{
			Initializer = FString::Printf(TEXT("(UClass*)%s"), *ParamName);
		}
		else if (Param->IsA(UObjectPropertyBase::StaticClass()))
		{
			Initializer = FString::Printf(
				TEXT("(%s)%s"),
				*Super::GetPropertyTypeCPP(Param, CPPF_ArgumentOrReturnValue), *ParamName
			);
		}
		else if (Param->IsA(UStrProperty::StaticClass()) || Param->IsA(UNameProperty::StaticClass()))
		{
			Initializer = ParamName;
		}
		else
		{
			// reference params are passed into a native wrapper function via a pointer,
			// so dereference the pointer so that the value can be copied into FDispatchParams
			if (Param->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
			{
				ParamName = FString::Printf(TEXT("(*%s)"), *ParamName);
			}

			if (Param->IsA(UIntProperty::StaticClass()) ||
				Param->IsA(UFloatProperty::StaticClass()))
			{
				Initializer = ParamName;
			}
			else if (Param->IsA(UBoolProperty::StaticClass()))
			{
				if (CastChecked<UBoolProperty>(Param)->IsNativeBool())
				{
					// explicitly convert uin8 to bool
					Initializer = FString::Printf(TEXT("!!%s"), *ParamName);
				}
				else
				{
					Initializer = ParamName;
				}
			}
			else if (Param->IsA(UStructProperty::StaticClass()))
			{
				auto StructProp = CastChecked<UStructProperty>(Param);
				if (IsStructPropertyTypeSupported(StructProp))
				{
					Initializer = ParamName;
				}
				else
				{
					FError::Throwf(
						TEXT("Unsupported function param struct type: %s"),
						*StructProp->Struct->GetName()
					);
				}
			}
		}
		
		if (Initializer.IsEmpty())
		{
			FError::Throwf(
				TEXT("Unsupported function param type: %s"), *Param->GetClass()->GetName()
			);
		}
		return Initializer;
	}
	else
	{
		return Super::InitializeFunctionDispatchParam(Function, Param, ParamIndex);
	}	
}

void FKlawrCodeGenerator::GenerateNativeReturnValueHandler(
	UProperty* ReturnValue, const FString& ReturnValueName, FKlawrCodeFormatter& GeneratedGlue
)
{
	if (ReturnValue)
	{
		if (ReturnValue->IsA(UIntProperty::StaticClass()) || 
			ReturnValue->IsA(UFloatProperty::StaticClass()) ||
			ReturnValue->IsA(UBoolProperty::StaticClass()) ||
			ReturnValue->IsA(UObjectPropertyBase::StaticClass()))
		{
			GeneratedGlue << FString::Printf(TEXT("return %s;"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UStrProperty::StaticClass()))
		{
			GeneratedGlue << FString::Printf(
				TEXT("return MakeStringCopyForCLR(*%s);"), *ReturnValueName
			);
		}
		else if (ReturnValue->IsA(UNameProperty::StaticClass()))
		{
			GeneratedGlue << FString::Printf(
				TEXT("return MakeStringCopyForCLR(*(%s.ToString()));"), *ReturnValueName
			);
		}
		else if (ReturnValue->IsA(UStructProperty::StaticClass()))
		{
			auto StructProp = CastChecked<UStructProperty>(ReturnValue);
			if (IsStructPropertyTypeSupported(StructProp))
			{
				GeneratedGlue << FString::Printf(TEXT("return %s;"), *ReturnValueName);
			}
			else
			{
				FError::Throwf(
					TEXT("Unsupported function return value struct type: %s"), 
					*StructProp->Struct->GetName()
				);
			}
		}
		else
		{
			FError::Throwf(
				TEXT("Unsupported function return type: %s"), *ReturnValue->GetClass()->GetName()
			);
		}
	}
}

bool FKlawrCodeGenerator::CanExportClass(UClass* Class)
{
	bool bCanExport = FScriptCodeGeneratorBase::CanExportClass(Class);
	if (bCanExport)
	{
		const FString ClassNameCPP = GetClassNameCPP(Class);
		// check for exportable functions
		bool bHasMembersToExport = false;
		for (TFieldIterator<UFunction> FuncIt(Class); FuncIt; ++FuncIt)
		{
			if (CanExportFunction(ClassNameCPP, Class, *FuncIt))
			{
				bHasMembersToExport = true;
				break;
			}
		}
		// check for exportable properties
		if (!bHasMembersToExport)
		{
			for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
			{
				if (CanExportProperty(ClassNameCPP, Class, *PropertyIt))
				{
					bHasMembersToExport = true;
					break;
				}
			}
		}
		bCanExport = bHasMembersToExport;
	}
	return bCanExport;
}

// TODO: remove ClassNameCPP from args, it's not used for anything
bool FKlawrCodeGenerator::CanExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function)
{
	bool bExport = Super::CanExportFunction(ClassNameCPP, Class, Function);
	if (bExport)
	{
		for (TFieldIterator<UProperty> ParamIt(Function); bExport && ParamIt; ++ParamIt)
		{
			bExport = IsPropertyTypeSupported(*ParamIt);
		}
	}
	return bExport;
}

// TODO: this method should be const, but can't be because Super::GetPropertyTypeCPP() isn't
UProperty* FKlawrCodeGenerator::GetNativeWrapperArgsAndReturnType(
	const UFunction* Function, FString& OutFormalArgs, FString& OutActualArgs
)
{
	OutFormalArgs = TEXT("void* self");
	OutActualArgs = TEXT("self");
	UProperty* ReturnValue = nullptr;

	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		UProperty* Param = *ParamIt;
		if (Param->GetPropertyFlags() & CPF_ReturnParm)
		{
			ReturnValue = Param;
		}
		else
		{
			OutFormalArgs += FString::Printf(
				TEXT(", %s %s"), *GetPropertyNativeType(Param), *Param->GetName()
			);
			OutActualArgs += FString::Printf(TEXT(", %s"), *Param->GetName());
		}
	}

	return ReturnValue;
}

void FKlawrCodeGenerator::GenerateNativeWrapperFunction(
	const UClass* Class, UFunction* Function, FKlawrCodeFormatter& GeneratedGlue
)
{
	FString FormalArgs, ActualArgs, ReturnValueType, ReturnValueName;
	UProperty* ReturnValue = GetNativeWrapperArgsAndReturnType(
		Function, FormalArgs, ActualArgs
	);
	FString ReturnValueTypeName(TEXT("void"));
	if (ReturnValue)
	{
		ReturnValueTypeName = GetPropertyNativeType(ReturnValue);
	}
	// define a native wrapper function that will be bound to a managed delegate
	FString WrapperName = FString::Printf(TEXT("%s_%s"), *Class->GetName(), *Function->GetName());
	GeneratedGlue << FString::Printf(
		TEXT("%s %s(%s)"),
		*ReturnValueTypeName, *WrapperName, *FormalArgs
	);
	GeneratedGlue << FKlawrCodeFormatter::OpenBrace();

	// call the wrapped UFunction
	// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
	//        regular function argument names.
	GeneratedGlue << TEXT("UObject* Obj = (UObject*)self;");
	// FIXME: Super::GenerateFunctionDispatch() doesn't indent code properly, get rid of it!
	// TODO: Maybe use an initializer list to FDispatchParams struct instead of the current multi-line init
	GeneratedGlue << Super::GenerateFunctionDispatch(Function);

	// for non-const reference parameters to the UFunction copy their values from the 
	// FDispatchParams struct
	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		UProperty* Param = *ParamIt;
		if (!Param->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm) &&
			Param->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
		{
			GeneratedGlue << FString::Printf(
				TEXT("*%s = Params.%s;"), *Param->GetName(), *Param->GetName()
			);
		}
	}

	if (ReturnValue)
	{
		GenerateNativeReturnValueHandler(
			ReturnValue, FString::Printf(TEXT("Params.%s"), *ReturnValue->GetName()), 
			GeneratedGlue
		);
	}

	GeneratedGlue 
		<< FKlawrCodeFormatter::CloseBrace()
		<< FKlawrCodeFormatter::LineTerminator();

	FExportedFunction FuncInfo;
	FuncInfo.Function = Function;
	FuncInfo.bHasReturnValue = (ReturnValue != nullptr);
	FuncInfo.NativeWrapperFunctionName = WrapperName;
	ClassExportedFunctions.FindOrAdd(Class).Add(FuncInfo);
}

FString FKlawrCodeGenerator::GenerateDelegateTypeName(const FString& FunctionName, bool bHasReturnValue) const
{
	if (bHasReturnValue)
	{
		return FunctionName + TEXT("Func");
	}
	else
	{
		return FunctionName + TEXT("Action");
	}
}

FString FKlawrCodeGenerator::GenerateDelegateName(const FString& FunctionName) const
{
	return FString(TEXT("_")) + FunctionName;
}

UProperty* FKlawrCodeGenerator::GetManagedWrapperArgsAndReturnType(
	const UFunction* Function, FString& OutFormalInteropArgs, FString& OutActualInteropArgs,
	FString& OutFormalManagedArgs, FString& OutActualManagedArgs
)
{
	OutFormalInteropArgs = TEXT("IntPtr self");
	OutActualInteropArgs = TEXT("_nativeObject");
	OutFormalManagedArgs.Empty();
	OutActualManagedArgs.Empty();
	UProperty* ReturnValue = nullptr;

	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		UProperty* Param = *ParamIt;
		if (Param->GetPropertyFlags() & CPF_ReturnParm)
		{
			ReturnValue = Param;
		}
		else
		{
			FString ArgName = Param->GetName();
			FString ArgType = GetPropertyInteropType(Param);
			FString ArgAttrs = GetPropertyInteropTypeAttributes(Param);
			FString ArgMods = GetPropertyInteropTypeModifiers(Param);
			OutFormalInteropArgs += FString::Printf(
				TEXT(", %s %s %s %s"), *ArgAttrs, *ArgMods, *ArgType, *ArgName
			);
			OutActualInteropArgs += FString::Printf(TEXT(", %s %s"), *ArgMods, *ArgName);
			if (!OutFormalManagedArgs.IsEmpty())
			{
				OutFormalManagedArgs += TEXT(", ");
			}
			// TODO: managed types can be more precise than interop types, because we can use the
			//       exported managed wrapper classes!
			if (!ArgMods.IsEmpty())
			{
				OutFormalManagedArgs += ArgMods + TEXT(" ");
			}
			OutFormalManagedArgs += FString::Printf(TEXT("%s %s"), *ArgType, *ArgName);
			if (!OutActualManagedArgs.IsEmpty())
			{
				OutActualManagedArgs += TEXT(", ");
			}
			OutActualManagedArgs += ArgName;
		}
	}

	return ReturnValue;
}

void FKlawrCodeGenerator::GenerateManagedWrapperFunction(
	const UClass* Class, const UFunction* Function, FKlawrCodeFormatter& GeneratedGlue
)
{
	FString FormalInteropArgs, ActualInteropArgs, FormalManagedArgs, ActualManagedArgs;
	UProperty* ReturnValue = GetManagedWrapperArgsAndReturnType(
		Function, FormalInteropArgs, ActualInteropArgs, FormalManagedArgs, ActualManagedArgs
	);
	const bool bHasReturnValue = (ReturnValue != nullptr);
	const bool bReturnsBool = (bHasReturnValue && ReturnValue->IsA(UBoolProperty::StaticClass()));
	const FString ReturnValueTypeName = 
		bHasReturnValue ? GetPropertyInteropType(ReturnValue) : TEXT("void");
	const FString DelegateTypeName = GenerateDelegateTypeName(Function->GetName(), bHasReturnValue);
	const FString DelegateName = GenerateDelegateName(Function->GetName());

	GeneratedGlue 
		// declare a managed delegate type matching the type of the native wrapper function
		<< UnmanagedFunctionPointerAttribute
		<< (bReturnsBool ? MarshalReturnedBoolAsUint8Attribute : FString())
		<< FString::Printf(
			TEXT("private delegate %s %s(%s);"),
			*ReturnValueTypeName, *DelegateTypeName, *FormalInteropArgs
		)
		// declare a delegate instance that will be bound to the native wrapper function
		<< FString::Printf(
			TEXT("private static %s %s;"),
			*DelegateTypeName, *DelegateName
		)
		// define a managed method that calls the native wrapper function through the delegate 
		// declared above
		<< FString::Printf(
			TEXT("public %s %s(%s)"),
			*ReturnValueTypeName, *Function->GetName(), *FormalManagedArgs
		)
		<< FKlawrCodeFormatter::OpenBrace();

	// call the delegate bound to the native wrapper function
	if (bHasReturnValue)
	{
		GeneratedGlue << FString::Printf(
			TEXT("return %s(%s);"), *DelegateName, *ActualInteropArgs
		);
	}
	else // method has a return value
	{
		GeneratedGlue << FString::Printf(
			TEXT("%s(%s);"), *DelegateName, *ActualInteropArgs
		);
	}
	
	GeneratedGlue
		<< FKlawrCodeFormatter::CloseBrace()
		<< FKlawrCodeFormatter::LineTerminator();
}

void FKlawrCodeGenerator::ExportFunction(
	const FString& ClassNameCPP, const UClass* Class, UFunction* Function,
	FKlawrCodeFormatter& NativeGlueCode, FKlawrCodeFormatter& ManagedGlueCode
)
{
	// The inheritance hierarchy is mirrored in the C# wrapper classes, so there's no need to
	// redefine functions from a base class (assuming that base class has also been exported).
	// However, this also means that functions that are defined in UObject (which isn't exported)
	// are not available in the C# wrapper classes, but that's not a problem for now.
	if (Function->GetOwnerClass() == Class)
	{
		GenerateNativeWrapperFunction(Class, Function, NativeGlueCode);
		GenerateManagedWrapperFunction(Class, Function, ManagedGlueCode);
	}
}

bool FKlawrCodeGenerator::IsPropertyTypeSupported(UProperty* Property) const
{
	bool bSupported = true;
	if (Property->IsA(UStructProperty::StaticClass()))
	{
		auto StructProp = CastChecked<UStructProperty>(Property);
		bSupported = IsStructPropertyTypeSupported(StructProp);
	}
	else if (Property->IsA(UStrProperty::StaticClass()))
	{
		if (!Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm) && 
			Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
		{
			// Non-const FString references can't be easily marshaled from/to a C# string.
			// A few modifications to the generator will be required. For each non-const 
			// FString parameter the generated native wrapper function will need to take a TCHAR* 
			// to a preallocated buffer and the buffer size, then the corresponding value from the
			// FDispatchParams struct will need to be copied to the preallocated buffer after the
			// UFunction is called. On the managed side the managed wrapper delegate will need to
			// use the StringBuilder type for each non-const FString reference parameter, and the
			// corresponding StringBuilder instance of the appropriate size will need to be created
			// before the delegate can be invoked. The need to preallocate the buffer on the managed
			// side is what makes this a bit tedious, since the user is unlikely to know what a
			// suitable buffer size would be!
			bSupported = false;
		}
	}
	else if (!Property->IsA(UIntProperty::StaticClass()) &&
		!Property->IsA(UFloatProperty::StaticClass()) &&
		!Property->IsA(UBoolProperty::StaticClass()) &&
		!Property->IsA(UObjectPropertyBase::StaticClass()) &&
		!Property->IsA(UClassProperty::StaticClass()) &&
		!Property->IsA(UNameProperty::StaticClass()))
	{
		bSupported = false;
	}
	else if (Property->IsA(ULazyObjectProperty::StaticClass()))
	{
		bSupported = false;
	}
	return bSupported;
}

bool FKlawrCodeGenerator::IsPropertyTypePointer(const UProperty* Property)
{
	return Property->IsA(UObjectPropertyBase::StaticClass()) 
		|| Property->IsA(UClassProperty::StaticClass())
		|| Property->IsA(ULazyObjectProperty::StaticClass());
}

bool FKlawrCodeGenerator::IsStructPropertyTypeSupported(const UStructProperty* Property)
{
	return (Property->Struct->GetFName() == Name_Vector2D) 
		|| (Property->Struct->GetFName() == Name_Vector) 
		|| (Property->Struct->GetFName() == Name_Vector4) 
		|| (Property->Struct->GetFName() == Name_Quat) 
		|| (Property->Struct->GetFName() == Name_LinearColor) 
		|| (Property->Struct->GetFName() == Name_Color) 
		|| (Property->Struct->GetFName() == Name_Transform);
}

FString FKlawrCodeGenerator::GetPropertyNativeType(UProperty* Property)
{
	FString NativeType;

	if (IsPropertyTypePointer(Property))
	{
		return TEXT("void*");
	}
	else if (Property->IsA(UBoolProperty::StaticClass()))
	{
		// the managed wrapper functions marshal C# bool to uint8, so for the sake of consistency
		// use uint8 instead of C++ bool in the native wrapper functions
		NativeType = TEXT("uint8");
	}
	else if (Property->IsA(UStrProperty::StaticClass()) || Property->IsA(UNameProperty::StaticClass()))
	{
		return TEXT("const TCHAR*");
	}
	else
	{
		NativeType = Super::GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue);
	}
	// TODO: handle constness?
	// return by reference must be converted to return by value because 
	// FDispatchParams::ReturnValue is only valid within the scope of a native wrapper function
	if (!(Property->GetPropertyFlags() & CPF_ReturnParm) &&
		Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
	{
		NativeType += TEXT("*");
	}

	return NativeType;
}

// FIXME: this method should be const, as should the argument, but Super::GetPropertyTypeCPP() isn't!
FString FKlawrCodeGenerator::GetPropertyInteropType(UProperty* Property)
{
	if (IsPropertyTypePointer(Property))
	{
		return TEXT("IntPtr");
	}
	else if (Property->IsA(UBoolProperty::StaticClass()))
	{
		return TEXT("bool");
	}
	else if (Property->IsA(UIntProperty::StaticClass()))
	{
		return TEXT("int");
	}
	else if (Property->IsA(UFloatProperty::StaticClass()))
	{
		return TEXT("float");
	}
	else if (Property->IsA(UDoubleProperty::StaticClass()))
	{
		return TEXT("double");
	}
	else if (Property->IsA(UStrProperty::StaticClass()) || Property->IsA(UNameProperty::StaticClass()))
	{
		return TEXT("string");
	}
	else
	{
		return Super::GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue);
	}
}

FString FKlawrCodeGenerator::GetPropertyInteropTypeAttributes(UProperty* Property)
{
	if (Property->IsA(UBoolProperty::StaticClass()))
	{
		// by default C# bool gets marshaled to unmanaged BOOL (4-bytes),
		// marshal it to uint8 instead (which is the size of an MSVC bool)
		return MarshalBoolParameterAsUint8Attribute;
	}
	
	// Not convinced the directional attributes are necessary, seems like the C# out and ref 
	// keywords should be sufficient.
	/*
	if (!IsPropertyTypePointer(Property))
	{
		if (!Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm))
		{
			if (Property->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				Attributes += TEXT("[In][Out]");
			}
			else if (Property->HasAnyPropertyFlags(CPF_OutParm))
			{
				Attributes += TEXT("[Out]");
			}
		}
	}
	*/
	return FString();
}

FString FKlawrCodeGenerator::GetPropertyInteropTypeModifiers(UProperty* Property)
{
	if (!IsPropertyTypePointer(Property))
	{
		if (!Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm))
		{
			if (Property->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				return TEXT("ref");
			}
			else if (Property->HasAnyPropertyFlags(CPF_OutParm))
			{
				return TEXT("out");
			}
		}
	}
	return FString();
}

bool FKlawrCodeGenerator::CanExportProperty(const FString& ClassNameCPP, UClass* Class, UProperty* Property)
{
	bool bCanExport = Super::CanExportProperty(ClassNameCPP, Class, Property);
	if (bCanExport)
	{
		bCanExport = IsPropertyTypeSupported(Property);
	}
	return bCanExport;
}

void FKlawrCodeGenerator::GenerateNativePropertyGetterWrapper(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property,
	FKlawrCodeFormatter& GeneratedGlue, FExportedProperty& ExportedProperty
)
{
	// define a native getter wrapper function that will be bound to a managed delegate
	FString PropertyNativeTypeName = GetPropertyNativeType(Property);
	FString GetterName = FString::Printf(TEXT("%s_Get_%s"), *Class->GetName(), *Property->GetName());
	ExportedProperty.NativeGetterWrapperFunctionName = GetterName;

	GeneratedGlue 
		<< FString::Printf(TEXT("%s %s(void* self)"), *PropertyNativeTypeName, *GetterName)
		<< FKlawrCodeFormatter::OpenBrace()
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		<< TEXT("UObject* Obj = (UObject*)self;")
		<< FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));"),
			*ClassNameCPP, *Property->GetName()
		)
		<< FString::Printf(
			TEXT("%s PropertyValue;\r\n"), 
			*GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue)
		)
		<< TEXT("Property->CopyCompleteValue(&PropertyValue, Property->ContainerPtrToValuePtr<void>(Obj));");
			
	GenerateNativeReturnValueHandler(Property, TEXT("PropertyValue"), GeneratedGlue);
	
	GeneratedGlue 
		<< FKlawrCodeFormatter::CloseBrace()
		<< FKlawrCodeFormatter::LineTerminator();
}

void FKlawrCodeGenerator::GenerateNativePropertySetterWrapper(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property,
	FKlawrCodeFormatter& GeneratedGlue, FExportedProperty& ExportedProperty
)
{
	// define a native setter wrapper function that will be bound to a managed delegate
	FString PropertyNativeTypeName = GetPropertyNativeType(Property);
	FString SetterName = FString::Printf(TEXT("%s_Set_%s"), *Class->GetName(), *Property->GetName());
	ExportedProperty.NativeSetterWrapperFunctionName = SetterName;

	GeneratedGlue 
		<< FString::Printf(
			TEXT("void %s(void* self, %s %s)"), 
			*SetterName, *PropertyNativeTypeName, *Property->GetName()
		)
		<< FKlawrCodeFormatter::OpenBrace()
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		<< TEXT("UObject* Obj = (UObject*)self;")
		<< FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));"), 
			*ClassNameCPP, *Property->GetName()
		)
		<< FString::Printf(
			TEXT("%s PropertyValue = %s;"), 
			*GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue), 
			*InitializeFunctionDispatchParam(NULL, Property, 0)
		)
		<< TEXT("Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Obj), &PropertyValue);")
		<< FKlawrCodeFormatter::CloseBrace()
		<< FKlawrCodeFormatter::LineTerminator();
}

void FKlawrCodeGenerator::GenerateManagedPropertyWrapper(
	UClass* Class, UProperty* Property, FKlawrCodeFormatter& GeneratedGlue, 
	FExportedProperty& ExportedProperty
)
{
	const FString GetterName = FString::Printf(TEXT("Get_%s"), *Property->GetName());
	const FString SetterName = FString::Printf(TEXT("Set_%s"), *Property->GetName());
	
	ExportedProperty.GetterDelegateName = GenerateDelegateName(GetterName);
	ExportedProperty.GetterDelegateTypeName = GenerateDelegateTypeName(GetterName, true);
	ExportedProperty.SetterDelegateName = GenerateDelegateName(SetterName);
	ExportedProperty.SetterDelegateTypeName = GenerateDelegateTypeName(SetterName, false);
	
	const bool bIsBoolProperty = Property->IsA(UBoolProperty::StaticClass());
	FString PropertyTypeName = GetPropertyInteropType(Property);
	FString SetterParamType = PropertyTypeName;
	if (bIsBoolProperty)
	{
		SetterParamType = FString::Printf(
			TEXT("%s %s"), *MarshalBoolParameterAsUint8Attribute, *PropertyTypeName
		);
	}
	
	GeneratedGlue
		// declare getter delegate type
		<< UnmanagedFunctionPointerAttribute
		<< (bIsBoolProperty ? MarshalReturnedBoolAsUint8Attribute : FString())
		<< FString::Printf(
			TEXT("private delegate %s %s(IntPtr self);"),
			*PropertyTypeName, *ExportedProperty.GetterDelegateTypeName
		)
		// declare setter delegate type
		<< UnmanagedFunctionPointerAttribute
		<< FString::Printf(
			TEXT("private delegate void %s(IntPtr self, %s %s);"),
			*ExportedProperty.SetterDelegateTypeName, *SetterParamType, *Property->GetName()
		)
		// declare delegate instances that will be bound to the native wrapper functions
		<< FString::Printf(
			TEXT("private static %s %s;"),
			*ExportedProperty.GetterDelegateTypeName, *ExportedProperty.GetterDelegateName
		)
		<< FString::Printf(
			TEXT("private static %s %s;"),
			*ExportedProperty.SetterDelegateTypeName, *ExportedProperty.SetterDelegateName
		)
		// define a property that calls the native wrapper functions through the delegates 
		// declared above
		<< FString::Printf(TEXT("public %s %s"), *PropertyTypeName, *Property->GetName())
		<< FKlawrCodeFormatter::OpenBrace()
		<< FString::Printf(
			TEXT("get { return %s(_nativeObject); }"), *ExportedProperty.GetterDelegateName
		)
		<< FString::Printf(
			TEXT("set { %s(_nativeObject, value); }"), *ExportedProperty.SetterDelegateName
		)
		<< FKlawrCodeFormatter::CloseBrace()
		<< FKlawrCodeFormatter::LineTerminator();
}

void FKlawrCodeGenerator::ExportProperty(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property,
	FKlawrCodeFormatter& NativeGlueCode, FKlawrCodeFormatter& ManagedGlueCode
)
{
	// Only wrap properties that are actually in this class, inheritance will take care of the 
	// properties from base classes.
	if (Property->GetOwnerClass() == Class)
	{
		FExportedProperty PropInfo;
		GenerateNativePropertyGetterWrapper(ClassNameCPP, Class, Property, NativeGlueCode, PropInfo);
		GenerateNativePropertySetterWrapper(ClassNameCPP, Class, Property, NativeGlueCode, PropInfo);
		GenerateManagedPropertyWrapper(Class, Property, ManagedGlueCode, PropInfo);
		ClassExportedProperties.FindOrAdd(Class).Add(PropInfo);
	}
}

void FKlawrCodeGenerator::GenerateNativeGlueCodeHeader(
	const UClass* Class, FKlawrCodeFormatter& GeneratedGlue
)
{
	GeneratedGlue 
		<< TEXT("#pragma once") LINE_TERMINATOR
		<< TEXT("namespace Klawr {")
		<< TEXT("namespace NativeGlue {") LINE_TERMINATOR;
}

void FKlawrCodeGenerator::GenerateNativeGlueCodeFooter(
	const UClass* Class, FKlawrCodeFormatter& GeneratedGlue
) const
{
	const auto ExportedProperties = ClassExportedProperties.Find(Class);
	const auto ExportedFunctions = ClassExportedFunctions.Find(Class);
	bool bHasExports = (ExportedProperties && ExportedProperties->Num()) 
		|| (ExportedFunctions && ExportedFunctions->Num());

	if (bHasExports)
	{
		// generate an array of function pointers to all the native wrapper functions
		FString FunctionsArrayName = FString::Printf(TEXT("%s_WrapperFunctions"), *Class->GetName());
		GeneratedGlue
			<< FString::Printf(TEXT("static void* %s[] ="), *FunctionsArrayName)
			<< FKlawrCodeFormatter::OpenBrace();

		if (ExportedProperties)
		{
			for (const auto& ExportedProperty : *ExportedProperties)
			{
				GeneratedGlue
					<< ExportedProperty.NativeGetterWrapperFunctionName + TEXT(",")
					<< ExportedProperty.NativeSetterWrapperFunctionName + TEXT(",");
			}
		}

		if (ExportedFunctions)
		{
			for (const auto& ExportedFunction : *ExportedFunctions)
			{
				GeneratedGlue << ExportedFunction.NativeWrapperFunctionName + TEXT(",");
			}
		}

		GeneratedGlue
			<< FKlawrCodeFormatter::CloseBrace()
			<< TEXT(";");
	}

	GeneratedGlue << TEXT("}} // namespace Klawr::NativeGlue");
}

void FKlawrCodeGenerator::GenerateManagedStaticConstructor(
	const UClass* Class, FKlawrCodeFormatter& GeneratedGlue
)
{
	const FString ClassNameCPP = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	GeneratedGlue << FString::Printf(TEXT("static %s()"), *ClassNameCPP);
	GeneratedGlue << FKlawrCodeFormatter::OpenBrace();
	
	// bind managed delegates to pointers to native wrapper functions
	GeneratedGlue 
		<< TEXT("var manager = AppDomain.CurrentDomain.DomainManager as IKlawrAppDomainManager;")
		<< FString::Printf(
			TEXT("var nativeFuncPtrs = manager.GetNativeFunctionPointers(\"%s\");"), *ClassNameCPP
		);
	
	int32 FunctionIdx = 0;
	auto ExportedProperties = ClassExportedProperties.Find(Class);
	if (ExportedProperties)
	{
		for (const FExportedProperty& PropInfo : *ExportedProperties)
		{
			GeneratedGlue << FString::Printf(
				TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;"),
				*PropInfo.GetterDelegateName, FunctionIdx, *PropInfo.GetterDelegateTypeName,
				*PropInfo.GetterDelegateTypeName
			);
			++FunctionIdx;
			GeneratedGlue << FString::Printf(
				TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;"),
				*PropInfo.SetterDelegateName, FunctionIdx, *PropInfo.SetterDelegateTypeName,
				*PropInfo.SetterDelegateTypeName
			);
			++FunctionIdx;
		}
	}
	
	auto ExportedFunctions = ClassExportedFunctions.Find(Class);
	if (ExportedFunctions)
	{
		for (const FExportedFunction& FuncInfo : *ExportedFunctions)
		{
			// FIXME: don't generate the delegate type and name again, get it from FExportedFunction
			const FString DelegateTypeName = GenerateDelegateTypeName(
				FuncInfo.Function->GetName(), FuncInfo.bHasReturnValue
			);
			GeneratedGlue << FString::Printf(
				TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;"),
				*GenerateDelegateName(FuncInfo.Function->GetName()),
				FunctionIdx, *DelegateTypeName, *DelegateTypeName
			);
			++FunctionIdx;
		}
	}
	
	GeneratedGlue << FKlawrCodeFormatter::CloseBrace();
}

void FKlawrCodeGenerator::GenerateManagedGlueCodeHeader(
	const UClass* Class, FKlawrCodeFormatter& GeneratedGlue
) const
{
	// usings
	GeneratedGlue 
		<< TEXT("using System;")
		<< TEXT("using System.Runtime.InteropServices;")
		<< TEXT("using Klawr.ClrHost.Interfaces;")
		<< FKlawrCodeFormatter::LineTerminator();

	// declare namespace
	GeneratedGlue << TEXT("namespace Klawr.UnrealEngine") << FKlawrCodeFormatter::OpenBrace();

	// declare class
	FString ClassDecl = FString::Printf(
		TEXT("public class %s%s"), Class->GetPrefixCPP(), *Class->GetName()
	);
	// if the base class was exported mirror the inheritance in the managed wrapper class
	UClass* SuperClass = Class->GetSuperClass();
	if (SuperClass && ExportedClasses.Contains(SuperClass->GetFName()))
	{
		ClassDecl += FString::Printf(
			TEXT(" : %s%s"), SuperClass->GetPrefixCPP(), *SuperClass->GetName()
		);
	}
	GeneratedGlue << ClassDecl << FKlawrCodeFormatter::OpenBrace();
	
	// declare the wrapped native object
	GeneratedGlue << TEXT("private IntPtr _nativeObject;") LINE_TERMINATOR;
}

void FKlawrCodeGenerator::GenerateManagedGlueCodeFooter(
	const UClass* Class, FKlawrCodeFormatter& GeneratedGlue
)
{
	// define static constructor
	GenerateManagedStaticConstructor(Class, GeneratedGlue);
	// close the class
	GeneratedGlue << FKlawrCodeFormatter::CloseBrace();
	// close the namespace
	GeneratedGlue << FKlawrCodeFormatter::CloseBrace();
}

void FKlawrCodeGenerator::ExportClass(
	UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, 
	bool bHasChanged
)
{
	if (!CanExportClass(Class))
	{
		return;
	}
	
	UE_LOG(LogScriptGenerator, Log, TEXT("Exporting class %s"), *Class->GetName());
	
	Super::ExportedClasses.Add(Class->GetFName());
	AllExportedClasses.Add(Class);
	AllSourceClassHeaders.Add(SourceHeaderFilename);

	const FString NativeGlueFilename = Super::GetScriptHeaderForClass(Class);
	FString ManagedGlueFilename = NativeGlueFilename;
	ManagedGlueFilename.RemoveFromEnd(TEXT(".h"));
	ManagedGlueFilename.Append(TEXT(".cs"));
	AllScriptHeaders.Add(NativeGlueFilename);

	FKlawrCodeFormatter NativeGlueCode(TEXT('\t'), 1);
	FKlawrCodeFormatter ManagedGlueCode(TEXT(' '), 4);
	GenerateNativeGlueCodeHeader(Class, NativeGlueCode);
	GenerateManagedGlueCodeHeader(Class, ManagedGlueCode);

	const FString ClassNameCPP = Super::GetClassNameCPP(Class);

	// export functions
	for (TFieldIterator<UFunction> FuncIt(Class); FuncIt; ++FuncIt)
	{
		UFunction* Function = *FuncIt;
		if (CanExportFunction(ClassNameCPP, Class, Function))
		{
			ExportFunction(ClassNameCPP, Class, Function, NativeGlueCode, ManagedGlueCode);
		}
	}

	// export properties
	for (TFieldIterator<UProperty> PropertyIt(Class); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		if (CanExportProperty(ClassNameCPP, Class, Property))
		{
			UE_LOG(LogScriptGenerator, Log, TEXT("  %s %s"), *Property->GetClass()->GetName(), *Property->GetName());
			ExportProperty(ClassNameCPP, Class, Property, NativeGlueCode, ManagedGlueCode);
		}
	}

	GenerateNativeGlueCodeFooter(Class, NativeGlueCode);
	GenerateManagedGlueCodeFooter(Class, ManagedGlueCode);

	Super::SaveHeaderIfChanged(NativeGlueFilename, NativeGlueCode.Content);
	Super::SaveHeaderIfChanged(ManagedGlueFilename, ManagedGlueCode.Content);
}

void FKlawrCodeGenerator::GenerateManagedWrapperProject()
{
	const FString ResourcesBasePath = 
		FPaths::EnginePluginsDir() / TEXT("Script/ScriptGeneratorPlugin/Resources/WrapperProjectTemplate");
	const FString ProjectBasePath = FPaths::EngineIntermediateDir() / TEXT("ProjectFiles/Klawr");

	const FString AssemblyInfoSubPath(TEXT("Properties/AssemblyInfo.cs"));
	const FString AssemblyInfoSrcFilename =	ResourcesBasePath / AssemblyInfoSubPath;
	const FString AssemblyInfoDestFilename = ProjectBasePath / AssemblyInfoSubPath;

	if (COPY_OK != IFileManager::Get().Copy(*AssemblyInfoDestFilename, *AssemblyInfoSrcFilename))
	{
		FError::Throwf(TEXT("Failed to copy '%s'"), *AssemblyInfoSrcFilename);
	}

	const FString ProjectName("UE4Wrapper.csproj");
	const FString ProjectTemplateFilename = ResourcesBasePath / ProjectName;
	const FString ProjectOutputFilename = ProjectBasePath / ProjectName;

	// load the template .csproj
	pugi::xml_document XmlDoc;
	pugi::xml_parse_result result = XmlDoc.load_file(
		*ProjectTemplateFilename, pugi::parse_default | pugi::parse_declaration | pugi::parse_comments
	);

	if (!result)
	{
		FError::Throwf(TEXT("Failed to load %s"), *ProjectTemplateFilename);
	}
	else
	{
		// add a reference to the CLR host interfaces assembly
		auto ReferencesNode = XmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Reference")).parent();
		if (ReferencesNode)
		{
			FString ClrHostInterfacesAssemblyPath = 
				FPaths::RootDir() 
				/ TEXT("Engine/Source/ThirdParty/Klawr/ClrHostInterfaces/bin/$(Configuration)") 
				/ ClrHostInterfacesAssemblyName + TEXT(".dll");
			FPaths::MakePathRelativeTo(ClrHostInterfacesAssemblyPath, *ProjectOutputFilename);
			FPaths::MakePlatformFilename(ClrHostInterfacesAssemblyPath);

			auto RefNode = ReferencesNode.append_child(TEXT("Reference"));
			RefNode.append_attribute(TEXT("Include")) = *ClrHostInterfacesAssemblyName;
			RefNode.append_child(TEXT("HintPath")).text() = *ClrHostInterfacesAssemblyPath;
		}

		// include all the generated C# wrapper classes in the project file
		auto SourceNode = XmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Compile")).parent();
		if (SourceNode)
		{
			FString ManagedGlueFilename, LinkFilename;
			for (const FString& ScriptHeaderFilename : AllScriptHeaders)
			{
				ManagedGlueFilename = ScriptHeaderFilename;
				ManagedGlueFilename.RemoveFromEnd(TEXT(".h"));
				ManagedGlueFilename.Append(TEXT(".cs"));
				FPaths::MakePathRelativeTo(ManagedGlueFilename, *ProjectOutputFilename);
				FPaths::MakePlatformFilename(ManagedGlueFilename);
				// group all generated .cs files under a virtual folder in the project file
				LinkFilename = TEXT("Generated\\");
				LinkFilename += FPaths::GetCleanFilename(ManagedGlueFilename);

				auto CompileNode = SourceNode.append_child(TEXT("Compile"));
				CompileNode.append_attribute(TEXT("Include")) = *ManagedGlueFilename;
				CompileNode.append_child(TEXT("Link")).text() = *LinkFilename;
			}
		}

		// add a post-build event to copy the C# wrapper assembly to the engine binaries dir
		FString AssemblyDestPath = FPlatformProcess::BaseDir();
		FPaths::NormalizeFilename(AssemblyDestPath);
		FPaths::CollapseRelativeDirectories(AssemblyDestPath);
		FPaths::MakePlatformFilename(AssemblyDestPath);
		
		FString PostBuildCmd = FString::Printf(
			TEXT("xcopy \"$(TargetPath)\" \"%s\" /Y"), *AssemblyDestPath
		);

		XmlDoc
			.child(TEXT("Project"))
				.append_child(TEXT("PropertyGroup"))
					.append_child(TEXT("PostBuildEvent")).text() = *PostBuildCmd;

		// preserve the BOM and line endings of the template .csproj when writing out the new file
		unsigned int OutputFlags =
			pugi::format_default | pugi::format_write_bom | pugi::format_save_file_text;

		if (!XmlDoc.save_file(*ProjectOutputFilename, TEXT("  "), OutputFlags))
		{
			FError::Throwf(TEXT("Failed to save %s"), *ProjectOutputFilename);
		}
	}
}

void FKlawrCodeGenerator::FinishExport()
{
	GlueAllNativeWrapperFiles();
	Super::RenameTempFiles();
	GenerateManagedWrapperProject();
}

void FKlawrCodeGenerator::GlueAllNativeWrapperFiles()
{
	// generate the file that will be included by ScriptPlugin.cpp
	FString GlueFilename = GeneratedCodePath / TEXT("GeneratedScriptLibraries.inl");
	FKlawrCodeFormatter GeneratedGlue(TEXT('\t'), 1);

	GeneratedGlue 
		<< TEXT("// This file is autogenerated, DON'T EDIT it, if you do your changes will be lost!")
		<< FKlawrCodeFormatter::LineTerminator();

	// include all source header files
	GeneratedGlue << TEXT("// The native classes which will be made scriptable:");
	for (const auto& HeaderFilename : AllSourceClassHeaders)
	{
		// re-base to make sure we're including the right files on a remote machine
		FString NewFilename(Super::RebaseToBuildPath(HeaderFilename));
		GeneratedGlue << FString::Printf(TEXT("#include \"%s\""), *NewFilename);
	}

	// include all script glue headers
	GeneratedGlue << TEXT("// The autogenerated native wrappers:");
	for (const auto& HeaderFilename : AllScriptHeaders)
	{
		FString NewFilename(FPaths::GetCleanFilename(HeaderFilename));
		GeneratedGlue << FString::Printf(TEXT("#include \"%s\""), *NewFilename);
	}
	
	// generate a function that feeds all the native wrapper functions to the CLR host
	GeneratedGlue
		<< FKlawrCodeFormatter::LineTerminator()
		<< TEXT("namespace Klawr {")
		<< TEXT("namespace NativeGlue {")
		<< FKlawrCodeFormatter::LineTerminator()
		<< TEXT("void RegisterWrapperClasses()")
		<< FKlawrCodeFormatter::OpenBrace();

	for (auto Class : AllExportedClasses)
	{
		const auto ExportedProperties = ClassExportedProperties.Find(Class);
		const auto ExportedFunctions = ClassExportedFunctions.Find(Class);
		bool bHasExports = (ExportedProperties && ExportedProperties->Num())
			|| (ExportedFunctions && ExportedFunctions->Num());

		if (bHasExports)
		{
			FString ClassName = Class->GetName();
			FString ClassNameCPP = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *ClassName);
			GeneratedGlue << FString::Printf(
				TEXT("IClrHost::Get()->AddClass(TEXT(\"%s\"), %s_WrapperFunctions, ")
				TEXT("sizeof(%s_WrapperFunctions) / sizeof(%s_WrapperFunctions[0]));"),
				*ClassNameCPP, *ClassName, *ClassName, *ClassName
			);
		}
	}

	GeneratedGlue 
		<< FKlawrCodeFormatter::CloseBrace()
		<< FKlawrCodeFormatter::LineTerminator()
		<< TEXT("}} // namespace Klawr::NativeGlue");

	Super::SaveHeaderIfChanged(GlueFilename, GeneratedGlue.Content);
}
