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

using System;
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Interfaces
{
    /// <summary>
    /// Manager for engine app domains (that can be unloaded), accessible in native code via COM.
    /// </summary>
    [ComVisible(true)]
    [GuidAttribute("CBFAB628-9E4D-4439-89FA-EF8B1D5FF966")]
    public interface IEngineAppDomainManager
    {
        /// <summary>
        /// Store pointers to native functions that wrap methods of a C++ class.
        /// </summary>
        /// <param name="nativeClassName">Name of C++ class to store function pointers for.</param>
        /// <param name="functionPointers">Array of pointers to native functions.</param>
        void SetNativeFunctionPointers(string nativeClassName, long[] functionPointers);

        /// <summary>
        /// Retrieve pointers to native functions that wrap methods of a C++ class.
        /// </summary>
        /// <param name="nativeClassName">Name of C++ class to retrieve function pointers for.</param>
        /// <returns>Array of pointers to native functions.</returns>
        IntPtr[] GetNativeFunctionPointers(string nativeClassName);

        /// <summary>
        /// Load the Klawr.UnrealEngine assembly into the engine app domain.
        /// </summary>
        void LoadUnrealEngineWrapperAssembly();
    }
}
