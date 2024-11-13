#pragma once

#define KDEXT_64BIT
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <cstring>
#include <codecvt>
#include <vector>
#include <iostream>
#include <format>
#include <string>
#include <new>
#include <winapifamily.h>
#include <sdkddkver.h>
#include <DbgEng.h>
#include <DbgHelp.h>
#include <DbgModel.h>
#include <Windows.h>
#include <strsafe.h>
#include <Psapi.h>
#include <sstream>