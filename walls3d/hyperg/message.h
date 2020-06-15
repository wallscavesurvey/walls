// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        message.h
//
// Purpose:     Message class for status and error messages
//
// Created:     03 Aug 93    Keith Andrews
//
// Modified:    11 Jan 94    Michael Pichler
//
// Modified:     9 Mai 94    Gerald Pani
//                           
//
//
//
// Description:
//
// Output of status and error messages to cerr. All messages are preceded
// by the program name (to be set with init).
//
// In case of fatal errors execution is terminated via exit (code), optionally
// a cleanup function can be given with init, which is called before exit.
//
//
//</file>


#ifndef hg_hyperg_message_h
#define hg_hyperg_message_h


#include "../utils/str.h"


class HgMessage
{
public:
	typedef void(*cleanupfunc)(void);

	static void init(                   // initialisation
		const RString& progname,          //   program name
		cleanupfunc = nil);               //   optional cleanup function

	static void message(                // messages
		const char* msg                   //   message
	);

	static void message(                // messages
		const RString& msg                //   message
	);

	static void error(                  // errors (non-fatal)
		const char* msg                   //   message
	);

	static void error(                  // errors (non-fatal)
		const RString& msg                //   message
	);

	static void fatalError(             // fatal errors (execution is terminated)
		const char* msg,                  //   message
		int code = -1                     //   exit code
	);

	static void fatalError(             // fatal errors (execution is terminated)
		const RString& msg,               //   message
		int code = -1                     //   exit code
	);

private:
	static RString progname_;
	static cleanupfunc cleanup_;
};


inline void HgMessage::message(const RString& msg) {
	message(msg.string());
}

inline void HgMessage::error(const RString& msg) {
	error(msg.string());
}

inline void HgMessage::fatalError(const RString& msg, int code) {
	fatalError(msg.string(), code);
}


#endif
