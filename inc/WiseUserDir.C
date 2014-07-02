#include <windows.h>
#include "wisedll.h"

void GetVariable(LPDLLCALLPARAMS lpDllParams,char *szVariable,char *szValue);
void SetVariable(LPDLLCALLPARAMS lpDllParams,char *szVariable,char *szValue);

int CALLBACK LibMain(HINSTANCE hInst, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{
  return(1);
}

// GetCPU: Uses the new Win32 API call to get the CPU type including
//         the Pentium processor. You must pass the name of the variable
//         to save the name of the cpu into in the parameter field.

__declspec(dllexport) BOOL CALLBACK GetCPU(LPDLLCALLPARAMS lpDllParams)
{
   SYSTEM_INFO SystemInfo;
   GetSystemInfo(&SystemInfo);
   if (lpDllParams->lpszParam) {
      switch (SystemInfo.dwProcessorType) {
       case PROCESSOR_INTEL_386: SetVariable(lpDllParams,lpDllParams->lpszParam,"I386"); break;
       case PROCESSOR_INTEL_486: SetVariable(lpDllParams,lpDllParams->lpszParam,"I486"); break;
       case PROCESSOR_INTEL_PENTIUM: SetVariable(lpDllParams,lpDllParams->lpszParam,"PENTIUM"); break;
       case PROCESSOR_MIPS_R4000: SetVariable(lpDllParams,lpDllParams->lpszParam,"R4000"); break;
       case PROCESSOR_ALPHA_21064: SetVariable(lpDllParams,lpDllParams->lpszParam,"ALPHA"); break;
      }
   }
   return FALSE;
}

// GetVariable: Returns the value of a variable.
//
// lpDllParams  Parameter structure passed from Wise Installation
// szVariable   Name of the variable (without %'s) to get value for
// szValue      String that will hold the variables value

void GetVariable(LPDLLCALLPARAMS lpDllParams,char *szVariable,char *szValue)
{
   WORD i;
   char szVar[32];

   *szVar = '%';
   lstrcpy(&szVar[1],szVariable);
   lstrcat(szVar,"%");
   for (i = 0 ; (i < lpDllParams->wCurrReps) &&
      (lstrcmp(&lpDllParams->lpszRepName[i * lpDllParams->wRepNameWidth],szVar) != 0) ; i++) ;
   if (i < lpDllParams->wCurrReps) {
      lstrcpy(szValue,&lpDllParams->lpszRepStr[i * lpDllParams->wRepStrWidth]);
   } else *szValue = '\0';
}

// SetVariable: Sets/Creates a variable.
//
// lpDllParams  Parameter structure passed from Wise Installation
// szVariable   Name of the variable (without %'s) to set value for
// szValue      String that contains the variables new value

void SetVariable(LPDLLCALLPARAMS lpDllParams,char *szVariable,char *szValue)
{
   WORD i;
   char szVar[32];

   *szVar = '%';
   lstrcpy(&szVar[1],szVariable);
   lstrcat(szVar,"%");
   for (i = 0 ; (i < lpDllParams->wCurrReps) &&
      (lstrcmp(&lpDllParams->lpszRepName[i * lpDllParams->wRepNameWidth],szVar) != 0) ; i++) ;
   if (i >= lpDllParams->wCurrReps) {
      if (i >= lpDllParams->wMaxReplaces) return; // Too many variables
      lstrcpy(&lpDllParams->lpszRepName[i * lpDllParams->wRepNameWidth],szVar);
      lpDllParams->wCurrReps++;
   }
   lstrcpy(&lpDllParams->lpszRepStr[i * lpDllParams->wRepStrWidth],szValue);
}
