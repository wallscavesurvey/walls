// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1992,93,94,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        scenedata.h
//
// Purpose:     interface to 3D scene data (abstract)
//              (no implementation associated with class SceneData)
//
// Created:     24 Apr 95   Michael Pichler (extracted from scene3d.h)
//
// Changed:     16 Jan 96   Michael Pichler
//
// Changed:     21 Feb 96   Alexander Nussbaumer (editing)
//
// $Id: scenedata.h,v 1.9 1996/03/12 10:23:23 mpichler Exp $
//
//</file>


#ifndef harmony_scene_scenedata_h
#define harmony_scene_scenedata_h

class Camera;
class GeometricObject;
class Material;
class Scene3D;
class StringArray;
class QvNode;
class QvPerspectiveCamera;
class QvOrthographicCamera;
class QvWWWAnchor;
class QvWWWInline;


#include "vectors.h"

#include <stdio.h>
#include <iostream>
using namespace std;


class SceneData
{
public:
	SceneData(Scene3D* scene)
	{
		scene_ = scene;
	}
	virtual ~SceneData() { }

	// *** input ***
	// readInput arguments format dependent (no virtual function)
	virtual void loadTextures() = 0;   // request all textures
	virtual const char* formatName() const = 0;  // "SDF", "VRML" etc.
	virtual int readInlineVRML(QvWWWInline*, FILE*) { return 0; }

	// *** output ***
	virtual int writeData(ostream& /*os*/, int /*format*/) { return 0; }
	// write SDF file to output stream
	virtual void printInfo(int /*all*/) { }   // print scene information
	virtual unsigned long getNumFaces() const = 0;  // number of faces (polygons)
	virtual unsigned long getNumPrimitives() const { return 0; }  // number of primitives
	virtual int supportsOutputformat(int  /*format*/) { return 0; }

	// *** drawing ***
	virtual void draw(int mode) = 0;   // draw whole scene

	// *** picking ***
	virtual void* pickObject(
		const point3D& A,                 //   ray start
		const vector3D& b,                //   ray direction
		float tnear,                      //   near distance
		float tfar,                       //   far distance
		GeometricObject** gobj = 0,       //   GeometricObject hit (return)
		QvNode** node = 0,                //   node hit (return)
		QvWWWAnchor** anchor = 0,         //   anchor hit (return)
		point3D* hitpoint = 0,            //   optionally calculates hit point
		vector3D* normal = 0,             //   and face normal vector (normalized)
		const StringArray** groups = 0,   //   optionally determines groups hit
		float* hittime = 0                //   optionally returns the hit time
	) = 0;

	// *** find objects ***
	// TODO: abstract return types
	virtual GeometricObject* findObject(const char* /*objname*/) { return 0; }
	virtual GeometricObject* findObject(int /*objnum*/) { return 0; }
	virtual Material* findMaterial(const char* /*matname*/) { return 0; }

	// *** anchors ***
	virtual void clearAnchors() { }   // clear all anchor definitions
	virtual void colorRebuild() { }   // rebuild after anchor color change

	// *** camera ***
	virtual Camera* getCamera() const = 0;     // get active camera
	virtual void storeCamera() = 0;            // store active camera
	virtual void restoreCamera() = 0;          // reset camera (to one read from file or latest stored)
	virtual void activateCamera(
		const char*,
		QvPerspectiveCamera*, QvOrthographicCamera*
	) { }

	// *** editing ***
	virtual void selectTransform() { }        // select/create Transform node for manipulation
	virtual void selectNode() { }             // until picking for VRML-Scenes is implmented
	// manipulate current transformation
	virtual void translateSelected(            // apply a translation
		const vector3D& trans) const { }
	virtual void rotateAxisSelected(           // apply a rotation
		char axis,                                //   'x', 'y', or 'z'
		float angle)  const { }                  //   radians
	  // void rotateSelected (const vector3D& axis, float angle);
	virtual void scaleSelected(float scale) const  // apply overall scaling
	{
		scaleAxisSelected(scale, scale, scale);
	}
	virtual void scaleAxisSelected(            // apply individual axis scaling
		float xscale, float yscale, float zscale) const { }

protected:
	Scene3D* scene_;

}; // SceneData


#endif
