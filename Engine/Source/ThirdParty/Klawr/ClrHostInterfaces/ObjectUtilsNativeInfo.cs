using System;
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Interfaces
{
    [ComVisible(true)]
    [Guid("06A91CEC-0B66-4DCC-B4AB-7DFF3F237F48")]
    [StructLayout(LayoutKind.Sequential)]
    public struct ObjectUtilsNativeInfo
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void RemoveObjectRefAction(IntPtr nativeObject);

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public RemoveObjectRefAction RemoveObjectRef;
    }
}
