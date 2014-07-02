//<copyright>
// 
// Copyright (c) 1992,93,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        sdfscene.C
//
// Purpose:     implementation of class SDFScene, as far as related to drawing
//              see also readscene.C (reading) and scenepick.C (hit tests)
//
// Created:     24 Apr 95   Michael Pichler (extracted from scene.C)
//
// Changed:      3 Oct 95   Michael Pichler
//
//
//</file>


#include "sdfscene.h"
#include "memleak.h"

#include "scene3d.h"
#include "geomobj.h"
#include "srcanch.h"
#include "sdfcam.h"
#include "light.h"
#include "material.h"
#include "hiertypes.h"

#include "vecutil.h"
#include "ge3d.h"

#include "hyperg/message.h"
#include "hyperg/verbose.h"

#include <string.h>
#include <math.h>

#ifndef MAXFLOAT
#ifdef SGI  /* to get rid of redefinition warnings in <values.h> */
# undef M_LN2
# undef M_PI
# undef M_SQRT2
#endif
//#include "values.h"
#endif



SDFScene::SDFScene (Scene3D* scene)
: SceneData (scene)
{
  num_objects_ = 0;
  num_gobj_ = num_cam_ = num_light_ = 0;
  numfaces_ = 0;
  activecam_ = origcam_ = NULL;
  matbuilt_ = 0;
  texturesloaded_ = 0;
} // SDFScene



SDFScene::~SDFScene ()
{
  // switch lights off (next scene may have fewer lights)
  DEBUGNL ("~SDFScene: switching light sources off");
  for (int i = 1;  i <= num_light_;  i++)  // ge3d light indices run from 1
    ge3d_switchlight (i, 0);

  // Delete the lists
  DEBUGNL ("~SDFScene: clearing data lists");

  for (GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();  gobjptr;
       gobjptr = (GeometricObject*) gobjlist_.next ())
    Delete gobjptr;  // virtual ~GeometricObject
  for (SDFCamera* camptr = (SDFCamera*) camlist_.first ();  camptr;
       camptr = (SDFCamera*) camlist_.next ())
    Delete camptr;
  for (Light* lightptr = (Light*) lightlist_.first ();  lightptr;
       lightptr = (Light*) lightlist_.next ())
    Delete lightptr;
  for (Material* matptr = (Material*) materlist_.first ();  matptr;
       matptr = (Material*) materlist_.next ())
    Delete matptr;

  gobjlist_.clear ();
  camlist_.clear ();
  lightlist_.clear ();
  allobjlist_.clear ();
  materlist_.clear ();

  // activecam_ is only a pointer into the camera list and therefore not deleted again
  delete origcam_;
/*
  // following only needed in case of a clear function for reusage of the same object
  num_objects_ = 0;
  num_gobj_ = num_cam_ = num_light_ = 0;
  numfaces_ = 0;
  activecam_ = NULL;
  origcam_ = NULL;
  matbuilt_ = 0;
  texturesloaded_ = 0;
*/
} // ~SDFScene


// int SDFScene::readFile (FILE* file) ... see readscene.C


// loadTextures

void SDFScene::loadTextures ()
{
  Material* matptr;

  // should give overall progress feedback instead on per image
  for (matptr = (Material*) materlist_.first ();  matptr;
       matptr = (Material*) materlist_.next ())
  {
    if (matptr->texhandle () < 0)
      scene_->loadTextureFile (matptr, matptr->texture ());
  }
}



// int SDFScene::writeData (ostream& os, int format) ... see readscene.C


// SDFScene::print
// all != 0: complete data, all == 0: short statistic

void SDFScene::printInfo (int all)
{
  GeometricObject* geomobjptr;
  SDFCamera* camptr;
  Light* lghtptr;
  Material* matptr;

  cout << endl;
  cout << "Number of geometric objects: " << num_gobj_ << endl;
  cout << "Number of cameras: " << num_cam_  << endl;
  cout << "Number of lights : " << num_light_ << endl;
  cout << endl;

  cout << "active camera: " << (activecam_ ? activecam_->name () : "<none>") << endl;
  cout << "scene bounding box : " << scene_->getMin () << " to " << scene_->getMax () << endl;
  cout << "'size' of the scene: " << scene_->size () << endl;
  cout << endl;

  for (matptr = (Material*) materlist_.first ();  matptr;
       matptr = (Material*) materlist_.next ())
  { matptr->print ();
  }
  cout << endl;

  for (geomobjptr = (GeometricObject*) gobjlist_.first (); geomobjptr;
       geomobjptr = (GeometricObject*) gobjlist_.next ())
  { geomobjptr->print (all);
  }

  for (camptr = (SDFCamera*) camlist_.first (); camptr;
       camptr = (SDFCamera*) camlist_.next ())
  { camptr->print ();
  }

  for (lghtptr = (Light*) lightlist_.first (); lghtptr;
       lghtptr = (Light*) lightlist_.next ())
  { lghtptr->print ();
  }
}  // SDFScene::print



// buildMatrix
// computes allobjarray [i]->obj->trfmat and invtrfmat if not yet done (flag)

void SDFScene::buildMatrix (objindexstruct* allobjarray, int i)
{
  objindexstruct* oaptr = allobjarray + i;  

  if (oaptr->flag)
    return;  // matrix already built


  Object3D* objptr = oaptr->obj;

  if (!objptr)
  { HgMessage::error ("unexistent object number in object hierarchy.");
    DEBUGNL ("hg3d: object number " << i << " does not exist; cannot build transformation matrix.");
    oaptr->flag = 1;
  }

  if (objptr->parent_)  // compute parent's matrix
  {
    buildMatrix (allobjarray, objptr->parent_);
    ge3d_push_new_matrix ((const float (*)[4]) allobjarray [objptr->parent_].obj->trfmat_);
  }
  else // object has no parent
    ge3dPushIdentity ();  // ignore possible garbage on stack

  // concatenate with r, s, t
  for (char* tptr = objptr->transfprior_;  *tptr;  tptr++)
    switch (*tptr)
    {
      case 't':  case 'T':
        ge3d_translate (objptr->channel_ [Object3D::ch_xtran],
          objptr->channel_ [Object3D::ch_ytran], objptr->channel_ [Object3D::ch_ztran]);
      break;

      case 'r':  case 'R':
      {
        for (char* rptr = objptr->rotprior_;  *rptr;  rptr++)
          switch (*rptr)
          { case 'x':  case 'X':  ge3d_rotate_axis (*rptr, objptr->channel_ [Object3D::ch_xrot]);  break;
            case 'y':  case 'Y':  ge3d_rotate_axis (*rptr, objptr->channel_ [Object3D::ch_yrot]);  break;
            case 'z':  case 'Z':  ge3d_rotate_axis (*rptr, objptr->channel_ [Object3D::ch_zrot]);  break;
          }
      }
      break;

      case 's':  case 'S':
        ge3d_scale (objptr->channel_ [Object3D::ch_xscale], objptr->channel_ [Object3D::ch_yscale], 
                    objptr->channel_ [Object3D::ch_zscale], objptr->channel_ [Object3D::ch_scale]);
      break;
    } // for, switch

  ge3d_get_and_pop_matrix (objptr->trfmat_);  // get a copy of the current transform. matrix

  // compute its inverse
  copymatrix (objptr->trfmat_, objptr->invtrfmat_);
  invertmatrix (objptr->invtrfmat_);

  oaptr->flag = 1;

} // SDFScene::buildMatrix



// buildMatrices
// compute the transformation matrix (and its inverse) for all objects,
// transform the bounding box of all geometric objects into world coordinates
// and compute scene bounding box and size

void SDFScene::buildMatrices ()
{
  int i;
  objindexstruct* allobjarray;


  allobjarray = New objindexstruct [num_objects_ + 1];  // used: allobjarray [1..num_objects_]

  objindexstruct* oaptr;
  for (i = 0, oaptr = allobjarray;  i <= num_objects_;  i++, oaptr++)
  { oaptr->obj = NULL;
    oaptr->flag = 0;  // not computed
  }

  for (Object3D* objptr = (Object3D*) allobjlist_.first ();  objptr;
                 objptr = (Object3D*) allobjlist_.next ())
  { int num = objptr->getobj_num ();
    if (num < 1 || num > num_objects_)
      cerr << "error: invalid object number " << num << "; cannot build transformation matrix." << endl;
    else
      allobjarray [num].obj = objptr;
  }


  for (i = 1;  i <= num_objects_;  i++)
    buildMatrix (allobjarray, i);

  Delete allobjarray;

  point3D wmin, wmax;

  if (!num_gobj_)  // no geometric objects
  { init3D (wmin, 0, 0, 0);
    init3D (wmax, 0, 0, 0);
  }
  else  // geometric objects
  {
    init3D (wmin, MAXFLOAT, MAXFLOAT, MAXFLOAT);
    init3D (wmax, -MAXFLOAT, -MAXFLOAT, -MAXFLOAT);

    // tell all geometric objects to compute their bounding box
    // in world coordinates, to update scene bounding box,
    // and to compute face normals (or any other data depending on transformation matrix)

    for (GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();
         gobjptr;         gobjptr = (GeometricObject*) gobjlist_.next ())
    { gobjptr->worldBounding (wmin, wmax);
      gobjptr->setupNormals ();
    }
  }

  scene_->setBoundings (wmin, wmax);

} // SDFScene::buildMatrices



// draw (all geometric objects, in current drawing mode)
// since other drawings AFTER Scene3D::draw are allowed,
// ge3d_swapbuffers must be perfomed by caller

void SDFScene::draw (int curmode)
{ 
  // assert: screen has been cleared
  if (!num_objects_ || !activecam_)  // no scene read
    return;

  // the following code should logically be done at the end of readscene, but GL
  // graphics library rejects all computations with a seg-fault before opening a window

  if (!matbuilt_)  // build matrices on first drawing
  {
    DEBUGNL ("Scene3D: drawing new scene first time (init ge3d, building matrices, transf. camera)");
    ge3d_init_ ();  // initialise ge3d library (multi matrix mode etc.)

    // build all transformation matrices and compute their inverses
    buildMatrices ();

    // compute position and lookat of all cameras (allows later change of camera)
    for (SDFCamera* camptr = (SDFCamera*) camlist_.first ();  camptr;
                    camptr = (SDFCamera*) camlist_.next ())
      camptr->computeParams ();

    matbuilt_ = 1;

    // save original camera (read in from file)
    origcam_ = New SDFCamera (*activecam_);  // (copy camera)
  }

  ge3d_setmode (curmode);  // activate drawing mode

  // set up camera (may change at any time)
  activecam_->setCamera (scene_);

  // light sources must be set up any time camera changes,
  // because light calculations are done in camera coordinate system
  if (curmode >= ge3d_flat_shading)
  {
    int i = 1;
    Light* lghtptr = (Light*) lightlist_.first ();

    while (lghtptr)
    {
      lghtptr->setlight (i);
      ge3d_switchlight (i, 1);
      lghtptr = (Light*) lightlist_.next ();
      i++;
    }
  }

  // do backfaceculling unless always twosided
  ge3dHint (hint_backfaceculling, scene_->twosidedpolys () != Scene3D::twosided_always);
  // do lighting unless unless turned off globally
  ge3dHint (hint_lighting, scene_->dolighting () != Scene3D::lighting_never);
  // for SDF texture maps always without lighting (TODO: implement in ge3d)
  ge3dHint (hint_texlighting, 0);

  int texturing = (curmode == ge3d_texturing);

  // TODO: for stand alone viewer should draw scene once without textures,
  // because requestTextures will block until all textures are read!
  if (texturing && !texturesloaded_)  // load textures on first need
  {
    texturesloaded_ = 1;
    scene_->requestTextures ();
  }

  // draw all geometric objects
  const colorRGB* col_anchorface = &scene_->col_anchorface;
  const colorRGB* col_anchoredge = &scene_->col_anchoredge;

  int hilitindex = (scene_->linksActive ()) ? scene_->linksColor () : 0;

  for (GeometricObject* geomobjptr = (GeometricObject*) gobjlist_.first ();  geomobjptr;
       geomobjptr = (GeometricObject*) gobjlist_.next ())
  { geomobjptr->draw (hilitindex, texturing, col_anchorface, col_anchoredge);
  }

//   if (showaxis_)
//   { point3D center;
//     lcm3D (0.5, worldmin_, 0.5, worldmax_, center);
//     // draw axis up to worldmin/max
//   }

  ge3dHint (hint_lighting, 1);

}  // SDFScene::draw


// find object by name

GeometricObject* SDFScene::findObject (const char* objname)
{
  for (GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();  gobjptr;
       gobjptr = (GeometricObject*) gobjlist_.next ())
  {
    if (!strcmp (objname, gobjptr->name ()))
      return gobjptr;
  }

  return 0;
}



// find object by object number

GeometricObject* SDFScene::findObject (int objnum)
{
  // here an array of geometric object would be nice
  for (GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();  gobjptr;
       gobjptr = (GeometricObject*) gobjlist_.next ())
  {
    if (objnum == gobjptr->getobj_num ())
      return gobjptr;
  }

  return 0;
}



Material* SDFScene::findMaterial (const char* matname)
{
  //cerr << "searching for material " << matname << endl;

  Material* matptr;
  for (matptr = (Material*) materlist_.first ();  matptr;
       matptr = (Material*) materlist_.next ())
  { if (!strcmp (matname, matptr->name ()))  // found
      return matptr;
  }

  return 0;  // not found
} // findMaterial



// clearAnchors
// clear all anchors of the scene

void SDFScene::clearAnchors ()
{
  for (GeometricObject* gobjptr = (GeometricObject*) gobjlist_.first ();  gobjptr;
       gobjptr = (GeometricObject*) gobjlist_.next ())
    gobjptr->clearAnchors ();
}


Camera* SDFScene::getCamera () const
{
  return activecam_;
}


void SDFScene::storeCamera ()
{
  if (activecam_ && origcam_)
    *origcam_ = *activecam_;  // store (copy) current active camera
}


void SDFScene::restoreCamera ()
{
  if (activecam_ && origcam_)
    *activecam_ = *origcam_;  // restore (copy) original camera to active camera
}
