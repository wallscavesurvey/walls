#pragma once
//wallisef.h --
#define IMP_DLL_NAME "wallsef.dll"
#define DLL_VERSION 4.0f
#define DLL_VERSION_STR "4.0"
struct SEF_PARAMS
{
	float version;
	UINT flags;
	char chrs_torepl[6];
	char chrs_repl[6];
};
typedef struct SEF_PARAMS *PSEF_PARAMS;
