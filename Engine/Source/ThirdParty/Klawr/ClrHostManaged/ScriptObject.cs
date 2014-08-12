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

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Base class for user defined classes that are accessible to the native side of the CLR host.
    ///
    /// This class is the managed counterpart of FKlawrContext, and its lifetime is tied to that of 
    /// a native FKlawrContext instance. While the corresponding FKlawrContext instance is alive 
    /// the ScriptObject instance will be kept alive.
    /// 
    /// ScriptObject instances must only be created in an engine app domain.
    /// </summary>
    public class ScriptObject
    {
        private readonly long _instanceID;

        /// <summary>
        /// Unique identifier for this instance that's guaranteed to be unique amongst all the
        /// ScriptObject instances within the app domain this instance was created in.
        /// </summary>
        public long InstanceID
        {
            get { return _instanceID; }
        }

        /// <summary>
        /// Construct a new instance and register it with the EngineAppDomainManager.
        /// </summary>
        ScriptObject()
        {
            var manager = (EngineAppDomainManager)AppDomain.CurrentDomain.DomainManager;
            _instanceID = manager.RegisterScriptObject(this);
        }
    }
}
