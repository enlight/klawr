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
#include "KlawrScriptContext.h"

namespace Klawr {

FScriptContext::FScriptContext()
{
	ScriptObjectInfo.InstanceID = 0;
}

FScriptContext::~FScriptContext()
{
	DestroyScriptObject();
}

bool FScriptContext::Initialize(const FString& Code, UObject* Owner)
{
	// TODO:
	// [editor-only] save the code out to a file
	// [editor-only][advanced] generate a C# class for each blueprint class in the inheritance hierarchy
	// [editor-only] update the .csproj
	// [editor-only] rebuild the .csproj
	// [editor-only] copy the assembly to a directory in the search path
	// [editor-only] reload the engine app domain

	// create an instance of the managed class
	return IClrHost::Get()->CreateScriptObject(TEXT("Klawr.UnrealEngine.TestActor"), Owner, ScriptObjectInfo);
}

void FScriptContext::DestroyScriptObject()
{
	if (ScriptObjectInfo.InstanceID != 0)
	{
		IClrHost::Get()->DestroyScriptObject(ScriptObjectInfo.InstanceID);
		ScriptObjectInfo.InstanceID = 0;
	}
}

void FScriptContext::BeginPlay()
{
	if (ScriptObjectInfo.BeginPlay)
	{
		ScriptObjectInfo.BeginPlay();
	}
}

void FScriptContext::Tick(float DeltaTime)
{
	if (ScriptObjectInfo.Tick)
	{
		ScriptObjectInfo.Tick(DeltaTime);
	}
}

void FScriptContext::Destroy()
{
	// clean up anything created in BeginPlay/Tick
	if (ScriptObjectInfo.Destroy)
	{
		ScriptObjectInfo.Destroy();
	}
	// the managed object will not be accessed from here on, so get rid of it now
	DestroyScriptObject();
}

bool FScriptContext::CanTick()
{
	return ScriptObjectInfo.Tick != nullptr;
}

bool FScriptContext::CallFunction(const FString& FunctionName)
{
	// TODO:
	return false;
}

void FScriptContext::InvokeScriptFunction(FFrame& Stack, RESULT_DECL)
{
	P_FINISH;
	CallFunction(Stack.CurrentNativeFunction->GetName());
}

} // namespace Klawr
