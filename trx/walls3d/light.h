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
// Name:        light.h
//
// Purpose:     interface to light source
//
// Created:     12 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:      1 Aug 94   Michael Pichler
//
//
//</file>



#ifndef hg_viewer_hg3d_light_h
#define hg_viewer_hg3d_light_h


#include "object3d.h"
#include "color.h"



class Light
: public Object3D
{
  public:
    Light (int obj_n, int par = 0, const char* name = 0);

    const char* type () const  { return "lgt"; }

    int readLgtFile (FILE*);
    void print ();
    void dependsOnCamera ();

    void setlight (int);  // to ge3d

    const colorRGB* intensity () const
    { return &intensity_; }
    int positional () const
    { return (int) positional_; }

  private:
    colorRGB intensity_;  // light specifics
    float positional_;  // positional or directional
    int camlgt_;  // relative to camera?
};


inline void Light::dependsOnCamera ()
{
  parent_ = 0;  // do not calculate overall trf-matrix
  camlgt_ = 1;
}


#endif
