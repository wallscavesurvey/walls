//<file>
//
// Name:        message.C
//
// Purpose:     Message class for status and error messages
//
// Created:     03 Aug 93    Keith Andrews
//
// Modified:    11 Jan 94    Michael Pichler
//
//</file>

#ifdef __PC__
#include "..\stdafx.h"
#endif

#include "message.h"

#include <iostream>
#include <stdlib.h>


// static members
RString HgMessage::progname_;
HgMessage::cleanupfunc HgMessage::cleanup_ = nil;


void HgMessage::init( const RString& progname, cleanupfunc cleanup)
{
  progname_ = progname;
  cleanup_ = cleanup;
}


void HgMessage::message(const char* msg)
{
#ifndef _CONSOLE
    char buf[1024];
    sprintf(buf, "%s: %s", (const char*) progname_, msg);
    AfxMessageBox(buf);
#else
    cerr << (const char*) progname_ << ": " << msg << endl;
#endif
}


void HgMessage::error(const char* msg)
{
#ifndef _CONSOLE
    char buf[1024];
    sprintf(buf, "%s. Error: %s", (const char*) progname_, msg);
    AfxMessageBox(buf);
#else
    cerr << (const char*) progname_ << ". Error: " << msg << endl;
#endif
}


void HgMessage::fatalError(const char* msg, int code)
{
#ifndef _CONSOLE
    char buf[1024];
    sprintf(buf, "%s. Fatal error: %s", (const char*) progname_, msg);
    AfxMessageBox(buf);
#else
    cerr << (const char*) progname_ << ". Fatal error: " << msg << endl;
#endif
  if (cleanup_)
        (*cleanup_)();

#ifndef _CONSOLE
    sprintf(buf, "%s. terminated with exit code %d.", (const char*) progname_, code);
    AfxMessageBox(buf);
#else
    cerr << (const char*) progname_ << " terminated with exit code " << code << "." << endl;
#endif
  ::exit (code);
}
