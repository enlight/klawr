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
using System.Runtime.InteropServices;

namespace Klawr.ClrHost.Interfaces
{
    /// <summary>
    /// Contains delegates encapsulating native logging functions.
    /// </summary>
    [ComVisible(true)]
    [Guid("40D06EA1-0EC2-4B01-951C-ED5081FCBCB8")]
    [StructLayout(LayoutKind.Sequential)]
    public struct LogUtilsProxy
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public delegate void LogAction(string text);

        /// <summary>
        /// Print an error to the UE4 console and log file, then crash (even if logging is disabled).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction LogFatalError;

        /// <summary>
        /// Print an error to the UE4 console and log file.
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction LogError;

        /// <summary>
        /// Print a warning to the UE4 console and log file.
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction LogWarning;

        /// <summary>
        /// Print a message to the UE4 console and log file.
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction Display;

        /// <summary>
        /// Print a message to the log file, but not to the UE4 console.
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction Log;

        /// <summary>
        /// Print a verbose message to a log file (if Verbose logging is enabled).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction LogVerbose;

        /// <summary>
        /// Print a verbose message to a log file (if VeryVerbose logging is enabled).
        /// </summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public LogAction LogVeryVerbose;
    }
}
