//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        material.C
//
// Purpose:     implementation of class Material
//
// Created:      8 Feb 93   Michael Pichler
//
// Changed:     13 Nov 95   Michael Pichler
//
// $Id: material.C,v 1.2 1995/11/14 15:08:15 mpichler Exp $
//
//</file>


#include "material.h"

#include "scene3d.h"

#include "ge3d.h"
#include "widgets/hlsutil.h"

#include <string.h>
#include <stdio.h>
#include <iostream>



// Material
// set natural color to (r, g, b),
// compute highlighted and dark color (for l_brightness)


Material::Material (const char* name, float r, float g, float b, const char* texture)
: name_ (name), texture_ (texture)
{
  initRGB (natural_, r, g, b);

  // L_ = (r + g + b) / 3;
  L_ = 0.299 * r + 0.587 * g + 0.114 * b;  // brightness "Y"

  if (texture && *texture)
    texhandle_ = -1;  // texture will be read on demand
  else
    texhandle_ = 0;

  const float lminpick = Scene3D::anchors_minbrightness;
  const float lmaxunpick = Scene3D::nonanch_maxbrightness;

/*
  // bright color: add some white
  bright_.R = (1 - LminPick) * r + LminPick;
  bright_.G = (1 - LminPick) * g + LminPick;
  bright_.B = (1 - LminPick) * b + LminPick;

  // dark color: add some black
  dark_.R = LmaxUnpick * r;
  dark_.G = LmaxUnpick * g;
  dark_.B = LmaxUnpick * b;
*/

  float H, L, S;  // modify L of HLS color model for bright and dark color

  RGBtoHLS (r, g, b, H, L, S);
  // bright: [LminPick_..1.0]
  HLStoRGB (H, (1 - lminpick) * L + lminpick, S, bright_.R, bright_.G, bright_.B);
  // dark: [0.0..LmaxUnpick_]
  HLStoRGB (H, L * lmaxunpick, S, dark_.R, dark_.G, dark_.B);

// cerr << "created Material " << name << " with 'natural' ("
//      << natural_.R << ", " << natural_.G << ", " << natural_.B << "), 'bright' ("
//      << bright_.R << ", " << bright_.G << ", " << bright_.B << "), 'dark' ("
//      << dark_.R << ", " << dark_.G << ", " << dark_.B << ")." << endl;
  
} // Material


Material::~Material ()
{
  if (texhandle_ > 0)
    ge3dFreeTexture (texhandle_);
}


void Material::print () const
{
  cout << "Material '" << name_ << "', natural RGB = ("
       << natural_.R << ", " << natural_.G << ", " << natural_.B << ')';
  if (texture_.length ())
    cout << ", Texture: " << texture_;
  cout << endl;
}


void Material::setge3d (
  int hilitindex, int texturing, int pickable,
  const colorRGB* col_anchorface,
  const colorRGB* col_anchoredge
) const
{
// cerr << "Material::setge3d. hilitindex: " << hilitindex << " (diffcolor = " << Scene3D::l_diffcolor << "), "
//      << "pickable: " << pickable << ", texturing: " << texturing << endl;

  switch (hilitindex)
  {
    case Scene3D::l_brightness:
    { const colorRGB* color = pickable ? &bright_ : &dark_;
      ge3dFillColor (color);
    }
    break;

    case Scene3D::l_diffcolor:
      if (pickable)
      { const float lminpick = Scene3D::anchors_minbrightness;
        float l = (1 - lminpick) * L_ + lminpick;  // [LminPick..1.0]
	ge3d_setfillcolor (l * col_anchorface->R, l * col_anchorface->G, l * col_anchorface->B);
      }
      else
	ge3d_setfillcolor (L_, L_, L_);  // gray shade
    break;

    case Scene3D::l_coloredges:
#if defined(HPUX) || defined(PMAX)
      ge3dFillColor (&((Material*) this)->natural_);  // goofy compiler
#else
      ge3dFillColor (&natural_);
#endif
      if (pickable)
      { // ge3d_setlinestyle (0xF00F);
        ge3dLineColor (col_anchoredge);
      }
      // else  ge3d_setlinestyle (-1);  // solid
    break;

    default:  // natural or boundingbox
#if defined(HPUX) || defined(PMAX)
      ge3dFillColor (&((Material*) this)->natural_);  // goofy compiler
#else
      ge3dFillColor (&natural_);
#endif
    break;
  } // switch hilitindex

  // on texture mapping the object colour should be combined with the texture colour
  // currently, ge3d does not - when it does, users may want to turn it off optionally

  if (texturing)
  {
    if (texhandle_ > 0)
    { ge3dDoTexturing (1);
      ge3dApplyTexture (texhandle_);
    }
    else
      ge3dDoTexturing (0);
  }

} // setge3d
