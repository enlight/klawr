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

#include "KlawrCodeGeneratorPluginPrivatePCH.h"
#include "KlawrCodeGenerator.h"
#include "KlawrCodeFormatter.h"
#include "pugixml.hpp"

namespace Klawr {

// Names of structs that can be used for interop (they have a corresponding struct type in managed code)
const FName FCodeGenerator::Name_Vector2D("Vector2D");
const FName FCodeGenerator::Name_Vector("Vector");
const FName FCodeGenerator::Name_Vector4("Vector4");
const FName FCodeGenerator::Name_Quat("Quat");
const FName FCodeGenerator::Name_Transform("Transform");
const FName FCodeGenerator::Name_LinearColor("LinearColor");
const FName FCodeGenerator::Name_Color("Color");

const FString FCodeGenerator::UnmanagedFunctionPointerAttribute = 
	TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl)]");

const FString FCodeGenerator::MarshalReturnedBoolAsUint8Attribute =
	TEXT("[return: MarshalAs(UnmanagedType.U1)]");

const FString FCodeGenerator::MarshalBoolParameterAsUint8Attribute =
	TEXT("[MarshalAs(UnmanagedType.U1)]");

const FString FCodeGenerator::ClrHostInterfacesAssemblyName = TEXT("Klawr.ClrHost.Interfaces");
const FString FCodeGenerator::ClrHostManagedAssemblyName = TEXT("Klawr.ClrHost.Managed");
const FString FCodeGenerator::NativeThisPointer = TEXT("(UObjectHandle)this");

FCodeGenerator::FCodeGenerator(
	const FString& InRootLocalPath, const FString& InRootBuildPath, 
	const FString& InOutputDirectory, const FString& InIncludeBase
)
	: GeneratedCodePath(InOutputDirectory)
	, RootLocalPath(InRootLocalPath)
	, RootBuildPath(InRootBuildPath)
	, IncludeBase(InIncludeBase)
{
	 
}

FString FCodeGenerator::GetClassCPPType(const UClass* Class)
{
	return FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
}

FString FCodeGenerator::GetPropertyCPPType(const UProperty* Property)
{
	static const FString EnumDecl(TEXT("enum "));
	static const FString StructDecl(TEXT("struct "));
	static const FString ClassDecl(TEXT("class "));
	static const FString TEnumAsByteDecl(TEXT("TEnumAsByte<enum "));
	static const FString TSubclassOfDecl(TEXT("TSubclassOf<class "));
	static const FString Space(TEXT(" "));

	FString CPPTypeName = Property->GetCPPType(NULL, CPPF_ArgumentOrReturnValue);
	
	if (CPPTypeName.StartsWith(EnumDecl) || CPPTypeName.StartsWith(StructDecl) || CPPTypeName.StartsWith(ClassDecl))
	{
		// strip enum/struct/class keyword
		CPPTypeName = CPPTypeName.Mid(CPPTypeName.Find(Space) + 1);
	}
	else if (CPPTypeName.StartsWith(TEnumAsByteDecl))
	{
		// strip enum keyword
		CPPTypeName = TEXT("TEnumAsByte<") + CPPTypeName.Mid(CPPTypeName.Find(Space) + 1);
	}
	else if (CPPTypeName.StartsWith(TSubclassOfDecl))
	{
		// strip class keyword
		CPPTypeName = TEXT("TSubclassOf<") + CPPTypeName.Mid(CPPTypeName.Find(Space) + 1);
		// Passing around TSubclassOf<UObject> doesn't really provide any more type safety than
		// passing around UClass (so far only UObjects can cross the native/managed boundary),
		// and TSubclassOf<> lacks the reference equality UClass has.
		if (CPPTypeName.StartsWith(TEXT("TSubclassOf<UObject>")))
		{
			CPPTypeName = TEXT("UClass*");
		}
	}
	// some type names end with a space, strip it away
	CPPTypeName.RemoveFromEnd(Space);
	return CPPTypeName;
}

FString FCodeGenerator::GenerateFunctionDispatchParamInitializer(const UProperty* Param)
{	
	if (!(Param->GetPropertyFlags() & CPF_ReturnParm))
	{
		FString ParamName = Param->GetName();
		FString Initializer;

		if (Param->IsA<UClassProperty>())
		{
			Initializer = FString::Printf(TEXT("static_cast<UClass*>(%s)"), *ParamName);
		}
		else if (Param->IsA<UObjectPropertyBase>())
		{
			Initializer = FString::Printf(TEXT("static_cast<%s>(%s)"), *GetPropertyCPPType(Param), *ParamName);
		}
		else if (Param->IsA<UStrProperty>() || Param->IsA<UNameProperty>())
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

			if (Param->IsA<UIntProperty>() || Param->IsA<UFloatProperty>())
			{
				Initializer = ParamName;
			}
			else if (Param->IsA<UBoolProperty>())
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
			else if (Param->IsA<UStructProperty>())
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
	else // Param is actually the return value
	{
		if (Param->IsA<UObjectPropertyBase>())
		{
			return TEXT("nullptr");
		}
		else
		{
			return FString::Printf(TEXT("%s()"), *GetPropertyCPPType(Param));
		}
	}	
}

void FCodeGenerator::GenerateFunctionDispatch(
	const UFunction* Function, FCodeFormatter& GeneratedGlue)
{
	const bool bHasParamsOrReturnValue = (Function->Children != NULL);
	if (bHasParamsOrReturnValue)
	{
		GeneratedGlue
			<< TEXT("struct FDispatchParams")
			<< FCodeFormatter::OpenBrace();

		for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
		{
			UProperty* Param = *ParamIt;
			GeneratedGlue << FString::Printf(
				TEXT("%s %s;"), *GetPropertyCPPType(Param), *Param->GetName()
			);
		}

		GeneratedGlue
			<< FCodeFormatter::CloseBrace()
			<< TEXT("Params =")
			<< FCodeFormatter::OpenBrace();

		int32 ParamIndex = 0;
		for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt, ++ParamIndex)
		{
			GeneratedGlue << FString::Printf(
				TEXT("%s,"), *GenerateFunctionDispatchParamInitializer(*ParamIt)
			);
		}

		GeneratedGlue 
			<< FCodeFormatter::CloseBrace()
			<< TEXT(";");
	}
	
	GeneratedGlue << FString::Printf(
		TEXT("static UFunction* Function = Obj->FindFunctionChecked(TEXT(\"%s\"));"), 
		*Function->GetName()
	);

	if (bHasParamsOrReturnValue)
	{
		GeneratedGlue
			<< TEXT("check(Function->ParmsSize == sizeof(FDispatchParams));")
			<< TEXT("Obj->ProcessEvent(Function, &Params);");
	}
	else
	{
		GeneratedGlue << TEXT("Obj->ProcessEvent(Function, NULL);");
	}
}

void FCodeGenerator::GenerateNativeReturnValueHandler(
	const UProperty* ReturnValue, const FString& ReturnValueName, FCodeFormatter& GeneratedGlue
)
{
	if (ReturnValue)
	{
		if (ReturnValue->IsA<UObjectPropertyBase>())
		{
			// TODO: the assumption here is that UClass instances will be kept alive anyway, so
			//       no need to add a reference... but that's just be wishful thinking, need to 
			//       investigate!
			if (!ReturnValue->IsA<UClassProperty>())
			{
				GeneratedGlue
					<< FString::Printf(TEXT("if (%s)"), *ReturnValueName)
					<< FCodeFormatter::OpenBrace()
						<< FString::Printf(
							TEXT("FScriptObjectReferencer::Get().AddObjectReference(%s);"),
							*ReturnValueName
						)
					<< FCodeFormatter::CloseBrace();
			}
			GeneratedGlue << FString::Printf(TEXT("return static_cast<UObject*>(%s);"), *ReturnValueName);
		}
		else if (ReturnValue->IsA<UIntProperty>() || 
			ReturnValue->IsA<UFloatProperty>() ||
			ReturnValue->IsA<UBoolProperty>())
		{
			GeneratedGlue << FString::Printf(TEXT("return %s;"), *ReturnValueName);
		}
		else if (ReturnValue->IsA<UStrProperty>())
		{
			GeneratedGlue << FString::Printf(
				TEXT("return MakeStringCopyForCLR(*%s);"), *ReturnValueName
			);
		}
		else if (ReturnValue->IsA<UNameProperty>())
		{
			GeneratedGlue << FString::Printf(
				TEXT("return MakeStringCopyForCLR(*(%s.ToString()));"), *ReturnValueName
			);
		}
		else if (ReturnValue->IsA<UStructProperty>())
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

bool FCodeGenerator::CanExportClass(const UClass* Class)
{
	bool bCanExport = 
		// skip classes that don't export DLL symbols
		Class->HasAnyClassFlags(CLASS_RequiredAPI | CLASS_MinimalAPI)
		// these have a custom wrappers, so no need to generate code for them
		&& (Class != UObject::StaticClass())
		&& (Class != UClass::StaticClass());

	if (bCanExport)
	{
		// check for exportable functions
		bool bHasMembersToExport = false;
		TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper);
		for ( ; FuncIt; ++FuncIt)
		{
			if (CanExportFunction(Class, *FuncIt))
			{
				bHasMembersToExport = true;
				break;
			}
		}
		// check for exportable properties
		if (!bHasMembersToExport)
		{
			TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper);
			for ( ; PropertyIt; ++PropertyIt)
			{
				if (CanExportProperty(Class, *PropertyIt))
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

bool FCodeGenerator::CanExportFunction(const UClass* Class, const UFunction* Function)
{
	// functions from base classes should only be exported when those classes are processed
	if (Function->GetOwnerClass() != Class)
	{
		return false;
	}

	// delegates and non-public functions are not supported yet
	if (Function->HasAnyFunctionFlags(FUNC_Delegate | FUNC_Private | FUNC_Protected))
	{
		return false;
	}

	// check all parameter types for this function are supported
	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		if (!IsPropertyTypeSupported(*ParamIt))
		{
			return false;
		}
	}

	return true;
}

UProperty* FCodeGenerator::GetNativeWrapperArgsAndReturnType(
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

void FCodeGenerator::GenerateNativeWrapperFunction(
	const UClass* Class, UFunction* Function, FCodeFormatter& GeneratedGlue
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
	GeneratedGlue << FCodeFormatter::OpenBrace();

	// call the wrapped UFunction
	// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
	//        regular function argument names.
	GeneratedGlue << TEXT("UObject* Obj = static_cast<UObject*>(self);");
	GenerateFunctionDispatch(Function, GeneratedGlue);

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
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();

	FExportedFunction FuncInfo;
	FuncInfo.Function = Function;
	FuncInfo.bHasReturnValue = (ReturnValue != nullptr);
	FuncInfo.NativeWrapperFunctionName = WrapperName;
	ClassExportedFunctions.FindOrAdd(Class).Add(FuncInfo);
}

FString FCodeGenerator::GenerateDelegateTypeName(const FString& FunctionName, bool bHasReturnValue) const
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

FString FCodeGenerator::GenerateDelegateName(const FString& FunctionName) const
{
	return FString(TEXT("_")) + FunctionName;
}

UProperty* FCodeGenerator::GetManagedWrapperArgsAndReturnType(
	const UFunction* Function, FString& OutFormalInteropArgs, FString& OutActualInteropArgs,
	FString& OutFormalManagedArgs, FString& OutActualManagedArgs
)
{
	OutFormalInteropArgs = TEXT("UObjectHandle self");
	OutActualInteropArgs = NativeThisPointer;
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
			FString ArgInteropType = GetPropertyInteropType(Param);
			FString ArgAttrs = GetPropertyInteropTypeAttributes(Param);
			FString ArgMods = GetPropertyInteropTypeModifiers(Param);
			
			OutFormalInteropArgs += TEXT(",");
			if (!ArgAttrs.IsEmpty())
			{
				OutFormalInteropArgs += TEXT(" ");
				OutFormalInteropArgs += ArgAttrs;
			}
			if (!ArgMods.IsEmpty())
			{
				OutFormalInteropArgs += TEXT(" ");
				OutFormalInteropArgs += ArgMods;
			}
			OutFormalInteropArgs += FString::Printf(
				TEXT(" %s %s"), *ArgInteropType, *ArgName
			);

			OutActualInteropArgs += TEXT(",");
			if (!ArgMods.IsEmpty())
			{
				OutActualInteropArgs += TEXT(" ");
				OutActualInteropArgs += ArgMods;
			}
			OutActualInteropArgs += TEXT(" ");
			if (Param->IsA<UObjectProperty>())
			{
				OutActualInteropArgs += TEXT("(UObjectHandle)");
			}
			OutActualInteropArgs += ArgName;
									
			if (!OutFormalManagedArgs.IsEmpty())
			{
				OutFormalManagedArgs += TEXT(", ");
			}
			if (!ArgMods.IsEmpty())
			{
				OutFormalManagedArgs += ArgMods + TEXT(" ");
			}
			FString ArgManagedType = GetPropertyManagedType(Param);
			OutFormalManagedArgs += FString::Printf(TEXT("%s %s"), *ArgManagedType, *ArgName);
			
			if (!OutActualManagedArgs.IsEmpty())
			{
				OutActualManagedArgs += TEXT(", ");
			}
			OutActualManagedArgs += ArgName;
		}
	}

	return ReturnValue;
}

FString FCodeGenerator::GenerateManagedReturnValueHandler(const UProperty* ReturnValue)
{
	if (ReturnValue)
	{
		if (ReturnValue->IsA<UClassProperty>())
		{
			return TEXT("return (UClass)value;");
		}
		else if (ReturnValue->IsA<UObjectProperty>())
		{
			FString WrapperTypeName = GetPropertyCPPType(ReturnValue);
			WrapperTypeName.RemoveFromEnd(TEXT("*"));
			return FString::Printf(
				TEXT("return new %s(value);"), *WrapperTypeName
			);
		}
		else
		{
			return TEXT("return value;");
		}
	}
	return FString();
}

void FCodeGenerator::GenerateManagedWrapperFunction(
	const UClass* Class, const UFunction* Function, FCodeFormatter& GeneratedGlue
)
{
	FString FormalInteropArgs, ActualInteropArgs, FormalManagedArgs, ActualManagedArgs;
	UProperty* ReturnValue = GetManagedWrapperArgsAndReturnType(
		Function, FormalInteropArgs, ActualInteropArgs, FormalManagedArgs, ActualManagedArgs
	);
	const bool bHasReturnValue = (ReturnValue != nullptr);
	const bool bReturnsBool = (bHasReturnValue && ReturnValue->IsA(UBoolProperty::StaticClass()));
	const FString ReturnValueInteropTypeName = 
		bHasReturnValue ? GetPropertyInteropType(ReturnValue) : TEXT("void");
	const FString ReturnValueManagedTypeName =
		bHasReturnValue ? GetPropertyManagedType(ReturnValue) : TEXT("void");
	const FString DelegateTypeName = GenerateDelegateTypeName(Function->GetName(), bHasReturnValue);
	const FString DelegateName = GenerateDelegateName(Function->GetName());

	GeneratedGlue 
		// declare a managed delegate type matching the type of the native wrapper function
		<< UnmanagedFunctionPointerAttribute
		<< (bReturnsBool ? MarshalReturnedBoolAsUint8Attribute : FString())
		<< FString::Printf(
			TEXT("private delegate %s %s(%s);"),
			*ReturnValueInteropTypeName, *DelegateTypeName, *FormalInteropArgs
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
			*ReturnValueManagedTypeName, *Function->GetName(), *FormalManagedArgs
		)
		<< FCodeFormatter::OpenBrace();

	// call the delegate bound to the native wrapper function
	if (bHasReturnValue)
	{
		GeneratedGlue 
			<< FString::Printf(TEXT("var value = %s(%s);"), *DelegateName, *ActualInteropArgs)
			<< GenerateManagedReturnValueHandler(ReturnValue);
	}
	else
	{
		GeneratedGlue << FString::Printf(TEXT("%s(%s);"), *DelegateName, *ActualInteropArgs);
	}
		
	GeneratedGlue
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();
}

void FCodeGenerator::ExportFunction(
	const FString& ClassNameCPP, const UClass* Class, UFunction* Function,
	FCodeFormatter& NativeGlueCode, FCodeFormatter& ManagedGlueCode
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

bool FCodeGenerator::IsPropertyTypeSupported(const UProperty* Property)
{
	bool bSupported = true;

	if (Property->IsA<UArrayProperty>() ||
		Property->ArrayDim > 1 ||
		Property->IsA<UDelegateProperty>() ||
		Property->IsA<UMulticastDelegateProperty>() ||
		Property->IsA<UWeakObjectProperty>() ||
		Property->IsA<UInterfaceProperty>())
	{
		bSupported = false;
	}
	else if (Property->IsA<UStructProperty>())
	{
		auto StructProp = CastChecked<UStructProperty>(Property);
		bSupported = IsStructPropertyTypeSupported(StructProp);
	}
	else if (Property->IsA<UStrProperty>())
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
	else if (!Property->IsA<UIntProperty>() &&
		!Property->IsA<UFloatProperty>() &&
		!Property->IsA<UBoolProperty>() &&
		!Property->IsA<UObjectPropertyBase>() &&
		!Property->IsA<UNameProperty>())
	{
		bSupported = false;
	}
	else if (Property->IsA<ULazyObjectProperty>())
	{
		bSupported = false;
	}

	return bSupported;
}

bool FCodeGenerator::IsPropertyTypePointer(const UProperty* Property)
{
	return Property->IsA<UObjectPropertyBase>() || Property->IsA<ULazyObjectProperty>();
}

bool FCodeGenerator::IsStructPropertyTypeSupported(const UStructProperty* Property)
{
	return (Property->Struct->GetFName() == Name_Vector2D) 
		|| (Property->Struct->GetFName() == Name_Vector) 
		|| (Property->Struct->GetFName() == Name_Vector4) 
		|| (Property->Struct->GetFName() == Name_Quat) 
		|| (Property->Struct->GetFName() == Name_LinearColor) 
		|| (Property->Struct->GetFName() == Name_Color) 
		|| (Property->Struct->GetFName() == Name_Transform);
}

FString FCodeGenerator::GetPropertyNativeType(const UProperty* Property)
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
		NativeType = GetPropertyCPPType(Property);
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

FString FCodeGenerator::GetPropertyInteropType(const UProperty* Property)
{
	if (Property->IsA<UObjectProperty>())
	{
		return TEXT("UObjectHandle");
	}
	else if (IsPropertyTypePointer(Property))
	{
		return TEXT("IntPtr");
	}
	else if (Property->IsA<UBoolProperty>())
	{
		return TEXT("bool");
	}
	else if (Property->IsA<UIntProperty>())
	{
		return TEXT("int");
	}
	else if (Property->IsA<UFloatProperty>())
	{
		return TEXT("float");
	}
	else if (Property->IsA<UDoubleProperty>())
	{
		return TEXT("double");
	}
	else if (Property->IsA<UStrProperty>() || Property->IsA<UNameProperty>())
	{
		return TEXT("string");
	}
	else
	{
		return GetPropertyCPPType(Property);
	}
}

FString FCodeGenerator::GetPropertyManagedType(const UProperty* Property)
{
	if (Property->IsA<UObjectProperty>())
	{
		static FString Pointer(TEXT("*"));
		FString TypeName = GetPropertyCPPType(Property);
		TypeName.RemoveFromEnd(Pointer);
		return TypeName;
	}
	return GetPropertyInteropType(Property);
}

FString FCodeGenerator::GetPropertyInteropTypeAttributes(const UProperty* Property)
{
	if (Property->IsA<UBoolProperty>())
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

FString FCodeGenerator::GetPropertyInteropTypeModifiers(const UProperty* Property)
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

bool FCodeGenerator::CanExportProperty(const UClass* Class, const UProperty* Property)
{
	// properties from base classes should only be exported when those classes are processed
	if (Property->GetOwnerClass() != Class)
	{
		return false;
	}

	// property must be DLL exported (well, not really, will remove this later)
	if (!(Class->ClassFlags & CLASS_RequiredAPI))
	{
		return false;
	}

	// only public, editable properties can be exported
	if (!Property->HasAnyFlags(RF_Public) ||
		(Property->GetPropertyFlags() & CPF_Protected) ||
		!(Property->GetPropertyFlags() & CPF_Edit))
	{
		return false;
	}

	return IsPropertyTypeSupported(Property);
}

void FCodeGenerator::GenerateNativePropertyGetterWrapper(
	const FString& ClassNameCPP, const UClass* Class, const UProperty* Property,
	FCodeFormatter& GeneratedGlue, FExportedProperty& ExportedProperty
)
{
	// define a native getter wrapper function that will be bound to a managed delegate
	FString PropertyNativeTypeName = GetPropertyNativeType(Property);
	FString GetterName = FString::Printf(TEXT("%s_Get_%s"), *Class->GetName(), *Property->GetName());
	ExportedProperty.NativeGetterWrapperFunctionName = GetterName;

	GeneratedGlue 
		<< FString::Printf(TEXT("%s %s(void* self)"), *PropertyNativeTypeName, *GetterName)
		<< FCodeFormatter::OpenBrace()
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		<< TEXT("UObject* Obj = static_cast<UObject*>(self);")
		<< FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));"),
			*ClassNameCPP, *Property->GetName()
		)
		<< FString::Printf(TEXT("%s PropertyValue;\r\n"), *GetPropertyCPPType(Property))
		<< TEXT("Property->CopyCompleteValue(&PropertyValue, Property->ContainerPtrToValuePtr<void>(Obj));");
			
	GenerateNativeReturnValueHandler(Property, TEXT("PropertyValue"), GeneratedGlue);
	
	GeneratedGlue 
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();
}

void FCodeGenerator::GenerateNativePropertySetterWrapper(
	const FString& ClassNameCPP, const UClass* Class, const UProperty* Property,
	FCodeFormatter& GeneratedGlue, FExportedProperty& ExportedProperty
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
		<< FCodeFormatter::OpenBrace()
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		<< TEXT("UObject* Obj = static_cast<UObject*>(self);")
		<< FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));"), 
			*ClassNameCPP, *Property->GetName()
		)
		<< FString::Printf(
			TEXT("%s PropertyValue = %s;"),
			*GetPropertyCPPType(Property), *GenerateFunctionDispatchParamInitializer(Property)
		)
		<< TEXT("Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Obj), &PropertyValue);")
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();
}

void FCodeGenerator::GenerateManagedPropertyWrapper(
	const UClass* Class, const UProperty* Property, FCodeFormatter& GeneratedGlue, 
	FExportedProperty& ExportedProperty
)
{
	const FString GetterName = FString::Printf(TEXT("Get_%s"), *Property->GetName());
	const FString SetterName = FString::Printf(TEXT("Set_%s"), *Property->GetName());
	
	ExportedProperty.GetterDelegateName = GenerateDelegateName(GetterName);
	ExportedProperty.GetterDelegateTypeName = GenerateDelegateTypeName(GetterName, true);
	ExportedProperty.SetterDelegateName = GenerateDelegateName(SetterName);
	ExportedProperty.SetterDelegateTypeName = GenerateDelegateTypeName(SetterName, false);
	
	const bool bIsBoolProperty = Property->IsA<UBoolProperty>();
	const FString InteropTypeName = GetPropertyInteropType(Property);
	const FString ManagedTypeName = GetPropertyManagedType(Property);
	FString SetterParamType = InteropTypeName;
	if (bIsBoolProperty)
	{
		SetterParamType = FString::Printf(
			TEXT("%s %s"), *MarshalBoolParameterAsUint8Attribute, *InteropTypeName
		);
	}
	FString GetterValue(TEXT("value"));
	FString SetterValue(TEXT("value"));
	if (Property->IsA<UObjectProperty>())
	{
		if (Property->IsA<UClassProperty>())
		{
			GetterValue = TEXT("(UClass)value");
		}
		else
		{
			GetterValue = FString::Printf(TEXT("new %s(value)"), *ManagedTypeName);
		}
		SetterValue = TEXT("(UObjectHandle)value");
	}
	
	GeneratedGlue
		// declare getter delegate type
		<< UnmanagedFunctionPointerAttribute
		<< (bIsBoolProperty ? MarshalReturnedBoolAsUint8Attribute : FString())
		<< FString::Printf(
			TEXT("private delegate %s %s(UObjectHandle self);"),
			*InteropTypeName, *ExportedProperty.GetterDelegateTypeName
		)
		// declare setter delegate type
		<< UnmanagedFunctionPointerAttribute
		<< FString::Printf(
			TEXT("private delegate void %s(UObjectHandle self, %s %s);"),
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
		<< FString::Printf(TEXT("public %s %s"), *ManagedTypeName, *Property->GetName())
		<< FCodeFormatter::OpenBrace()
			<< TEXT("get")
			<< FCodeFormatter::OpenBrace()
				<< FString::Printf(TEXT("var value = %s(%s);"), 
					*ExportedProperty.GetterDelegateName, *NativeThisPointer
				)
				<< GenerateManagedReturnValueHandler(Property)
			<< FCodeFormatter::CloseBrace()
			<< FString::Printf(
				TEXT("set { %s(%s, %s); }"), 
				*ExportedProperty.SetterDelegateName, *NativeThisPointer, *SetterValue
			)
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();
}

void FCodeGenerator::ExportProperty(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property,
	FCodeFormatter& NativeGlueCode, FCodeFormatter& ManagedGlueCode
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

void FCodeGenerator::GenerateNativeGlueCodeHeader(
	const UClass* Class, FCodeFormatter& GeneratedGlue
)
{
	GeneratedGlue 
		<< TEXT("#pragma once") LINE_TERMINATOR
		<< TEXT("namespace Klawr {")
		<< TEXT("namespace NativeGlue {") LINE_TERMINATOR;
}

void FCodeGenerator::GenerateNativeGlueCodeFooter(
	const UClass* Class, FCodeFormatter& GeneratedGlue
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
			<< FCodeFormatter::OpenBrace();

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
			<< FCodeFormatter::CloseBrace()
			<< TEXT(";");
	}

	GeneratedGlue << TEXT("}} // namespace Klawr::NativeGlue");
}

void FCodeGenerator::GenerateManagedStaticConstructor(
	const UClass* Class, FCodeFormatter& GeneratedGlue
)
{
	auto ExportedProperties = ClassExportedProperties.Find(Class);
	auto ExportedFunctions = ClassExportedFunctions.Find(Class);

	if (((ExportedFunctions == nullptr) || (ExportedFunctions->Num() == 0)) && 
		((ExportedProperties == nullptr) || (ExportedProperties->Num() == 0)))
	{
		return;
	}

	const FString ClassNameCPP = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	GeneratedGlue << FString::Printf(TEXT("static %s()"), *ClassNameCPP);
	GeneratedGlue << FCodeFormatter::OpenBrace();
	
	// bind managed delegates to pointers to native wrapper functions
	GeneratedGlue 
		<< TEXT("var manager = AppDomain.CurrentDomain.DomainManager as IEngineAppDomainManager;")
		<< FString::Printf(
			TEXT("var nativeFuncPtrs = manager.GetNativeFunctionPointers(\"%s\");"), *ClassNameCPP
		);
	
	int32 FunctionIdx = 0;
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
	
	GeneratedGlue << FCodeFormatter::CloseBrace();
}

bool FCodeGenerator::ShouldGenerateManagedWrapper(const UClass* Class)
{
	return (Class != UObject::StaticClass())
		&& (Class != UClass::StaticClass());
}

bool FCodeGenerator::ShouldGenerateScriptObjectClass(const UClass* Class)
{
	return (Class != UClass::StaticClass());

	// FIXME: GetBoolMetaDataHierarchical() is only available when WITH_EDITOR is defined, and it's
	//        not defined when building this plugin :/. The non hierarchical version of GetMetaData()
	//        should be available though (because HACK_HEADER_GENERATOR should be defined for this
	//        plugin) so it is possible to do what GetBoolMetaDataHierarchical() does.
	//return Class->GetBoolMetaDataHierarchical(FBlueprintMetadata::MD_IsBlueprintBase);
}

void FCodeGenerator::GenerateManagedGlueCodeHeader(
	const UClass* Class, FCodeFormatter& GeneratedGlue
) const
{
	FString ClassNameCPP = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	FString ClassDecl = FString::Printf(TEXT("public class %s"), *ClassNameCPP);
	// mirror the inheritance in the managed wrapper class while skipping base classes that haven't
	// been exported (this may change in the future)
	UClass* SuperClass = nullptr;
	// find the top-most exported ancestor in the inheritance hierarchy
	for (UClass* CurrentAncestor = Class->GetSuperClass(); CurrentAncestor; )
	{
		if (AllExportedClasses.Contains(CurrentAncestor))
		{
			SuperClass = CurrentAncestor;
		}
		CurrentAncestor = CurrentAncestor->GetSuperClass();
	}
	
	if (SuperClass)
	{
		ClassDecl += FString::Printf(
			TEXT(" : %s%s"), SuperClass->GetPrefixCPP(), *SuperClass->GetName()
		);
	}
		
	GeneratedGlue 
		// usings
		<< TEXT("using System;")
		<< TEXT("using System.Runtime.InteropServices;")
		<< TEXT("using Klawr.ClrHost.Interfaces;")
		<< TEXT("using Klawr.ClrHost.Managed;")
		<< FCodeFormatter::LineTerminator()

		// declare namespace
		<< TEXT("namespace Klawr.UnrealEngine") 
		<< FCodeFormatter::OpenBrace();

	if (ShouldGenerateManagedWrapper(Class))
	{ 
		GeneratedGlue
			// declare class
			<< ClassDecl 
			<< FCodeFormatter::OpenBrace()

				// constructor
				<< FString::Printf(
					TEXT("public %s(UObjectHandle nativeObject) : base(nativeObject)"), *ClassNameCPP
				)
				<< FCodeFormatter::OpenBrace()
				<< FCodeFormatter::CloseBrace()
	
				// StaticClass()
				<< TEXT("public new static UClass StaticClass()")
				<< FCodeFormatter::OpenBrace()
					<< FString::Printf(TEXT("return (UClass)typeof(%s);"), *ClassNameCPP)
				<< FCodeFormatter::CloseBrace()
			
			<< FCodeFormatter::LineTerminator();
	}
}

void FCodeGenerator::GenerateManagedGlueCodeFooter(
	const UClass* Class, FCodeFormatter& GeneratedGlue
)
{
	if (ShouldGenerateManagedWrapper(Class))
	{
		// define static constructor
		GenerateManagedStaticConstructor(Class, GeneratedGlue);
		// close the class
		GeneratedGlue
			<< FCodeFormatter::CloseBrace()
			<< FCodeFormatter::LineTerminator();
	}	
	
	if (ShouldGenerateScriptObjectClass(Class))
	{
		GenerateManagedScriptObjectClass(Class, GeneratedGlue);
	}
	
	// close the namespace
	GeneratedGlue << FCodeFormatter::CloseBrace();
}

void FCodeGenerator::GenerateManagedScriptObjectClass(
	const UClass* Class, FCodeFormatter& GeneratedGlue
)
{
	// Users should be allowed to subclass the generated wrapper class, but if the subclass is to be
	// used via Blueprints it must implement the IScriptObject interface (which would be an abstract
	// class if C# supported multiple inheritance). To simplify things an abstract class is
	// generated that is derived from the wrapper class and implements the IScriptObject interface,
	// the user can then simply subclass this abstract class.

	FString ClassNameCPP = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	GeneratedGlue
		<< FString::Printf(
			TEXT("public abstract class %sScriptObject : %s, IScriptObject"),
			*ClassNameCPP, *ClassNameCPP
		)
		<< FCodeFormatter::OpenBrace()
			<< TEXT("private readonly long _instanceID;")
			<< FCodeFormatter::LineTerminator()
			// InstanceID property
			<< TEXT("public long InstanceID")
			<< FCodeFormatter::OpenBrace()
				<< TEXT("get { return _instanceID; }")
			<< FCodeFormatter::CloseBrace()
			<< FCodeFormatter::LineTerminator()
			// constructor
			<< FString::Printf(
				TEXT("public %sScriptObject(long instanceID, UObjectHandle nativeObject) : base(nativeObject)"),
				*ClassNameCPP
			)
			<< FCodeFormatter::OpenBrace()
				<< TEXT("_instanceID = instanceID;")
			<< FCodeFormatter::CloseBrace()
			<< FCodeFormatter::LineTerminator()
			<< TEXT("public virtual void BeginPlay() {}")
			<< TEXT("public virtual void Tick(float deltaTime) {}")
			<< TEXT("public virtual void Destroy() {}")
		<< FCodeFormatter::CloseBrace();
}

void FCodeGenerator::ExportClass(
	UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, 
	bool bHasChanged
)
{
	if (Class->HasAnyClassFlags(CLASS_Temporary | CLASS_Deprecated))
	{
		return;
	}

	if (AllExportedClasses.Contains(Class))
	{
		// already processed
		return;
	}

	UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Exporting class %s"), *Class->GetName());

	// even if a class can't be properly exported generate a C# wrapper for it, because it may 
	// still be used as a function parameter in a function that is exported by another class
	ExportedClassNames.Add(Class->GetFName());
	AllExportedClasses.Add(Class);
	
	FCodeFormatter NativeGlueCode(TEXT('\t'), 1);
	FCodeFormatter ManagedGlueCode(TEXT(' '), 4);

	bool bCanExport = CanExportClass(Class);
	if (bCanExport)
	{
		GenerateNativeGlueCodeHeader(Class, NativeGlueCode);
	}

	// some internal classes like UObjectProperty don't have an associated source header file,
	// no native wrapper functions are generated for those types, and perhaps we shouldn't generate
	// any C# wrappers for those either?
	if (!SourceHeaderFilename.IsEmpty())
	{
		// NOTE: Ideally when no native wrapper functions are generated for a class the corresponding
		//       header need not be included, however the class may still be referenced in the
		//       wrapper functions of other classes and if the header isn't included the compilation
		//       will fail. The compilation error is usually a cryptic error C2664, and occurs while 
		//       attempting to upcast a pointer to the class into a UObject*, this fails because the
		//       class is only forward declared at that point and as such the compiler is unaware that
		//       the class is derived from UObject.
		AllSourceClassHeaders.Add(SourceHeaderFilename);
	}
	
	GenerateManagedGlueCodeHeader(Class, ManagedGlueCode);
	
	if (bCanExport)
	{
		const FString ClassNameCPP = GetClassCPPType(Class);

		// export functions
		TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper);
		for ( ; FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			if (CanExportFunction(Class, Function))
			{
				ExportFunction(ClassNameCPP, Class, Function, NativeGlueCode, ManagedGlueCode);
			}
		}

		// export properties
		TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper);
		for ( ; PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			if (CanExportProperty(Class, Property))
			{
				UE_LOG(
					LogKlawrCodeGenerator, Log, 
					TEXT("  %s %s"), *Property->GetClass()->GetName(), *Property->GetName()
				);
				ExportProperty(ClassNameCPP, Class, Property, NativeGlueCode, ManagedGlueCode);
			}
		}

		GenerateNativeGlueCodeFooter(Class, NativeGlueCode);

		const FString NativeGlueFilename = GeneratedCodePath / (Class->GetName() + TEXT(".klawr.h"));
		AllScriptHeaders.Add(NativeGlueFilename);
		WriteToFile(NativeGlueFilename, NativeGlueCode.Content);
	}

	GenerateManagedGlueCodeFooter(Class, ManagedGlueCode);

	const FString ManagedGlueFilename = GeneratedCodePath / (Class->GetName() + TEXT(".cs"));
	AllManagedWrapperFiles.Add(ManagedGlueFilename);
	WriteToFile(ManagedGlueFilename, ManagedGlueCode.Content);
}

void FCodeGenerator::GenerateManagedWrapperProject()
{
	const FString ResourcesBasePath = 
		FPaths::EnginePluginsDir() / TEXT("Klawr/KlawrCodeGeneratorPlugin/Resources/WrapperProjectTemplate");
	const FString ProjectBasePath = FPaths::EngineIntermediateDir() / TEXT("ProjectFiles/Klawr");

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.CopyDirectoryTree(*ProjectBasePath, *ResourcesBasePath, true))
	{
		FError::Throwf(TEXT("Failed to copy wrapper template!"));
	}

	const FString ProjectName("Klawr.UnrealEngine.csproj");
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
			FString AssemblyPath = 
				FPaths::RootDir() 
				/ TEXT("Engine/Source/ThirdParty/Klawr/ClrHostInterfaces/bin/$(Configuration)") 
				/ ClrHostInterfacesAssemblyName + TEXT(".dll");
			FPaths::MakePathRelativeTo(AssemblyPath, *ProjectOutputFilename);
			FPaths::MakePlatformFilename(AssemblyPath);

			auto RefNode = ReferencesNode.append_child(TEXT("Reference"));
			RefNode.append_attribute(TEXT("Include")) = *ClrHostInterfacesAssemblyName;
			RefNode.append_child(TEXT("HintPath")).text() = *AssemblyPath;
		}

		// add a reference to the CLR host managed assembly
		if (ReferencesNode)
		{
			FString AssemblyPath =
				FPaths::RootDir()
				/ TEXT("Engine/Source/ThirdParty/Klawr/ClrHostManaged/bin/$(Configuration)")
				/ ClrHostManagedAssemblyName + TEXT(".dll");
			FPaths::MakePathRelativeTo(AssemblyPath, *ProjectOutputFilename);
			FPaths::MakePlatformFilename(AssemblyPath);

			auto RefNode = ReferencesNode.append_child(TEXT("Reference"));
			RefNode.append_attribute(TEXT("Include")) = *ClrHostManagedAssemblyName;
			RefNode.append_child(TEXT("HintPath")).text() = *AssemblyPath;
		}

		// include all the generated C# wrapper classes in the project file
		auto SourceNode = XmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Compile")).parent();
		if (SourceNode)
		{
			FString LinkFilename;
			for (FString ManagedGlueFilename : AllManagedWrapperFiles)
			{
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

void FCodeGenerator::BuildManagedWrapperProject()
{
	FString BuildFilename = FPaths::EngineIntermediateDir() / TEXT("ProjectFiles/Klawr/Build.bat");
	FPaths::CollapseRelativeDirectories(BuildFilename);
	FPaths::MakePlatformFilename(BuildFilename);
	int32 ReturnCode = 0;
	FString StdOut;
	FString StdError;

	FPlatformProcess::ExecProcess(
		*BuildFilename, TEXT(""), &ReturnCode, &StdOut, &StdError
	);

	if (ReturnCode != 0)
	{
		FError::Throwf(TEXT("Failed to build Klawr.UnrealEngine assembly!"));
	}
}

void FCodeGenerator::FinishExport()
{
	GlueAllNativeWrapperFiles();
	GenerateManagedWrapperProject();
	BuildManagedWrapperProject();
}

void FCodeGenerator::GlueAllNativeWrapperFiles()
{
	// generate the file that will be included by ScriptPlugin.cpp
	FString GlueFilename = GeneratedCodePath / TEXT("KlawrGeneratedNativeWrappers.inl");
	FCodeFormatter GeneratedGlue(TEXT('\t'), 1);

	GeneratedGlue 
		<< TEXT("// This file is autogenerated, DON'T EDIT it, if you do your changes will be lost!")
		<< FCodeFormatter::LineTerminator();

	// include all source header files
	GeneratedGlue << TEXT("// The native classes which will be made scriptable:");
	for (const auto& HeaderFilename : AllSourceClassHeaders)
	{
		// re-base to make sure we're including the right files on a remote machine
		FString NewFilename(RebaseToBuildPath(HeaderFilename));
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
		<< FCodeFormatter::LineTerminator()
		<< TEXT("namespace Klawr {")
		<< TEXT("namespace NativeGlue {")
		<< FCodeFormatter::LineTerminator()
		<< TEXT("void RegisterWrapperClasses()")
		<< FCodeFormatter::OpenBrace();

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
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator()
		<< TEXT("}} // namespace Klawr::NativeGlue");

	WriteToFile(GlueFilename, GeneratedGlue.Content);
}

void FCodeGenerator::WriteToFile(const FString& Path, const FString& Content)
{
	FString DiskContent;
	FFileHelper::LoadFileToString(DiskContent, *Path);

	const bool bContentChanged = (DiskContent.Len() == 0) || FCString::Strcmp(*DiskContent, *Content);
	if (bContentChanged)
	{
		if (!FFileHelper::SaveStringToFile(Content, *Path))
		{
			UE_LOG(LogKlawrCodeGenerator, Warning, TEXT("Failed to save '%s'"), *Path);
		}
	}
}

FString FCodeGenerator::RebaseToBuildPath(const FString& Filename) const
{
	FString RebasedFilename(Filename);
	FPaths::MakePathRelativeTo(RebasedFilename, *IncludeBase);
	return RebasedFilename;
}

} // namespace Klawr
