#ifndef HG_PCUTIL_PCSTUFF
#define HG_PCUTIL_PCSTUFF



#include "../stdafx.h"

// PC stuff ...

typedef struct  
    {
      HINSTANCE hInstance;
      HWND hWnd;
      PROCESS_INFORMATION pi;
      DWORD retStat_;
    } EXECAPPINFO, *LPEXECAPPINFO;


extern BOOL AppExec(const char* cmdLine, LPEXECAPPINFO pInfo, int nCmdShow = SW_SHOW);
extern BOOL IsAppRunning(LPEXECAPPINFO pInfo);
extern BOOL KillApp(LPEXECAPPINFO pInfo, BOOL dontQuit = FALSE, UINT fuRemove = PM_REMOVE);
extern int  WaitAppExec(const char* cmdLine, int nCmdShow = SW_SHOW);
extern void SpawnApp(const char *appName, int nCmdShow = SW_SHOW);

extern BOOL WINAPI DoesFileExist(LPTSTR lpszFile);

extern BOOL writeBinFile(const char *buf, int len, const char *name);

extern void InfoMsg(int strId);
extern void InfoMsg(const char *s);
extern void ErrorMsg(int strId);
extern void ErrorMsg(const char *s);
extern void ErrorMsg(const char *s, int i);
extern void ErrorMsg(const char *s1, const char *s2);
extern void MyExit(int i);
extern void MyExit(const char *s);

extern char *getpass( const char *__prompt, unsigned int Count = 8);

extern void MyExit();

extern int  DoNothing(BOOL sleep = TRUE);

BOOL findFileType(  
                    const char *magicFileName,          // magic file
                    const char *testFileBuf, int bLen,  // first bLen bytes of file to examine
                    char *outBuf, int outBufLen         // output buffer 
                 );

#endif
