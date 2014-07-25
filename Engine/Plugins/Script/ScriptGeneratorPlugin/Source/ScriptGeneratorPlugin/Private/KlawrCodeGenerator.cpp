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
#include "pugixml.hpp"

// Names of structs that can be used for interop (they have a corresponding struct type in managed code)
const FName FKlawrCodeGenerator::Name_Vector2D("Vector2D");
const FName FKlawrCodeGenerator::Name_Vector("Vector");
const FName FKlawrCodeGenerator::Name_Vector4("Vector4");
const FName FKlawrCodeGenerator::Name_Quat("Quat");
const FName FKlawrCodeGenerator::Name_Transform("Transform");
const FName FKlawrCodeGenerator::Name_LinearColor("LinearColor");
const FName FKlawrCodeGenerator::Name_Color("Color");

const FString FKlawrCodeGenerator::NativeTab(TEXT("\t"));
const FString FKlawrCodeGenerator::ManagedTab(TEXT("    "));

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
		FString Initializer;
		if (Param->IsA(UIntProperty::StaticClass())	|| 
			Param->IsA(UFloatProperty::StaticClass()) ||
			Param->IsA(UStrProperty::StaticClass()) ||
			Param->IsA(UNameProperty::StaticClass()) ||
			Param->IsA(UBoolProperty::StaticClass()))
		{
			Param->GetName(Initializer);
		}
		else if (Param->IsA(UStructProperty::StaticClass()))
		{
			auto StructProp = CastChecked<UStructProperty>(Param);
			if (IsStructPropertyTypeSupported(StructProp))
			{
				Param->GetName(Initializer);
			}
			else
			{
				FError::Throwf(
					TEXT("Unsupported function param struct type: %s"), 
					*StructProp->Struct->GetName()
				);
			}
		}
		else if (Param->IsA(UClassProperty::StaticClass()))
		{
			Initializer = TEXT("(UClass*)");
			Initializer += Param->GetName();
		}
		else if (Param->IsA(UObjectPropertyBase::StaticClass()))
		{
			Initializer = FString::Printf(
				TEXT("(%s)%s"), 
				*Super::GetPropertyTypeCPP(Param, CPPF_ArgumentOrReturnValue), *Param->GetName()
			);
		}
		else
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

FString FKlawrCodeGenerator::GenerateNativeReturnValueHandler(
	UProperty* ReturnValue, const FString& ReturnValueName
)
{
	FString ReturnHandler = NativeIndent;
	if (ReturnValue)
	{
		if (ReturnValue->IsA(UIntProperty::StaticClass()) || 
			ReturnValue->IsA(UFloatProperty::StaticClass()) ||
			ReturnValue->IsA(UBoolProperty::StaticClass()) ||
			ReturnValue->IsA(UObjectPropertyBase::StaticClass()))
		{
			ReturnHandler += FString::Printf(TEXT("return %s;\r\n"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UStrProperty::StaticClass()))
		{
			ReturnHandler += FString::Printf(TEXT("return *%s;\r\n"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UNameProperty::StaticClass()))
		{
			ReturnHandler += FString::Printf(TEXT("return *(%s.ToString());\r\n"), *ReturnValueName);
		}
		else if (ReturnValue->IsA(UStructProperty::StaticClass()))
		{
			auto StructProp = CastChecked<UStructProperty>(ReturnValue);
			if (IsStructPropertyTypeSupported(StructProp))
			{
				ReturnHandler += FString::Printf(TEXT("return %s;\r\n"), *ReturnValueName);
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
	return ReturnHandler;
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

FString FKlawrCodeGenerator::GenerateNativeWrapperFunction(
	const UClass* Class, UFunction* Function, const UClass* FuncSuper
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
	FString GeneratedGlue = NativeIndent + FString::Printf(
		TEXT("%s %s_%s(%s)\r\n{\r\n"),
		*ReturnValueTypeName, *Class->GetName(), *Function->GetName(), *FormalArgs
	);
	IndentNativeCode();
	// call the wrapped UFunction
	if (!FuncSuper)
	{
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		GeneratedGlue += NativeIndent + TEXT("UObject* Obj = (UObject*)self;\r\n");
		// FIXME: Super::GenerateFunctionDispatch() doesn't indent code properly, get rid of it!
		// TODO: Maybe use an initializer list to FDispatchParams struct instead of the current multi-line init
		GeneratedGlue += Super::GenerateFunctionDispatch(Function);

		if (ReturnValue)
		{
			GeneratedGlue += GenerateNativeReturnValueHandler(
				ReturnValue, FString::Printf(TEXT("Params.%s"), *ReturnValue->GetName())
			);
		}
	}
	else // the function is actually implemented in a base class, so call the base class version
	{
		GeneratedGlue += FString::Printf(
			TEXT("return %s_%s(%s);\r\n"),
			*FuncSuper->GetName(), *Function->GetName(), *ActualArgs
		);
	}
	UnindentNativeCode();
	GeneratedGlue += TEXT("}\r\n\r\n");

	FExportedFunction FuncInfo;
	FuncInfo.Function = Function;
	FuncInfo.bHasReturnValue = (ReturnValue != nullptr);
	ClassExportedFunctions.FindOrAdd(Class).Add(FuncInfo);

	return GeneratedGlue;
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

void FKlawrCodeGenerator::GenerateManagedWrapperArgsAndReturnType(
	const UFunction* Function, FString& OutFormalInteropArgs, FString& OutActualInteropArgs,
	FString& OutFormalManagedArgs, FString& OutActualManagedArgs, FString& OutReturnValueType
)
{
	OutFormalInteropArgs = TEXT("IntPtr self");
	OutActualInteropArgs = TEXT("_nativeObject");
	OutFormalManagedArgs.Empty();
	OutActualManagedArgs.Empty();
	OutReturnValueType = TEXT("void");

	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		UProperty* Param = *ParamIt;
		if (Param->GetPropertyFlags() & CPF_ReturnParm)
		{
			OutReturnValueType = GetPropertyInteropType(Param);
		}
		else
		{
			FString ArgName = Param->GetName();
			FString ArgType = GetPropertyInteropType(Param);
			OutFormalInteropArgs += FString::Printf(TEXT(", %s %s"), *ArgType, *ArgName);
			OutActualInteropArgs += FString::Printf(TEXT(", %s"), *ArgName);
			if (!OutFormalManagedArgs.IsEmpty())
			{
				OutFormalManagedArgs += TEXT(", ");
			}
			// TODO: managed types can be more precise than interop types, because we can use the
			//       exported managed wrapper classes!
			OutFormalManagedArgs += FString::Printf(TEXT("%s %s"), *ArgType, *ArgName);
			if (!OutActualManagedArgs.IsEmpty())
			{
				OutActualManagedArgs += TEXT(", ");
			}
			OutActualManagedArgs += ArgName;
		}
	}
}

FString FKlawrCodeGenerator::EmitUnmanagedFunctionPointerAttribute() const
{
	return ManagedIndent + TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl)]\r\n");
}

FString FKlawrCodeGenerator::GenerateManagedWrapperFunction(
	const UClass* Class, const UFunction* Function, const UClass* FuncSuper
)
{
	FString FormalInteropArgs, ActualInteropArgs, FormalManagedArgs, ActualManagedArgs;
	FString ReturnValueTypeName;
	GenerateManagedWrapperArgsAndReturnType(
		Function, FormalInteropArgs, ActualInteropArgs, FormalManagedArgs, ActualManagedArgs, 
		ReturnValueTypeName
	);
	bool bHasReturnValue = (ReturnValueTypeName.Compare(TEXT("void")) != 0);
	// declare a managed delegate type matching the type of the native wrapper function
	FString GeneratedGlue = EmitUnmanagedFunctionPointerAttribute();
	const FString DelegateTypeName = GenerateDelegateTypeName(Function->GetName(), bHasReturnValue);
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("private delegate %s %s(%s);\r\n"),
		*ReturnValueTypeName, *DelegateTypeName, *FormalInteropArgs
	);
	// declare a delegate instance that will be bound to the native wrapper function
	const FString DelegateName = GenerateDelegateName(Function->GetName());
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("private static %s %s;\r\n"),
		*DelegateTypeName, *DelegateName
	);
	// define a managed method that calls the native wrapper function through the delegate 
	// declared above
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("public %s %s(%s)\r\n"),
		*ReturnValueTypeName, *Function->GetName(), *FormalManagedArgs
	);
	GeneratedGlue += ManagedIndent + TEXT("{\r\n");
	IndentManagedCode();
	// call the delegate bound to the native wrapper function
	if (!FuncSuper)
	{
		if (ReturnValueTypeName.Compare(TEXT("void")) == 0)
		{
			GeneratedGlue += ManagedIndent + FString::Printf(
				TEXT("%s(%s);\r\n"), *DelegateName, *ActualInteropArgs
			);
		}
		else // method has a return value
		{
			// TODO: convert return type
			GeneratedGlue += ManagedIndent + FString::Printf(
				TEXT("return %s(%s);\r\n"), *DelegateName, *ActualInteropArgs
			);
		}
	}
	else // the function is actually implemented in a base class, so call the base class version
	{
		GeneratedGlue += ManagedIndent + FString::Printf(
			TEXT("return base.%s(%s);\r\n"), *Function->GetName(), *ActualManagedArgs
		);
	}
	UnindentManagedCode();
	GeneratedGlue += ManagedIndent + TEXT("}\r\n\r\n");

	return GeneratedGlue;
}

void FKlawrCodeGenerator::ExportFunction(
	const FString& ClassNameCPP, const UClass* Class, UFunction* Function,
	FString& NativeGlueCode, FString& ManagedGlueCode
)
{
	// FIXME: Do we really need to export a wrapper that calls the base class function?
	//        In theory that shouldn't be necessary because the inheritance hierarchy is mirrored
	//        in the managed wrapper classes, though is it possible for a subclass to be exported
	//        while its base class isn't? Need to experiment!
	UClass* FuncSuper = nullptr;
	if (Function->GetOwnerClass() != Class)
	{
		// find the base definition of the function
		if (Super::ExportedClasses.Contains(Function->GetOwnerClass()->GetFName()))
		{
			FuncSuper = Function->GetOwnerClass();
		}
	}

	NativeGlueCode += GenerateNativeWrapperFunction(Class, Function, FuncSuper);
	ManagedGlueCode += GenerateManagedWrapperFunction(Class, Function, FuncSuper);
}

bool FKlawrCodeGenerator::IsPropertyTypeSupported(UProperty* Property) const
{
	bool bSupported = true;
	if (Property->IsA(UStructProperty::StaticClass()))
	{
		auto StructProp = CastChecked<UStructProperty>(Property);
		bSupported = IsStructPropertyTypeSupported(StructProp);
	}
	else if (!Property->IsA(UIntProperty::StaticClass()) &&
		!Property->IsA(UFloatProperty::StaticClass()) &&
		!Property->IsA(UStrProperty::StaticClass()) &&
		!Property->IsA(UNameProperty::StaticClass()) &&
		!Property->IsA(UBoolProperty::StaticClass()) &&
		!Property->IsA(UObjectPropertyBase::StaticClass()) &&
		!Property->IsA(UClassProperty::StaticClass()))
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
	if (IsPropertyTypePointer(Property))
	{
		return TEXT("void*");
	}
	else if (Property->IsA(UStrProperty::StaticClass()) || Property->IsA(UNameProperty::StaticClass()))
	{
		return TEXT("const TCHAR*");
	}
	else
	{
		return Super::GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue);
	}
}

// FIXME: this method should be const, as should the argument, but Super::GetPropertyTypeCPP() isn't!
FString FKlawrCodeGenerator::GetPropertyInteropType(UProperty* Property)
{
	if (IsPropertyTypePointer(Property))
	{
		return TEXT("IntPtr");
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

bool FKlawrCodeGenerator::CanExportProperty(const FString& ClassNameCPP, UClass* Class, UProperty* Property)
{
	// only editable properties can be exported
	if (!(Property->PropertyFlags & CPF_Edit))
	{
		return false;
	}
	return IsPropertyTypeSupported(Property);
}

FString FKlawrCodeGenerator::GenerateNativePropertyGetterWrapper(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property, UClass* PropertySuper
)
{
	// define a native getter wrapper function that will be bound to a managed delegate
	FString PropertyNativeTypeName = GetPropertyNativeType(Property);
	FString GetterName = FString::Printf(
		TEXT("%s_Get_%s"), *Class->GetName(), *Property->GetName()
	);
	FString GeneratedGlue = NativeIndent + FString::Printf(
		TEXT("%s %s(void* self)\r\n"), *PropertyNativeTypeName, *GetterName
	);
	GeneratedGlue += NativeIndent + TEXT("{\r\n");
	IndentNativeCode();
	if (!PropertySuper)
	{
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		GeneratedGlue += NativeIndent + TEXT("UObject* Obj = (UObject*)self;\r\n");
		GeneratedGlue += NativeIndent + FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));\r\n"),
			*ClassNameCPP, *Property->GetName()
		);
		GeneratedGlue += NativeIndent + FString::Printf(
			TEXT("%s PropertyValue;\r\n"), *GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue)
		);
		GeneratedGlue += NativeIndent + TEXT("Property->CopyCompleteValue(&PropertyValue, Property->ContainerPtrToValuePtr<void>(Obj));\r\n");
		GeneratedGlue += GenerateNativeReturnValueHandler(Property, TEXT("PropertyValue"));
		
	}
	else // the property is actually implemented in a base class, so call the base class getter
	{
		// FIXME: Do we really need to export a wrapper that calls the base getter/setter?
		GeneratedGlue += NativeIndent + FString::Printf(
			TEXT("return %s_Get_%s(self);\r\n"),
			*PropertySuper->GetName(), *Property->GetName()
		);
	}
	UnindentNativeCode();
	GeneratedGlue += NativeIndent + TEXT("}\r\n\r\n");
	
	return GeneratedGlue;
}

FString FKlawrCodeGenerator::GenerateNativePropertySetterWrapper(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property, UClass* PropertySuper
)
{
	// define a native setter wrapper function that will be bound to a managed delegate
	FString PropertyNativeTypeName = GetPropertyNativeType(Property);
	FString SetterName = FString::Printf(
		TEXT("%s_Set_%s"), *Class->GetName(), *Property->GetName()
	);
	FString GeneratedGlue = NativeIndent + FString::Printf(
		TEXT("void %s(void* self, %s %s)\r\n"), 
		*SetterName, *PropertyNativeTypeName, *Property->GetName()
	);
	GeneratedGlue += NativeIndent + TEXT("{\r\n");
	IndentNativeCode();
	if (!PropertySuper)
	{
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		GeneratedGlue += NativeIndent + TEXT("UObject* Obj = (UObject*)self;\r\n");
		GeneratedGlue += NativeIndent + FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));\r\n"), 
			*ClassNameCPP, *Property->GetName()
		);
		GeneratedGlue += NativeIndent + FString::Printf(
			TEXT("%s PropertyValue = %s;\r\n"), 
			*GetPropertyTypeCPP(Property, CPPF_ArgumentOrReturnValue), 
			*InitializeFunctionDispatchParam(NULL, Property, 0)
		);
		GeneratedGlue += NativeIndent + TEXT("Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Obj), &PropertyValue);\r\n");
	}
	else // the property is actually implemented in a base class, so call the base class setter
	{
		// FIXME: Do we really need to export a wrapper that calls the base getter/setter?
		GeneratedGlue += NativeIndent + FString::Printf(
			TEXT("%s_Set_%s(self, %s);\r\n"),
			*PropertySuper->GetName(), *Property->GetName(), *Property->GetName()
		);
	}
	UnindentNativeCode();
	GeneratedGlue += NativeIndent + TEXT("}\r\n\r\n");

	return GeneratedGlue;
}

FString FKlawrCodeGenerator::GenerateManagedPropertyWrapper(
	UClass* Class, UProperty* Property
)
{
	const FString GetterName = FString::Printf(TEXT("Get_%s"), *Property->GetName());
	const FString SetterName = FString::Printf(TEXT("Set_%s"), *Property->GetName());
	
	FExportedProperty PropInfo;
	PropInfo.GetterDelegateName = GenerateDelegateName(GetterName);
	PropInfo.GetterDelegateTypeName = GenerateDelegateTypeName(GetterName, true);
	PropInfo.SetterDelegateName = GenerateDelegateName(SetterName);
	PropInfo.SetterDelegateTypeName = GenerateDelegateTypeName(SetterName, false);
	ClassExportedProperties.FindOrAdd(Class).Add(PropInfo);
	
	FString PropertyTypeName = GetPropertyInteropType(Property);
	// declare managed delegate types matching the types of the native wrapper functions
	FString GeneratedGlue = EmitUnmanagedFunctionPointerAttribute();
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("private delegate %s %s(IntPtr self);\r\n"),
		*PropertyTypeName, *PropInfo.GetterDelegateTypeName
	);
	GeneratedGlue += EmitUnmanagedFunctionPointerAttribute();
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("private delegate void %s(IntPtr self, %s %s);\r\n"),
		*PropInfo.SetterDelegateTypeName, *PropertyTypeName, *Property->GetName()
	);
	// declare delegate instances that will be bound to the native wrapper functions
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("private static %s %s;\r\n"),
		*PropInfo.GetterDelegateTypeName, *PropInfo.GetterDelegateName
	);
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("private static %s %s;\r\n"),
		*PropInfo.SetterDelegateTypeName, *PropInfo.SetterDelegateName
	);
	// define a managed property that calls the native wrapper functions through the delegates 
	// declared above
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("public %s %s\r\n"),
		*PropertyTypeName, *Property->GetName()
	);
	GeneratedGlue += ManagedIndent + TEXT("{\r\n");
	IndentManagedCode();
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("get { return %s(_nativeObject); }\r\n"), *PropInfo.GetterDelegateName
	);
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("set { %s(_nativeObject, value); }\r\n"), *PropInfo.SetterDelegateName
	);
	UnindentManagedCode();
	GeneratedGlue += ManagedIndent + TEXT("}\r\n\r\n");
	
	return GeneratedGlue;
}

void FKlawrCodeGenerator::ExportProperty(
	const FString& ClassNameCPP, UClass* Class, UProperty* Property,
	FString& NativeGlueCode, FString& ManagedGlueCode
)
{
	UClass* PropertySuper = nullptr;
	if (Property->GetOwnerClass() != Class)
	{
		// find the base class where this property was defined
		if (ExportedClasses.Contains(Property->GetOwnerClass()->GetFName()))
		{
			PropertySuper = Property->GetOwnerClass();
		}
	}

	NativeGlueCode += GenerateNativePropertyGetterWrapper(ClassNameCPP, Class, Property, PropertySuper);
	NativeGlueCode += GenerateNativePropertySetterWrapper(ClassNameCPP, Class, Property, PropertySuper);
	if (!PropertySuper)
	{
		ManagedGlueCode += GenerateManagedPropertyWrapper(Class, Property);
	}
}

FString FKlawrCodeGenerator::GenerateNativeGlueCodeHeader(const UClass* Class) const
{
	FString GeneratedGlue(TEXT("#pragma once\r\n\r\n"));
	GeneratedGlue += TEXT("namespace KlawrNativeGlue {\r\n\r\n");

	return GeneratedGlue;
}

FString FKlawrCodeGenerator::GenerateNativeGlueCodeFooter(const UClass* Class) const
{
	FString GeneratedGlue;
	// TODO: pass the pointers to all the native wrapper functions to the CLR host
	GeneratedGlue += TEXT("} // namespace KlawrNativeGlue\r\n");

	return GeneratedGlue;
}

FString FKlawrCodeGenerator::GenerateManagedStaticConstructor(const UClass* Class)
{
	FString GeneratedGlue = ManagedIndent + FString::Printf(
		TEXT("static %s%s()\r\n"), Class->GetPrefixCPP(), *Class->GetName()
	);
	GeneratedGlue += ManagedIndent + TEXT("{\r\n");
	IndentManagedCode();
	// bind managed delegates to pointers to native wrapper functions
	GeneratedGlue += ManagedIndent + TEXT("var manager = AppDomain.CurrentDomain.Manager as ICustomAppDomainManager;\r\n");
	GeneratedGlue += ManagedIndent + TEXT("var nativeFuncPtrs = manager.GetNativeFunctionPointers();\r\n");
	
	int32 FunctionIdx = 0;
	auto ExportedProperties = ClassExportedProperties.Find(Class);
	if (ExportedProperties)
	{
		for (const FExportedProperty& PropInfo : *ExportedProperties)
		{
			GeneratedGlue += ManagedIndent + FString::Printf(
				TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;\r\n"),
				*PropInfo.GetterDelegateName, FunctionIdx, *PropInfo.GetterDelegateTypeName,
				*PropInfo.GetterDelegateTypeName
			);
			++FunctionIdx;
			GeneratedGlue += ManagedIndent + FString::Printf(
				TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;\r\n"),
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
			const FString DelegateTypeName = GenerateDelegateTypeName(
				FuncInfo.Function->GetName(), FuncInfo.bHasReturnValue
			);
			GeneratedGlue += ManagedIndent + FString::Printf(
				TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;\r\n"),
				*GenerateDelegateName(FuncInfo.Function->GetName()),
				FunctionIdx, *DelegateTypeName, *DelegateTypeName
			);
			++FunctionIdx;
		}
	}
	
	UnindentManagedCode();
	GeneratedGlue += ManagedIndent + TEXT("}\r\n\r\n");

	return GeneratedGlue;
}

FString FKlawrCodeGenerator::GenerateManagedGlueCodeHeader(const UClass* Class)
{
	// declare namespace
	FString GeneratedGlue(TEXT("namespace Klawr.UnrealEngine\r\n{\r\n"));
	IndentManagedCode();
	// declare class
	GeneratedGlue += ManagedIndent + FString::Printf(
		TEXT("public class %s%s"), Class->GetPrefixCPP(), *Class->GetName()
	);
	// if the base class was exported mirror the inheritance in the managed wrapper class
	UClass* SuperClass = Class->GetSuperClass();
	if (SuperClass && ExportedClasses.Contains(SuperClass->GetFName()))
	{
		GeneratedGlue += FString::Printf(
			TEXT(" : %s%s"), SuperClass->GetPrefixCPP(), *SuperClass->GetName()
		);
	}
	GeneratedGlue += TEXT("\r\n");
	GeneratedGlue += ManagedIndent + TEXT("{\r\n");
	IndentManagedCode();
	// declare the wrapped native object
	GeneratedGlue += ManagedIndent + TEXT("private IntPtr _nativeObject;\r\n\r\n");

	return GeneratedGlue;
}

FString FKlawrCodeGenerator::GenerateManagedGlueCodeFooter(const UClass* Class)
{
	// define static constructor
	FString GeneratedGlue = GenerateManagedStaticConstructor(Class);
	// close the class
	UnindentManagedCode();
	GeneratedGlue += ManagedIndent + TEXT("}\r\n");
	// close the namespace
	UnindentManagedCode();
	GeneratedGlue += TEXT("}\r\n");

	return GeneratedGlue;
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
	AllSourceClassHeaders.Add(SourceHeaderFilename);

	const FString NativeGlueFilename = Super::GetScriptHeaderForClass(Class);
	FString ManagedGlueFilename = NativeGlueFilename;
	ManagedGlueFilename.RemoveFromEnd(TEXT(".h"));
	ManagedGlueFilename.Append(TEXT(".cs"));
	AllScriptHeaders.Add(NativeGlueFilename);

	FString NativeGlueCode = GenerateNativeGlueCodeHeader(Class);
	FString ManagedGlueCode = GenerateManagedGlueCodeHeader(Class);

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

	NativeGlueCode += GenerateNativeGlueCodeFooter(Class);
	ManagedGlueCode += GenerateManagedGlueCodeFooter(Class);

	Super::SaveHeaderIfChanged(NativeGlueFilename, NativeGlueCode);
	Super::SaveHeaderIfChanged(ManagedGlueFilename, ManagedGlueCode);
}

void FKlawrCodeGenerator::GenerateManagedWrapperProject()
{
	const FString ProjectName("UE4Wrapper.csproj");
	const FString ProjectTemplateFilename = 
		FPaths::EnginePluginsDir() / TEXT("Script/ScriptGeneratorPlugin/Resources") / ProjectName;
	const FString ProjectOutputFilename = 
		FPaths::EngineIntermediateDir() / TEXT("ProjectFiles") / ProjectName;

	// load the template .csproj
	pugi::xml_document Project;
	pugi::xml_parse_result result = Project.load_file(
		*ProjectTemplateFilename, pugi::parse_default | pugi::parse_declaration | pugi::parse_comments
	);

	if (!result)
	{
		FError::Throwf(TEXT("Failed to load %s"), *ProjectTemplateFilename);
	}
	else
	{
		// add references to all the generated C# wrapper classes to the .csproj
		auto SourceNode = Project.first_element_by_path(TEXT("/Project/ItemGroup/Compile")).parent();
		if (SourceNode)
		{
			FString ManagedGlueFilename;
			for (const FString& ScriptHeaderFilename : AllScriptHeaders)
			{
				ManagedGlueFilename = ScriptHeaderFilename;
				ManagedGlueFilename.RemoveFromEnd(TEXT(".h"));
				ManagedGlueFilename.Append(TEXT(".cs"));
				FPaths::MakePathRelativeTo(ManagedGlueFilename, *ProjectOutputFilename);
				FPaths::MakePlatformFilename(ManagedGlueFilename);
				SourceNode
					.append_child(TEXT("Compile"))
					.append_attribute(TEXT("Include"))
					.set_value(*ManagedGlueFilename);
			}
		}

		// preserve the BOM and line endings of the template .csproj when writing out the new file
		unsigned int OutputFlags =
			pugi::format_default | pugi::format_write_bom | pugi::format_save_file_text;

		if (!Project.save_file(*ProjectOutputFilename, TEXT("  "), OutputFlags))
		{
			FError::Throwf(TEXT("Failed to save %s"), *ProjectOutputFilename);
		}
	}
}

void FKlawrCodeGenerator::FinishExport()
{
	GlueAllGeneratedFiles();
	Super::RenameTempFiles();
	GenerateManagedWrapperProject();
}

void FKlawrCodeGenerator::GlueAllGeneratedFiles()
{
	// generate the file that will be included by ScriptPlugin.cpp
	FString GlueFilename = GeneratedCodePath / TEXT("GeneratedScriptLibraries.inl");
	FString GeneratedGlue;

	// include all source header files
	for (const auto& HeaderFilename : AllSourceClassHeaders)
	{
		// re-base to make sure we're including the right files on a remote machine
		FString NewFilename(Super::RebaseToBuildPath(HeaderFilename));
		GeneratedGlue += FString::Printf(TEXT("#include \"%s\"\r\n"), *NewFilename);
	}

	// include all script glue headers
	for (const auto& HeaderFilename : AllScriptHeaders)
	{
		FString NewFilename(FPaths::GetCleanFilename(HeaderFilename));
		GeneratedGlue += FString::Printf(TEXT("#include \"%s\"\r\n"), *NewFilename);
	}
	
	Super::SaveHeaderIfChanged(GlueFilename, GeneratedGlue);
}

void FKlawrCodeGenerator::IndentManagedCode()
{
	ManagedIndent += ManagedTab;
}

void FKlawrCodeGenerator::UnindentManagedCode()
{
	ManagedIndent.RemoveFromEnd(ManagedTab);
}

void FKlawrCodeGenerator::IndentNativeCode()
{
	NativeIndent += NativeTab;
}

void FKlawrCodeGenerator::UnindentNativeCode()
{
	NativeIndent.RemoveFromEnd(NativeTab);
}
