using System;
using System.Runtime.InteropServices;
using Klawr.ClrHost.Managed;
using Klawr.ClrHost.Interfaces;
using Klawr.ClrHost.Managed.SafeHandles;

namespace Klawr.UnrealEngine
{
    // NOTE: IScriptObject should probably be considered deprecated in favor of UKlawrScriptComponent
    public class TestActor : AActorScriptObject
    {
        public TestActor(long instanceID, UObjectHandle nativeObject) : base(instanceID, nativeObject)
        {
            Console.WriteLine("TestActor()");
        }

        public override void BeginPlay()
        {
            Console.WriteLine("BeginPlay()");
            // FIXME: AActor::K2_GetWorld() is no more, and AActor::GetWorld() is not exposed to
            //        Blueprints at all... may need to expose UObject::GetWorld() manually. 
            /*
			var world = K2_GetWorld();
			var worldClass = UWorld.StaticClass();
            bool isWorldClass = world.IsA(worldClass);
            var anotherWorldClass = (UClass)typeof(UWorld);
            bool isSameClass = worldClass == anotherWorldClass;
            bool isWorldClassDerivedFromObjectClass = worldClass.IsChildOf(UObject.StaticClass());
            world.Dispose();
            */
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
