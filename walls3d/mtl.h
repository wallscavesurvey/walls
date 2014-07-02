#if 0
//<copyright>
// 
// Copyright (c) 1995
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>
#endif

/*
 * File:     mtl.h
 *
 * Author:   Michael Pichler
 *
 * Created:  29 Jun 95
 *
 * Changed:  13 Nov 95
 *
 */



#ifndef ge3d_mtl_h
#define ge3d_mtl_h


#include "color.h"


/* definition of multiple materials (for facesets) */

typedef struct
{
  int
    num_base,
    num_ambient,
    num_diffuse,
    num_specular,
    num_emissive,
    num_shininess,
    num_transparency;
  const colorRGB
    *rgb_base;
  colorRGB
    *rgb_ambient,
    *rgb_diffuse,
    *rgb_specular,
    *rgb_emissive;
  float
    *val_shininess,
    *val_transparency;
} materialsGE3D;


#define initmtl3D(m)  \
  m->num_base = m->num_ambient = m->num_diffuse = m->num_specular = m->num_emissive = 0,  \
  m->num_shininess = m->num_transparency = 0,  \
  m->rgb_base = m->rgb_ambient = m->rgb_diffuse = m->rgb_specular = m->rgb_emissive = NULL,  \
  m->val_shininess = m->val_transparency = NULL


/* material bindings */

enum ge3d_matbinding_t
{
  matb_default,
  matb_overall,
  matb_perpart,
  matb_perpartindexed,
  matb_perface,
  matb_perfaceindexed,
  matb_pervertex,
  matb_pervertexindexed
};


/* material scope definition */

enum ge3d_material_t  /* constants for ge3dMaterial */
{
  mat_front,
  mat_back,
  mat_front_and_back
};


#endif
