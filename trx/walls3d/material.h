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
// Name:        material.h
//
// Purpose:     interface to materials
//
// Created:      8 Feb 93   Michael Pichler
//
// Changed:     13 Nov 95   Michael Pichler
//
// $Id: material.h,v 1.2 1995/11/14 15:08:15 mpichler Exp $
//
//</file>


#ifndef harmony_scene_material_h
#define harmony_scene_material_h


#include "utils/str.h"
#include "color.h"


class Material
{
  public:
    Material (const char* name, float r, float g, float b, const char* texture);
    ~Material ();

    const char* name () const  { return name_; }

    void setge3d (                      // sets color_ [colindex] to ge3d library
      int hilitindex,                   //   highlighting method (or 0 for no highlight)
      int texturing,                    //   texturing flag
      int pickable,                     //   pickable flag (emphasize or grey out)
      const colorRGB* col_anchorface,   //   fill colour of anchor faces
      const colorRGB* col_anchoredge    //   edge colour of anchor faces
    ) const;

    void print () const;
    const colorRGB* natural () const  { return &natural_; }

    const char* texture () const  { return texture_; }
    int texhandle () const  { return texhandle_; }
    void texhandle (int h)  { texhandle_ = h; }

  private:                              // many current Hardware types only support diffuse color
    RString name_;
    colorRGB natural_, bright_, dark_;  // natural, highlighted, and dark color
    float L_;                           // brightness
    RString texture_;                   // texture name
    int texhandle_;                     // texture handle (for ge3d)
};



#endif
