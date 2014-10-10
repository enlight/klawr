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

using Klawr.ClrHost.Managed.SafeHandles;
using System;

namespace Klawr.UnrealEngine
{
    /// <summary>
    /// Base class of all UObject wrapper classes.
    /// </summary>
    public class UObject : IDisposable
    {
        private UObjectHandle _nativeObject;
        private bool _isDisposed = false;

        /// <summary>
        /// Handle to the native UObject instance.
        /// </summary>
        public UObjectHandle NativeObject
        {
            get { return _nativeObject; }
        }
		
        /// <summary>
        /// Construct a new instance from the given native UObject instance.
        /// </summary>
        /// <param name="nativeObject">Handle to a native UObject instance.</param>
        public UObject(UObjectHandle nativeObject)
        {
            _nativeObject = nativeObject;
        }

        /// <summary>
        /// Convert a UObject to a UObjectHandle (which contains a pointer to the native UObject).
        /// </summary>
        /// <param name="obj">UObject instance to convert, or null.</param>
        /// <returns>UObjectHandle instance.</returns>
        public static explicit operator UObjectHandle(UObject obj)
        {
            return (obj != null) ? obj.NativeObject : UObjectHandle.Null;
        }

        /// <summary>
        /// Get the UClass instance for this object.
        /// </summary>
        /// <returns>A UClass instance.</returns>
        public static UClass StaticClass()
        {
            return (UClass)typeof(UObject);
        }

        /// <summary>
        /// Check if this object is of the specified UE type.
        /// </summary>
        /// <param name="baseClass">Expected UE type of this UObject instance.</param>
        /// <returns>true if the instance is of the specified UE type, false otherwise</returns>
        public bool IsA(UClass baseClass)
        {
            var thisClass = (UClass)this.GetType();
            if (thisClass == baseClass)
            {
                return true;
            }
            return thisClass.IsChildOf(baseClass);
        }

        /// <summary>
        /// Dispose of any unmanaged (and managed) resources.
        /// <remarks>It is recommended to dispose of any UObject instance as soon as it is no longer
        /// needed (rather that wait for the garbage collector to do it) so that Unreal Engine can 
        /// reclaim any resources taken up the wrapped native UObject instance.</remarks>
        /// </summary>
        /// <param name="isDisposing">false when called from the finalizer (in which case managed 
        /// resources must not be disposed of), true otherwise</param>
        protected virtual void Dispose(bool isDisposing)
        {
            if (!_isDisposed)
            {
                if (isDisposing)
                {
                    _nativeObject.Dispose();
                }
                _isDisposed = true;
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }
    }
}
