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

#include "ScriptBlueprintGeneratedClass.h"
#include "KlawrClrHost.h"

class FKlawrContext : public FScriptContextBase
{
public:
	FKlawrContext();
	virtual ~FKlawrContext();

public: // FScriptContextBase interface

	virtual bool Initialize(const FString& Code, UObject* Owner) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Destroy() override;
	virtual bool CanTick() override;
	virtual bool CallFunction(const FString& FunctionName) override;
	virtual bool SetFloatProperty(const FString& PropertyName, float NewValue) override;
	virtual float GetFloatProperty(const FString& PropertyName) override;
	virtual bool SetIntProperty(const FString& PropertyName, int32 NewValue) override;
	virtual int32 GetIntProperty(const FString& PropertyName) override;
	virtual bool SetObjectProperty(const FString& PropertyName, UObject* NewValue) override;
	virtual UObject* GetObjectProperty(const FString& PropertyName) override;
	virtual bool SetBoolProperty(const FString& PropertyName, bool NewValue) override;
	virtual bool GetBoolProperty(const FString& PropertyName) override;
	virtual bool SetStringProperty(const FString& PropertyName, const FString& NewValue) override;
	virtual FString GetStringProperty(const FString& PropertyName) override;
	virtual void InvokeScriptFunction(FFrame& Stack, RESULT_DECL) override;
#if WITH_EDITOR
	virtual void GetScriptDefinedFields(TArray<FScriptField>& OutFields) override;
#endif // WITH_EDITOR

private:
	void DestroyScriptObject();

private:
	Klawr::ScriptObjectInstanceInfo ScriptObjectInfo;
};
