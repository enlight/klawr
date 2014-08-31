using Klawr.ClrHost.Interfaces;

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

        public virtual void OnComponentCreated() { }
        public virtual void OnComponentDestroyed() { }

        protected virtual void OnRegister() { }
        protected virtual void OnUnregister() { }
        
        public virtual void InitializeComponent() { }
        public virtual void TickComponent(float deltaTime) { }
    }
}
