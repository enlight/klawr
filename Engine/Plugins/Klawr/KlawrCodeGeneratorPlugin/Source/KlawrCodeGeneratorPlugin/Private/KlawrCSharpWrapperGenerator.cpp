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
#include "KlawrCSharpWrapperGenerator.h"
#include "KlawrCodeFormatter.h"
#include "KlawrCodeGenerator.h"

namespace Klawr {

const FString FCSharpWrapperGenerator::UnmanagedFunctionPointerAttribute = 
#ifdef UNICODE
	TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]");
#else
	TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]");
#endif // UNICODE

const FString FCSharpWrapperGenerator::MarshalReturnedBoolAsUint8Attribute =
	TEXT("[return: MarshalAs(UnmanagedType.U1)]");

const FString FCSharpWrapperGenerator::MarshalBoolParameterAsUint8Attribute =
	TEXT("[MarshalAs(UnmanagedType.U1)]");

const FString FCSharpWrapperGenerator::NativeThisPointer = TEXT("(UObjectHandle)this");

FCSharpWrapperGenerator::FCSharpWrapperGenerator(
	const UClass* Class, const UClass* InWrapperSuperClass, FCodeFormatter& CodeFormatter)
	: WrapperSuperClass(InWrapperSuperClass)
	, GeneratedGlue(CodeFormatter)
{
	Class->GetName(FriendlyClassName);
	NativeClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *FriendlyClassName);
	bShouldGenerateManagedWrapper = ShouldGenerateManagedWrapper(Class);
	bShouldGenerateScriptObjectClass = ShouldGenerateScriptObjectClass(Class);
}

const UClass* FCSharpWrapperGenerator::GetWrapperSuperClass(
	const UClass* Class, const TArray<const UClass*>& ExportedClasses
)
{
	// mirror the inheritance in the managed wrapper class while skipping base classes that haven't
	// been exported (this may change in the future)
	UClass* superClass = nullptr;
	// find the top-most exported ancestor in the inheritance hierarchy
	for (UClass* currentAncestor = Class->GetSuperClass(); currentAncestor;)
	{
		if (ExportedClasses.Contains(currentAncestor))
		{
			superClass = currentAncestor;
		}
		currentAncestor = currentAncestor->GetSuperClass();
	}
	return superClass;
}

void FCSharpWrapperGenerator::GenerateHeader()
{
	FString classDecl = FString::Printf(TEXT("public class %s"), *NativeClassName);
		
	if (WrapperSuperClass)
	{
		classDecl += FString::Printf(
			TEXT(" : %s%s"), WrapperSuperClass->GetPrefixCPP(), *WrapperSuperClass->GetName()
		);
	}
		
	GeneratedGlue 
		// usings
		<< TEXT("using System;")
		<< TEXT("using System.Runtime.InteropServices;")
		<< TEXT("using Klawr.ClrHost.Interfaces;")
		<< TEXT("using Klawr.ClrHost.Managed;")
		<< TEXT("using Klawr.ClrHost.Managed.SafeHandles;")
		<< FCodeFormatter::LineTerminator()

		// declare namespace
		<< TEXT("namespace Klawr.UnrealEngine") 
		<< FCodeFormatter::OpenBrace();

	if (bShouldGenerateManagedWrapper)
	{ 
		GeneratedGlue
			// declare class
			<< classDecl 
			<< FCodeFormatter::OpenBrace()

				// constructor
				<< FString::Printf(
					TEXT("public %s(UObjectHandle nativeObject) : base(nativeObject)"), 
					*NativeClassName
				)
				<< FCodeFormatter::OpenBrace()
				<< FCodeFormatter::CloseBrace()
	
				// StaticClass()
				<< TEXT("public new static UClass StaticClass()")
				<< FCodeFormatter::OpenBrace()
					<< FString::Printf(TEXT("return (UClass)typeof(%s);"), *NativeClassName)
				<< FCodeFormatter::CloseBrace()
			
			<< FCodeFormatter::LineTerminator();
	}
}

void FCSharpWrapperGenerator::GenerateFooter()
{
	if (bShouldGenerateManagedWrapper)
	{
		// define static constructor
		GenerateManagedStaticConstructor();
		// close the class
		GeneratedGlue
			<< FCodeFormatter::CloseBrace()
			<< FCodeFormatter::LineTerminator();
	}

	if (bShouldGenerateScriptObjectClass)
	{
		GenerateManagedScriptObjectClass();
	}

	// close the namespace
	GeneratedGlue << FCodeFormatter::CloseBrace();
}

void FCSharpWrapperGenerator::GenerateFunctionWrapper(const UFunction* Function)
{
	FString formalInteropArgs, actualInteropArgs, formalManagedArgs, actualManagedArgs;
	UProperty* returnValue = GetWrapperArgsAndReturnType(
		Function, formalInteropArgs, actualInteropArgs, formalManagedArgs, actualManagedArgs
	);
	const bool bHasReturnValue = (returnValue != nullptr);
	const bool bReturnsBool = (bHasReturnValue && returnValue->IsA(UBoolProperty::StaticClass()));
	const FString returnValueInteropTypeName = 
		bHasReturnValue ? GetPropertyInteropType(returnValue) : TEXT("void");
	const FString returnValueManagedTypeName =
		bHasReturnValue ? GetPropertyManagedType(returnValue) : TEXT("void");
	const FString delegateTypeName = GetDelegateTypeName(Function->GetName(), bHasReturnValue);
	const FString delegateName = GetDelegateName(Function->GetName());

	GeneratedGlue 
		// declare a managed delegate type matching the type of the native wrapper function
		<< UnmanagedFunctionPointerAttribute
		<< (bReturnsBool ? MarshalReturnedBoolAsUint8Attribute : FString())
		<< FString::Printf(
			TEXT("private delegate %s %s(%s);"),
			*returnValueInteropTypeName, *delegateTypeName, *formalInteropArgs
		)
		// declare a delegate instance that will be bound to the native wrapper function
		<< FString::Printf(
			TEXT("private static %s %s;"),
			*delegateTypeName, *delegateName
		)
		// define a managed method that calls the native wrapper function through the delegate 
		// declared above
		<< FString::Printf(
			TEXT("public %s %s(%s)"),
			*returnValueManagedTypeName, *Function->GetName(), *formalManagedArgs
		)
		<< FCodeFormatter::OpenBrace();

	// call the delegate bound to the native wrapper function
	if (bHasReturnValue)
	{
		GeneratedGlue 
			<< FString::Printf(TEXT("var value = %s(%s);"), *delegateName, *actualInteropArgs)
			<< GetReturnValueHandler(returnValue);
	}
	else
	{
		GeneratedGlue << FString::Printf(TEXT("%s(%s);"), *delegateName, *actualInteropArgs);
	}
		
	GeneratedGlue
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();

	FExportedFunction funcInfo;
	funcInfo.DelegateName = delegateName;
	funcInfo.DelegateTypeName = delegateTypeName;
	ExportedFunctions.Add(funcInfo);
}

void FCSharpWrapperGenerator::GeneratePropertyWrapper(const UProperty* Property)
{
	const FString getterName = FString::Printf(TEXT("Get_%s"), *Property->GetName());
	const FString setterName = FString::Printf(TEXT("Set_%s"), *Property->GetName());
	
	FExportedProperty propertyInfo;
	propertyInfo.GetterDelegateName = GetDelegateName(getterName);
	propertyInfo.GetterDelegateTypeName = GetDelegateTypeName(getterName, true);
	propertyInfo.SetterDelegateName = GetDelegateName(setterName);
	propertyInfo.SetterDelegateTypeName = GetDelegateTypeName(setterName, false);
	ExportedProperties.Add(propertyInfo);
	
	const bool bIsBoolProperty = Property->IsA<UBoolProperty>();
	const FString interopTypeName = GetPropertyInteropType(Property);
	const FString managedTypeName = GetPropertyManagedType(Property);
	FString setterParamType = interopTypeName;
	if (bIsBoolProperty)
	{
		setterParamType = FString::Printf(
			TEXT("%s %s"), *MarshalBoolParameterAsUint8Attribute, *interopTypeName
		);
	}
	FString getterValue(TEXT("value"));
	FString setterValue(TEXT("value"));
	if (Property->IsA<UObjectProperty>())
	{
		if (Property->IsA<UClassProperty>())
		{
			getterValue = TEXT("(UClass)value");
		}
		else
		{
			getterValue = FString::Printf(TEXT("new %s(value)"), *managedTypeName);
		}
		setterValue = TEXT("(UObjectHandle)value");
	}
	
	GeneratedGlue
		// declare getter delegate type
		<< UnmanagedFunctionPointerAttribute
		<< (bIsBoolProperty ? MarshalReturnedBoolAsUint8Attribute : FString())
		<< FString::Printf(
			TEXT("private delegate %s %s(UObjectHandle self);"),
			*interopTypeName, *propertyInfo.GetterDelegateTypeName
		)
		// declare setter delegate type
		<< UnmanagedFunctionPointerAttribute
		<< FString::Printf(
			TEXT("private delegate void %s(UObjectHandle self, %s %s);"),
			*propertyInfo.SetterDelegateTypeName, *setterParamType, *Property->GetName()
		)
		// declare delegate instances that will be bound to the native wrapper functions
		<< FString::Printf(
			TEXT("private static %s %s;"),
			*propertyInfo.GetterDelegateTypeName, *propertyInfo.GetterDelegateName
		)
		<< FString::Printf(
			TEXT("private static %s %s;"),
			*propertyInfo.SetterDelegateTypeName, *propertyInfo.SetterDelegateName
		)
		// define a property that calls the native wrapper functions through the delegates 
		// declared above
		<< FString::Printf(TEXT("public %s %s"), *managedTypeName, *Property->GetName())
		<< FCodeFormatter::OpenBrace()
			<< TEXT("get")
			<< FCodeFormatter::OpenBrace()
				<< FString::Printf(TEXT("var value = %s(%s);"), 
					*propertyInfo.GetterDelegateName, *NativeThisPointer
				)
				<< GetReturnValueHandler(Property)
			<< FCodeFormatter::CloseBrace()
			<< FString::Printf(
				TEXT("set { %s(%s, %s); }"), 
				*propertyInfo.SetterDelegateName, *NativeThisPointer, *setterValue
			)
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator();
}

bool FCSharpWrapperGenerator::ShouldGenerateManagedWrapper(const UClass* Class)
{
	return (Class != UObject::StaticClass())
		&& (Class != UClass::StaticClass());
}

bool FCSharpWrapperGenerator::ShouldGenerateScriptObjectClass(const UClass* Class)
{
	return (Class != UClass::StaticClass());

	// FIXME: GetBoolMetaDataHierarchical() is only available when WITH_EDITOR is defined, and it's
	//        not defined when building this plugin :/. The non hierarchical version of GetMetaData()
	//        should be available though (because HACK_HEADER_GENERATOR should be defined for this
	//        plugin) so it is possible to do what GetBoolMetaDataHierarchical() does.
	//return Class->GetBoolMetaDataHierarchical(FBlueprintMetadata::MD_IsBlueprintBase);
}

void FCSharpWrapperGenerator::GenerateManagedStaticConstructor()
{
	if ((ExportedFunctions.Num() == 0) && (ExportedProperties.Num() == 0))
	{
		return;
	}

	GeneratedGlue << FString::Printf(TEXT("static %s()"), *NativeClassName);
	GeneratedGlue << FCodeFormatter::OpenBrace();
	
	// bind managed delegates to pointers to native wrapper functions
	GeneratedGlue 
		<< TEXT("var manager = AppDomain.CurrentDomain.DomainManager as IEngineAppDomainManager;")
		<< FString::Printf(
			TEXT("var nativeFuncPtrs = manager.GetNativeFunctionPointers(\"%s\");"), *NativeClassName
		);
	
	int32 functionIdx = 0;
	for (const FExportedProperty& propInfo : ExportedProperties)
	{
		GeneratedGlue << FString::Printf(
			TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;"),
			*propInfo.GetterDelegateName, functionIdx, *propInfo.GetterDelegateTypeName,
			*propInfo.GetterDelegateTypeName
		);
		++functionIdx;
		GeneratedGlue << FString::Printf(
			TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;"),
			*propInfo.SetterDelegateName, functionIdx, *propInfo.SetterDelegateTypeName,
			*propInfo.SetterDelegateTypeName
		);
		++functionIdx;
	}
		
	for (const FExportedFunction& funcInfo : ExportedFunctions)
	{
		// FIXME: don't generate the delegate type and name again, get it from FExportedFunction
		GeneratedGlue << FString::Printf(
			TEXT("%s = Marshal.GetDelegateForFunctionPointer(nativeFuncPtrs[%d], typeof(%s)) as %s;"),
			*funcInfo.DelegateName,	functionIdx, *funcInfo.DelegateTypeName, 
			*funcInfo.DelegateTypeName
		);
		++functionIdx;
	}
		
	GeneratedGlue << FCodeFormatter::CloseBrace();
}

void FCSharpWrapperGenerator::GenerateManagedScriptObjectClass()
{
	// Users should be allowed to subclass the generated wrapper class, but if the subclass is to be
	// used via Blueprints it must implement the IScriptObject interface (which would be an abstract
	// class if C# supported multiple inheritance). To simplify things an abstract class is
	// generated that is derived from the wrapper class and implements the IScriptObject interface,
	// the user can then simply subclass this abstract class.

	GeneratedGlue
		<< FString::Printf(
			TEXT("public abstract class %sScriptObject : %s, IScriptObject"),
			*NativeClassName, *NativeClassName
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
				*NativeClassName
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

UProperty* FCSharpWrapperGenerator::GetWrapperArgsAndReturnType(
	const UFunction* Function, FString& OutFormalInteropArgs, FString& OutActualInteropArgs,
	FString& OutFormalManagedArgs, FString& OutActualManagedArgs
)
{
	OutFormalInteropArgs = TEXT("UObjectHandle self");
	OutActualInteropArgs = NativeThisPointer;
	OutFormalManagedArgs.Empty();
	OutActualManagedArgs.Empty();
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
			FString argName = param->GetName();
			FString argInteropType = GetPropertyInteropType(param);
			FString argAttrs = GetPropertyInteropTypeAttributes(param);
			FString argMods = GetPropertyInteropTypeModifiers(param);
			
			OutFormalInteropArgs += TEXT(",");
			if (!argAttrs.IsEmpty())
			{
				OutFormalInteropArgs += TEXT(" ");
				OutFormalInteropArgs += argAttrs;
			}
			if (!argMods.IsEmpty())
			{
				OutFormalInteropArgs += TEXT(" ");
				OutFormalInteropArgs += argMods;
			}
			OutFormalInteropArgs += FString::Printf(
				TEXT(" %s %s"), *argInteropType, *argName
			);

			OutActualInteropArgs += TEXT(",");
			if (!argMods.IsEmpty())
			{
				OutActualInteropArgs += TEXT(" ");
				OutActualInteropArgs += argMods;
			}
			OutActualInteropArgs += TEXT(" ");
			if (param->IsA<UObjectProperty>())
			{
				OutActualInteropArgs += TEXT("(UObjectHandle)");
			}
			OutActualInteropArgs += argName;
									
			if (!OutFormalManagedArgs.IsEmpty())
			{
				OutFormalManagedArgs += TEXT(", ");
			}
			if (!argMods.IsEmpty())
			{
				OutFormalManagedArgs += argMods + TEXT(" ");
			}
			FString ArgManagedType = GetPropertyManagedType(param);
			OutFormalManagedArgs += FString::Printf(TEXT("%s %s"), *ArgManagedType, *argName);
			
			if (!OutActualManagedArgs.IsEmpty())
			{
				OutActualManagedArgs += TEXT(", ");
			}
			OutActualManagedArgs += argName;
		}
	}

	return returnValue;
}

FString FCSharpWrapperGenerator::GetReturnValueHandler(const UProperty* ReturnValue)
{
	if (ReturnValue)
	{
		if (ReturnValue->IsA<UClassProperty>())
		{
			return TEXT("return (UClass)value;");
		}
		else if (ReturnValue->IsA<UObjectProperty>())
		{
			FString wrapperTypeName = FCodeGenerator::GetPropertyCPPType(ReturnValue);
			wrapperTypeName.RemoveFromEnd(TEXT("*"));
			return FString::Printf(
				TEXT("return new %s(value);"), *wrapperTypeName
			);
		}
		else
		{
			return TEXT("return value;");
		}
	}
	return FString();
}

FString FCSharpWrapperGenerator::GetPropertyInteropType(const UProperty* Property)
{
	if (Property->IsA<UObjectProperty>())
	{
		return TEXT("UObjectHandle");
	}
	else if (Property->IsA<UObjectPropertyBase>())
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
		return FCodeGenerator::GetPropertyCPPType(Property);
	}
}

FString FCSharpWrapperGenerator::GetPropertyManagedType(const UProperty* Property)
{
	if (Property->IsA<UObjectProperty>())
	{
		static FString pointer(TEXT("*"));
		FString typeName = FCodeGenerator::GetPropertyCPPType(Property);
		typeName.RemoveFromEnd(pointer);
		return typeName;
	}
	return GetPropertyInteropType(Property);
}

FString FCSharpWrapperGenerator::GetPropertyInteropTypeAttributes(const UProperty* Property)
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
	if (!Property->IsA<UObjectPropertyBase>())
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

FString FCSharpWrapperGenerator::GetPropertyInteropTypeModifiers(const UProperty* Property)
{
	if (!Property->IsA<UObjectPropertyBase>())
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

FString FCSharpWrapperGenerator::GetDelegateTypeName(
	const FString& FunctionName, bool bHasReturnValue
)
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

FString FCSharpWrapperGenerator::GetDelegateName(const FString& FunctionName)
{
	return FString(TEXT("_")) + FunctionName;
}

} // namespace Klawr
