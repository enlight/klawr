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
    /// Manager for the default app domain (that can't be unloaded), accessible via COM.
    /// 
    /// This interface is implemented by the managed side of the CLR host, and is used by the
    /// native side of the CLR host to interact with the managed side.
    /// </summary>
    [ComVisible(true)]
    [GuidAttribute("D90BC9C3-F4DC-4103-BEFA-966FA8C4B7EF")]
    public interface IDefaultAppDomainManager
    {
        /// <summary>
        /// Create an engine app domain that can be unloaded.
        /// </summary>
        /// <param name="applicationBase">Base path for the application domain, any assemblies to be
        /// loaded by the new app domain must reside in this directory. If an empty/null string is 
        /// passed in then the default application base is used (which will be the directory where 
        /// the application executable is located).</param>
        /// <returns>The identifier of the new engine app domain.</returns>
        int CreateEngineAppDomain(string applicationBase);

        /// <summary>
        /// Unload an engine app domain.
        /// </summary>
        /// <param name="domainId">The identifier of the engine app domain to be unloaded.</param>
        /// <returns>true if the engine app domain was successfully unloaded, false otherwise</returns>
        bool DestroyEngineAppDomain(int domainId);

        /// <summary>
        /// Unload all engine app domains.
        /// </summary>
        void DestroyAllEngineAppDomains();
    }
}
