
#include <stdlib.h>
#include <time.h>
#include <pcutil/bsdstuff.h>

#ifdef LOCAL_DB_20
#define _WINSOCKAPI_    // just for now
#ifndef _INC_WINDOWS
#include <windows.h>
#endif /* _INC_WINDOWS */
#endif

//
// GETTIMEO.CPP - gettimeofday() returns current system time
//
int gettimeofday(struct timeval  *tp, struct timezone * tzp)
{
    struct timeb  foo;
    int tz_flag;


    // Check for MSC TimeZone environment variable 
    tz_flag = ((getenv("TZ") != (char*)0) ? 1 : 0);

    if (tz_flag)
        tzset();  // To setup internal global variables

    ftime(&foo);  // get time and tz settings (if any) 

    if(tp) 
    {
        tp->tv_sec = foo.time;
        tp->tv_usec = ((long) foo.millitm) * 1000L;
    } 
    else
        return(-1);

    if(tzp) 
    {
        if (tz_flag) 
        {                                       // If TZ env var set,
            tzp->tz_minuteswest = foo.timezone; //  use MSC TZ setting
            tzp->tz_dsttime = foo.dstflag;
        } 
        else 
        {
             tzp->tz_minuteswest = 0;           // BOGUS Values!
             tzp->tz_dsttime = 0;
        }
    }
//  dreturn(" = %d\n", 0);
    return 0;
}


void sleep(int cseconds)
{
    Sleep(cseconds * 1000L);
}


// some dummy functions, just to satisfy the linker ...

void sethostent(int )   // does nothing ..
{}

struct passwd myPassWord = {    
    "",         // char    *pw_name;
    "",         // char    *pw_passwd;
    0,          // uid_t   pw_uid;
    0,          // short   pad;
    0,          // gid_t   pw_gid;
    0,          // short   pad1;
#ifndef __SYSTEM_FIVE
    0,          // int pw_quota;   /* ULTRIX, BSD-4.2 */
#else /*    SYSTEM_FIVE */
    "",         // char    *pw_age;    /* System V */
#endif /*   __SYSTEM_FIVE */
    "",         // char    *pw_comment;
    "",         // char    *pw_gecos;
    "",         // char    *pw_dir;
    ""          // char    *pw_shell;
};


struct passwd *getpwent()
{
    return (struct passwd*)&myPassWord;
}


struct passwd *getpwuid(uid_t )
{
    return (struct passwd*)&myPassWord;
}


struct passwd *getpwnam()
{
    return (struct passwd*)NULL;
}



uid_t getuid()
{
    return 0;
}


int sethostname(char* , int )
{
//  strcpy(HostName, name);
    return 0;
};

int gethostid() { return 1; };
int sethostid(int ) { return 0; };


char *getlogin()
{
/*
  char  IniPath[MAXPATH];

  getcwd(IniPath, MAXPATH);
  strcat(IniPath, "\\");
  strcat(IniPath, IniFile);

  char *ptr = getenv("PCTCP");      // PC/TCP IniFile ...
  if (ptr)
  {
    strcpy(IniPath, ptr);
    GetPrivateProfileString((LPSTR)"pctcp general", "user", "tdieting", UserName, 80, IniPath);
  }
  else
    MessageBox(CrtWindow, "I cannot find your PC/TCP - configuration file (usually specified through the PCTCP-environement variable) !", "!!! Error !!!", MB_OK | MB_ICONEXCLAMATION);

  return UserName;
*/

//  gethostname(HostName, 80);
//  strcpy(UserName, getlogin());

  return "";
}


