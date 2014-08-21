using System;
using System.Runtime.InteropServices;
using Klawr.ClrHost.Managed;

namespace Klawr.UnrealEngine
{
    public class TestActor : AActorScriptObject
    {
        public TestActor(long instanceID, UObjectHandle nativeObject) : base(instanceID, nativeObject)
        {
            Console.WriteLine("TestActor()");
        }

        public override void BeginPlay()
        {
            Console.WriteLine("BeginPlay()");
			var world = K2_GetWorld();
            world.Dispose();
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
