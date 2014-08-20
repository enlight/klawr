using System;
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Encapsulates a native UObject pointer and takes care of properly disposing of it.
    /// </summary>
    public class UObjectHandle : SafeHandle
    {
        public UObjectHandle() 
            : base(IntPtr.Zero, true)
        {
        }

        /// <summary>
        /// Construct a new handle to a native UObject instance.
        /// 
        /// This constructor is currently only used to construct IScriptObject(s).
        /// </summary>
        /// <param name="nativeObject">Pointer to a native UObject instance.</param>
        /// <param name="ownsHandle">true if the handle should release the native object when 
        /// disposed, false otherwise</param>
        internal UObjectHandle(IntPtr nativeObject, bool ownsHandle)
            : base(IntPtr.Zero, ownsHandle)
        {
            SetHandle(nativeObject);
        }

        public override bool IsInvalid
        {
            get { return handle == IntPtr.Zero; }
        }

        protected override bool ReleaseHandle()
        {
            // TODO: make sure no exceptions escape from here
            var manager = AppDomain.CurrentDomain.DomainManager as EngineAppDomainManager;
            var objectUtils = manager.GetObjectUtils();
            objectUtils.ReleaseObject(handle);
            handle = IntPtr.Zero;
            return true;
        }
    }
}
