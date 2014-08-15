using System;
using System.Runtime.InteropServices;

namespace Klawr.UnrealEngine
{
    public class TestActor : AActorScriptObject
    {
        public TestActor(long instanceID, IntPtr nativeObject) : base(instanceID, nativeObject)
        {
            Console.WriteLine("TestActor()");
            bool isHidden = base.bHidden;
            var forward = base.GetActorForwardVector();
        }

        public override void BeginPlay()
        {
            Console.WriteLine("BeginPlay()");
        }

        public override void Tick(float deltaTime)
        {
            Console.WriteLine("Tick()");
        }

        public override void Destroy()
        {
            Console.WriteLine("Destroy()");
        }
    }
}
