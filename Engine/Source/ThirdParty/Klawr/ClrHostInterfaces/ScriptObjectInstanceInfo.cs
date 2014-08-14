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

using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Interfaces
{
    /// <summary>
    /// Contains native/managed interop information for a ScriptObject instance.
    /// </summary>
    [ComVisible(true)]
    [GuidAttribute("B4F3A010-B7B6-42D6-82F9-331C56487AA2")]
    [StructLayout(LayoutKind.Sequential)]
    public struct ScriptObjectInstanceInfo
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void BeginPlayAction();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void TickAction(float deltaTime);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DestroyAction();

        /// <summary>
        /// ID of a ScriptObject instance.
        /// </summary>
        public long InstanceID;
        /// <summary>
        /// Delegate instance encapsulating ScriptObject.BeginPlay().
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public BeginPlayAction BeginPlay;
        /// <summary>
        /// Delegate instance encapsulating ScriptObject.Tick().
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public TickAction Tick;
        /// <summary>
        /// Delegate instance encapsulating ScriptObject.Destroy().
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public DestroyAction Destroy;
    };
}
