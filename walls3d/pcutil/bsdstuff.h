#ifndef hg_pcutil_bsdstuff_h
#define hg_pcutil_bsdstuff_h

#ifdef LOCAL_DB_20
#ifndef _INC_WINDOWS
#include <windows.h>
#endif /* _INC_WINDOWS */
#else
#include <winsock.h>
#endif

#include <sys/timeb.h>


#ifndef _SYS_TYPES_
#define _SYS_TYPES_

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef long        daddr_t;
typedef char *      caddr_t;
typedef unsigned short  ino_t;
//typedef short       dev_t;
typedef long        off_t;

typedef short   uid_t;          /* POSIX compliance    */
typedef short   gid_t;          /* POSIX compliance    */


struct  passwd { /* see getpwent(3) */
    char    *pw_name;
    char    *pw_passwd;
    uid_t   pw_uid;
    short   pad;
    gid_t   pw_gid;
    short   pad1;
#ifndef __SYSTEM_FIVE
    int pw_quota;   /* ULTRIX, BSD-4.2 */
#else /*    SYSTEM_FIVE */
    char    *pw_age;    /* System V */
#endif /*   __SYSTEM_FIVE */
    char    *pw_comment;
    char    *pw_gecos;
    char    *pw_dir;
    char    *pw_shell;
};

#ifndef _POSIX_SOURCE

struct comment {
    char    *c_dept;
    char    *c_name;
    char    *c_acct;
    char    *c_bin;
};
#endif /* _POSIX_SOURCE */


struct passwd *getpwent(), *getpwuid(uid_t), *getpwnam();
uid_t getuid();

/* Microsoft C hack to avoid getting time_t multiply defined.
*/
#ifndef TIME_T_DEFINED
typedef long time_t;
#define TIME_T_DEFINED
#endif


/* 
    stuff added for select()
*/

#define NBBY    8       // number of bits in a byte

#ifdef   FD_SETSIZE     // no matter what they want, they only get 20
#undef   FD_SETSIZE     // descriptors from DOS, to reset it if they
#endif // FD_SETSIZE    // tried to change it.

#define  FD_SETSIZE 20  // maximum number of DOS file descrs

#define MAXSELFD    20  // maxnumber of descrs select() groks

typedef long    fd_mask;// longest easily used number type

#define NFDBITS (sizeof(fd_mask)* NBBY) // Number of File Descr BITS

#define NFDSHIFT    0   // how far to shift over sizeof(fd_mask)

#endif // _SYS_TYPES_


#define NOFILE      4096

/*
 * Macros for counting and rounding.
 */
#ifndef  howmany        // used to find howmany longs needed for allowed
#define  howmany(x,y)   (((x) + ((y) - 1)) / (y)) // number of FDs  
#endif // howmany

#ifndef  roundup
#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
#endif // roundup


struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday( struct timeval *tp, struct timezone *tzp);

// especially for MS-Visual C++:
#define setstate clear
#define zapeof(i) ((unsigned char)(i))


/*
 * Maximum size of hostname recognized and stored in the kernel.
 * Lots of utilities use this.
 */
#define MAXHOSTNAMELEN 64


int sethostname(char*, int);

int gethostid();
int sethostid(int);

char *getlogin();

void sethostent(int stayopen);

void sleep(int cseconds);

extern char *crypt (const char *pw, char *salt);

#endif

