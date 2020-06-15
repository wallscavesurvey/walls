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
// Name:        sdfscene3d.h
//
// Purpose:     interface to 3D scene, SDF representation
//
// Created:     24 Apr 95   Michael Pichler (extracted from scene3d.h)
//
// Changed:     16 Jan 96   Michael Pichler
//
// $Id: sdfscene.h,v 1.4 1996/01/30 11:20:30 mpichler Exp $
//
//</file>


#ifndef harmony_scene_sdfscene_h
#define harmony_scene_sdfscene_h

#include "scenedata.h"

#include "slist.h"
struct objindexstruct;
class SDFCamera;


class SDFScene : public SceneData
{

	friend class Object3D;
	friend class SDFCamera;

public:
	SDFScene(Scene3D* scene);
	~SDFScene();

	// *** input ***
	int readFile(FILE* file);
	const char* formatName() const { return "SDF"; }
	void loadTextures();

	// *** output ***
	int writeData(ostream& os, int format);  // write SDF file to output stream
	void printInfo(int all);           // print scene information
	int supportsOutputformat(int /*format*/) { return 1; }

	unsigned long getNumFaces() const  // number of faces (polygons)
	{
		return numfaces_;
	}

	// *** drawing ***
	void draw(int mode);               // draw whole scene (in current mode)

	// *** picking ***
	void* pickObject(
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
	);

	// *** find objects ***
	// TODO: abstract return types
	GeometricObject* findObject(const char* objname);
	GeometricObject* findObject(int objnum);
	Material* findMaterial(const char* matname);

	// *** anchors ***
	void clearAnchors();               // clear all anchor definitions

	// *** camera ***
	Camera* getCamera() const;         // get active camera
	void storeCamera();                // store active camera
	void restoreCamera();              // reset camera (to one read from file or latest stored)

private:
	// scene reading subroutines
	int readActFile(FILE*, int&);
	int readPosFile(FILE*, int);
	int readCamFile(FILE*);
	int readMtlFile(FILE*);

	// scene writing subroutines (SDF)
	void writeActors(ostream&);
	void writePositions(ostream&);
	void writeCameras(ostream&);
	void writeMaterials(ostream&);
	void writeLights(ostream&);
	void writeObjects(ostream&);

	// VRML writing subroutines
	void writeVRML(ostream&);

	void buildMatrices();  // compute accumulative transformation matrices
	void buildMatrix(objindexstruct*, int);

	slist gobjlist_,    // geometric objects
		camlist_,     // sdf-cameras
		lightlist_,   // lights
		allobjlist_,  // for building matrices
		materlist_;   // materials

	int num_objects_,
		num_gobj_,
		num_cam_,
		num_light_;
	unsigned long numfaces_;

	SDFCamera* activecam_;
	SDFCamera* origcam_;  // original camera (read from file)

	int matbuilt_;  // flag: trf matrices built (set on first drawing)
	int texturesloaded_;  // flag: textures loaded

	// prevent copying (declared, but not implemented)
	SDFScene(const SDFScene&);
	SDFScene& operator = (const SDFScene&);

}; // SDFScene


#endif
