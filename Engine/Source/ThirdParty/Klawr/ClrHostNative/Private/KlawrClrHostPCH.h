#pragma once

// This is the include file for standard system include files, or project specific include files
// that are used frequently, but are changed infrequently.

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <tchar.h>
#include <string>

#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif // _UNICODE

#include <assert.h>

// TODO: reference additional headers here
