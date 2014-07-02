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
// Name:        geomobj.h
//
// Purpose:     interface to geometric (drawable) 3D object
//
// Created:     12 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     26 Jul 95   Michael Pichler
//
//
//</file>


#ifndef hg_viewer_hg3d_geomobj_h
#define hg_viewer_hg3d_geomobj_h


#include "object3d.h"
#include "strarray.h"

#include "color.h"
#include <iostream>

using namespace std;

class RString;
class SourceAnchor;
class SDFScene;


class GeometricObject
: public Object3D
{
  public:
    GeometricObject (int obj_n, int par = 0, const char* name = 0);
    virtual ~GeometricObject ();

    virtual int readObjFile (FILE*, SDFScene*) = 0;

    virtual void print (int) = 0;
    virtual void writeData (ostream&) const = 0;  // file output
    virtual void writeVRML (ostream&) const = 0;  // output in VRML format

    virtual unsigned long numFaces () const;  // number of faces (if applicable)

    // *** drawing ***
    void draw (int hilitindex, int texturing, const colorRGB* col_anchorface, const colorRGB* col_anchoredge);
    // load transformation matrix and call draw_

    // *** anchors ***
    void setAnchor (                    // define a source anchor
      long id, const RString& aobj,
      const char* groupname
    );

    virtual void groupAnchorsChanged ()  { }
    // anchor groups have changed (higlighting info may have to be rebuilt)

    int hasAnchors () const  { return anchorlist_.isnotempty (); }

    void clearAnchors ();  // clear all anchors

    const SourceAnchor* getNextAnchor (const StringArray* groups);
    // find next anchor (cyclic) that matches groups; object anchors always
    // match, group anchors only if their group is contained in groups;

    const SourceAnchor* getCurrentAnchor () const
    { return (SourceAnchor*) anchorlist_.current (); }
    // most recent return value of getNextAnchor; may return any anchor
    // or NULL when neither getNextAnchor nor checkAnchorSelection has been called

    // *** selection ***
    void select (const char* group = 0); // selected objects are highlighted with a box
    void unselect ();                    // remove selection
    virtual void groupSelectionChanged ()  { }
    const char* selectedGroup ()  { return selectedgroup_; }

    void checkAnchorSelection ();       // check whether selection matches any anchor
    // in that case the anchor becomes the current anchor of the object
    // typically called on select and on changes of anchor definitions

    // *** picking ***
    int ray_hits (const point3D& A, const vector3D& b, float tnear, float& tmin,
                  vector3D* normal, const StringArray** groups);
                               // tests with bounding box and calls rayhits_

    void worldBounding (point3D& wmin, point3D& wmax);
                               // sets bounding box in world coordinates
                               // and updates scene (world) bounding box

    virtual void setupNormals ()  { }
                               // sets up face normals (depend on transformation matrix)

  protected:
    // bounding box in object coordinates
    point3D minbound_oc_, maxbound_oc_;  // to be set in readobjfile
    slist anchorlist_;  // list of GroupAnchors

    int hasobjectanchors_;  // object anchors require less work for highlighting,
    int hasgroupanchors_;   // so derived objects are interested if group anchors exist

  private:
    virtual int rayhits_ (const point3D& A, const vector3D& b, float tnear, float& tmin,
                          vector3D* normal, const StringArray** groups) = 0;  // hit test
    virtual void draw_ (int hilitindex, int texturing, const colorRGB* colaface, const colorRGB* colaedge) = 0;

    point3D minbound_wc_, maxbound_wc_;  // bounding box in world coordinates
    int selected_;
    const char* selectedgroup_;
};  // GeometricObject


#endif
