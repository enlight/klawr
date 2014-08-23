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

using Klawr.ClrHost.Interfaces;
using Klawr.ClrHost.Managed;
using System;
using System.Collections.Generic;

namespace Klawr.UnrealEngine
{
    /// <summary>
    /// Wraps a native UClass instance.
    /// <remarks>Reference equality can be used to test if two UClass instances represent the same
    /// UE type, just like with native UClass instances.</remarks>
    /// </summary>
    public class UClass : UObject
    {
        /// <summary>
        /// Name of the class, excluding any U/A prefix.
        /// </summary>
        public string Name
        {
            get;
            private set;
        }

        // FIXME: it may be better to fill up the cache all at once, rather than doing so lazily 
        //        (requiring a managed/native/managed transition every time)
        private static Dictionary<Type, UClass> _typeClassCache = new Dictionary<Type, UClass>();

        /// <summary>
        /// Maps native UClass instances to managed UClass instances.
        /// </summary>
        private static Dictionary<UObjectHandle /* native UClass */, UClass> _handleClassCache = 
            new Dictionary<UObjectHandle, UClass>();

        /// <summary>
        /// This constructor is private because new instances are created as needed when casting 
        /// from UObjectHandle to UClass.
        /// </summary>
        /// <param name="nativeObject"></param>
        private UClass(UObjectHandle nativeObject, string className)
            : base(nativeObject)
        {
            Name = className;
        }

        /// <summary>
        /// Convert a UObject-derived type to a UClass.
        /// </summary>
        /// <exception cref="InvalidCastException">Thrown if the passed in type is not derived from 
        /// UObject.</exception>
        /// <exception cref="NullReferenceException"
        /// <param name="objectType">A type derived from UObject.</param>
        /// <returns></returns>
        public static explicit operator UClass(Type objectType)
        {
            // FIXME: Couldn't figure out how to turn this into a generic method to constrain
            // objectType to UObject(s) :(
            if (!typeof(UObject).IsAssignableFrom(objectType))
            {
                throw new InvalidCastException(
                    String.Format("Can't cast {0} to UClass!", objectType.Name)
                );
            }

            UClass clazz;
            if (!_typeClassCache.TryGetValue(objectType, out clazz))
            {
                // internally UE strips the U/A prefix from C++ class names, so when looking for
                // a class by name we must do the same
                clazz = (UClass)ObjectUtils.GetClassByName(objectType.Name.TrimStart('U', 'A'));
                if (clazz != null)
                {
                    _typeClassCache.Add(objectType, clazz);    
                }
            }
            return clazz;
        }

        /// <summary>
        /// Convert a native UClass to a managed UClass.
        /// </summary>
        /// <param name="nativeClass">Pointer to a native UClass instance.</param>
        /// <returns>A managed UClass instance, or null.</returns>
        public static explicit operator UClass(UObjectHandle nativeClass)
        {
            UClass clazz = null;
            if (!nativeClass.IsInvalid)
            {
                if (!_handleClassCache.TryGetValue(nativeClass, out clazz))
                {
                    clazz = new UClass(nativeClass, ObjectUtils.GetClassName(nativeClass));
                    _handleClassCache.Add(nativeClass, clazz);
                }
            }
            return clazz;
        }

        /// <summary>
        /// Get the UClass instance for this object.
        /// </summary>
        /// <returns>A UClass instance.</returns>
        public new static UClass StaticClass()
        {
            return (UClass)typeof(UClass);
        }

        /// <summary>
        /// Check if this UE type is derived from the specified UE type.
        /// </summary>
        /// <param name="baseClass">A base class.</param>
        /// <returns>true is this class is derived from baseClass, false otherwise</returns>
        public bool IsChildOf(UClass baseClass)
        {
            return ObjectUtils.IsClassChildOf(this.NativeObject, baseClass.NativeObject);
        }

        public override string ToString()
        {
            return Name + " UClass";
        }
    }

    /// <summary>
    /// Generic class used to pass around UClass instances while enforcing type safety.
    /// </summary>
    /// <typeparam name="TBaseClass">A UObject-derived type.</typeparam>
    public class TSubclassOf<TBaseClass> where TBaseClass : UObject
    {
        /// <summary>
        /// The UClass instance currently contained in this object, may be null.
        /// </summary>
        public UClass Class
        {
            get;
            private set;
        }

        /// <summary>
        /// Construct a TSubclassOf instance encapsulating the given UClass instance.
        /// </summary>
        /// <param name="clazz">UClass instance to assign to this TSubclassOf instance, or null.</param>
        public TSubclassOf(UClass clazz)
        {
            if (clazz != null)
            {
                if (!clazz.IsChildOf((UClass)typeof(TBaseClass)))
                {
                    throw new ArgumentException(
                        String.Format(
                            "{0} is not a subclass of {1}!",
                            clazz.Name, typeof(TBaseClass).Name
                        ), 
                        "clazz"
                    );
                }
            }
            Class = clazz;
        }

        /// <summary>
        /// Convert a UClass to a TSubclassOf.
        /// </summary>
        /// <exception cref="InvalidCastException">Thrown if the UClass instance doesn't correspond
        /// to a type derived from the TBaseClass generic parameter.</exception>
        /// <param name="clazz">The UClass instance to be converted, or null.</param>
        /// <returns>A TSubclassOf instance encapsulating the passed in UClass instance, or null.</returns>
        public static implicit operator TSubclassOf<TBaseClass>(UClass clazz)
        {
            return (clazz != null) ? new TSubclassOf<TBaseClass>(clazz) : null;
        }

        /// <summary>
        /// Convert a TSubclassOf to a UClass.
        /// </summary>
        /// <param name="subclass">The TSubclassOf instance to be converted, or null.</param>
        /// <returns>The UClass instance encapsulated by the TSubclassOf instance, or null.</returns>
        public static implicit operator UClass(TSubclassOf<TBaseClass> subclass)
        {
            return (subclass != null) ? subclass.Class : null;
        }

        /// <summary>
        /// Convert a TSubclassOf to a UObjectHandle.
        /// </summary>
        /// <param name="subclass">TSubclassOf instance to convert, or null.</param>
        /// <returns>UObjectHandle instance.</returns>
        public static explicit operator UObjectHandle(TSubclassOf<TBaseClass> subclass)
        {
            return (subclass != null) ? (UObjectHandle)subclass.Class : UObjectHandle.Null;
        }
    }
}
