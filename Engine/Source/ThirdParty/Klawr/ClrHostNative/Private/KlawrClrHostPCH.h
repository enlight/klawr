#pragma once

// This is the include file for standard system include files, or project specific include files
// that are used frequently, but are changed infrequently.

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <tchar.h>
//#include <string>

//#ifdef _UNICODE
//typedef std::wstring tstring;
//#else
//typedef std::string tstring;
//#endif // _UNICODE
//# 

// these must be identical to UE types of the same name
typedef unsigned char      uint8;  // 8-bit unsigned
typedef unsigned short int uint16; // 16-bit unsigned
typedef unsigned int       uint32; // 32-bit unsigned
typedef unsigned long long uint64; // 64-bit unsigned

typedef signed char      int8;  // 8-bit signed
typedef signed short int int16; // 16-bit signed
typedef signed int       int32; // 32-bit signed
typedef signed long long int64; // 64-bit signed

#include "DebugMacros.h"

#include <mscoree.h>

// TODO: reference additional headers here
