#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <WinSDKVer.h>

#ifndef WINVER
#define WINVER _WIN32_WINNT_VISTA
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA	// Changed from 0x0501 which Windows XP or later (AFTER tooltip structure in CWallsMapView was shortened!!!!).
#endif						

#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE90	// Change this to the appropriate value to target IE 5.0 or later.
#endif

