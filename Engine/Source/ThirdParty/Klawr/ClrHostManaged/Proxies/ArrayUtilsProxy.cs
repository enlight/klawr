//
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
//

using Klawr.ClrHost.Managed.SafeHandles;
using Klawr.UnrealEngine;
using System;
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Contains pointers to native TArray manipulation functions.
    /// </summary>
    /// <remarks>This struct has a native counterpart by the same name defined in the
    /// Klawr.ClrHost.Native project, and it is also exposed to native code via COM.</remarks>
    [ComVisible(true)]
    [Guid("B4A6ED98-4CCC-49E1-9818-D9AF7DD5FA0E")]
    [StructLayout(LayoutKind.Sequential)]
    public struct ArrayUtilsProxy
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 NumFunc(ArrayHandle arrayHandle);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate IntPtr GetRawPtrFunc(ArrayHandle arrayHandle, Int32 index);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public delegate string GetStringFunc(ArrayHandle arrayHandle, Int32 index);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate FScriptName GetNameFunc(ArrayHandle arrayHandle, Int32 index);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate UObjectHandle GetObjectFunc(ArrayHandle arrayHandle, Int32 index);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SetUInt8AtAction(ArrayHandle arrayHandle, Int32 index, byte item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SetInt16AtAction(ArrayHandle arrayHandle, Int32 index, Int16 item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SetInt32AtAction(ArrayHandle arrayHandle, Int32 index, Int32 item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SetInt64AtAction(ArrayHandle arrayHandle, Int32 index, Int64 item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public delegate void SetStringAtAction(ArrayHandle arrayHandle, Int32 index, string item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SetNameAtAction(ArrayHandle arrayHandle, Int32 index, FScriptName item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void SetObjectAtAction(
            ArrayHandle arrayHandle, Int32 index, UObjectHandle item
        );

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 AddFunc(ArrayHandle arrayHandle);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ResetAction(ArrayHandle arrayHandle, Int32 newCapacity);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindFunc(ArrayHandle arrayHandle, IntPtr itemPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindUInt8Func(ArrayHandle arrayHandle, byte item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindInt16Func(ArrayHandle arrayHandle, Int16 item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindInt32Func(ArrayHandle arrayHandle, Int32 item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindInt64Func(ArrayHandle arrayHandle, Int64 item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public delegate Int32 FindStringFunc(ArrayHandle arrayHandle, string item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindNameFunc(ArrayHandle arrayHandle, FScriptName item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate Int32 FindObjectFunc(ArrayHandle arrayHandle, UObjectHandle item);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void InsertAction(ArrayHandle arrayHandle, Int32 index);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void RemoveAtAction(ArrayHandle arrayHandle, Int32 index);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DestroyAction(IntPtr arrayHandle);

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public NumFunc Num;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public GetRawPtrFunc GetRawPtr;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public GetStringFunc GetString;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public GetNameFunc GetName;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public GetObjectFunc GetObject;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetUInt8AtAction SetUInt8At;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetInt16AtAction SetInt16At;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetInt32AtAction SetInt32At;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetInt64AtAction SetInt64At;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetStringAtAction SetStringAt;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetNameAtAction SetNameAt;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public SetObjectAtAction SetObjectAt;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public AddFunc Add;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public ResetAction Reset;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindFunc Find;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindUInt8Func FindUInt8;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindInt16Func FindInt16;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindInt32Func FindInt32;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindInt64Func FindInt64;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindStringFunc FindString;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindNameFunc FindName;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public FindObjectFunc FindObject;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public InsertAction Insert;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public RemoveAtAction RemoveAt;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public DestroyAction Destroy;
    }
}
