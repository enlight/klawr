using System.Runtime.InteropServices;

namespace Klawr.UnrealEngine
{
    [StructLayout(LayoutKind.Sequential)]
    public struct FVector2D
    {
        public float X;
        public float Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct FVector
    {
        public float X;
        public float Y;
        public float Z;
    }

    // TODO: The FVector4 class is 16-byte aligned, but the mirror FVector4 USTRUCT is not,
    //       which one of those is this struct actually going to correspond to? And does it matter?
    [StructLayout(LayoutKind.Sequential)]
    public struct FVector4
    {
        public float X;
        public float Y;
        public float Z;
        public float W;
    }

    // TODO: Check the alignment is correct
    [StructLayout(LayoutKind.Sequential)]
    public struct FQuat
    {
        public float X;
        public float Y;
        public float Z;
        public float W;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct FTransform
    {
        public FQuat Rotation;
        public FVector Translation;
        public FVector Scale3D;
    }

    // TODO: Check the alignment is correct
    [StructLayout(LayoutKind.Sequential)]
    public struct FColor
    {
        public byte B, G, R, A;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct FLinearColor
    {
        public float B, G, R, A;
    }
}
