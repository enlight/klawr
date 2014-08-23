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
using System.Linq;

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Manager for engine app domains (that can be unloaded).
    /// </summary>
    public sealed class EngineAppDomainManager : AppDomainManager, IEngineAppDomainManager
    {
        public struct ScriptObjectInfo
        {
            public IScriptObject Instance;
            public ScriptObjectInstanceInfo.BeginPlayAction BeginPlay;
            public ScriptObjectInstanceInfo.TickAction Tick;
            public ScriptObjectInstanceInfo.DestroyAction Destroy;
        }
        // only set for the engine app domain manager
        private Dictionary<string /*Native Class*/, IntPtr[]> _nativeFunctionPointers = new Dictionary<string, IntPtr[]>();
        // all currently registered script objects
        private Dictionary<long /*Instance ID*/, ScriptObjectInfo> _scriptObjects = new Dictionary<long, ScriptObjectInfo>();
        // identifier of the most recently registered ScriptObject instance
        private long _lastScriptObjectID = 0;
        // cache of previously created script object types
        private Dictionary<string /*Full Type Name*/, Type> _scriptObjectTypeCache = new Dictionary<string, Type>();

        // NOTE: the base implementation of this method does nothing, so no need to call it
        public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
        {
            // register the custom domain manager with the unmanaged host
            this.InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
        }

        public void SetNativeFunctionPointers(string nativeClassName, long[] functionPointers)
        {
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
            return _nativeFunctionPointers[nativeClassName];
        }

        public void LoadUnrealEngineWrapperAssembly()
        {
            AssemblyName wrapperAssembly = new AssemblyName();
            wrapperAssembly.Name = "Klawr.UnrealEngine";
            Assembly.Load(wrapperAssembly);
        }

        public bool CreateScriptObject(string className, IntPtr nativeObject, ref ScriptObjectInstanceInfo info)
        {
            var objType = FindScriptObjectTypeByName(className);
            if (objType != null)
            {
                var constructor = objType.GetConstructor(new Type[] { typeof(long), typeof(UObjectHandle) });
                if (constructor != null)
                {
                    var instanceID = GenerateScriptObjectID();
                    // The handle created here is set not to release the native object when the
                    // handle is disposed because that object is actually the owner of the script 
                    // object created here, and no additional references are created to owners at
                    // the moment so there is no reference to remove.
                    var objectHandle = new UObjectHandle(nativeObject, false);
                    var obj = (IScriptObject)constructor.Invoke(
                        new object[] { instanceID, objectHandle }
                    );
                    var objInfo = RegisterScriptObject(obj);
                    info.InstanceID = instanceID;
                    info.BeginPlay = objInfo.BeginPlay;
                    info.Tick = objInfo.Tick;
                    info.Destroy = objInfo.Destroy;
                    return true;
                }
            }
            // TODO: log an error
            return false;
        }

        public void DestroyScriptObject(long scriptObjectInstanceID)
        {
            var instance = UnregisterScriptObject(scriptObjectInstanceID);
            instance.Dispose();
        }
        
        /// <summary>
        /// Note that the identifier returned by this method is only unique amongst all ScriptObject 
        /// instances registered with this manager instance. The returned identifier can be used to 
        /// uniquely identify a ScriptObject instance within the app domain it was created in.
        /// </summary>
        /// <returns>Unique identifier.</returns>
        public long GenerateScriptObjectID()
        {
            return Interlocked.Increment(ref _lastScriptObjectID);
        }
        /// <summary>
        /// Register the given IScriptObject instance with the manager.
        /// 
        /// The manager will keep a reference to the given object until the object is unregistered.
        /// </summary>
        /// <param name="scriptObject"></param>
        /// <returns></returns>
        public ScriptObjectInfo RegisterScriptObject(IScriptObject scriptObject)
        {
            ScriptObjectInfo info;
            info.Instance = scriptObject;
            info.BeginPlay = new ScriptObjectInstanceInfo.BeginPlayAction(scriptObject.BeginPlay);
            info.Tick = new ScriptObjectInstanceInfo.TickAction(scriptObject.Tick);
            info.Destroy = new ScriptObjectInstanceInfo.DestroyAction(scriptObject.Destroy);
            _scriptObjects.Add(scriptObject.InstanceID, info);
            return info;
        }

        /// <summary>
        /// Unregister a IScriptObject that was previously registered with the manager.
        /// 
        /// The manager will remove the reference it previously held to the object.
        /// </summary>
        /// <param name="scriptObjectInstanceID">ID of a registered IScriptObject instance.</param>
        /// <returns>The script object matching the given ID.</returns>
        public IScriptObject UnregisterScriptObject(long scriptObjectInstanceID)
        {
            var instance = _scriptObjects[scriptObjectInstanceID].Instance;
            _scriptObjects.Remove(scriptObjectInstanceID);
            return instance;
        }

        /// <summary>
        /// Search all loaded (non-dynamic) assemblies for a Type matching the given name and
        /// implementing the IScriptObject interface.
        /// </summary>
        /// <param name="typeName">The full name of a type (including the namespace).</param>
        /// <returns>Matching Type instance, or null if no match was found.</returns>
        private Type FindScriptObjectTypeByName(string typeName)
        {
            Type objType = null;
            if (!_scriptObjectTypeCache.TryGetValue(typeName, out objType))
            {
                objType = AppDomain.CurrentDomain.GetAssemblies()
                    .Where(assembly => !assembly.IsDynamic)
                    .SelectMany(assembly => assembly.GetTypes())
                    .FirstOrDefault(
                        t => t.FullName.Equals(typeName) 
                            && t.GetInterfaces().Contains(typeof(IScriptObject))
                    );

                if (objType != null)
                {
                    // cache the result to speed up future searches
                    _scriptObjectTypeCache.Add(typeName, objType);
                }
            }
            return objType;
        }

        /// <summary>
        /// Search all loaded (non-dynamic) assemblies for a Type matching the given name.
        /// </summary>
        /// <param name="typeName">The full name of a type (including the namespace).</param>
        /// <returns>Matching Type instance, or null if no match was found.</returns>
        private static Type FindTypeByName(string typeName)
        {
            return AppDomain.CurrentDomain.GetAssemblies()
                .Where(assembly => !assembly.IsDynamic)
                .SelectMany(assembly => assembly.GetTypes())
                .FirstOrDefault(t => t.FullName.Equals(typeName));
        }

        public void BindObjectUtils(ref ObjectUtilsNativeInfo info)
        {
            ObjectUtils.BindToNativeFunctions(ref info);
            UObjectHandle.ReleaseHandleCallback = new Action<IntPtr>(ObjectUtils.ReleaseObject);
        }
    }
}
