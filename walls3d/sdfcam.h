// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        sdfcam.h
//
// Purpose:     interface to Camera object (SDF)
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:      4 May 95   Michael Pichler
//
//
//</file>



#ifndef harmony_scene_sdfcamera_h
#define harmony_scene_sdfcamera_h


#include "object3d.h"
#include "camera.h"


class SDFCamera
	: public Object3D, public Camera
{
public:
	SDFCamera(int obj_n, int par = 0, const char* name = 0);

	const char* type() const { return "cam"; }

	void print();

	void computeParams();

	int getdispstat() const { return (dispstat_); }

private:
	int dispstat_;   // 0 OFF  1 ON (camera used)

	friend int SDFScene::readActFile(FILE*, int&);
	friend int SDFScene::readPosFile(FILE*, int);
	friend int SDFScene::readCamFile(FILE*);
	friend void SDFScene::writeCameras(ostream&);

}; // SDFCamera


#endif
