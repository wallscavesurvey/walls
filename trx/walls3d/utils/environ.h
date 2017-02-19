// This may look like C code, but it is really -*- C++ -*-

// <copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
// </copyright>

//<file>
//
// Name:       environ.h
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

#ifndef hg_utils_environ_h
#define hg_utils_environ_h

// #ifndef __PC__
// #  ifndef sun
// #    include <sysent.h>
// #  else
// #    ifndef SYSV
// #        include <sysent.h>
// #    endif
// #  endif
// #endif

#include <utils/hgunistd.h>

#include <stdlib.h>

// #include "options.h"
#include "str.h"

//<class>
//
// Name:       Environ
//
// Purpose:    Access to environment and other system values
//
//
// Public Interface:
//
// - boolean getEnv( const RString& var, RString& value)
//   Get string from environment variable var. If var doesn't exist
//   return false.
//   
// - boolean getEnv( const RString& var, int& value)
//   Get string from environment variable var and convert it to an
//   integer. If var doesn't exist return false.
//   
// - boolean getEnv( const RString& var, boolean& value)
//   Get string from environment variable var and set value to true if
//   var holds "true" or "1", otherwise set value to false. If var
//   doesn't exist return false. 
//   
// - void setEnv( const RString& var, const RString& value)
// - void setEnv( const RString& var, int value)
// - void setEnv( const RString& var, boolean value)
//   Set environment variable var with value. Overwrites old value or 
//   creates new one.
//   
// - RString user()
//   Return the username of the process userid.
//   
// - boolean getpw( RString& user, RString& passwd)
//   Get current username and encrypted passwd
// 
// - RString timeString( time_t t, boolean local = true)
//   Make a timestring ("92/03/04 01:04:05") from the given time t
//   (from system time call) in local or gm time.
// 
// - RString gmTimeString()
//   Same as timeString( ::time( nil), false).
//   
// - const RString& hostName()
//   Returns the name of this host.
// 
// - const RString& hostAddress()
//   Returns the internet address ("aaa.bbb.ccc.ddd") of this host.
// 
// - boolean hostAddress( const RString host, RString& addr)
//   Returns the internet address ("aaa.bbb.ccc.ddd") of host in addr,
//   if true, otherwise an error string.
// 
// - boolean hostName( const RString addr, RString& name)
//   Returns the official name of host with internet address addr ("aaa.bbb.ccc.ddd"),
//   if true, otherwise an error string.
// 
// Description:
//
//
//</class>

class Environ {
  public:
     static boolean getEnv( const RString& var, RString& value);
     static boolean getEnv( const RString& var, int& value);
     static boolean getEnv( const RString& var, boolean& value);
     static void setEnv( const RString& var, const RString& value);
     static void setEnv( const RString& var, int value);
     static void setEnv( const RString& var, boolean value);
     static const RString& user();
     static boolean getpw( RString& user, RString& passwd); 
     static const RString& timeString( time_t t, boolean local = true);
     static const RString& gmTimeString();
     static boolean gmDate2Sec( const RString& date, time_t& secs);
     static const RString& hostName();
     static const RString& hostAddress();
     static boolean hostAddress( const RString& host, RString& addr);
     static boolean hostName( const RString& hostAddr, RString& name);
};

#endif





