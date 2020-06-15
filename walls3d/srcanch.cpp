//<copyright>
// 
// Copyright (c) 1994,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        srcanch.C
//
// Purpose:     implementation of class SourceAnchor
//
// Created:      1 Jun 94   Michael Pichler
//
// Changed:     31 Jul 95   Michael Pichler
//
//
//</file>


#include "srcanch.h"

#include <string.h>


SourceAnchor::SourceAnchor(long id, const RString& aobject, const char* groupname)
	: anchorobj_(aobject)
{
	anchorid_ = id;

	// RStrings do not differ between NULL pointers and empty strings
	if (groupname && *groupname)  // group anchor
	{
		groupname_ = new char[strlen(groupname) + 1];
		strcpy(groupname_, groupname);
	}
	else  // object anchor
		groupname_ = 0;
}


SourceAnchor::~SourceAnchor()
{
	delete groupname_;
}
