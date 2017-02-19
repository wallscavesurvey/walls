//<copyright>
// 
// Copyright (c) 1994,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        writescene.C
//
// Purpose:     implementation of class scene3D, as far as related to writing
//              scene data to file; see also scene.C, readscene.C, and scenepick.C
//
// Created:      1 Aug 94   Michael Pichler
//
// Changed:     30 Jan 95   Michael Pichler
//
//
//</file>


#include "scene3d.h"

#include "slist.h"
#include "geomobj.h"
#include "srcanch.h"
#include "sdfcam.h"
#include "light.h"
#include "material.h"
#include "vecutil.h"

#include "hyperg/message.h"
#include "hyperg/verbose.h"

#include <iostream>
#include <stdio.h>
#include <string.h>

#define FILE_SENTINEL '@'
// marker between logical file parts

const char* const sdf_header =
  "# Hyper-G 3D scene description file\n"
  "# written with Harmony 3D Scene Viewer\n";

const char* const act_header =
  "Obj                                               Rot  Tran  Disp\n"
  "Num Name         File Name            Typ Parent Prior Prior Stat Chans Color  \n"
  "--- ------------ -------------------- --- ------ ----- ----- ---- ----- -------\n";

const char* const cam_header =
  "Obj              Proj Focal       Aspect -------------- Viewport --------------\n"
  "Num Name         Type Lngth Aper  Ratio  Left  Right Bot   Top   Hither Yon\n"
  "--- ------------ ---- ----- ----- ------ --------------------------------------\n";


// write the Scene (SDF; VRML is passed to writeVRML; Inventor, DXF may follow)

int SDFScene::writeData (ostream& os, int format)
{
  if (!os)
    return -1;

  if (format == Scene3D::write_VRML)
  { writeVRML (os);
    return 0;
  }

  // otherwise: write in SDF format
  os << sdf_header << FILE_SENTINEL << endl;

  writeActors (os);
  writePositions (os);
  writeCameras (os);
  writeMaterials (os);
  writeLights (os);
  writeObjects (os);

  return 0;
} // writeData


// functions for writing the several parts of the SDF file

void SDFScene::writeActors (ostream& os)
{
  os << act_header;

  os.setf (ios::left, ios::adjustfield);
  for (const Object3D* objptr = (Object3D*) allobjlist_.first ();  objptr;
                       objptr = (Object3D*) allobjlist_.next ())
  {
    int objnum = objptr->getobj_num ();
    os.width (3);   os << objnum << ' ';

    const char* objname = objptr->name ();
    os.width (12);  os << (objname && *objname ? objname : "unset") << ' ';

    int shared_info = objptr->sharedInfo ();
    if (shared_info)
    { os << '&';  os.width (19);  os << shared_info << ' ';
    }
    else
    { os.width (21); os << "unused";
    }

    const char* objtype = objptr->type ();
    os << objtype;

    os << "   ";  os.width (4);  os << objptr->getparent ();

    os << "  " << objptr->getRotPrior ()
       << "   " << objptr->getTrfPrior ();

    if (!strcmp (objtype, "cam") && activecam_ && objnum != activecam_->getobj_num ())  // inactive camera
      os << "   OFF";
    else  // other objects not displayed became dummies on reading
      os << "   ON ";
    os << "   ";  os.width (3);  os << (int) Object3D::num_channels;  // always store all channels

    os << " GREEN\n";  // unused
  }
  os.unsetf (ios::left);

  os << FILE_SENTINEL << endl;

} // writeActors


void SDFScene::writePositions (ostream& os)
{
  int i, j;

  os << "     |";  // line 1: channel labels
  for (i = 0;  i < num_objects_;  i++)
    for (j = 0;  j < Object3D::num_channels;  j++)
      os << ' ' << Object3D::channelName (j);
  os << '\n';

  os << "Frame| units of measurment unused\n";

  os << "  #  |";  // line 3: channel numbers
  for (i = 0;  i < num_objects_;  i++)
    for (j = 0;  j < Object3D::num_channels;  j++)
      os << ' ' << i+1 << '/' << j+1;
  os << '\n';

  os << "--------------------\n";  // separator on line 4

  os << " 1   |";  // line 5: channel data

  for (const Object3D* objptr = (Object3D*) allobjlist_.first ();  objptr;
                       objptr = (Object3D*) allobjlist_.next ())
    for (j = 0;  j < Object3D::num_channels;  j++)
      os << ' ' << objptr->getChannel (j);
  os << '\n';

  os << FILE_SENTINEL << endl;

} // writePositions


void SDFScene::writeCameras (ostream& os)
{
  os << cam_header;

  for (const SDFCamera* camptr = (SDFCamera*) camlist_.first ();  camptr;
                        camptr = (SDFCamera*) camlist_.next ())
  {
    os << camptr->getobj_num () << ' ' << camptr->name () << ' ';
    os << camptr->projtype_ << ' ' << camptr->focallen_ << ' ';
    os << camptr->aper_*camptr->aspect_ << ' ' << camptr->aspect_ << ' ';  // horicontal aper
    os << camptr->left_ << ' ' << camptr->right_ << ' ';
    os << camptr->bottom_ << ' ' << camptr->top_ << ' ';
    os << camptr->hither_ << ' ' << camptr->yon_ << '\n';
  }

  os << FILE_SENTINEL << endl;

} // writeCameras


void SDFScene::writeMaterials (ostream& os)
{
  for (const Material* matptr = (Material*) materlist_.first ();  matptr;
                       matptr = (Material*) materlist_.next ())
  {
    os << "newmtl " << matptr->name () << '\n';
    const colorRGB* color = matptr->natural ();
    const char* texture = matptr->texture ();
    os << "Ka " << color->R << ' ' << color->G << ' ' << color->B << '\n';
    os << "Kd " << color->R << ' ' << color->G << ' ' << color->B << '\n';
    if (texture && *texture)
      os << "map_Kd " << texture << '\n';
    os << "illum 1\n\n";  // ambient + diffuse
  }

  os << FILE_SENTINEL << endl;
} // writeMaterials


void SDFScene::writeLights (ostream& os)
{
  for (const Light* lgtptr = (Light*) lightlist_.first ();  lgtptr;
                    lgtptr = (Light*) lightlist_.next ())
  {
    os << "# " << lgtptr->name () << '\n';

    const colorRGB* intensity = lgtptr->intensity ();
    os << "intensity " << intensity->R << ' ' << intensity->G << ' ' << intensity->B << '\n';
    if (!lgtptr->positional ())
      os << "directional" << '\n';

    os << FILE_SENTINEL << endl;
  }

} // writeLights


void SDFScene::writeObjects (ostream& os)
{
  for (const GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();  gobjptr;
                              gobjptr = (GeometricObject*) gobjlist_.next ())
  {
    if (!gobjptr->sharedInfo ())
    {
      gobjptr->writeData (os);
      os << FILE_SENTINEL << endl;
    }
  }

} // writeObjects


#ifdef TODO  /* writeDepth */
// writeDepth
// writes a depth bitmap of the scene
// the current approach of calculating the depth of each pixel "by hand" is *very* slow
// reading the Z-buffer seems impossible: pixmode (PM_ZDATA, 1) has no effect on lrectread
// OpenGL supports reading the Z-buffer, so it will be done that way ...

int SDFScene::writeDepth (ostream& os, int width, int height, int /*format*/)
{
  if (!os)
    return -1;

  if (!activecam_)
    return 0;

  // to avoid anomalies
  if (width < 2)  width = 2;
  if (height < 2)  height = 2;

  cerr << "calculating depth information (this may take a while) ..." << endl;

  unsigned long size = width * height;
  float* data = new float [size];
  float* dptr = data;

  if (!data)
  { cerr << "not enough memory for " << width << " x " << height << " pixel" << endl;
    return 0;
  }
  // the loop of pickObject will be integrated here!!!
  // now it's just a test wheter the speed is acceptable at all

  int i, j;
  float val, minval = MAXFLOAT, maxval = 0.0;

  for (j = 0;  j < height;  j++)
  {
    for (i = 0;  i < width;  i++)
    {
      if (pickObject ((i+0.5) / width, (height-j-0.5) / height, 0, 0, 0, &val))  // hit
      {
        *dptr++ = val;
        if (val < minval)  minval = val;
        if (val > maxval)  maxval = val;
      }
      else
        *dptr++ = -1.0;
    } // for each col

cerr << '*';  // will be replaced by progress (with stop option!!!)

  } // for each row

  // due to the calculation "range" is stored instead of "depth" - good/bad?

  if (minval >= maxval)
  { cerr << "no appropriate depth information in 3D scene - propably nothing visible" << endl;
    delete data;
    return 0;
  }

cerr << "\nminimum: " << minval << ", maximum: " << maxval << endl;

cerr << "writing depth PGM of size " << width << " x " << height << endl;

  // header: P5 = PGM binary, width, height, maxval (0..255)
  os << "P5\n# creator: Harmony 3D Scene Viewer\n" << width << ' ' << height << " 255";
  os << '\n';  // exactly one WS ('\t' would be safer under DOS)

  // hittimes are transformed to grey tones,
  // minval --> white (255),  maxval --> black/"dark" (0 ... should be variable!!!)
  // 255 * (maxval - x) / (maxval - minval)  does this
  minval = maxval - minval;  // denominator, > 0

  // data array (plain bytes)
  dptr = data;
  unsigned char pixel;
  while (size--)
  {
    if (*dptr < 0.0)  // nothing hit
      pixel = 0;
    else
      pixel = (unsigned char) (255 * (maxval - *dptr) / minval + 0.5);
    os << pixel;
    dptr++;
  }

  delete data;
  return 0;
} // writeDepth
#endif
