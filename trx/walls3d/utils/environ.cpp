//<copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:       environ.C
//
// Purpose:    
//
// Created:     1 Feb 92    Gerald Pani
//
// Modified:   21 Jul 93    Gerald Pani
//
//
//
// Description:
//
//
//</file>

//be sure to NOT include several Windows-header files !!
#include <stdafx.h>
//#define WIN32_LEAN_AND_MEAN

#include "hgunistd.h"

#include <ctype.h>
#include <string.h>

#ifdef __PC__
#include <pcutil\bsdstuff.h>
#endif

#ifdef LOCAL_DB_20
#include <pcutil\pcstuff.h>
#endif

#include <time.h>

#ifdef mips
# include <stdio.h>
#endif

#ifdef __PC__
# include <stdio.h>
#endif

//#ifndef __PC__
//#include <sys/socket.h>
//#include <netdb.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <sys/utsname.h>
//#include <pwd.h>
//#endif

#ifdef SYSV
#include <errno.h>
#endif

#ifdef __osf__
#include <errno.h>
#endif

#include "types.h"
#include "environ.h"

boolean Environ::getEnv( const RString& var, RString& value) {
     char* tmp;
     if (tmp = ::getenv( var.string())) {
      value = tmp;
      return true;
     }
     return false;
}

boolean Environ::getEnv( const RString& var, int& value) {
     char* tmp;
     if (tmp = ::getenv( var.string())) {
      value = atoi( tmp);
      return true;
     }
     return false;
}

boolean Environ::getEnv( const RString& var, boolean& value) {
     char* tmp;
     if (tmp = ::getenv( var.string())) {
      if (!::strcmp( tmp, "true") || !::strcmp( tmp, "1"))
           value = true;
      else
           value = false;
      return true;
     }
     return false;
}


static int env_index( const RString& var);
static int env_entries();
static void add_env( const RString& var, const RString& value);
static boolean change_env( const RString& var, const RString& value);

void Environ::setEnv( const RString& var, const RString& value) {
   if (! change_env( var, value))
      add_env( var, value);
}
void Environ::setEnv( const RString& var, int value) {
   char tmp[20]; // ?
   ::sprintf( tmp, "%d", value) ;
   setEnv( var, tmp);
}
void Environ::setEnv( const RString& var, boolean value) {
   setEnv( var, value? "true": "false");
}

const RString& Environ::user() {
     static RString user;
     RString passwd;
     getpw( user, passwd);
     return user;
}

boolean Environ::getpw( RString& user, RString& passwd) {
     struct passwd* pwd;
     if (!(pwd = ::getpwuid( ::getuid()))) {
      ::perror( "Environ::getpw:getpwuid");
      user = nil;
      passwd = nil;
      return false;
     }
     else {
      user = pwd->pw_name;
      passwd = pwd->pw_passwd;
      return true;
     }
}


const RString& Environ::timeString( time_t t, boolean local) {
     static RString timeString;
     char timestr[18];
     struct tm* tm = local ? ::localtime( &t) : ::gmtime( &t);
     ::sprintf( timestr, "%02d/%02d/%02d %02d:%02d:%02d", 
         tm->tm_year, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
     return timeString = timestr;
}       

const RString& Environ::gmTimeString() {
     time_t time = ::time( nil);
     return timeString( time, false);
}       

#if defined(__osf__) || defined(ultrix) || (defined(sparc) && OSMajorVersion >= 5) || defined(hpux) || defined(sgi) || defined(LINUX) || defined(__PC__)
#define hg_utils_environ_C_have_mktime
#endif

#if !defined(sgi) && !defined(hpux) && !(defined(sparc) && OSMajorVersion >= 5) && !defined(LINUX) && !defined(__PC__)
#define hg_utils_environ_C_have_tm_zone
#endif

boolean Environ::gmDate2Sec( const RString& date, time_t& secs) {
     static boolean havetimezone;
     static time_t timezone = 0;
     if (!havetimezone) {
      tzset();
      havetimezone = true;
#ifdef hg_utils_environ_C_have_mktime
      // calculate offset from GMT
      struct tm tms;
      tms.tm_sec = 0;
      tms.tm_min = 0;
      tms.tm_hour = 0;
      tms.tm_mday = 2;
      tms.tm_mon = 0;
      tms.tm_year = 70;
      tms.tm_wday = 0;
      tms.tm_yday = 0;
#if defined(sparc) && OSMajorVersion >= 5
      tms.tm_isdst = 1;
#else
      tms.tm_isdst = 0;
#endif

#ifdef hg_utils_environ_C_have_tm_zone
      tms.tm_zone = nil;
      tms.tm_gmtoff = 0L;
#endif
      time_t secs = mktime( &tms);
      timezone = secs - 86400;
#endif
     }
     
     struct tm tms;
     tms.tm_sec = -timezone;
     tms.tm_min = 0;
     tms.tm_hour = 0;
     tms.tm_mday = 1;
     tms.tm_mon = 0;
     tms.tm_year = 70;
     tms.tm_wday = 0;
     tms.tm_yday = 0;
#if defined(sparc) && OSMajorVersion >= 5
     tms.tm_isdst = 1;
#else
     tms.tm_isdst = 0;
#endif
     
#ifdef hg_utils_environ_C_have_tm_zone
     tms.tm_zone = nil;
     tms.tm_gmtoff = 0L;
#endif

     if (isdigit(date[0])) {
      tms.tm_year = (date[0] - '0')*10;
      if (isdigit(date[1])) {
           tms.tm_year += date[1] - '0';
           if (date[2] == '/' && isdigit(date[3])) {
            tms.tm_mon = (date[3] - '0')*10;
            if (isdigit(date[4])) {
             tms.tm_mon += date[4] - '0';
             if (date[5] == '/' && isdigit(date[6])) {
                  tms.tm_mday = (date[6] - '0')*10;
                  if (isdigit(date[7])) {
                   tms.tm_mday += date[7] - '0';
                   if (date[8] == ' ' && isdigit(date[9])) {
                    tms.tm_hour = (date[9] - '0')*10;
                    if (isdigit(date[10])) {
                         tms.tm_hour += date[10] - '0';
                         if (date[11] == ':' && isdigit(date[12])) {
                          tms.tm_min = (date[12] - '0')*10;
                          if (isdigit(date[13])) {
                               tms.tm_min += date[13] - '0';
                               if (date[14] == ':' && isdigit(date[15])) {
                                tms.tm_sec = (date[15] - '0')*10;
                                if (isdigit(date[16])) {
                                 tms.tm_sec += date[16] - '0';
                                }
                                tms.tm_sec -= timezone;
                               }
                          }
                         }
                    }
                   }
                  }
             }
            }
            tms.tm_mon--;
           }
      }
     }

#ifdef hg_utils_environ_C_have_mktime
     secs = mktime( &tms);
#else
     secs = timegm( &tms);
#endif
     if (secs == -1) {
      if (tms.tm_year != 69 
          || tms.tm_mon != 11
          || tms.tm_mday != 31
          || tms.tm_hour != 23
          || tms.tm_min != 59
          || tms.tm_sec != 59)
           return false;
     }
     return true;
}

#undef hg_utils_environ_C_have_mktime
#undef hg_utils_environ_C_have_tm_zone


#if (defined(sparc) && OSMajorVersion >= 5) || defined(__osf__)

const RString& Environ::hostName() {
     static RString hostName;
     if (!hostName.length()) {
      struct utsname uts;
      if ( ::uname( &uts) != -1)
           hostName = uts.nodename;
      else
           ::perror( "Environ::hostName:uname"); 
     }
     return hostName;
}

#else
#ifdef sgi
extern "C" {
     int gethostname( char *name, int namelen);
}
#endif

const RString& Environ::hostName() {
#ifdef LOCAL_DB_20
//    ErrorMsg("Steve, das mußt du noch implementierten:  ENVIRON.CPP hostName");
	return RString("");
#else
    static RString hostName;
    if (!hostName.length()) {
        char buffer[32];
        if (::gethostname( buffer, 32) == -1)
            ::perror( "Environ::hostName:gethostname");
        else
            hostName = buffer;
    }
    return hostName;
#endif
}

#endif


const RString& Environ::hostAddress() {
     static RString hAddress;

     if (!hAddress.length()) {
      hostAddress( hostName(), hAddress);
     }
     return hAddress;
}

boolean Environ::hostAddress( const RString& hostName, RString& addr) 
{
#ifdef LOCAL_DB_20
//    ErrorMsg("Steve, das mußt du noch implementierten:  ENVIRON.CPP hostAddress");
	return false;
#else

    ::sethostent(0);
    hostent* host;     
    if (!(host = ::gethostbyname( hostName.string())) ||
        !(in_addr*)host->h_addr || !((in_addr*)host->h_addr)->S_un.S_addr) 
    {
        RString simpleHostName = hostName.gSubstrDelim(0, '.');
        if (!(host = ::gethostbyname( simpleHostName.string())) ||
            !(in_addr*)host->h_addr || !((in_addr*)host->h_addr)->S_un.S_addr) 
        {
            addr = "gethostbyname:";
#ifdef SYSV
            addr += strerror(errno);
#else
            if (errno < sys_nerr)
                addr += sys_errlist[errno];
#endif
            return false;
        }
    }

    {
        in_addr *ia = (in_addr*)host->h_addr;
        TRACE("Local Name: %s, Official Name: %s\n", hostName.string(), host->h_name);
        char* inaddr;
        if (!(inaddr = inet_ntoa(*ia))) 
        {
            addr = "inet_ntoa:";
#ifdef SYSV
            addr += strerror(errno);
#else
            if (errno < sys_nerr)
                addr += sys_errlist[errno];
#endif
            return false;
        }
        else 
        {
            addr = inaddr;
            return true;
        }
    }
#endif
}

boolean Environ::hostName( const RString& hostAddr, RString& name) {
#ifdef LOCAL_DB_20
//    ErrorMsg("Steve, das mußt du noch implementierten:  ENVIRON.CPP hostName");
	return false;
#else
     unsigned long addr;
     if ( (addr = inet_addr( hostAddr.string())) == -1) {
      name = "inet_addr:malformed request";
      return false;
     }
     ::sethostent(0);
     hostent* host;     
     if (!(host = ::gethostbyaddr( (char*)&addr, sizeof( addr), AF_INET))) {
      name = "gethostbyaddr:";
#ifdef SYSV
      name += strerror(errno);
#else
      if (errno < sys_nerr)
           name += sys_errlist[errno];
#endif
      return false;
     }
     else {
      name = host->h_name;
      return true;
     }
#endif
}






extern char** environ ;
static char** const origenviron = environ;

static int env_index (const RString& name) {
   for (int i=0 ; environ[i] ; i++)
      if (!strncmp(environ[i], name.string(), name.length())  &&  environ[i][name.length()]=='=')
         return i ;
   return -1 ; // "<name>=" not found
}

static int env_entries() {
   int i = 0 ;
   while (environ[i]) i++ ;
   return i ;
}

static void add_env (const RString& name, const RString& value) {
   char** newenv ;
   if (::origenviron == environ)
      // copy original environment to the heap
      newenv = (char**) malloc ((env_entries()+2) * sizeof(char*)) ;
   else 
      // already on the heap; just realloc
      newenv = (char**) realloc (environ, (env_entries()+2) * sizeof(char*)) ;

   char** ostr ;
   char** nstr ;
   for (ostr=environ, nstr=newenv ; *ostr ; *nstr++ = *ostr++) ;
   char* newentry = (char*) malloc ((name.length()+value.length()+2) * sizeof(char)) ;
   sprintf (newentry, "%s=%s", name.string(), value.string()) ;
   *nstr++ = newentry ;
   *nstr = nil ;
   environ = newenv ;
}

static boolean change_env (const RString& name, const RString& value) {
   int ix = env_index (name) ;
   if (ix < 0)
      return false ;
   if (strlen (environ[ix]) >= name.length()+value.length()+1)
      // enough space in the original string
      sprintf (environ[ix], "%s=%s", name.string(), value.string()) ;
   else {
      // ++++ cannot free the old entry because it might not be on the heap, 
      // but in the original environment space
      environ[ix] = (char*) malloc (name.length()+value.length()+2) ;
      sprintf (environ[ix], "%s=%s", name.string(), value.string()) ;
   }
   return true ;
}

