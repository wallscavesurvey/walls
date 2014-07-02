
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the RESELLIB_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// RESELLIB_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifdef RESELLIB_DLL
#ifdef RESELLIB_EXPORTS
#define RESELLIB_API __declspec(dllexport)
#else
#define RESELLIB_API __declspec(dllimport)
#endif
#else
#define RESELLIB_API
#endif

#define RESEL_MASK (3+4+8+32)
#define RESEL_SHOWKEY_MASK 3
#define RESEL_SHOWNO_MASK 4
#define RESEL_OR_AUTHOR_MASK 8
#define RESEL_BLUE_MASK 16
#define RESEL_SORT_MASK 32

//extern RESELLIB_API int resel_Flags;

RESELLIB_API int re_Select(int argc,char **argv);
RESELLIB_API char *re_Errstr(void);
RESELLIB_API void re_SetFlags(int resel_Flags);
