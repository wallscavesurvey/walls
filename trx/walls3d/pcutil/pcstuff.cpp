#include "..\\stdafx.h"
#include "pcstuff.h"
#include <stdio.h>
#include <process.h>
#include <string.h>
#include "mfcutil.h"

static BOOL wm_quit = FALSE;

#if defined(_DEBUG) & defined(_VERBOSE)
#define new DEBUG_NEW
#endif

extern void __cdecl cleanup(int i);

int DoNothing(BOOL sleep)
{
    MSG M;

    if (sleep)
        Sleep(1);

	if (wm_quit)
		return -1;

#ifndef _CONSOLE
    RestoreBusyCursor();
#endif

    if (PeekMessage(&M, 0, 0, 0, PM_REMOVE))
    {
        if ( M.message == WM_QUIT )
        {
			wm_quit = TRUE;		// store this important information !
            return -1;
        }
        TranslateMessage(&M);
        DispatchMessage(&M);
    }
    return 0;
} // DoNothing


// this function writes a buffer filled with binary data to a file
//
BOOL writeBinFile(const char *buf, int len, const char *name)
{
    FILE* f = fopen(name,"wb");
    if (f)
    {
        int ret;
        int len16, ret16;

        ret = 0;

        do
        {
            if (len > 32767)      // write in blocks of 32767
                len16 = 32767;
            else
                len16 = len;

            ret16 = fwrite(buf, 1, len16, f);

            if (ret16>0)
            {
                ret += ret16;
                buf += ret16;
                len -= ret16;
            }
            else
            {   
                ret = ret16;
                len = 0;    
            }
        } while (len>0);
        fclose(f);

        return TRUE;    // everything ok !
    }
    else
        return FALSE;   // error occured !
} // writeBinFile


void InfoMsg(int strId)
{
    CString tmpStr;
    tmpStr.LoadString(strId);
    MessageBox(GetFocus(), tmpStr, "Information", MB_OK | MB_ICONINFORMATION);
}


void InfoMsg(const char *s)
{
    MessageBox(GetFocus(), s, "Information", MB_OK | MB_ICONINFORMATION);
}


void ErrorMsg(int strId)
{
    CString tmpStr;
    tmpStr.LoadString(strId);
    MessageBox(GetFocus(), tmpStr, "Error!", MB_OK | MB_ICONEXCLAMATION);
}


void ErrorMsg(const char *s)
{
    MessageBox(GetFocus(), s, "Error!", MB_OK | MB_ICONEXCLAMATION);
}


void ErrorMsg(const char *s, int i)
{
  char buffer[1024];

  sprintf(buffer,"%s%i", s, i);
  MessageBox(GetFocus(), buffer, "Error!", MB_OK | MB_ICONEXCLAMATION);
}


void ErrorMsg(const char *s1, const char *s2)
{
  char buffer[1024];

  sprintf(buffer,"%s%s", s1, s2);
  MessageBox(GetFocus(), buffer, "Error!", MB_OK | MB_ICONEXCLAMATION);
}


void MyExit()
{
  ErrorMsg("!! A FATAL ERROR occurred - the program will exit !!");
  cleanup(-1);
}


void MyExit(int i)
{
  ErrorMsg("!! A FATAL ERROR occurred - the program will exit !!\nError:", i);
  cleanup(-1);
}


void MyExit(const char *s)
{
  ErrorMsg("!! A FATAL ERROR occurred - the program will exit !!\nError:", s);
  cleanup(-1);
}


/*
BOOL CALLBACK ETWProc( HWND hWnd, LPARAM lParam )
{
    HINSTANCE hInstance;
//    LPENUMINFO lpInfo;
    LPEXECAPPINFO lpInfo;
    char className[80];

    if (!GetClassName(hWnd, className, 79))
        className[0] = 0;

    lpInfo = (LPEXECAPPINFO) lParam;

    hInstance = (HINSTANCE)(GetWindowLong(hWnd, GWL_HINSTANCE));
    HANDLE hWndId = (HANDLE)(GetWindowLong(hWnd, GWL_ID));

    // If this window has no parent, then it is a toplevel window for the thread.
    // Remember the last one we find since it is probably the main window.
    if ( (hInstance == lpInfo->hInstance) && strcmp(className, "#32772") && GetParent(hWnd) == NULL)
    {
        lpInfo->hWnd = hWnd;

        return FALSE;
    }

    return TRUE;
} // ETWProc
*/

//
// this function launches an other program and fills in the 'EXECAPPINFO' structure
// which is required later to check whether the application is still running.
//
BOOL AppExec(const char* cmdLine, LPEXECAPPINFO pInfo, int nCmdShow)
{
#ifndef _MAC
    STARTUPINFO siStartInfo;

    memset(&siStartInfo, 0, sizeof(STARTUPINFO));
    siStartInfo.wShowWindow = nCmdShow;

    pInfo->hWnd = 0;
    if (!CreateProcess(NULL, (char*)cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &siStartInfo, &(pInfo->pi)))
        return FALSE;
//    UINT myInst = WinExec(cmdLine, nCmdShow);
//    HINSTANCE myInst = ShellExecute(GetFocus(), NULL, "g:\\uniinfo\\wingif.exe", "g:\\temp\\00042a42.gif", "", nCmdShow);
//    pInfo->pi.hProcess = (HANDLE)_spawnl(_P_NOWAIT, "g:\\uniinfo\\wingif.exe", "g:\\uniinfo\\wingif.exe", "g:\\temp\\00042a42.gif", NULL);
//    pInfo->hInstance = (HINSTANCE)(myInst);
//    pInfo->hInstance = (HINSTANCE)(pInfo->pi.hProcess);
//    if ((UINT)(pInfo->hInstance) < 32)
//        return (UINT)(pInfo->hInstance);

    // Wait for the main window to be created
    WaitForInputIdle( pInfo->pi.hProcess, 60*1000 );

//    BOOL bEnum = EnumThreadWindows( pInfo->pi.dwThreadId, ETWProc, (LPARAM)pInfo );
//    BOOL bEnum = EnumWindows((WNDENUMPROC)&ETWProc, (LPARAM)pInfo);


//    CloseHandle(pInfo->pi.hThread);
//    CloseHandle(pInfo->pi.hProcess);

//    return (pInfo->hWnd!=0);
#endif
     return TRUE;   // /*IsAppRunning(pInfo)*/;
} // AppExec


//
// this function checks whether an application is still running.
// It requires an 'EXECAPPINFO' structure which you can get if
// you launch the program using 'AppExec' (above) 
// 
BOOL IsAppRunning(LPEXECAPPINFO pInfo)
{
#ifndef _MAC
    if (!pInfo || !pInfo->pi.hProcess) 
        return FALSE;

    pInfo->retStat_ = 1;
    BOOL success = GetExitCodeProcess(pInfo->pi.hProcess, &pInfo->retStat_);

    return (pInfo->retStat_==STILL_ACTIVE) ? TRUE : FALSE;
#else
  return TRUE;
#endif
} // IsAppRunning


BOOL KillApp(LPEXECAPPINFO pInfo, BOOL dontQuit, UINT fuRemove)
{
// not supported on Mac!!!!!
#ifndef _MAC
    MSG M;

    while (IsAppRunning(pInfo))
    {
//        SendMessage(pInfo->hWnd, WM_CLOSE, 0, 0);
        TerminateProcess(pInfo->pi.hProcess, 0);
        if (PeekMessage(&M, 0, 0, 0, fuRemove))
        {
            if ( (M.message == WM_QUIT) && !dontQuit)
            {
                CloseHandle(pInfo->pi.hThread);
                CloseHandle(pInfo->pi.hProcess);

                memset(pInfo, 0, sizeof(EXECAPPINFO));
                cleanup(-1);
            }
                
            if (fuRemove & PM_REMOVE)
            {
                TranslateMessage(&M);
                DispatchMessage(&M);
            }
        }
    }
      
    if (pInfo && pInfo->pi.hProcess)
    {
        CloseHandle(pInfo->pi.hThread);
        CloseHandle(pInfo->pi.hProcess);
        memset(pInfo, 0, sizeof(EXECAPPINFO));
    }
#endif

    return TRUE;            // assume kill was successful
} // KillApp    


//
// this function launches an other program and stops the caller programm
// until the other one is finished or an error occured.
//
int WaitAppExec(const char* cmdLine, int nCmdShow)
{
    MSG M;

    LPEXECAPPINFO pid = new EXECAPPINFO;
    if (!AppExec(cmdLine, pid, nCmdShow))
    {                           // error occured
        delete(pid);
        return -1;
    }

    while (IsAppRunning(pid))
    {
        if (PeekMessage(&M, 0, 0, 0, PM_REMOVE))
        {
            if (M.message == WM_QUIT)
            {
                CloseHandle(pid->pi.hThread);
                CloseHandle(pid->pi.hProcess);
                delete(pid);
                cleanup(-1);
            }
            TranslateMessage(&M);
            DispatchMessage(&M);
        }
    }

    if (pid && pid->pi.hProcess)
    {
        CloseHandle(pid->pi.hThread);
        CloseHandle(pid->pi.hProcess);
    }
    int exitStat = (DWORD)pid->retStat_;
    delete(pid);
    return exitStat;
} // WaitAppExec


void SpawnApp(const char *appName, int nCmdShow)
{
    // launch client, 
    // procedure returns when client exits, or when server gets WM_QUIT
    EXECAPPINFO pid;
    MSG M;          
    
    BOOL retVal = AppExec(appName, &pid, nCmdShow);
    if (!retVal)
    {
        char buffer[100];
        sprintf(buffer, "Sorry, but I couldn't launch '%s' for some reason ! (Error:%d)", appName, retVal);
        ErrorMsg(buffer);
        return;
    }
    
    while (IsAppRunning(&pid))
    {
        if (PeekMessage(&M, 0, 0, 0, PM_REMOVE))
        {
            if (M.message == WM_QUIT)       // got a quit, must also kill client !
            {       
                KillApp(&pid, TRUE);
                break;
            }
            TranslateMessage(&M);
            DispatchMessage(&M);
        }
    }
} // SpawnApp


/*
 * DoesFileExist
 *
 * Purpose:
 *  Determines if a file path exists
 *
 * Parameters:
 *  lpszFile        LPTSTR - file name
 *
 * Return Value:
 *  BOOL            TRUE if file exists, else FALSE.
 *
 */
BOOL WINAPI DoesFileExist(LPTSTR lpszFile)
{
	static const TCHAR *arrIllegalNames[] = 
	{
		TEXT("LPT1"), TEXT("LPT2"), TEXT("LPT3"),
		TEXT("COM1"), TEXT("COM2"), TEXT("COM3"), TEXT("COM4"),
		TEXT("CON"),  TEXT("AUX"),  TEXT("PRN")
	};

	// check is the name is an illegal name (eg. the name of a device)
	for (int i = 0; i < (sizeof(arrIllegalNames)/sizeof(arrIllegalNames[0])); i++)
	{
		if (lstrcmpi(lpszFile, arrIllegalNames[i])==0)
			return FALSE;
	}

	// check the file's attributes
	DWORD dwAttrs = GetFileAttributes(lpszFile);
	if (dwAttrs == 0xFFFFFFFF)
	{
		// look in path for file
		TCHAR ch2[2];
		LPTSTR lpszFilePart;
#ifdef _MAC
	/// pwb to do
#else
		return SearchPath(NULL, lpszFile, NULL, 2, ch2, &lpszFilePart) != 0;
#endif
	}
	else if (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)
	{
		return FALSE;
	}
	return TRUE;
}
