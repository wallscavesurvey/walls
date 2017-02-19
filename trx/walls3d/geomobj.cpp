//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        geomobj.C
//
// Purpose:     implementation of class GeometricObject
//
// Created:     12 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     31 Jul 95   Michael Pichler
//
//
//</file>



#include "geomobj.h"
#include "srcanch.h"
#include "vecutil.h"
#include "strarray.h"

#include "ge3d.h"

#include "utils/str.h"
#include "hyperg/verbose.h"

#include <string.h>
#include <iostream>



GeometricObject::GeometricObject (int obj_n, int par, const char* name)
: Object3D (obj_n, par, name)
{
  hasobjectanchors_ = hasgroupanchors_ = 0;

  // for standalone viewer:
  // pickable, iff name contains "(filename)T", where T is the type of file:
  // T for text, I for raster image or 3 for 3D scene
  // Harmony viewer: all anchors are cleared and the real ones set

  if (name && strchr (name, '(') && strchr (name, ')'))
  { anchorlist_.put (new SourceAnchor (obj_n, "", 0));
    hasobjectanchors_ = 1;
  }

  selected_ = 0;
  selectedgroup_ = 0;
}



GeometricObject::~GeometricObject ()
{
  clearAnchors ();  // free anchor list
}



unsigned long GeometricObject::numFaces () const
{
  return 0;  // abstract object types cannot count this
}



// select
// if group is non nil select group, otherwise select whole object
// returns 1 on change of selection

void GeometricObject::select (const char* group)
{
  selected_ = !group;

  if ((group && !selectedgroup_) || (selectedgroup_ && !group)  // turned on/off group selection
      || (group && strcmp (group, selectedgroup_)))             // or changed selected group
  {
    selectedgroup_ = group;
    groupSelectionChanged ();
  }
}



// unselect
// returns 1 on change of selection

void GeometricObject::unselect ()
{
  selected_ = 0;

  if (selectedgroup_)
  {
    selectedgroup_ = 0;
    groupSelectionChanged ();
  }
}



void GeometricObject::worldBounding (point3D& sc_min, point3D& sc_max)
{
  // set bounding box in world coordinates
  point3D& wmin = minbound_wc_;
  point3D& wmax = maxbound_wc_;

  ge3d_push_new_matrix ((const float (*) [4]) trfmat_);

  computeBoundingbox (minbound_oc_, maxbound_oc_, wmin, wmax);

  ge3d_pop_matrix ();

  // now min/max-bound_wc is the bounding box in world coordinates

  // update scene (world) bounding box
  if (wmin.x < sc_min.x)  sc_min.x = wmin.x;
  if (wmin.y < sc_min.y)  sc_min.y = wmin.y;
  if (wmin.z < sc_min.z)  sc_min.z = wmin.z;
  if (wmax.x > sc_max.x)  sc_max.x = wmax.x;
  if (wmax.y > sc_max.y)  sc_max.y = wmax.y;
  if (wmax.z > sc_max.z)  sc_max.z = wmax.z;

} // worldBounding



// setAnchor
// add a source anchor definition

void GeometricObject::setAnchor (long id, const RString& aobj, const char* groupname)
{
  anchorlist_.put (new SourceAnchor (id, aobj, groupname));
  if (groupname)  // group anchor (oterwise object anchor)
  { hasgroupanchors_ = 1;
    groupAnchorsChanged ();  // notify derived class
  }
  else
    hasobjectanchors_ = 1;
}


// clearAnchors
// delete all anchors

void GeometricObject::clearAnchors ()
{
  // delete anchor data
  for (SourceAnchor* anch = (SourceAnchor*) anchorlist_.first ();  anch;
                     anch = (SourceAnchor*) anchorlist_.next ())
  { delete anch;
  }
  hasobjectanchors_ = hasgroupanchors_ = 0;
  anchorlist_.clear ();  // delete list
}


// getNextAnchor
// search next anchor that matches groups (group or object anchor)
// caller responsible for correct selection (highlighting)

const SourceAnchor* GeometricObject::getNextAnchor (const StringArray* groups)
{
  if (anchorlist_.isempty ())  // no anchors
    return 0;

  SourceAnchor* curranch = (SourceAnchor*) anchorlist_.current ();
  if (!curranch)
    curranch = (SourceAnchor*) anchorlist_.first ();
  // now curranch is non NULL and points to the current source anchor

  SourceAnchor* anch = curranch;

  do
  {
    anch = (SourceAnchor*) anchorlist_.next ();
    if (!anch)  // wrap around list
      anch = (SourceAnchor*) anchorlist_.first ();

    const char* anchorgroup = anch->groupName ();

    if (!anchorgroup  // object anchors always match (regardless of groups hit)
    || groups && groups->contains (anchorgroup))  // hit group of anchor
    {
      return anch;
    }

  } while (anch != curranch);  // until curranch examined again

  return 0;  // no anchor matched

} // getNextAnchor



// checkAnchorSelection
// check whether an anchor matches the currently selected group

void GeometricObject::checkAnchorSelection ()
{
  if (anchorlist_.isempty ())  // no anchors
    return;

  const char* selgroup = selectedgroup_;

  DEBUGNL ("GeometricObject::checkAnchorSelection");

  SourceAnchor* curranch = (SourceAnchor*) anchorlist_.current ();
  if (!curranch)
    curranch = (SourceAnchor*) anchorlist_.first ();
  // now curranch is non NULL and points to the current source anchor

  SourceAnchor* anch = curranch;

  do
  {
    const char* anchorgroup = anch->groupName ();

    if (!anchorgroup && !selgroup || anchorgroup && selgroup && !strcmp (anchorgroup, selgroup))
      return;  // anchorlist_.current now points to a matching anchor

    anch = (SourceAnchor*) anchorlist_.next ();
    if (!anch)  // wrap around list
      anch = (SourceAnchor*) anchorlist_.first ();

  } while (anch != curranch);  // until curranch examined again

  // no anchor matches current selection - set current to NULL
  anchorlist_.last ();
  anchorlist_.next ();

} // checkAnchorSelection



// rayhits
// tests with bounding box (world coord.) and calls rayhits_ only
// if the ray hits the bounding box (at t: 0 < t < tmin) or the camera is inside the box
// (hit at t > tnear possible when boundig box hit at t < tnear)

int GeometricObject::ray_hits (const point3D& A, const vector3D& b, float tnear,
                               float& tmin, point3D* normal, const StringArray** groups)
{
  if (rayhitscube (A, b, 0, tmin, minbound_wc_, maxbound_wc_)
   || pointinsidecube (A, minbound_wc_, maxbound_wc_))
  { // cout << '+' << obj_num;
    return  rayhits_ (A, b, tnear, tmin, normal, groups);
  }
  else
  { // cout << '-' << obj_num;
    return 0;
  }
}



// draw
// load transformation matrix and call draw_
// colorindex tells whether/how to highlight the object
// color(s) must be set inside draw_ ()

void GeometricObject::draw (int hilitindex, int texturing, const colorRGB* col_anchorface, const colorRGB* col_anchoredge)
{
  ge3d_push_this_matrix ((const float (*) [4]) trfmat_);

  draw_ (hilitindex, texturing, col_anchorface, col_anchoredge); 

  if (texturing)  // turn off texturing
    ge3dDoTexturing (0);

//  if (colorindex == Scene3D::l_boundingbox && anchorid_)  // draw bounding box

  if (selected_)  // hightlight selection with bounding box
  {
    const short l_pat = (short) 0xF00F;  // 16 bit

    // dashes in complementary color
    ge3d_setlinestyle (~l_pat);
//    ge3d_setlinecolor (1.0 - col_anchoredge->R, 1.0 - col_anchoredge->G, 1.0 - col_anchoredge->B);
    ge3d_setlinecolor (1.0, 1.0 - col_anchoredge->G, 1.0 - col_anchoredge->B);  // reddish complement
    ge3dWireCube (&minbound_oc_, &maxbound_oc_);
    // dashes in color anchor edge
    ge3d_setlinestyle (l_pat);
    ge3dLineColor (col_anchoredge);
    ge3dWireCube (&minbound_oc_, &maxbound_oc_);
    ge3d_setlinestyle (-1);
  }

  ge3d_pop_matrix ();
}
