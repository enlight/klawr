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

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Default app domain manager.
    /// </summary>
    public sealed class DefaultAppDomainManager : AppDomainManager, IDefaultAppDomainManager
    {
        private Dictionary<int /* Domain ID */, AppDomain> _engineAppDomains =
            new Dictionary<int, AppDomain>();

        // NOTE: the base implementation of this method does nothing, so no need to call it
        public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
        {
            // register the custom domain manager with the unmanaged host
            this.InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
        }
        
        public int CreateEngineAppDomain()
        {
            var currentSetup = AppDomain.CurrentDomain.SetupInformation;
            var setup = new AppDomainSetup()
            {
                AppDomainManagerAssembly = "Klawr.ClrHost.Managed",
                AppDomainManagerType = "Klawr.ClrHost.Managed.EngineAppDomainManager",
                ApplicationName = "Klawr.UnrealEngine",
                ApplicationBase = currentSetup.ApplicationBase,
                PrivateBinPath = currentSetup.PrivateBinPath
            };
            // this will instantiate a new app domain manager and call InitializeNewDomain()
            var engineAppDomain = AppDomain.CreateDomain("EngineDomain", null, setup);
            var domainId = engineAppDomain.Id;
            _engineAppDomains.Add(domainId, engineAppDomain);
            return domainId;
        }

        public bool DestroyEngineAppDomain(int domainId)
        {
            AppDomain appDomain;
            if (!_engineAppDomains.TryGetValue(domainId, out appDomain))
            {
                throw new InvalidOperationException(
                    String.Format("Engine app domain with ID {0} doesn't exist!", domainId)
                );
            }
            else
            {
                try
                {
                    _engineAppDomains.Remove(domainId);
                    AppDomain.Unload(appDomain);
                    return true;
                }
                catch (AppDomainUnloadedException)
                {
                    // ho hum, log an error maybe?
                }
            }
            return false;
        }

        public void DestroyAllEngineAppDomains()
        {
            foreach (var appDomain in _engineAppDomains.Values)
            {
                try
                {
                    AppDomain.Unload(appDomain);
                }
                catch (AppDomainUnloadedException)
                {
                    // oh well
                }
            }
            _engineAppDomains.Clear();
        }
    }
}
