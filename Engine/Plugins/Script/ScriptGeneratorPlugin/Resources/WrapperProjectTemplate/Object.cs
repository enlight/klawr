using System;
using Klawr.ClrHost.Managed;

namespace Klawr.UnrealEngine
{
    public class UObject : IDisposable
    {
        protected UObjectHandle _nativeObject;
        private bool _isDisposed = false;

        public UObjectHandle NativeObject
        {
            get { return _nativeObject; }
        }
		
        public UObject(UObjectHandle nativeObject)
        {
            _nativeObject = nativeObject;
        }

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
