//-------------------------------------------------------------------------------
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
//-------------------------------------------------------------------------------
#pragma once

#include <tchar.h>

namespace Klawr {

/*
 * @brief Makes a copy of the given string, the resulting copy can be safely released by the CLR.
 *
 * The CLR will attempt to release any c-string that is returned to it from a native function after
 * it creates a corresponding managed string. However, it can only release the c-string if it was 
 * allocated on the correct heap. This function will allocate a copy of the passed in c-string on 
 * the same heap the CLR will attempt to release it from, this copy is what should be returned to 
 * the CLR instead of the original c-string.
 *
 * @return A copy of the c-string passed in.
 */
TCHAR* MakeStringCopyForCLR(const TCHAR* stringToCopy);

} // namespace Klawr