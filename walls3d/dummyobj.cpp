//<copyright>
// 
// Copyright (c) 1993,94
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        dummyobj.C
//
// Purpose:     implementation of class DummyObject
//
// Created:      3 Apr 92   Michael Hofer and Michael Pichler
//
// Changed:     12 Jan 94   Michael Pichler
//
//
//</file>



#include "dummyobj.h"

#include <iostream>




void DummyObject::print()
{
	cout << "Dummyobject. ";

	Object3D::printobj();  // name, number, channels
}
