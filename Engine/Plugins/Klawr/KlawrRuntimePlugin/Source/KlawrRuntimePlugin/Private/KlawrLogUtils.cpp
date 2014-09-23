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

#include "KlawrRuntimePluginPrivatePCH.h"
#include "KlawrNativeUtils.h"
#include "KlawrClrHost.h"

namespace Klawr {
	namespace LogUtils {
		
		void LogFatalError(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, Fatal, TEXT("%s"), message);
		}
		
		void LogError(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("%s"), message);
		}

		void LogWarning(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, Warning, TEXT("%s"), message);
		}

		void Display(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, Display, TEXT("%s"), message);
		}

		void Log(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("%s"), message);
		}
		
		void LogVerbose(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, Verbose, TEXT("%s"), message);
		}
		
		void LogVeryVerbose(const TCHAR* message)
		{
			UE_LOG(LogKlawrRuntimePlugin, VeryVerbose, TEXT("%s"), message);
		}

	} // namespace LogUtils

	LogUtilsProxy FNativeUtils::Log =
	{
		LogUtils::LogFatalError,
		LogUtils::LogError,
		LogUtils::LogWarning,
		LogUtils::Display,
		LogUtils::Log,
		LogUtils::LogVerbose,
		LogUtils::LogVeryVerbose,
	};

} // namespace Klawr