// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1995,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        vrmlscene.h
//
// Purpose:     interface to 3D scene, VRML representation
//
// Created:     24 Apr 95   Michael Pichler
//
// Changed:     16 Jan 96   Michael Pichler
//
// Changed:     21 Feb 96   Alexander Nussbaumer (editing)
//
// $Id: vrmlscene.h,v 1.10 1996/03/12 09:58:39 mpichler Exp $
//
//</file>


#ifndef harmony_scene_vrmlscene_h
#define harmony_scene_vrmlscene_h

#include "Interviews/boolean.h"
#include "scenedata.h"
#include <iostream>
using namespace std;


class QvGroup;
class QvInput;
class QvNode;
class QvPerspectiveCamera;
class QvOrthographicCamera;
class QvTransform;


class VRMLScene: public SceneData
{ 
  public:
    VRMLScene (Scene3D* scene);
    ~VRMLScene ();

    // *** input ***
    int readInput (QvInput& in);
    const char* formatName () const  { return "VRML"; }
    void loadTextures ()  { }           // TODO
    int readInlineVRMLFILE (                // read inline VRML scene
      QvWWWInline* node, FILE*);        // returns nonzero on success
    virtual int readInlineVRMLFile (    // read inline VRML data
      QvWWWInline* node,                //   into node
      const char* filename              //   from a file
    );
    void increaseNumFaces (unsigned num)
    { numfaces_ += num; }
    void increaseNumPrimitives ()  // text, cubes, cones, cylinders, spheres
    { numprimitives_++; }

    // *** output ***
    int writeData (ostream& os, int format);
    void printInfo (int all);           // print scene information
    unsigned long getNumFaces () const  // number of faces (polygons)
    { return numfaces_; }
    unsigned long getNumPrimitives () const  // no. of primitives
    { return numprimitives_; }

    // *** drawing ***
    void draw (int mode);

    // *** picking ***
    virtual void* pickObject (
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

    // *** anchors ***
    void clearAnchors ()  { }           // TODO
    void colorRebuild ();

    // *** camera ***
    Camera* getCamera () const  { return camera_; }
    void storeCamera ();                // store active camera
    void restoreCamera ();              // reset camera
    // scene defines its camera, name is registered in viewpoint list
    void hasCamera (QvPerspectiveCamera*, const char* name);
    void hasCamera (QvOrthographicCamera*, const char* name);

    void activateCamera (
      const char*,
      QvPerspectiveCamera*, QvOrthographicCamera*
    );
    void activatePCam (QvPerspectiveCamera*);   // set active camera
    void activateOCam (QvOrthographicCamera*);  // set active camera
    const QvPerspectiveCamera* activePCam () const
    { return activepcam_; }
    const QvOrthographicCamera* activeOCam () const
    { return activeocam_; }

    // *** light sources ***
    void hasLight ()   { haslight_ = 1; }   // scene contains light source
    int nextLight ()                    // get next useable light index
    { return ++numlights_; }
    int numLights () const              // return no. of used lights
    { return numlights_; }
    void deactivateLights (             // deactivate all light sources
      int n = 0                         //   except the first n ones
    );

    // *** find objects ***
    //anuss: TODO: documentation
    QvNode* findNode (QvGroup* parent, int nodetype, int recursive = false);
    QvNode* findNodeRecursive (QvNode* node, int nodetype);
    QvGroup* findParent (QvNode* node = NULL);

    // *** editing ***
    // anuss
/*    void translateSelected (const vector3D& trans) const;
    void rotateAxisSelected (           // rotate selected object
      char axis,                        //   'x', 'y', or 'z'
      float angle)  const;              //   radians
    // void rotateSelected (const vector3D& axis, float angle);
    void scaleSelected (float scale) const
    { scaleAxisSelected (scale, scale, scale); }
    void scaleAxisSelected (float xscale, float yscale, float zscale) const;
    void selectNode ();
    void selectTransform ();
*/
  private:
    void drawVRML ();

    QvNode* root_;  // scene graph

    Camera* camera_;     // active camera
    Camera* bakcamera_;  // stored camera
    // active VRML camera
    QvPerspectiveCamera* activepcam_;
    QvOrthographicCamera* activeocam_;

    int matbuilt_;   // flag: trf matrices built (set on first drawing)
    int hascamera_;  // flag: camera definition contained in VRML scene
    int haslight_;   // flag: light source contained in VRML scene
    int numlights_;  // no. of currently activated lights

    unsigned long numfaces_;
    unsigned long numprimitives_;

    QvTransform* selectedtransform_;  // anuss: selected transformation
    float vrmlversion_;    // anuss: current VRML Version
}; // VRMLScene


#endif
