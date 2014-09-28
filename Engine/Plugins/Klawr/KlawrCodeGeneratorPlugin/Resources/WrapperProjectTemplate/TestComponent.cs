using Klawr.ClrHost.Interfaces;
using Klawr.ClrHost.Managed;

namespace Klawr.UnrealEngine
{
    public class TestComponent : UKlawrScriptComponent
    {
        public TestComponent(long instanceID, UObjectHandle nativeComponent)
            : base(instanceID, nativeComponent)
        {
        }

        // TODO: figure out how to make this work
        //public new static UClass StaticClass()
        //{
        //    return (UClass)typeof(TestComponent);
        //}

        public override void InitializeComponent()
        {
            var owner = base.GetOwner();
            var ownerClass = owner.GetActorClass();
        }
    }
}
