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
// Name:        polyhed.h
//
// Purpose:     interface to polyhedron
//
// Created:     12 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     27 Jul 94   Michael Pichler
//
//
//</file>



#ifndef hg_viewer_hg3d_polyhed_h
#define hg_viewer_hg3d_polyhed_h


#include "geomobj.h"
#include "face.h"


class MaterialGroup;


class Polyhedron
: public GeometricObject
{
  public:
    Polyhedron (int obj_n, int par = 0, const char* name = 0, const Polyhedron* copy = 0);
    ~Polyhedron ();

    int readObjFile (FILE*, SDFScene*);
    void print (int);
    void writeData (ostream&) const;
    void writeVRML (ostream&) const;

    unsigned long numFaces () const  { return num_faces_; }

    int sharedInfo () const  { return copy_ ? copy_->getobj_num () : 0; }

    void setupNormals ();               // trfmat available - compute normals
    void groupAnchorsChanged ();        // anchor groups have changed
    void groupSelectionChanged ();      // selected group has changed

    void draw_ (int hilitindex, int texturing, const colorRGB* col_anchorface, const colorRGB* col_anchoredge);
    // do drawing in object coordinates

    int rayhits_ (const point3D& A, const vector3D& b, float tnear, float& tmin,
                  vector3D* normal, const StringArray** groups);

  private:
    void definePickableFaces ();        // set facepickable_
    void defineSelectedFaces ();        // set faceselected_

    void writeFacesVRML (ostream&, const face*, int n, const Material*) const;

    const Polyhedron* copy_;            // pointer to master for copies (nil for original)

    int num_verts_;
    point3D* vertexlist_;               // array of vertex coordinates
    int num_normals_;
    vector3D* normallist_;              // array of normal coordinates
    int num_texverts_;
    point2D* texvertlist_;              // array of texture coordinates
    int num_faces_;
    face* facelist_;                    // array of faces (vertexindices and normalindices)
    int* facepickable_;                 // flag-array: face in anchor group or not
    int* faceselected_;                 // flag-array: face in selected group or not

    slist* matgrouplist_;               // list of material groups
                                        // (face index, face count, material pointer)
    slist* facegrouplist_;              // lsit of face groups (for picking)
};


#endif
