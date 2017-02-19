//<copyright>
// 
// Copyright (c) 1993,94
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        light.C
//
// Purpose:     implementation of class Light
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     17 Mar 94   Michael Pichler
//
//
//</file>



#include "light.h"
#include "vecutil.h"

#include "ge3d.h"


#include <iostream>
#include <stdio.h>
#include <ctype.h>

#pragma warning(disable:4305)

Light::Light (int obj_n, int par, const char* name)
: Object3D (obj_n, par, name)
{
  initRGB (intensity_, 0.7, 0.7, 0.7);  // default intensity
  // default: positional light source
  positional_ = 1.0;
  camlgt_ = 0;
}



// implementation of Light::readlgtfile see readscene.C



void Light::print ()
{
  cout << "Light. ";

  printobj ();

  cout << "  Intensity (rgb): " 
    << intensity_.R << ", "
    << intensity_.G << ", "
    << intensity_.B << endl;

  cout << "  " << (positional_ ? "positional" : "directional (inifite)") << " light source";
  if (camlgt_)
    cout << ", bound to active camera";
  cout << '.' << endl;

  cout << endl;
}



void Light::setlight (int index)
{ 
  // note: position cannot be stored, as GL's viewing transformation is stored
  // in the model matrix and not in the projection matrix
  // otherwise lighting calculations are incorrect

//cerr << "setting a light source. positional: " << positional_ << ", camera light: " << camlgt_ << endl;

  ge3dLightSource (index, &intensity_, trfmat_, positional_, camlgt_);
}
