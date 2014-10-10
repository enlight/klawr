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

/** Generates a C# wrapper class for a UObject-derived class. */
class FCSharpWrapperGenerator
{
public:
	FCSharpWrapperGenerator(
		const UClass* Class, const UClass* InWrapperSuperClass, class FCodeFormatter& CodeFormatter
	);

	void GenerateHeader();
	void GenerateFunctionWrapper(const UFunction* Function);
	void GeneratePropertyWrapper(const UProperty* Property);
	void GenerateFooter();

	/** Get number of properties wrapped. */
	int32 GetPropertyCount() const { return ExportedProperties.Num(); }
	/** Get number of functions wrapped. */
	int32 GetFunctionCount() const { return ExportedFunctions.Num(); }

	static const UClass* GetWrapperSuperClass(
		const UClass* Class, const TArray<const UClass*>& ExportedClasses
	);

private:
	struct FExportedProperty
	{
		FString GetterDelegateName;
		FString GetterDelegateTypeName;
		FString SetterDelegateName;
		FString SetterDelegateTypeName;
	};

	struct FExportedFunction
	{
		FString DelegateName;
		FString DelegateTypeName;
	};

private:
	void GenerateStandardPropertyWrapper(const UProperty* Property);
	void GenerateArrayPropertyWrapper(const UArrayProperty* Property);
	static bool ShouldGenerateManagedWrapper(const UClass* Class);
	static bool ShouldGenerateScriptObjectClass(const UClass* Class);
	void GenerateDisposeMethod();
	void GenerateManagedStaticConstructor();
	void GenerateManagedScriptObjectClass();
	static UProperty* GetWrapperArgsAndReturnType(
		const UFunction* Function, FString& OutFormalInteropArgs, FString& OutActualInteropArgs,
		FString& OutFormalManagedArgs, FString& OutActualManagedArgs
	);
	static FString GetReturnValueHandler(const UProperty* ReturnValue);
	static FString GetPropertyInteropType(const UProperty* Property);
	static FString GetPropertyManagedType(const UProperty* Property);
	static FString GetPropertyInteropTypeAttributes(const UProperty* Property);
	static FString GetPropertyInteropTypeModifiers(const UProperty* Property);
	static FString GetDelegateTypeName(const FString& FunctionName, bool bHasReturnValue);
	static FString GetDelegateName(const FString& FunctionName);
	static FString GetArrayPropertyWrapperType(const UArrayProperty* ArrayProperty);

private:
	const UClass* WrapperSuperClass;
	FString FriendlyClassName;
	FString NativeClassName;
	class FCodeFormatter& GeneratedGlue;
	bool bShouldGenerateManagedWrapper;
	bool bShouldGenerateScriptObjectClass;
	TArray<FExportedFunction> ExportedFunctions;
	TArray<FExportedProperty> ExportedProperties;
	// names of members of the generated C# class that need to be disposed
	TArray<FString>DisposableMembers;

	static const FString UnmanagedFunctionPointerAttribute;
	static const FString MarshalReturnedBoolAsUint8Attribute;
	static const FString MarshalBoolParameterAsUint8Attribute;
	static const FString NativeThisPointer;
};

} // namespace Klawr
