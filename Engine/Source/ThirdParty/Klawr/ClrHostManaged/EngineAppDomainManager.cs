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
using System.Runtime.InteropServices;
using System.Linq.Expressions;

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

        public struct ScriptComponentInfo
        {
            public IDisposable Instance;
            public ScriptComponentProxy Proxy;
        }

        private delegate void SetProxyDelegateAction(ref ScriptComponentProxy proxy, Delegate value);

        private struct ScriptComponentProxyMethodInfo
        {
            public string Name;
            public Type DelegateType;
            public Type[] ParameterTypes;
            public SetProxyDelegateAction BindToProxy;
        }

        private struct ScriptComponentMethodInfo
        {
            public Type DelegateType;
            public MethodInfo Method;
            public SetProxyDelegateAction BindToProxy;
        }

        private struct ScriptComponentTypeInfo
        {
            public ConstructorInfo Constructor;
            public ScriptComponentMethodInfo[] Methods;
        }

        // only set for the engine app domain manager
        private Dictionary<string /*Native Class*/, IntPtr[]> _nativeFunctionPointers = new Dictionary<string, IntPtr[]>();
        // all currently registered script objects
        private Dictionary<long /*Instance ID*/, ScriptObjectInfo> _scriptObjects = new Dictionary<long, ScriptObjectInfo>();
        // identifier of the most recently registered ScriptObject instance
        private long _lastScriptObjectID = 0;
        // cache of previously created script object types
        private Dictionary<string /*Full Type Name*/, Type> _scriptObjectTypeCache = new Dictionary<string, Type>();

        private ScriptComponentProxyMethodInfo[] _scriptComponentProxyMethods;
        // all currently registered script components
        private Dictionary<long /*Instance ID*/, ScriptComponentInfo> _scriptComponents = new Dictionary<long, ScriptComponentInfo>();
        // cache of previously created script component types
        private Dictionary<string /*Full Type Name*/, ScriptComponentTypeInfo> _scriptComponentTypeCache = new Dictionary<string, ScriptComponentTypeInfo>();

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
            // TODO: this may not be the best place to call it
            CacheScriptComponentProxyInfo();

            var wrapperAssembly = new AssemblyName();
            wrapperAssembly.Name = "Klawr.UnrealEngine";
            Assembly.Load(wrapperAssembly);
        }

        public bool LoadAssembly(string assemblyName)
        {
            var assembly = new AssemblyName();
            assembly.Name = assemblyName;

            try
            {
                Assembly.Load(assembly);
            }
            catch (Exception except)
            {
                Console.WriteLine(except.ToString());
                return false;
            }

            return true;
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

        public void BindObjectUtils(ref ObjectUtilsProxy proxy)
        {
            new ObjectUtils(ref proxy);
            UObjectHandle.ReleaseHandleCallback = new Action<IntPtr>(ObjectUtils.ReleaseObject);
        }

        public void BindLogUtils(ref LogUtilsProxy proxy)
        {
            new LogUtils(ref proxy);
            // redirect output to the UE console and log file
            System.Console.SetOut(new UELogWriter());
        }

        public bool CreateScriptComponent(
            string className, IntPtr nativeComponent, ref ScriptComponentProxy proxy
        )
        {
            ScriptComponentTypeInfo componentTypeInfo;
            if (FindScriptComponentTypeByName(className, out componentTypeInfo))
            {
                if (componentTypeInfo.Constructor != null)
                {
                    var instanceID = GenerateScriptObjectID();
                    // The handle created here is set not to release the native object when the
                    // handle is disposed because that object is actually the owner of the script 
                    // object created here, and no additional references are created to owners at
                    // the moment so there is no reference to remove.
                    var objectHandle = new UObjectHandle(nativeComponent, false);
                    var component = (IDisposable)componentTypeInfo.Constructor.Invoke(
                        new object[] { instanceID, objectHandle }
                    );
                    // initialize the script component proxy
                    proxy.InstanceID = instanceID;
                    foreach (var methodInfo in componentTypeInfo.Methods)
                    {
                        methodInfo.BindToProxy(
                            ref proxy,
                            Delegate.CreateDelegate(
                                methodInfo.DelegateType, component, methodInfo.Method
                            )
                        );
                    }
                    // keep anything that may be called from native code alive
                    RegisterScriptComponent(instanceID, component, proxy);
                    return true;
                }
            }
            // TODO: log an error
            return false;
        }

        public void DestroyScriptComponent(long instanceID)
        {
            var instance = UnregisterScriptComponent(instanceID);
            instance.Dispose();
        }

        private void RegisterScriptComponent(
            long instanceID, IDisposable scriptComponent, ScriptComponentProxy proxy
        )
        {
            ScriptComponentInfo componentInfo;
            componentInfo.Instance = scriptComponent;
            componentInfo.Proxy = proxy;
            _scriptComponents.Add(instanceID, componentInfo);
        }

        private IDisposable UnregisterScriptComponent(long instanceID)
        {
            var instance = _scriptComponents[instanceID].Instance;
            _scriptComponents.Remove(instanceID);
            return instance;
        }

        /// <summary>
        /// Search all loaded (non-dynamic) assemblies for a Type matching the given name, the Type
        /// should be derived from UKlawrScriptComponent (but this is not enforced yet).
        /// </summary>
        /// <param name="typeName">The full name of a type (including the namespace).</param>
        /// <param name="componentTypeInfo">Structure to be filled in with type information.</param>
        /// <returns>true if type information matching the given type name was found, false otherwise</returns>
        private bool FindScriptComponentTypeByName(
            string typeName, out ScriptComponentTypeInfo componentTypeInfo
        )
        {
            if (!_scriptComponentTypeCache.TryGetValue(typeName, out componentTypeInfo))
            {
                var componentType = FindTypeByName(typeName);
                if (componentType != null)
                {
                    componentTypeInfo = GetComponentTypeInfo(componentType);
                    // cache the result to speed up future searches
                    _scriptComponentTypeCache.Add(typeName, componentTypeInfo);
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        private ScriptComponentTypeInfo GetComponentTypeInfo(Type componentType)
        {
            ScriptComponentTypeInfo typeInfo;
            typeInfo.Constructor = componentType.GetConstructor(
                new Type[] { typeof(long), typeof(UObjectHandle) }
            );

            // Currently all script component classes must directly subclass UKlawScriptComponent, 
            // they cannot subclass another script component. The virtual methods in 
            // UKlawScriptComponent have default implementations that do nothing, these can be 
            // overridden in subclasses, but if they're not then they should never be called from 
            // native code to avoid a pointless native/managed transition. BindingFlags.DeclaredOnly
            // is sufficient to detect if a method has been overridden in a subclass for now, but
            // this will have to be revisited if script classes are allowed to subclass other
            // script classes in the future.
            BindingFlags bindingFlags = BindingFlags.DeclaredOnly
                | BindingFlags.Instance
                | BindingFlags.Public
                | BindingFlags.NonPublic;

            var implementedMethodList = new List<ScriptComponentMethodInfo>();
            foreach (var proxyMethod in _scriptComponentProxyMethods)
            {
                // FIXME: catch and log exceptions
                ScriptComponentMethodInfo methodInfo;
                var method = componentType.GetMethod(
                    proxyMethod.Name, bindingFlags, null, proxyMethod.ParameterTypes, null
                );
                if (method != null)
                {
                    methodInfo.DelegateType = proxyMethod.DelegateType;
                    methodInfo.Method = method;
                    methodInfo.BindToProxy = proxyMethod.BindToProxy;
                    implementedMethodList.Add(methodInfo);
                }
            }
            typeInfo.Methods = implementedMethodList.ToArray();
            return typeInfo;
        }

        private void CacheScriptComponentProxyInfo()
        {
            // grab all the public delegate instance fields
            var fields = typeof(ScriptComponentProxy)
                .GetFields(BindingFlags.Public | BindingFlags.Instance)
                .Where(field => field.FieldType.IsSubclassOf(typeof(Delegate)))
                .ToArray();

            // store the info that will be useful later on
            _scriptComponentProxyMethods = new ScriptComponentProxyMethodInfo[fields.Length];
            for (int i = 0; i < fields.Length; i++)
            {
                var fieldInfo = fields[i];
                var methodInfo = fieldInfo.FieldType.GetMethod("Invoke");
                var parameters = methodInfo.GetParameters();

                ScriptComponentProxyMethodInfo info;
                info.Name = fieldInfo.Name;
                info.DelegateType = fieldInfo.FieldType;
                info.ParameterTypes = new Type[parameters.Length];
                for (int j = 0; j < parameters.Length; j++)
                {
                    info.ParameterTypes[j] = parameters[j].ParameterType;
                }
                info.BindToProxy = BuildProxyDelegateSetter(fieldInfo);
                _scriptComponentProxyMethods[i] = info;
            }
        }

        /// <summary>
        /// Build a delegate that sets one of the delegate fields in ScriptComponentProxy.
        /// 
        /// Setting a field on struct could've been done via reflection, but this is faster, and 
        /// works without boxing/unboxing or using undocumented features like __makeref().
        /// </summary>
        /// <param name="field">The field whose value the delegate should set when invoked.</param>
        /// <returns>A delegate.</returns>
        private SetProxyDelegateAction BuildProxyDelegateSetter(FieldInfo field)
        {
            var proxyExpr = Expression.Parameter(typeof(ScriptComponentProxy).MakeByRefType());
            var valueExpr = Expression.Parameter(typeof(Delegate), "value");
            var lambdaExpr = Expression.Lambda<SetProxyDelegateAction>(
                Expression.Assign(
                    Expression.Field(proxyExpr, field),
                    Expression.Convert(valueExpr, field.FieldType)
                ),
                proxyExpr, valueExpr
            );
            return lambdaExpr.Compile();
        }

        public string[] GetScriptComponentTypes()
        {
            // this type is defined in the UE4 wrappers assembly
            var scriptComponentType = FindTypeByName("Klawr.UnrealEngine.UKlawrScriptComponent");
            if (scriptComponentType == null)
            {
                return new string[] { };
            }

            return AppDomain.CurrentDomain.GetAssemblies()
                .Where(assembly => !assembly.IsDynamic)
                .SelectMany(assembly => assembly.GetTypes())
                .Where(t => t.IsSubclassOf(scriptComponentType))
                .Select(t => t.FullName)
                .ToArray();
        }
    }
}
