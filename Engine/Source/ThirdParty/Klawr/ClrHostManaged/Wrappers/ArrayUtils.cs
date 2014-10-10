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
using System;

namespace Klawr.ClrHost.Managed
{
    internal class ArrayUtils
    {
        private static ArrayUtilsProxy _proxy;

        internal ArrayUtils(ref ArrayUtilsProxy proxy)
        {
            _proxy = proxy;
        }

        public static int Num(ArrayHandle arrayHandle)
        {
            return _proxy.Num(arrayHandle);
        }

        public static IntPtr GetRawPtr(ArrayHandle arrayHandle, Int32 index)
        {
            return _proxy.GetRawPtr(arrayHandle, index);
        }

        public static string GetString(ArrayHandle arrayHandle, Int32 index)
        {
            return _proxy.GetString(arrayHandle, index);
        }

        public static UObjectHandle GetObject(ArrayHandle arrayHandle, Int32 index)
        {
            return _proxy.GetObject(arrayHandle, index);
        }

        public static void SetUInt8At(ArrayHandle arrayHandle, Int32 index, byte item)
        {
            _proxy.SetUInt8At(arrayHandle, index, item);
        }

        public static void SetInt16At(ArrayHandle arrayHandle, Int32 index, Int16 item)
        {
            _proxy.SetInt16At(arrayHandle, index, item);
        }

        public static void SetInt32At(ArrayHandle arrayHandle, Int32 index, Int32 item)
        {
            _proxy.SetInt32At(arrayHandle, index, item);
        }

        public static void SetInt64At(ArrayHandle arrayHandle, Int32 index, Int64 item)
        {
            _proxy.SetInt64At(arrayHandle, index, item);
        }

        public static void SetStringAt(ArrayHandle arrayHandle, Int32 index, string item)
        {
            _proxy.SetStringAt(arrayHandle, index, item);
        }

        public static void SetObjectAt(ArrayHandle arrayHandle, Int32 index, UObjectHandle item)
        {
            _proxy.SetObjectAt(arrayHandle, index, item);
        }

        public static Int32 Add(ArrayHandle arrayHandle)
        {
            return _proxy.Add(arrayHandle);
        }

        public static void Reset(ArrayHandle arrayHandle, Int32 newCapacity)
        {
            _proxy.Reset(arrayHandle, newCapacity);
        }

        public static Int32 Find(ArrayHandle arrayHandle, IntPtr itemPtr)
        {
            return _proxy.Find(arrayHandle, itemPtr);
        }

        public static Int32 FindUInt8(ArrayHandle arrayHandle, byte item)
        {
            return _proxy.FindUInt8(arrayHandle, item);
        }

        public static Int32 FindInt16(ArrayHandle arrayHandle, Int16 item)
        {
            return _proxy.FindInt16(arrayHandle, item);
        }

        public static Int32 FindInt32(ArrayHandle arrayHandle, Int32 item)
        {
            return _proxy.FindInt32(arrayHandle, item);
        }

        public static Int32 FindInt64(ArrayHandle arrayHandle, Int64 item)
        {
            return _proxy.FindInt64(arrayHandle, item);
        }

        public static Int32 FindString(ArrayHandle arrayHandle, string item)
        {
            return _proxy.FindString(arrayHandle, item);
        }

        public static Int32 FindObject(ArrayHandle arrayHandle, UObjectHandle item)
        {
            return _proxy.FindObject(arrayHandle, item);
        }

        public static void Insert(ArrayHandle arrayHandle, Int32 index)
        {
            _proxy.Insert(arrayHandle, index);
        }

        public static void RemoveAt(ArrayHandle arrayHandle, Int32 index)
        {
            _proxy.RemoveAt(arrayHandle, index);
        }

        public static void Destroy(IntPtr arrayHandle)
        {
            _proxy.Destroy(arrayHandle);
        }
    }
}
