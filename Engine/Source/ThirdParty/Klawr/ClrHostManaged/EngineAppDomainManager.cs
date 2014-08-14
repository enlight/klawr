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
using System.Threading;

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Manager for engine app domains (that can be unloaded).
    /// </summary>
    public sealed class EngineAppDomainManager : AppDomainManager, IEngineAppDomainManager
    {
        // only set for the engine app domain manager
        private Dictionary<string, IntPtr[]> _nativeFunctionPointers;
        // all currently registered script objects
        private Dictionary<long, ScriptObject> _scriptObjects = new Dictionary<long, ScriptObject>();
        // identifier of the most recently registered ScriptObject instance
        private long _lastScriptObjectID = 0;

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

        public bool CreateScriptObject(string className, ref ScriptObjectInstanceInfo info)
        {
            var objType = typeof(EngineAppDomainManager).Assembly.GetType(className);
            var constructor = objType.GetConstructor(Type.EmptyTypes);
            if (constructor != null)
            {
                var obj = (ScriptObject)constructor.Invoke(null);
                info.InstanceID = RegisterScriptObject(obj);
                info.BeginPlay = new ScriptObjectInstanceInfo.BeginPlayAction(obj.BeginPlay);
                info.Tick = new ScriptObjectInstanceInfo.TickAction(obj.Tick);
                info.Destroy = new ScriptObjectInstanceInfo.DestroyAction(obj.Destroy);
                obj.InstanceInfo = info;
                return true;
            }
            // TODO: log an error
            return false;
        }

        public void DestroyScriptObject(long scriptObjectInstanceID)
        {
            UnregisterScriptObject(scriptObjectInstanceID);
        }

        /// <summary>
        /// Register the given ScriptObject instance with the manager.
        /// 
        /// The manager will keep a reference to the given object until the object is unregistered.
        /// 
        /// Note that the identifier returned by this method is only unique amongst all ScriptObject 
        /// instances registered with this manager instance. Since all ScriptObject instances
        /// automatically register themselves with the manager of the app domain within which they
        /// are constructed the returned identifier can be used to uniquely identify a ScriptObject
        /// instance within the app domain it was created in.
        /// </summary>
        /// <returns>A unique identifier for the registered object.</returns>
        public long RegisterScriptObject(ScriptObject scriptObject)
        {
            var uniqueID = Interlocked.Increment(ref _lastScriptObjectID);
            _scriptObjects.Add(uniqueID, scriptObject);
            return uniqueID;
        }

        /// <summary>
        /// Unregister a ScriptObject that was previously registered with the manager.
        /// 
        /// The manager will remove the reference it previously held to the object.
        /// </summary>
        /// <param name="scriptObjectInstanceID">ID of a registered ScriptObject instance.</param>
        public void UnregisterScriptObject(long scriptObjectInstanceID)
        {
            _scriptObjects.Remove(scriptObjectInstanceID);
        }
    }
}
