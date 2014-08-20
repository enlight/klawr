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

using Klawr.ClrHost.Interfaces;
using System;
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Managed
{
    internal class ObjectUtils
    {
        private ObjectUtilsNativeInfo.RemoveObjectRefAction RemoveObjectRef_Native;
        
        public ObjectUtils(ref ObjectUtilsNativeInfo info)
        {
            RemoveObjectRef_Native = info.RemoveObjectRef;
        }

        /// <summary>
        /// Release a reference to a native UObject instance.
        /// </summary>
        /// <param name="handle">Pointer to a native UObject instance.</param>
        public void ReleaseObject(IntPtr nativeObject)
        {
            if (RemoveObjectRef_Native != null)
            {
                RemoveObjectRef_Native(nativeObject);
            }
        }

        // TODO: move this somewhere more appropriate, maybe make it an extension method for Marshal
        public static TDelegate GetDelegateFromFunctionPointer<TDelegate>(IntPtr functionPointer) where TDelegate : class /* Delegate */
        {
            return Marshal.GetDelegateForFunctionPointer(functionPointer, typeof(TDelegate)) as TDelegate;
        }
    }
}
