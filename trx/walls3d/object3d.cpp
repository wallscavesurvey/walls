//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        object.C
//
// Purpose:     implementation of class object3D
//
// Created:     12 Mar 92   Michael Hofer (until May 92) and Michael Pichler
//
// Changed:      1 Aug 94   Michael Pichler (Peter Mikl in May 95)
//
// Changed:     26 Jul 95   Michael Pichler
//
//
//</file>



#include "object3d.h"
#include "vecutil.h"

#include <string.h>
#include <iostream>

#ifndef M_PI  /* normally in values.h, included by vecutil.h */
#include <math.h>
#endif

#define RADIANS(d)  ((d) * (M_PI / 180.0))


Object3D::Object3D (int obj_n, int par, const char* name)
{
  obj_num_ = obj_n;
  parent_ = par;
  if (name)
  { strncpy (name_, name, 31);  // assuming char name_ [32]
    name_ [31] = '\0';
  }
  else  *name_ = '\0';  // ""

  channel_ [ch_xtran] = channel_ [ch_ytran] = channel_ [ch_ztran] = 0.0;
  channel_ [ch_xrot] = channel_ [ch_yrot] = channel_ [ch_zrot] = 0.0;
  channel_ [ch_xscale] = channel_ [ch_yscale] = channel_ [ch_zscale] = channel_ [ch_scale] = 1.0;

  strcpy (transfprior_, "trs");
  strcpy (rotprior_, "xyz");
}


static const char* channel_name [Object3D::num_channels] =
{ "XTRAN", "YTRAN", "ZTRAN", "XROT", "YROT", "ZROT",
  "XSCALE", "YSCALE", "ZSCALE", "SCALE"
};

const char* Object3D::channelName (int i)
{
  return channel_name [i];
}



// printobj prints name, number and channel values of an object
// designed for call in print () of derived classes


void Object3D::printobj ()
{
  cout << "name: '" << name_ << "', objectnumber: " << obj_num_;
  if (parent_)
    cout << ", parent: " << parent_;
  cout << endl;

  cout << "  channelvalues (TRS):";
  for (int i = 0; i < 10; i++)
     cout << "  " << channel_ [i];
  cout << endl;
}



// writeVRMLTransformation
// write Object's transformation in VRML format to output stream

void Object3D::writeVRMLTransformation (ostream& os) const
{
  if (parent_)  // use cumulative transformation matrix
  {
    os << "MatrixTransform { matrix\n";
    for (int row = 0;  row < 4;  row++)
    {
      const float* mat = trfmat_ [row];
      os << '\t' << mat [0] << ' ' << mat [1] << ' ' << mat [2] << ' ' << mat [3] << '\n';
    }
    os << "}\n";

    return;
  }

  // otherwise use transformation channels
  float x, y, z, mult;

  // note: when transfprior equals "trs" AND there is only up to one
  // rotation then all transformations may be combined into a "Transform" node

  for (const char* tptr = transfprior_;  *tptr;  tptr++)
    switch (*tptr)
    {
      case 't':  case 'T':  // translation
        x = channel_ [Object3D::ch_xtran];
        y = channel_ [Object3D::ch_ytran];
        z = channel_ [Object3D::ch_ztran];
        if (x || y || z)
          os << "Translation { translation " << x << ' ' << y << ' ' << z << " }\n";
      break;

      case 'r':  case 'R':  // rotation
      {
        x = channel_ [Object3D::ch_xrot];
        y = channel_ [Object3D::ch_yrot];
        z = channel_ [Object3D::ch_zrot];
        for (const char* rptr = rotprior_;  *rptr;  rptr++)
          switch (*rptr)
          {
            case 'x':  case 'X':
              if (x)
                os << "Rotation { rotation 1 0 0 " << RADIANS (x) << " }\n";
            break;
            case 'y':  case 'Y':
              if (y)
                os << "Rotation { rotation 0 1 0 " << RADIANS (y) << " }\n";
            break;
            case 'z':  case 'Z':
              if (z)
                os << "Rotation { rotation 0 0 1 " << RADIANS (z) << " }\n";
            break;
          }  // for, switch (rotation)
      }
      break;

      case 's':  case 'S':  // scaling
        mult = channel_ [Object3D::ch_scale];
        x = mult * channel_ [Object3D::ch_xscale];
        y = mult * channel_ [Object3D::ch_yscale];
        z = mult * channel_ [Object3D::ch_zscale];
        if (x != 1.0 || y != 1.0 || z != 1.0)
        {
          os << "Scale { scaleFactor ";
          os << x << ' ' << y << ' ' << z << " }\n";
        }
      break;
    } // for, switch (transformation)

} // writeVRMLTransformation
