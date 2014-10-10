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

namespace Klawr.ClrHost.Managed.SafeHandles
{
    /// <summary>
    /// Encapsulates a native UE TArray wrapper pointer and takes care of properly disposing of it.
    /// </summary>
    public class ArrayHandle : SafeHandle
    {
        /// <summary>
        /// Encapsulates a native null pointer.
        /// </summary>
        public static readonly ArrayHandle Null = new ArrayHandle(IntPtr.Zero, false);

        /// <summary>
        /// Construct a new handle.
        /// </summary>
        /// <remarks>This constructor is used by the interop code, user code should not invoke it.</remarks>
        public ArrayHandle()
            : base(IntPtr.Zero, true)
        {
        }

        /// <summary>
        /// Construct a new handle to a native TArray wrapper instance.
        /// </summary>
        /// <param name="nativePtr">Pointer to a native array wrapper instance.</param>
        /// <param name="ownsHandle">true if the handle should release the native object when 
        /// disposed, false otherwise</param>
        public ArrayHandle(IntPtr nativePtr, bool ownsHandle)
            : base(IntPtr.Zero, ownsHandle)
        {
            SetHandle(nativePtr);
        }

        public override bool IsInvalid
        {
            get { return handle == IntPtr.Zero; }
        }

        protected override bool ReleaseHandle()
        {
            ArrayUtils.Destroy(handle);
            handle = IntPtr.Zero;
            return true;
        }

        /// <summary>
        /// Equality is determined by the equality of the encapsulated IntPtr.
        /// </summary>
        /// <param name="obj">ArrayHandle to compare with.</param>
        /// <returns>true if specified ArrayHandle is equal to this one, false otherwise</returns>
        public override bool Equals(object obj)
        {
            return handle.Equals(obj);
        }

        public override int GetHashCode()
        {
            return handle.GetHashCode();
        }
    }
}
