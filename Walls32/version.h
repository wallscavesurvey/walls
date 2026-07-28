// version.h
#pragma once

#define STRINGIFY_HELPER(x)		#x
#define STRINGIFY(x)			STRINGIFY_HELPER(x)

#ifndef VER_MAJOR
#define VER_MAJOR				2
#endif
#ifndef VER_MINOR
#define VER_MINOR				2
#endif
#ifndef VER_PATCH
#define VER_PATCH				1
#endif
#ifndef VER_BUILD
#define VER_BUILD				0
#endif

#ifndef BUILD_DATE
#define BUILD_DATE				"2026-06-15"
#endif

#define VERSION					STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_PATCH) "." STRINGIFY(VER_BUILD)

