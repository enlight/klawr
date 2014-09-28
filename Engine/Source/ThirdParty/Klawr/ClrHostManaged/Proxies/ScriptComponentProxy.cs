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

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// A native proxy for a managed UKlawrScriptComponent instance.
    /// 
    /// This structure is filled in by managed code and passed to native code that can then invoke
    /// the delegates in this structure via function pointers.
    /// </summary>
    /// <remarks>The size and layout of this structure must remain identical to that of its native
    /// counterpart.</remarks>
    [ComVisible(true)]
    [Guid("E4378A9D-8E83-415F-B5CC-F6BE73F7137A")]
    [StructLayout(LayoutKind.Sequential)]
    public struct ScriptComponentProxy
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnComponentCreatedAction();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnComponentDestroyedAction();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnRegisterAction();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnUnregisterAction();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void InitializeComponentAction();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void TickComponentAction(float deltaTime);

        /// <summary>
        /// ID of the script component instance this proxy represents.
        /// </summary>
        public long InstanceID;

        /// <summary>
        /// Delegate instance encapsulating UKlawrScriptComponent.OnComponentCreated() override (may be null).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public OnComponentCreatedAction OnComponentCreated;

        /// <summary>
        /// Delegate instance encapsulating UKlawrScriptComponent.OnComponentDestroyed() override (may be null).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public OnComponentDestroyedAction OnComponentDestroyed;

        /// <summary>
        /// Delegate instance encapsulating UKlawrScriptComponent.OnRegister() override (may be null).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public OnRegisterAction OnRegister;
        
        /// <summary>
        /// Delegate instance encapsulating UKlawrScriptComponent.OnUnregister() override (may be null).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public OnUnregisterAction OnUnregister;

        /// <summary>
        /// Delegate instance encapsulating UKlawrScriptComponent.InitializeComponent() override (may be null).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public InitializeComponentAction InitializeComponent;
        
        /// <summary>
        /// Delegate instance encapsulating UKlawrScriptComponent.TickComponent() override (may be null).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public TickComponentAction TickComponent;
    };
}
