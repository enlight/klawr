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

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace Klawr.ClrHost.Managed.Collections
{
    /// <summary>
    /// A generic IList/IReadOnlyList implementation that uses a native UE TArray 
    /// as a backing store.
    /// </summary>
    /// <typeparam name="T">Any interoperable type.</typeparam>
    public class ArrayList<T> : IList<T>, IReadOnlyList<T>, IDisposable
    {
        private bool _isDisposed = false;
        private INativeArray<T> _nativeArray;
        // tracks how many times the array has been modified, 
        // this allows enumerators to detect cases where the user modifies the array while
        // enumerating it (which isn't allowed)... of course this won't do much good if
        // native code modifies the array
        private int _modificationCount;

        #region Properties

        public int Count
        {
            get { return _nativeArray.Num(); }
        }

        public bool IsReadOnly
        {
            get { return false; }
        }

        public T this[int index]
        {
            get
            {
                return _nativeArray[index];
            }
            set
            {
                _nativeArray[index] = value;
                ++_modificationCount;
            }
        }

        #endregion

        /// <summary>
        /// Construct a new instance from a wrapped UE TArray.
        /// </summary>
        /// <param name="proxy">Wrapped UE TArray instance. The newly constructed object assumes
        /// ownership of the wrapper and will dispose of it when it itself is disposed of.</param>
        public ArrayList(INativeArray<T> proxy)
        {
            _nativeArray = proxy;
        }

        #region Methods

        public void Add(T item)
        {
            _nativeArray.Add(item);
            ++_modificationCount;
        }

        public void Clear()
        {
            _nativeArray.Reset();
            ++_modificationCount;
        }

        public bool Contains(T item)
        {
            return _nativeArray.Find(item) != -1;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            if ((arrayIndex < 0) || (arrayIndex >= array.Length))
            {
                throw new ArgumentOutOfRangeException("arrayIndex");
            }

            if ((array.Length - arrayIndex) < Count)
            {
                throw new ArgumentException("array is too small!");
            }

            var currentIndex = arrayIndex;
            foreach (T item in this)
            {
                if (currentIndex < array.Length)
                {
                    array[currentIndex++] = item;
                }
                else
                {
                    break;
                }
            }
        }

        IEnumerator<T> IEnumerable<T>.GetEnumerator()
        {
            return new Enumerator(this);
        }

        public int IndexOf(T item)
        {
            return _nativeArray.Find(item);
        }

        public void Insert(int index, T item)
        {
            _nativeArray.Insert(item, index);
            ++_modificationCount;
        }

        public bool Remove(T item)
        {
            ++_modificationCount;
            return _nativeArray.RemoveSingle(item);
        }

        public void RemoveAt(int index)
        {
            ++_modificationCount;
            _nativeArray.RemoveAt(index);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return new Enumerator(this);
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
                    _nativeArray.Dispose();
                }
                _isDisposed = true;
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            int itemIndex = 0;
            stringBuilder.Append('[');
            foreach (var item in this)
            {
                if (itemIndex > 0)
                {
                    stringBuilder.Append(", ");
                }
                stringBuilder.Append(item.ToString());
                ++itemIndex;
            }
            stringBuilder.Append(']');
            return stringBuilder.ToString();
        }
        #endregion

        private class Enumerator : IEnumerator<T>, IEnumerator
        {
            private ArrayList<T> _arrayList;
            private int _index;
            private int _modificationCount;
            private T _current;

            internal Enumerator(ArrayList<T> arrayList)
            {
                _arrayList = arrayList;
                _index = 0;
                _modificationCount = arrayList._modificationCount;
                _current = default(T);
            }

            public T Current
            {
                get { return _current; }
            }

            object IEnumerator.Current
            {
                get { return _current; }
            }

            public bool MoveNext()
            {
                if (_modificationCount == _arrayList._modificationCount)
                {
                    if (_index < _arrayList.Count)
                    {
                        _current = _arrayList[_index++];
                        return true;
                    }
                    else
                    {
                        _current = default(T);
                        return false;
                    }
                }
                else
                {
                    throw new InvalidOperationException("Enumerator has been invalidated!");
                }
            }

            public void Reset()
            {
                // only needs to be implemented for COM interoperability
                throw new NotImplementedException();
            }

            public void Dispose() { }
        }
    }
}
