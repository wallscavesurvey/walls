//<copyright>
// 
// Copyright (c) 1995
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        wrldraw.C
//
// Purpose:     drawing routines of VRML nodes
//
// Created:     24 Apr 95   Michael Pichler
//
// Changed:     16 Feb 96   Michael Pichler
//
// $Id: wrldraw.C,v 1.14 1996/02/21 17:06:24 mpichler Exp $
//
//</file>


#if defined(PMAX) || defined (HPUX9)
enum Part { goofyPart };        // cfront confused about QvCone::Part and QvCylinder::Part
enum Binding { goofyBinding };  // cfront confused about QvMaterialBinding/QvNormalBinding::Binding
#endif


#include "qvlib/QvElement.h"
#include "qvlib/QvNodes.h"
#include "qvlib/QvExtensions.h"
#include "qvlib/QvUnknownNode.h"

#include "scene3d.h"
#include "camera.h"
#include "vrmlscene.h"
#include "vecutil.h"

#include "ge3d.h"

#include "hyperg/verbose.h"

#include <math.h>
#include <iostream>


static QvMaterial* draw_curmtl = 0;
static int draw_hilit = 0;  // which material component to use (QvMaterial::hilit_*)
static int draw_hilanchors = 0;  // how to highlight anchors    (- " -)
static int draw_hilnonanch = 0;  // how to draw non-anchors    (- " -)



/***** drawVRML *****/
// wrapper around root_->draw to set state flags

void VRMLScene::drawVRML ()
{
  static int anchorshilit [Scene3D::num_colors] =
  { // mapping of Scene anchor highlighting (Scene3D::l_*) to Material highlighting
    0, QvMaterial::hilit_bright /*brightness*/, QvMaterial::hilit_none /*BBox*/,
    QvMaterial::hilit_colshade /* color */, QvMaterial::hilit_none /*edges*/
  };
  static int nonanchhilit [Scene3D::num_colors] =
  { 0, QvMaterial::hilit_dark /*brightness*/, QvMaterial::hilit_none /*BBox*/,
    QvMaterial::hilit_greyshade /* color */, QvMaterial::hilit_none /*edges*/
  };
  // BBox hightlighting anachronism; color edge hightlighting: TODO (also in ge3d)

  if (scene_->linksActive ())
  {
    draw_hilanchors = anchorshilit [scene_->linksColor ()];
    draw_hilnonanch = nonanchhilit [scene_->linksColor ()];
  }
  else
    draw_hilanchors = draw_hilnonanch = QvMaterial::hilit_none;

  draw_curmtl = 0;  // no material active
  draw_hilit = draw_hilnonanch;  // now outside of any anchor

  // cerr << "using material " << draw_hilit << " and " << draw_hilanchors << " for anchors " << endl;

  ge3d_push_matrix ();  // top node need not be a separator

  root_->draw ();  // ass.: root_ != 0

  ge3d_pop_matrix ();

} // drawVRML



/***** groups *****/

void QvGroup::draw ()
{
  int n = getNumChildren ();
  for (int i = 0; i < n; i++)
    getChild (i)->draw ();
}


void QvLOD::draw ()
{
  Camera* cam = scene_->getCamera ();
  int numchildren = getNumChildren ();

  if (!cam || !numchildren)  // nothing to draw
  { lastdrawn_ = -1;
    return;
  }

  point3D campos, wcenter;
  cam->getposition (campos);

  // transform center_ into world coordinates
  ge3dTransformMcWc (center_, &wcenter);
  // cerr << "camera pos.: " << campos << ", world center: " << wcenter;
  dec3D (wcenter, *center_);
  float distance = norm3D (wcenter);
  // cerr << ", distance: " << distance << endl;

  int numranges = range.num;
  const float* ranges = range.values;

  int i;
  for (i = 0; i < numranges; i++)
  { if (distance < *ranges++)  // use child i
      break;
  } // to use all ranges there should be numranges+1 children

  if (i >= numchildren)  // default: last child
    i = numchildren-1;

  lastdrawn_ = i;
  getChild (i)->draw ();
} // QvLOD


void QvSeparator::draw ()
{
  int numlights = vrmlscene_->numLights ();
  //cerr << "QvSeparator::draw: push ()" << endl;
  ge3d_push_matrix ();

  int n = getNumChildren ();
  for (int i = 0; i < n; i++)
    getChild (i)->draw ();

  //cerr << "QvSeparator::draw: pop ()" << endl;
  ge3d_pop_matrix ();
  vrmlscene_->deactivateLights (numlights);
}


void QvSwitch::draw ()
{
  int which = (int) whichChild.value;

  if (which == QV_SWITCH_NONE)  // draw nothing
    return;
  if (which == QV_SWITCH_ALL)  // draw all children
    QvGroup::draw ();
  else if (which < getNumChildren ())  // draw one child
    getChild (which)->draw ();
}


void QvTransformSeparator::draw ()
{
  ge3d_push_matrix ();

  int n = getNumChildren ();
  for (int i = 0; i < n; i++)
    getChild (i)->draw ();

  ge3d_pop_matrix ();
}



#define DRAW(className)  \
void className::draw ()  { }



/***** coordinates *****/

// already handled in build
DRAW(QvCoordinate3)
DRAW(QvNormal)
DRAW(QvTextureCoordinate2)



/***** properties *****/

DRAW(QvFontStyle)

void QvMaterial::draw ()
{
//cerr << "QvMaterial::draw" << endl;
  // material binding (front/back) is not part of VRML (applys to both sides)

const materialsGE3D* mat = materials_ + draw_hilit;
ge3dMaterial ((int)(mat_front_and_back),mat);
// sets only basecolor in wireframe/hidden line or without lighting

// cerr << "num base: " << mat->num_base;
// if (mat->num_base)
// cerr << ", base RGB: "<< mat->rgb_base->R << ", " << mat->rgb_base->G << ", " << mat->rgb_base->B;
// cerr << endl;
// cerr << "num ambient: " << mat->num_ambient;
// if (mat->num_ambient)
// cerr << ", ambient RGB: "<< mat->rgb_ambient->R << ", " << mat->rgb_ambient->G << ", " << mat->rgb_ambient->B;
// cerr << endl;

  draw_curmtl = this;
}

DRAW(QvMaterialBinding)
DRAW(QvNormalBinding)


void QvShapeHints::draw ()
{
  if (scene_->twosidedpolys () == Scene3D::twosided_auto)
    ge3dHint (hint_backfaceculling, backfaceculling_);
}


void QvTexture2::draw ()
{
  if (curdrawmode_ == ge3d_texturing)
  {
    if (handle_ > 0)
    { ge3dDoTexturing (1);
      ge3dApplyTexture (handle_);
      ge3dTextureRepeat (reps_, rept_);
    }
    else  // turn off texturing
      ge3dDoTexturing (0);
  }
}


void QvTexture2Transform::draw ()
{
  // TODO: care for hierarchy (push/pop at Separator)

  if (curdrawmode_ == ge3d_texturing)
    ge3dMultTextureMatrix ((const float (*)[4]) mat_);
}



/***** lights *****/


void QvDirectionalLight::draw ()
{
  if (curdrawmode_ < ge3d_flat_shading)
    return;

  if (on.value)
  { int index = vrmlscene_->nextLight ();
    ge3dSetLightSource (index, &color_, &direction_, 0 /* directional */, 0);
    ge3d_switchlight (index, 1);
  }
}


void QvPointLight::draw ()
{
  if (curdrawmode_ < ge3d_flat_shading)
    return;

  if (on.value)
  { int index = vrmlscene_->nextLight ();
    ge3dSetLightSource (index, &color_, position_, 1 /* positional */, 0);
    ge3d_switchlight (index, 1);
  }
}


void QvSpotLight::draw ()
{
  if (curdrawmode_ < ge3d_flat_shading)
    return;

  if (on.value)
  { int index = vrmlscene_->nextLight ();
    ge3dSpotLight (index, &color_, position_, direction_, dropOffRate.value, cutangle_);
    ge3d_switchlight (index, 1);
  }
}



/***** cameras *****/


void QvOrthographicCamera::draw ()
{
  // TODO (?): provide mechanism for zooming with ortho cameras
//   Camera* cam = vrmlscene_->getCamera ();
//   ge3dCamera (/*cam_orthographic | */ cam_relative,
//     pos_, rotangle_*180/M_PI, rotaxis_, height_,
//     scene_->getWinAspect (),
//     0.0, cam->getyon ()
//   );

//   // orthographic transformation already set up by global Camera
//   ge3dRotate (rotaxis_, -rotangle_);
//   ge3d_translate (-pos_->x, -pos_->y, -pos_->z);

  ge3dMultMatrix ((const float (*)[4]) mat_);
}


void QvPerspectiveCamera::draw ()
{
//   Camera* cam = vrmlscene_->getCamera ();
//   // focal distance not relevant here (may be used for default overview)
//   ge3dCamera (/* cam_perspective | */ cam_relative,
//     pos_, rotangle_*180/M_PI, rotaxis_, yangle_*180/M_PI,
//     scene_->getWinAspect (),
//     cam->gethither (), cam->getyon ()
//   );

//   // perspective transformation already set up by global Camera
//   ge3dRotate (rotaxis_, -rotangle_);
//   ge3d_translate (-pos_->x, -pos_->y, -pos_->z);

  ge3dMultMatrix ((const float (*)[4]) mat_);
}


/***** transformations *****/

// TODO: may calculate overall transformations right after loading

void QvTransform::draw ()
{
  // cast: gorasche, 19950709 for WIN32
  ge3dMultMatrix ((const float (*)[4]) mat_);
}

void QvRotation::draw ()
{
  ge3dMultMatrix ((const float (*)[4]) mat_);
}

void QvMatrixTransform::draw ()
{
  ge3dMultMatrix (mat_);
}

void QvTranslation::draw ()
{
  ge3dTranslate (trans_);
}

void QvScale::draw ()
{
  ge3dScale (scale_);
}



/***** shapes *****/


void QvAsciiText::draw ()  // TODO
{
}


void QvCube::draw ()
{
  ge3dCube (&omin_, &omax_);
}


void QvCone::draw ()
{
  float h = height.value;
  ge3dCylinder (bottomRadius.value, 0 /* topradius */, - h / 2.0, h, parts_);
}


void QvCylinder::draw ()
{
  float r = radius.value;
  float h = height.value;
  ge3dCylinder (r, r, - h / 2.0, h, parts_);
}


void QvSphere::draw ()
{
  ge3dSphere (radius.value);
}


void QvIndexedFaceSet::draw ()
{
// cerr << "QvIndexedFaceSet::draw (" << numvertinds_ << ", " << numnormalinds_ << ", " << numtextureinds_ << ")" << endl;
// cerr << "vertexlist: " << vertexlist_ << ", []: " << *vertexlist_ << endl;
// cerr << "vertexindices: " << vertindices_ << ", []: " << *vertindices_ << endl;

  ge3dFaceSet (
    vertexlist_, numvertinds_, vertindices_,
    materials_ + draw_hilit, matbinding_, nummatinds_, matindices_,
    normallist_, numnormalinds_, normalindices_, facenormals_,
    texvertlist_, numtextureinds_, textureindices_
  );

} // QvIndexedFaceSet


void QvIndexedLineSet::draw ()
{
//cerr << "QvIndexedLineSet::draw" << endl;

  // assert: int and long of QvMFLong are of same size - see <vrml/QvMFLong>
  // assert: arrays of point3D's are compatible to arrays of float triples

  ge3dLineSet (vertexlist_, numvertinds_, vertindices_);
  // currently materials ignored (TODO) - (normals and textures not relevant)

} // QvIndexedLineSet


void QvPointSet::draw ()
{
  ge3dPointSet (points_, num_);  // TODO: materials (normals, textures not relevant)

} // QvPointSet



/***** WWW *****/


void QvWWWAnchor::draw ()
{
  int oldhilit = draw_hilit;  // care for nested anchors

  if (draw_hilit != draw_hilanchors)  // activate anchor highlighting
  {
    draw_hilit = draw_hilanchors;
    //cerr << "anchor: changing draw_hilit to " << draw_hilit << ", saved " << oldhilit << "; " << draw_curmtl << endl;
    if (draw_curmtl)
      draw_curmtl->draw ();
  }

  // spec: WWWAnchor must behave like a separator (although it is not derived from it)
  int numlights = vrmlscene_->numLights ();
  ge3d_push_matrix ();

  // draw children of group
  int n = getNumChildren ();
  for (int i = 0; i < n; i++)
    getChild (i)->draw ();

  ge3d_pop_matrix ();
  vrmlscene_->deactivateLights (numlights);

/*
  // test: draw a bounding box around anchor objects
  if (scene_->linksActive ())
  { // color change in effect for further primitives
    ge3dLineColor (&scene_->col_anchoredge);
    ge3dWireCube (&omin_, &omax_);
  }
*/

  if (draw_hilit != oldhilit)  // restore non-anchor material
  {
    //cerr << "anchor: resetting draw_hilit to " << oldhilit << "; " << draw_curmtl << endl;
    draw_hilit = oldhilit;  // draw_hilnonanch unless nested anchors
    if (draw_curmtl)
      draw_curmtl->draw ();
  } // TODO: keep track of draw_curmtl in Separators (restore; including Anchors)

} // QvWWWAnchor


void QvWWWInline::draw ()
{
  // fetch first time needed
  if (state_ == s_virgin)
  {
    DEBUGNL ("QvWWWInline first encountered. Sending request");
    state_ = s_requested;
    // the request of the inline URL is VRML specific, but was put into Scene3D to allow
    // separate treatment for different viewer types (Harmony, standalone, etc.)
    scene_->requestInlineURL (this, name.value.getString (), parentURL_.getString ());
  }

  // draw bounding box until scene fetched
  if (hasextent_ && state_ != s_completed)
  {
    ge3dWireCube (&omin_, &omax_);
  }

  QvGroup::draw ();  // draw children (if already fetched)
} // QvWWWInline


/***** misc *****/


DRAW(QvInfo)

void QvUnknownNode::draw ()
{ QvGroup::draw ();
}



/***** extensions *****/


DRAW(QvLabel)


void QvLightModel::draw ()
{
  if (scene_->dolighting () == Scene3D::lighting_auto)
    ge3dHint (hint_lighting, dolighting_);
}
