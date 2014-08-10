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

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Default app domain manager.
    /// </summary>
    public sealed class DefaultAppDomainManager : AppDomainManager, IDefaultAppDomainManager
    {
        // only set for the default app domain manager
        private AppDomain _engineAppDomain;

        // NOTE: the base implementation of this method does nothing, so no need to call it
        public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
        {
            // register the custom domain manager with the unmanaged host
            this.InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
        }
        
        public void CreateEngineAppDomain()
        {
            if (_engineAppDomain != null)
            {
                throw new InvalidOperationException("Engine app domain already exists!");
            }
            else
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
                _engineAppDomain = AppDomain.CreateDomain("EngineDomain", null, setup);
            }
        }

        public void DestroyEngineAppDomain()
        {
            if (_engineAppDomain == null)
            {
                throw new InvalidOperationException("Engine app domain doesn't exist!");
            }
            else
            {
                AppDomain.Unload(_engineAppDomain);
                _engineAppDomain = null;
            }
        }
    }
}
