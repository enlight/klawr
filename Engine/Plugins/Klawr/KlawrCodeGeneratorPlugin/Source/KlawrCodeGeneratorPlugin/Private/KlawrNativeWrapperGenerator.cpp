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
#include "KlawrNativeWrapperGenerator.h"
#include "KlawrCodeFormatter.h"
#include "KlawrCodeGenerator.h"

namespace Klawr {

FNativeWrapperGenerator::FNativeWrapperGenerator(const UClass* Class, FCodeFormatter& CodeFormatter)
	: GeneratedGlue(CodeFormatter)
{
	Class->GetName(FriendlyClassName);
	NativeClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *FriendlyClassName);
}

void FNativeWrapperGenerator::GenerateHeader()
{
	GeneratedGlue
		<< TEXT("#pragma once") LINE_TERMINATOR
		<< TEXT("namespace Klawr {")
		<< TEXT("namespace NativeGlue {")
		<< FCodeFormatter::LineTerminator()
		<< FString::Printf(TEXT("struct %s"), *FriendlyClassName)
		<< FCodeFormatter::OpenBrace();
}

void FNativeWrapperGenerator::GenerateFooter()
{
	GeneratedGlue 
		<< FCodeFormatter::CloseBrace()
		<< TEXT(";");

	if (ExportedProperties.Num() || ExportedFunctions.Num())
	{
		// generate an array of function pointers to all the native wrapper functions
		FString arrayName = FString::Printf(TEXT("%s_WrapperFunctions"), *FriendlyClassName);
		GeneratedGlue
			<< FString::Printf(TEXT("static void* %s[] ="), *arrayName)
			<< FCodeFormatter::OpenBrace();

		for (const auto& exportedProperty : ExportedProperties)
		{
			if (!exportedProperty.GetterWrapperFunctionName.IsEmpty())
			{
				GeneratedGlue << exportedProperty.GetterWrapperFunctionName + TEXT(",");
			}
			if (!exportedProperty.SetterWrapperFunctionName.IsEmpty())
			{
				GeneratedGlue << exportedProperty.SetterWrapperFunctionName + TEXT(",");
			}
		}
		
		for (const auto& exportedFunction : ExportedFunctions)
		{
			GeneratedGlue << exportedFunction.WrapperFunctionName + TEXT(",");
		}
		
		GeneratedGlue
			<< FCodeFormatter::CloseBrace()
			<< TEXT(";");
	}

	GeneratedGlue << TEXT("}} // namespace Klawr::NativeGlue");
}

void FNativeWrapperGenerator::GenerateFunctionWrapper(const UFunction* Function)
{
	FString formalArgs, actualArgs;
	UProperty* returnValue = GetWrapperArgsAndReturnType(
		Function, formalArgs, actualArgs
	);
	FString returnValueTypeName(TEXT("void"));
	if (returnValue)
	{
		returnValueTypeName = GetPropertyType(returnValue);
	}
	// define a native wrapper function that will be bound to a managed delegate
	GeneratedGlue << FString::Printf(
		TEXT("static %s %s(%s)"),
		*returnValueTypeName, *Function->GetName(), *formalArgs
	);
	GeneratedGlue << FCodeFormatter::OpenBrace();

	// call the wrapped UFunction
	// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
	//        regular function argument names.
	GeneratedGlue << TEXT("UObject* Obj = static_cast<UObject*>(self);");
	GenerateFunctionDispatch(Function);

	// for non-const reference parameters to the UFunction copy their values from the 
	// FDispatchParams struct
	for (TFieldIterator<UProperty> paramIt(Function); paramIt; ++paramIt)
	{
		UProperty* param = *paramIt;
		if (!param->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm) &&
			param->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
		{
			if (param->IsA<UNameProperty>())
			{
				GeneratedGlue << FString::Printf(
					TEXT("*%s = NameToScriptName(Params.%s);"), *param->GetName(), *param->GetName()
				);
			}
			else
			{
				GeneratedGlue << FString::Printf(
					TEXT("*%s = Params.%s;"), *param->GetName(), *param->GetName()
				);
			}
		}
	}

	if (returnValue)
	{
		GenerateReturnValueHandler(
			returnValue, FString::Printf(TEXT("Params.%s"), *returnValue->GetName())
		);
	}

	GeneratedGlue
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();

	FExportedFunction funcInfo;
	funcInfo.WrapperFunctionName = FString::Printf(
		TEXT("%s::%s"), *FriendlyClassName, *Function->GetName()
	);
	ExportedFunctions.Add(funcInfo);
}

void FNativeWrapperGenerator::GeneratePropertyWrapper(const UProperty* prop)
{
	FExportedProperty exportedProperty;
	auto arrayProp = Cast<UArrayProperty>(prop);
	if (arrayProp)
	{
		exportedProperty.GetterWrapperFunctionName = GenerateArrayPropertyGetterWrapper(arrayProp);
		exportedProperty.SetterWrapperFunctionName.Empty();
	}
	else
	{
		exportedProperty.GetterWrapperFunctionName = GeneratePropertyGetterWrapper(prop);
		exportedProperty.SetterWrapperFunctionName = GeneratePropertySetterWrapper(prop);
	}
	ExportedProperties.Add(exportedProperty);
}

FString FNativeWrapperGenerator::GetPropertyType(const UProperty* Property)
{
	FString typeName;

	if (Property->IsA<UObjectPropertyBase>())
	{
		return TEXT("void*");
	}
	else if (Property->IsA<UBoolProperty>())
	{
		// the managed wrapper functions marshal C# bool to uint8, so for the sake of consistency
		// use uint8 instead of C++ bool in the native wrapper functions
		typeName = TEXT("uint8");
	}
	else if (Property->IsA<UStrProperty>())
	{
		return TEXT("const TCHAR*");
	}
	else if (Property->IsA<UNameProperty>())
	{
		// FName arguments will get marshaled as FScriptName
		typeName = TEXT("FScriptName");
	}
	else
	{
		typeName = FCodeGenerator::GetPropertyCPPType(Property);
	}
	// TODO: handle constness?
	// return by reference must be converted to return by value because 
	// FDispatchParams::ReturnValue is only valid within the scope of a native wrapper function
	if (!(Property->GetPropertyFlags() & CPF_ReturnParm) &&
		Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
	{
		typeName += TEXT("*");
	}

	return typeName;
}

UProperty* FNativeWrapperGenerator::GetWrapperArgsAndReturnType(
	const UFunction* Function, FString& OutFormalArgs, FString& OutActualArgs
)
{
	OutFormalArgs = TEXT("void* self");
	OutActualArgs = TEXT("self");
	UProperty* returnValue = nullptr;

	for (TFieldIterator<UProperty> paramIt(Function); paramIt; ++paramIt)
	{
		UProperty* param = *paramIt;
		if (param->GetPropertyFlags() & CPF_ReturnParm)
		{
			returnValue = param;
		}
		else
		{
			OutFormalArgs += FString::Printf(
				TEXT(", %s %s"), *GetPropertyType(param), *param->GetName()
			);
			OutActualArgs += FString::Printf(TEXT(", %s"), *param->GetName());
		}
	}

	return returnValue;
}

FString FNativeWrapperGenerator::GetFunctionDispatchParamInitializer(const UProperty* Param)
{
	if (!(Param->GetPropertyFlags() & CPF_ReturnParm))
	{
		FString paramName = Param->GetName();
		FString initializer;

		if (Param->IsA<UClassProperty>())
		{
			initializer = FString::Printf(TEXT("static_cast<UClass*>(%s)"), *paramName);
		}
		else if (Param->IsA<UObjectPropertyBase>())
		{
			initializer = FString::Printf(
				TEXT("static_cast<%s>(%s)"), *FCodeGenerator::GetPropertyCPPType(Param), *paramName
			);
		}
		else if (Param->IsA<UStrProperty>())
		{
			initializer = paramName;
		}
		else
		{
			// reference params are passed into a native wrapper function via a pointer,
			// so dereference the pointer so that the value can be copied into FDispatchParams
			if (Param->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
			{
				paramName = FString::Printf(TEXT("(*%s)"), *paramName);
			}

			if (Param->IsA<UNameProperty>())
			{
				initializer = FString::Printf(TEXT("ScriptNameToName(%s)"), *paramName);
			}
			else if (Param->IsA<UIntProperty>() || Param->IsA<UFloatProperty>())
			{
				initializer = paramName;
			}
			else if (Param->IsA<UBoolProperty>())
			{
				if (CastChecked<UBoolProperty>(Param)->IsNativeBool())
				{
					// explicitly convert uint8 to bool
					initializer = FString::Printf(TEXT("!!%s"), *paramName);
				}
				else
				{
					initializer = paramName;
				}
			}
			else if (Param->IsA<UStructProperty>())
			{
				auto structProp = CastChecked<UStructProperty>(Param);
				if (FCodeGenerator::IsStructPropertyTypeSupported(structProp))
				{
					initializer = paramName;
				}
				else
				{
					FError::Throwf(
						TEXT("Unsupported function param struct type: %s"),
						*structProp->Struct->GetName()
					);
				}
			}
		}
		
		if (initializer.IsEmpty())
		{
			FError::Throwf(
				TEXT("Unsupported function param type: %s"), *Param->GetClass()->GetName()
			);
		}
		return initializer;
	}
	else // Param is actually the return value
	{
		if (Param->IsA<UObjectPropertyBase>())
		{
			return TEXT("nullptr");
		}
		else
		{
			return FString::Printf(TEXT("%s()"), *FCodeGenerator::GetPropertyCPPType(Param));
		}
	}
}

void FNativeWrapperGenerator::GenerateReturnValueHandler(
	const UProperty* ReturnValue, const FString& ReturnValueName
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
							TEXT("FObjectReferencer::AddObjectRef(%s);"), *ReturnValueName
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
				TEXT("return NameToScriptName(%s);"), *ReturnValueName
			);
		}
		else if (ReturnValue->IsA<UStructProperty>())
		{
			auto structProp = CastChecked<UStructProperty>(ReturnValue);
			if (FCodeGenerator::IsStructPropertyTypeSupported(structProp))
			{
				GeneratedGlue << FString::Printf(TEXT("return %s;"), *ReturnValueName);
			}
			else
			{
				FError::Throwf(
					TEXT("Unsupported function return value struct type: %s"), 
					*structProp->Struct->GetName()
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

void FNativeWrapperGenerator::GenerateFunctionDispatch(const UFunction* Function)
{
	const bool bHasParamsOrReturnValue = (Function->Children != NULL);
	if (bHasParamsOrReturnValue)
	{
		GeneratedGlue
			<< TEXT("struct FDispatchParams")
			<< FCodeFormatter::OpenBrace();

		for (TFieldIterator<UProperty> paramIt(Function); paramIt; ++paramIt)
		{
			UProperty* param = *paramIt;
			GeneratedGlue << FString::Printf(
				TEXT("%s %s;"), *FCodeGenerator::GetPropertyCPPType(param), *param->GetName()
			);
		}

		GeneratedGlue
			<< FCodeFormatter::CloseBrace()
			<< TEXT("Params =")
			<< FCodeFormatter::OpenBrace();

		int32 paramIndex = 0;
		for (TFieldIterator<UProperty> paramIt(Function); paramIt; ++paramIt, ++paramIndex)
		{
			GeneratedGlue << FString::Printf(
				TEXT("%s,"), *GetFunctionDispatchParamInitializer(*paramIt)
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

FString FNativeWrapperGenerator::GeneratePropertyGetterWrapper(const UProperty* Property)
{
	// define a native getter wrapper function that will be bound to a managed delegate
	const FString propertyTypeName = GetPropertyType(Property);
	const FString getterName = FString::Printf(TEXT("Get_%s"), *Property->GetName());
	
	GeneratedGlue 
		<< FString::Printf(TEXT("static %s %s(void* self)"), *propertyTypeName, *getterName)
		<< FCodeFormatter::OpenBrace()
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		<< TEXT("UObject* Obj = static_cast<UObject*>(self);")
		<< FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));"),
			*NativeClassName, *Property->GetName()
		)
		<< FString::Printf(
			TEXT("%s PropertyValue;\r\n"), *FCodeGenerator::GetPropertyCPPType(Property)
		)
		<< TEXT("Property->CopyCompleteValue(&PropertyValue, Property->ContainerPtrToValuePtr<void>(Obj));");
			
	GenerateReturnValueHandler(Property, TEXT("PropertyValue"));
	
	GeneratedGlue 
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();

	return FString::Printf(TEXT("%s::%s"), *FriendlyClassName, *getterName);
}

FString FNativeWrapperGenerator::GeneratePropertySetterWrapper(const UProperty* Property)
{
	// define a native setter wrapper function that will be bound to a managed delegate
	const FString propertyTypeName = GetPropertyType(Property);
	const FString setterName = FString::Printf(TEXT("Set_%s"), *Property->GetName());

	GeneratedGlue 
		<< FString::Printf(
			TEXT("static void %s(void* self, %s %s)"), 
			*setterName, *propertyTypeName, *Property->GetName()
		)
		<< FCodeFormatter::OpenBrace()
		// FIXME: "Obj" isn't very unique, should pick a name that isn't likely to conflict with
		//        regular function argument names.
		<< TEXT("UObject* Obj = static_cast<UObject*>(self);")
		<< FString::Printf(
			TEXT("static UProperty* Property = FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\"));"), 
			*NativeClassName, *Property->GetName()
		)
		<< FString::Printf(
			TEXT("%s PropertyValue = %s;"),
			*FCodeGenerator::GetPropertyCPPType(Property), 
			*GetFunctionDispatchParamInitializer(Property)
		)
		<< TEXT("Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Obj), &PropertyValue);")
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();

	return FString::Printf(TEXT("%s::%s"), *FriendlyClassName, *setterName);
}

FString FNativeWrapperGenerator::GenerateArrayPropertyGetterWrapper(const UArrayProperty* arrayProp)
{
	// define a native getter wrapper function that will be bound to a managed delegate
	const FString propertyTypeName = GetPropertyType(arrayProp);
	const FString getterName = FString::Printf(TEXT("Get_%s"), *arrayProp->GetName());

	GeneratedGlue
		<< FString::Printf(TEXT("static FArrayHelper* %s(%s* self)"), *getterName, *NativeClassName)
		<< FCodeFormatter::OpenBrace()
			<< FString::Printf(
				TEXT("static UArrayProperty* prop = Cast<UArrayProperty>(FindScriptPropertyHelper(%s::StaticClass(), TEXT(\"%s\")));"),
				*NativeClassName, *arrayProp->GetName()
			)
			<< FString::Printf(
				TEXT("return new TArrayHelper<%s>(&self->%s, prop);"), 
				*FCodeGenerator::GetPropertyCPPType(arrayProp->Inner), *arrayProp->GetName()
			)
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();

	return FString::Printf(TEXT("%s::%s"), *FriendlyClassName, *getterName);
}

} // namespace Klawr
