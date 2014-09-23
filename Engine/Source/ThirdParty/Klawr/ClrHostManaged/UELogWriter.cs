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
using System.Globalization;
using System.IO;
using System.Text;

namespace Klawr.ClrHost.Managed
{
    /// <summary>
    /// Writes text to the UE console and log file.
    /// </summary>
    class UELogWriter : TextWriter
    {
        private StringBuilder _stringBuilder = new StringBuilder();

        public UELogWriter() 
            : this(CultureInfo.CurrentCulture)
        {
        }

        public UELogWriter(IFormatProvider formatProvider)
            : base(formatProvider)
        {
        }

        public override Encoding Encoding
        {
            get { return Encoding.Unicode; }
        }

        /// <summary>
        /// Output any buffered text.
        /// </summary>
        public override void Flush()
        {
            if (_stringBuilder.Length > 0)
            {
                LogUtils.Display(_stringBuilder.ToString());
                _stringBuilder.Clear();
            }
        }

        /// <summary>
        /// Output any buffered text if it ends with a line terminator.
        /// </summary>
        private void FlushOnNewLine()
        {
            // look for the line terminator at the end of the buffered text
            var terminatorChars = base.NewLine.ToCharArray();
            int terminatorStart = _stringBuilder.Length - terminatorChars.Length;
            if (terminatorStart >= 0)
            {
                int terminatorIndex = terminatorChars.Length - 1;
                for (int i = _stringBuilder.Length - 1; i >= terminatorStart; --i, --terminatorIndex)
                {
                    if (_stringBuilder[i] != terminatorChars[terminatorIndex])
                    {
                        break;
                    }
                    else if (terminatorIndex == 0)
                    {
                        // LogUtils.Display() will automatically append a line terminator, 
                        // so strip away the line terminator that's already there to avoid a blank
                        // line the user didn't expect
                        LogUtils.Display(_stringBuilder.ToString(0, terminatorStart));
                        _stringBuilder.Clear();
                    }
                }
            }
        }

        /// <summary>
        /// Write out a single character.
        /// </summary>
        /// <remarks>Output is buffered until a line terminator is encountered.</remarks>
        public override void Write(char value)
        {
            _stringBuilder.Append(value);
            FlushOnNewLine();
        }

        /// <summary>
        /// Write out a range of characters.
        /// </summary>
        /// <remarks>Output is buffered unless it ends with a line terminator.</remarks>
        public override void Write(char[] buffer, int index, int count)
        {
            _stringBuilder.Append(buffer, index, count);
            FlushOnNewLine();
        }

        /// <summary>
        /// Write out a line terminator.
        /// </summary>
        /// <remarks>Any buffered text will be output first, followed by a line terminator.</remarks>
        public override void WriteLine()
        {
            // LogUtils.Display() will automatically append a line terminator, so there's no need
            // to add a line terminator at this point
            LogUtils.Display(_stringBuilder.ToString());
            _stringBuilder.Clear();
        }

        /// <summary>
        /// Write out a string followed by a line terminator.
        /// </summary>
        public override void WriteLine(string value)
        {
            Write(value);
            WriteLine();
        }
    }
}
