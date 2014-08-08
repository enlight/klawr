using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using Klawr.ClrHost.Interfaces;

namespace Klawr.ClrHost.Managed
{
    // only IKlawrAppDomainManager should be used via COM
    //[ComVisible(true)]
    //[GuidAttribute("0E23AAC2-C76D-480C-882C-220214739598")]
    /// <summary>
    /// One of these gets instantiated for every app domain.
    /// </summary>
    public sealed class KlawrAppDomainManager : AppDomainManager, IKlawrAppDomainManager
    {
        private bool _isDefaultAppDomainManager = false;
        // only set for the engine app domain manager
        private Dictionary<string, IntPtr[]> _nativeFunctionPointers;
        // only set for the default app domain manager
        private AppDomain _engineAppDomain;

        public KlawrAppDomainManager()
        {
            _isDefaultAppDomainManager = AppDomain.CurrentDomain.IsDefaultAppDomain();
        }

        // NOTE: the base implementation of this method does nothing, so no need to call it
        public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
        {
            // register the custom domain manager with the unmanaged host
            this.InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
            
            if (!_isDefaultAppDomainManager)
            {
                InitializeEngineAppDomain();
            }
        }
        
        public void CreateEngineAppDomain()
        {
            if (!_isDefaultAppDomainManager)
            {
                throw new InvalidOperationException(
                    "Engine app domain can only be created by the default app domain manager!"
                );
            }
            else if (_engineAppDomain != null)
            {
                throw new InvalidOperationException("Engine app domain already exists!");
            }
            else
            {
                // this will instantiate a new app domain manager and call InitializeNewDomain()
                _engineAppDomain = AppDomain.CreateDomain("EngineDomain");
            }
        }

        // NOTE: _engineAppDomain is still invalid at this point because this method is called
        // by AppDomain.CreateDomain()
        public void InitializeEngineAppDomain()
        {
            var domainManager = AppDomain.CurrentDomain.DomainManager as IKlawrAppDomainManager;
            // TODO: load Klawr.UnrealEngine assembly into the new app domain
            // TODO: initialize the wrapper?
        }

        public void DestroyEngineAppDomain()
        {
            if (!_isDefaultAppDomainManager)
            {
                throw new InvalidOperationException(
                    "Engine app domain can only be destroyed by the default app domain manager!"
                );
            }
            else if (_engineAppDomain == null)
            {
                throw new InvalidOperationException("Engine app domain doesn't exist!");
            }
            else
            {
                AppDomain.Unload(_engineAppDomain);
                _engineAppDomain = null;
            }
        }

        public void SetNativeFunctionPointers(string nativeClassName, IntPtr[] functionPointers)
        {
            if (_isDefaultAppDomainManager)
            {
                throw new InvalidOperationException(
                    "The default app domain manager doesn't store pointers to native functions."
                );
            }
            else
            {
                if (_nativeFunctionPointers == null)
                {
                    _nativeFunctionPointers = new Dictionary<string, IntPtr[]>();
                }
                _nativeFunctionPointers[nativeClassName] = functionPointers;
            }
        }

        public IntPtr[] GetNativeFunctionPointers(string nativeClassName)
        {
            if (_isDefaultAppDomainManager)
            {
                throw new InvalidOperationException(
                    "The default app domain manager doesn't store pointers to native functions."
                );
            }
            else if (_nativeFunctionPointers != null)
            {
                return _nativeFunctionPointers[nativeClassName];
            }
            else
            {
                return null;
            }
        }
    }
}
