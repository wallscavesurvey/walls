// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WALLCSS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WALLCSS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WALLCSS_EXPORTS
#define WALLCSS_API __declspec(dllexport)
#else
#define WALLCSS_API __declspec(dllimport)
#endif

// This class is exported from the wallcss.dll
class WALLCSS_API Cwallcss {
public:
	Cwallcss(void);
	// TODO: add your methods here.
};

extern WALLCSS_API int nwallcss;

WALLCSS_API int fnwallcss(void);
