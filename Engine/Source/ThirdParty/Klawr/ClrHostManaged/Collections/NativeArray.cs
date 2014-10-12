//
// The MIT License (MIT)
//
// Copyright (c) 2014 Vadim Macagon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

using Klawr.ClrHost.Managed.SafeHandles;
using Klawr.UnrealEngine;
using System;
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Managed.Collections
{
    public interface INativeArray<T> : IDisposable
    {
        T this[int index] { get; set; }

        int Num();
        void Add(T item);
        void Reset(int newCapacity = 0);
        int Find(T item);
        void Insert(T item, int index);
        bool RemoveSingle(T item);
        void RemoveAt(int index);
    }

    /// <summary>
    /// A wrapper for a native UE TArray that is a member of a native UObject derived class.
    /// </summary>
    /// <typeparam name="T">Array element type.</typeparam>
    public abstract class NativeArrayPropertyBase<T> : INativeArray<T>
    {
        private bool _isDisposed = false;
        // the native UObject instance that owns the native TArray<T> that corresponds to this
        // Array<T> instance, while this handle is valid the native TArray<T> instance is valid
        private UObjectHandle _objectHandle;
        protected ArrayHandle NativeArrayHandle { get; private set; }

        public T this[int index]
        {
            get
            {
                return GetValue(index);
            }
            set
            {
                SetValue(index, value);
            }
        }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="objectHandle">Handle to the native object that owns the native array.</param>
        /// <param name="arrayHandle">Handle to the corresponding native array. The newly 
        /// constructed object will assume ownership of the handle and will dispose of it when
        /// it itself is disposed of.</param>
        public NativeArrayPropertyBase(UObjectHandle objectHandle, ArrayHandle arrayHandle)
        {
            _objectHandle = objectHandle;
            NativeArrayHandle = arrayHandle;
        }

        protected abstract T GetValue(int index);
        protected abstract void SetValue(int index, T item);
        public abstract int Find(T item);
        
        public int Num()
        {
            return ArrayUtils.Num(NativeArrayHandle);
        }

        public void Add(T item)
        {
            SetValue(ArrayUtils.Add(NativeArrayHandle), item);
        }

        public void Reset(int newCapacity = 0)
        {
            ArrayUtils.Reset(NativeArrayHandle, newCapacity);
        }
                
        public void Insert(T item, int index)
        {
            ArrayUtils.Insert(NativeArrayHandle, index);
            SetValue(index, item);
        }

        public bool RemoveSingle(T item)
        {
            var index = Find(item);
            if (index != -1)
            {
                RemoveAt(index);
                return true;
            }
            return false;
        }

        public void RemoveAt(int index)
        {
            ArrayUtils.RemoveAt(NativeArrayHandle, index);
        }

        /// <summary>
        /// Dispose of any unmanaged (and managed) resources.
        /// </summary>
        /// <param name="isDisposing">false when called from the finalizer (in which case managed 
        /// resources must not be disposed of), true otherwise</param>
        protected virtual void Dispose(bool isDisposing)
        {
            if (!_isDisposed)
            {
                if (isDisposing)
                {
                    NativeArrayHandle.Dispose();
                }
                _isDisposed = true;
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<bool> ]]> that is a member of a native UObject 
    /// derived class.
    /// </summary>
    public class BoolArrayProperty : NativeArrayPropertyBase<bool>
    {
        public BoolArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle) 
            : base(objectHandle, arrayHandle)
        {
        }

        protected override bool GetValue(int index)
        {
            return Marshal.ReadByte(ArrayUtils.GetRawPtr(NativeArrayHandle, index)) != 0;
        }

        protected override void SetValue(int index, bool item)
        {
            ArrayUtils.SetUInt8At(NativeArrayHandle, index, Convert.ToByte(item));
        }

        public override int Find(bool item)
        {
            return ArrayUtils.FindUInt8(NativeArrayHandle, Convert.ToByte(item));
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<uint8> ]]> that is a member of a native UObject 
    /// derived class.
    /// </summary>
    public class ByteArrayProperty : NativeArrayPropertyBase<byte>
    {
        public ByteArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle)
            : base(objectHandle, arrayHandle)
        {
        }

        protected override byte GetValue(int index)
        {
            return Marshal.ReadByte(ArrayUtils.GetRawPtr(NativeArrayHandle, index));
        }

        protected override void SetValue(int index, byte item)
        {
            ArrayUtils.SetUInt8At(NativeArrayHandle, index, item);
        }

        public override int Find(byte item)
        {
            return ArrayUtils.FindUInt8(NativeArrayHandle, item);
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<int16> ]]> that is a member of a native UObject 
    /// derived class.
    /// </summary>
    public class Int16ArrayProperty : NativeArrayPropertyBase<Int16>
    {
        public Int16ArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle)
            : base(objectHandle, arrayHandle)
        {
        }

        protected override Int16 GetValue(int index)
        {
            return Marshal.ReadInt16(ArrayUtils.GetRawPtr(NativeArrayHandle, index));
        }

        protected override void SetValue(int index, Int16 item)
        {
            ArrayUtils.SetInt16At(NativeArrayHandle, index, item);
        }

        public override int Find(Int16 item)
        {
            return ArrayUtils.FindInt16(NativeArrayHandle, item);
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<int32> ]]> that is a member of a native UObject 
    /// derived class.
    /// </summary>
    public class Int32ArrayProperty : NativeArrayPropertyBase<Int32>
    {
        public Int32ArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle)
            : base(objectHandle, arrayHandle)
        {
        }

        protected override Int32 GetValue(int index)
        {
            return Marshal.ReadInt32(ArrayUtils.GetRawPtr(NativeArrayHandle, index));
        }

        protected override void SetValue(int index, Int32 item)
        {
            ArrayUtils.SetInt32At(NativeArrayHandle, index, item);
        }

        public override int Find(Int32 item)
        {
            return ArrayUtils.FindInt32(NativeArrayHandle, item);
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<int64> ]]> that is a member of a native UObject 
    /// derived class.
    /// </summary>
    public class Int64ArrayProperty : NativeArrayPropertyBase<Int64>
    {
        public Int64ArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle)
            : base(objectHandle, arrayHandle)
        {
        }

        protected override Int64 GetValue(int index)
        {
            return Marshal.ReadInt64(ArrayUtils.GetRawPtr(NativeArrayHandle, index));
        }

        protected override void SetValue(int index, Int64 item)
        {
            ArrayUtils.SetInt64At(NativeArrayHandle, index, item);
        }

        public override int Find(Int64 item)
        {
            return ArrayUtils.FindInt64(NativeArrayHandle, item);
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<FString> ]]> and <![CDATA[ TArray<FName> ]]> 
    /// that is a member of a native UObject derived class.
    /// </summary>
    public class StringArrayProperty : NativeArrayPropertyBase<string>
    {
        public StringArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle)
            : base(objectHandle, arrayHandle)
        {
        }

        protected override string GetValue(int index)
        {
            return ArrayUtils.GetString(NativeArrayHandle, index);
        }

        protected override void SetValue(int index, string item)
        {
            ArrayUtils.SetStringAt(NativeArrayHandle, index, item);
        }

        public override int Find(string item)
        {
            return ArrayUtils.FindString(NativeArrayHandle, item);
        }
    }

    /// <summary>
    /// A wrapper for a native UE <![CDATA[ TArray<T*> ]]> that is a member of a native UObject 
    /// derived class, where T is UObject or any UObject-derived type.
    /// </summary>
    public class ObjectArrayProperty<T> : NativeArrayPropertyBase<T> where T : UObject
    {
        public ObjectArrayProperty(UObjectHandle objectHandle, ArrayHandle arrayHandle)
            : base(objectHandle, arrayHandle)
        {
        }

        protected override T GetValue(int index)
        {
            return (T)Activator.CreateInstance(
                typeof(T), new object[] { ArrayUtils.GetObject(NativeArrayHandle, index) }
            );
        }

        protected override void SetValue(int index, T item)
        {
            ArrayUtils.SetObjectAt(NativeArrayHandle, index, item.NativeObject);
        }

        public override int Find(T item)
        {
            return ArrayUtils.FindObject(NativeArrayHandle, item.NativeObject);
        }
    }
}
