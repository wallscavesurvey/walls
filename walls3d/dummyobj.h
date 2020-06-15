// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993,94
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        dummyobj.h
//
// Purpose:     interface to 3D dummy object
//
// Created:      3 Apr 93   Michael Hofer and Michael Pichler
//
// Changed:      1 Aug 94   Michael Pichler
//
//
//</file>



#ifndef hg_viewer_hg3d_dummyobj_h
#define hg_viewer_hg3d_dummyobj_h


#include "object3d.h"



class DummyObject
	: public Object3D
{
public:
	DummyObject(int obj_n, int par = 0, char* name = 0)
		: Object3D(obj_n, par, name) { }

	const char* type() const { return "dum"; }

	void print();

};


#endif
