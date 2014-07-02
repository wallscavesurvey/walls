//<copyright>
// 
// Copyright (c) 1995
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        writevrml.C
//
// Purpose:     implementation of SDFScene::writeVRML (conversion SDF to VRML)
//
// Created:      5 May 95   Michael Pichler (with some assistance by Peter Mikl in May 95)
//
// Changed:     27 Jul 95
//
//
//</file>


#include "sdfscene.h"
#include "geomobj.h"
#include "material.h"
#include "light.h"

#include "ge3d.h"
#include <iostream>


const char* const vrml_header = "#VRML V1.0 ascii\n";


// writeVRML
// write the current SDF scene in VRML format

// assumes that transformation matrices have been built (for objects
// with parents) and makes use of ge3d calls for matrix calculations.
// Therefore it can only be used interactive and not in batch mode.

void SDFScene::writeVRML (ostream& os)
{
  os << vrml_header << endl;
  os << "# written with Walls3D" << endl;

  os << "\nSeparator {" << endl;

  os << "\nShapeHints { vertexOrdering COUNTERCLOCKWISE shapeType SOLID faceType CONVEX }\n" << endl;

  // TODO: camera

  // material definitions
  for (const Material* matptr = (Material*) materlist_.first ();  matptr;
                       matptr = (Material*) materlist_.next ())
  {
    const colorRGB* color = matptr->natural ();
    os << "DEF Mat_" << matptr->name () << " Material { ";
    os << "diffuseColor " << color->R << ' ' << color->G << ' ' << color->B << " }\n";
    // TODO: texture
  }
  os << endl;

  static point3D deflgtpos = { 0.0, 0.0, 0.0 };
  static vector3D deflgtdir = { -1.0, 0.0, 0.0 };

  // light sources
  for (const Light* lgtptr = (Light*) lightlist_.first ();  lgtptr;
                    lgtptr = (Light*) lightlist_.next ())
  {
    // transforming position/orientation (SDF and VRML conventions differ)
    // TODO: light sources relative to camera

    ge3d_push_new_matrix ((const float (*) [4]) lgtptr->getTrfMat ());

    const colorRGB* intens = lgtptr->intensity ();
    if (lgtptr->positional ())
    {
      point3D pos;
      ge3dTransformMcWc (&deflgtpos, &pos);

      os << "PointLight {\n"
         << "\tcolor " << intens->R << ' ' << intens->G << ' ' << intens->B << '\n'
         << "\tlocation " << pos.x << ' ' << pos.y << ' ' << pos.z << '\n'
         << "}\n" << endl;
    }
    else // directional light
    { // SDF (WF): vector towards light source, default (-1, 0, 0) ("from left")
      // WRL (IV): vector parallel to light emission, default (0, 0, -1) ("to the back")
      // TODO (construct sample scene first)
      os << "DirectionalLight {\n"
         << "}\n" << endl;
    }

    ge3d_pop_matrix ();
  } // lights

  // geometric objects
  for (const GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();  gobjptr;
                              gobjptr = (GeometricObject*) gobjlist_.next ())
  {
    os << "TransformSeparator {\n";
    gobjptr->writeVRMLTransformation (os);

    gobjptr->writeVRML (os);  // also handles shared objects

    os << "}\n" << endl;

  }  // for gobjptr

  os << "} # EOF" << endl;  // outermost Seperator

} // writeVRML
