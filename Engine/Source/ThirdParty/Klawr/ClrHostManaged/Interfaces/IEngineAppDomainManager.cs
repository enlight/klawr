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
using Klawr.ClrHost.Managed;

namespace Klawr.ClrHost.Interfaces
{
    /// <summary>
    /// Manager for engine app domains (that can be unloaded), accessible in native code via COM.
    /// </summary>
    [ComVisible(true)]
    [Guid("CBFAB628-9E4D-4439-89FA-EF8B1D5FF966")]
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
        [ComVisible(false)]
        IntPtr[] GetNativeFunctionPointers(string nativeClassName);

        /// <summary>
        /// Load the Klawr.UnrealEngine assembly into the engine app domain.
        /// </summary>
        void LoadUnrealEngineWrapperAssembly();

        /// <summary>
        /// Load the specified assembly into the engine app domain.
        /// </summary>
        /// <param name="assemblyName">Name of assembly to load (without a file extension).</param>
        bool LoadAssembly(string assemblyName);

        /// <summary>
        /// Create a new ScriptObject instance of the given class.
        /// </summary>
        /// <param name="className">The name of a class derived from ScriptObject, the namespace
        /// must also be provided, e.g. "MyExample.MyScriptObject"</param>
        /// <param name="nativeObject">Pointer to a native owner/base UObject instance, may be null.</param>
        /// <param name="info">Information about the newly created ScriptObject instance.</param>
        /// <returns>true if the object was created successfuly, false otherwise</returns>
        bool CreateScriptObject(string className, IntPtr nativeObject, ref ScriptObjectInstanceInfo info);

        /// <summary>
        /// Destroy a ScriptObject instance.
        /// </summary>
        /// <param name="scriptObjectInstanceID">The ID of the ScriptObject instance to destroy.</param>
        void DestroyScriptObject(long scriptObjectInstanceID);

        /// <summary>
        /// The native side of the CLR host will use this method to pass a set of native utility 
        /// functions to the managed side of the CLR host. This method must be called during 
        /// initialization of the engine app domain, before any native UObject instance is 
        /// passed to the managed side.
        /// </summary>
        void BindUtils(
            ref ObjectUtilsProxy objectUtilsProxy,
            ref LogUtilsProxy logUtilsProxy,
            ref ArrayUtilsProxy arrayUtilsProxy
        );
                
        bool CreateScriptComponent(
            string className, IntPtr nativeComponent, ref ScriptComponentProxy proxy
        );

        void DestroyScriptComponent(long scriptComponentID);

        /// <summary>
        /// Get the fully qualified names (including namespace) of all currently loaded managed 
        /// types derived from UKlawrScriptComponent.
        /// </summary>
        /// <returns>Script component type names.</returns>
        string[] GetScriptComponentTypes();
    }
}
