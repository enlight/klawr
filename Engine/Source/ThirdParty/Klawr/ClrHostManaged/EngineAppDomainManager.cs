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
using System.Collections.Generic;
using System.Reflection;

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Manager for engine app domains (that can be unloaded).
    /// </summary>
    public sealed class EngineAppDomainManager : AppDomainManager, IEngineAppDomainManager
    {
        // only set for the engine app domain manager
        private Dictionary<string, IntPtr[]> _nativeFunctionPointers;

        // NOTE: the base implementation of this method does nothing, so no need to call it
        public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
        {
            // register the custom domain manager with the unmanaged host
            this.InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
        }

        public void SetNativeFunctionPointers(string nativeClassName, long[] functionPointers)
        {
            if (_nativeFunctionPointers == null)
            {
                _nativeFunctionPointers = new Dictionary<string, IntPtr[]>();
            }
            // the function pointers are passed in as long to avoid pointer truncation on a 
            // 64-bit platform when this method is called via COM, but to actually use them
            // they need to be converted to IntPtr
            var unboxedFunctionPointers = new IntPtr[functionPointers.Length];
            for (var i = 0; i < functionPointers.Length; ++i)
            {
                unboxedFunctionPointers[i] = (IntPtr)(functionPointers[i]);
            }
            _nativeFunctionPointers[nativeClassName] = unboxedFunctionPointers;
        }

        public IntPtr[] GetNativeFunctionPointers(string nativeClassName)
        {
            if (_nativeFunctionPointers != null)
            {
                return _nativeFunctionPointers[nativeClassName];
            }
            else
            {
                return null;
            }
        }

        public void LoadUnrealEngineWrapperAssembly()
        {
            AssemblyName wrapperAssembly = new AssemblyName();
            wrapperAssembly.Name = "Klawr.UnrealEngine";
            Assembly.Load(wrapperAssembly);
        }
    }
}
