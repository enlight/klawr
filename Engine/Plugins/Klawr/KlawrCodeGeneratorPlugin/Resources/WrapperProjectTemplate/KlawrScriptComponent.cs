using Klawr.ClrHost.Interfaces;
using Klawr.ClrHost.Managed;
using Klawr.ClrHost.Managed.SafeHandles;

namespace Klawr.UnrealEngine
{
    public abstract class UKlawrScriptComponent : UActorComponent
    {
        private readonly long _instanceID;

        public long InstanceID
        {
            get { return _instanceID; }
        }

        public UKlawrScriptComponent(long instanceID, UObjectHandle nativeComponent)
            : base(nativeComponent)
        {
            _instanceID = instanceID;
        }

        public new static UClass StaticClass()
        {
            return (UClass)typeof(UKlawrScriptComponent);
        }

        protected virtual void OnRegister() { }
        protected virtual void OnUnregister() { }
        
        public virtual void InitializeComponent() { }
        public virtual void TickComponent(float deltaTime) { }
    }
}
