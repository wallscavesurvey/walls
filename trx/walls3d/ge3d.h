#if 0
//<copyright>
// 
// Copyright (c) 1992,93,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>
#endif

/*
 * File:     ge3d.h
 *
 * Authors:  Michael Hofer (until May 92) and Michael Pichler
 *
 * Created:   1 Apr 92
 *
 * Changed:  13 Nov 95
 *
 */


/*
 * Interface to the Graphics Engine 3D
 *
 */


#ifndef ge3d_ge3d_h
#define ge3d_ge3d_h


#ifdef __cplusplus
#define GE3D_PROTOTYPES
extern "C" {
#endif

#include "ge3drc.h"  /* revision control */

#include "face.h"
#include "vectors.h"
#include "color.h"
#include "mtl.h"


/* changed from mode_t to ge3d_mode_t (collision with CC/types.h) */

enum ge3d_mode_t
{ ge3d_wireframe,
  ge3d_hidden_line,
  ge3d_flat_shading,
  ge3d_smooth_shading,
  ge3d_texturing
};

enum ge3d_hint_t
{ hint_depthbuffer,
  hint_backfaceculling,
  hint_lighting,
  hint_texlighting
};

enum ge3d_bitplane_t
{ ge3d_normal_planes,
  ge3d_overlay_planes
};

enum ge3d_bitmapformat_t
{ ge3d_ubyte_RGB_TB,    /* RGB triples of unsigned byte, no filling byte, top-to-bottom */
  ge3d_ubyte_I_BT,      /* intensity, bottom-to-top */
  ge3d_ubyte_IA_BT,     /* intensity and Alpha, bottom-to-top */
  ge3d_ubyte_RGB_BT,    /* RGB triples, bottom-to-top */
  ge3d_ubyte_RGBA_BT    /* RGBA quadruples, bottom-to-top */
};

enum ge3d_antialiasing_t
{ ge3d_aa_lines = 0x1,
  ge3d_aa_polygons = 0x2,
  ge3d_aa_all = ~0
};

enum ge3d_camera_t  /* constants for ge3dCamera */
{
  /* if neither cam_perspective nor cam_orthographic set, projection matrix unchanged */
  cam_perspective = 0x1,
  cam_orthographic = 0x2,
  /* if neither cam_absolute nor cam_relative set, modelview matrix unchanged */
  cam_absolute = 0x4,
  cam_relative = 0x8
};

enum ge3d_cylinder_t  /* cylinder parts */
{
  cyl_sides = 0x1,
  cyl_bottom = 0x2,
  cyl_top = 0x4,
  cyl_all = 0x7
};


#ifdef GE3D_PROTOTYPES


void ge3d_openwindow ();
void ge3d_linkXwindow (void*, void*);
void ge3d_init_ ();
void ge3dHint (int /*ge3d_hint_t*/, int /*value*/);

void ge3d_clearscreen ();
void ge3d_swapbuffers ();

void ge3d_moveto (float, float, float);
void ge3dMoveTo (const point3D*);
void ge3d_lineto (float, float, float);
void ge3dLineTo (const point3D*);
void ge3d_line (float, float, float, float, float, float);
void ge3dLine (const point3D*, const point3D*);
void ge3d_wirecube (float, float, float, float, float, float);
/* void ge3dWireCube (const point3D*, const point3D*) -- macro */
void ge3dPolyLines2D (float*);

void ge3dCube (const point3D*, const point3D*);
void ge3dShadedPolygon (int, const point3D*, const colorRGB*);

/* void ge3d_polygon (...); */  /* macro */
void ge3dPolygon (const point3D* vl, int nv, const int* vi,
  const vector3D* nl, int nn, const int* ni, const vector3D* fn,
  const point2D* tv, int nt, const int* ti);
/* void ge3d_polyhedron (point3D*, vector3D*, int, face*); */  /* macro */
void ge3dPolyhedron (point3D*, vector3D*, point2D*, int, face*);
void ge3d_wirepolyhedron (point3D*, vector3D*, int, face*);
void ge3dFaceSet (const point3D* vl, int nv, const int* vi,
  const materialsGE3D* ml, int mb, int nm, const int* mi,
  const vector3D* nl, int nn, const int* ni, const vector3D* fn,
  const point2D* tl, int nt, const int* ti);
void ge3dLineSet (const point3D* vl, int nv, const int* vi);
void ge3dPointSet (const point3D*, int);

void ge3dSphere (float);
void ge3dCylinder (float br, float tr, float b, float h, int);

void ge3d_setmode (int);

void ge3d_setbackgcolor (float, float, float);
void ge3dBackgroundColor (const colorRGB*);
void ge3d_setfillcolor (float, float, float);
void ge3dMaterial (int /*ge3d_material_t*/, materialsGE3D const *);  /* front/back, definitions */
void ge3dDefaultMaterial ();
void ge3dFillColor (const colorRGB*);
void ge3d_setlinecolor (float, float, float);
void ge3dLineColor (const colorRGB*);
void ge3d_setlinestyle (short);
void ge3d_setlinewidth (short);
void ge3dAntialiasing (int);  /* ge3d_antialiasing_t */
int ge3dAntialiasingSupport ();

void ge3d_text (float, float, float, const char*);
void ge3dText (const point3D*, const char*);

void ge3d_rect (float, float, float, float);
void ge3d_rectf (float, float, float, float);
void ge3dFilledRect (float, float, float, float, float);
void ge3d_circle (float, float, float);
void ge3d_circf (float, float, float);
void ge3d_arc (float, float, float, float, float);

void ge3dSetLightSource (int index, const colorRGB*, const point3D*, float positional, int camlgt);
void ge3dLightSource (int index, const colorRGB*, /*const*/ matrix4D, float positional, int camlgt);
void ge3dSpotLight (int index, const colorRGB*, const point3D* pos, const vector3D* dir,
  float droprate, float cutangle);
void ge3d_switchlight (int, int);

void ge3d_transform_mc_wc (float, float, float, float*, float*, float*);
void ge3dTransformMcWc (const point3D*, point3D*);
void ge3d_transformvector_mc_wc (float, float, float, float*, float*, float*);
void ge3dTransformVectorMcWc (const vector3D*, vector3D*);

void ge3d_setcamera (const point3D*, const point3D*, const point3D*, float, float, float, float, float);
void ge3d_ortho_cam (const point3D*, const point3D*, const point3D*, float, float, float, float);
void ge3dCamera (int /*ge3d_camera_t*/, const point3D*, float, const vector3D*, float, float, float, float);

void ge3d_push_matrix ();
void ge3dPushIdentity ();
void ge3dLoadIdentity ();
void ge3d_push_this_matrix (const float [4][4]);
void ge3d_push_new_matrix (const float [4][4]);
void ge3dMultMatrix (const float [4][4]);

void ge3d_print_cur_matrix ();
void ge3d_get_and_pop_matrix (matrix4D);
void ge3d_pop_matrix ();

void ge3d_rotate_axis (char, float);
void ge3dRotate (const vector3D*, float);  /* (angle in radians!) */
void ge3d_translate (float, float, float);
void ge3dTranslate (const vector3D*);
void ge3d_scale (float, float, float, float);
void ge3dScale (const float*);

int ge3dTexturingSupport ();
int ge3dCreateTexture (int width, int height, const void* data, int /* ge3d_bitmapformat_t */);
void ge3dFreeTexture (int handle);
void ge3dDoTexturing (int toggle);
void ge3dApplyTexture (int handle);
void ge3dTextureRepeat (int s, int t);
void ge3dLoadTextureIdentity ();
void ge3dMultTextureMatrix (const float [4][4]);
void ge3dTexturedPolygon (int nverts, const point3D*, const point2D*, int handle);

int ge3dRequestOverlay ();
void ge3dBitplanes (int);
void ge3dClearOverlay ();
void ge3dMapColori (int, short, short, short);
void ge3dMapColorRGB (int, const colorRGB*);
void ge3dColorIndex (int);

void ge3d_close ();


#else


void ge3d_openwindow ();
void ge3d_linkXwindow ();
void ge3d_init_ ();
void ge3dHint ();

void ge3d_clearscreen ();
void ge3d_swapbuffers ();

void ge3d_moveto ();
void ge3dMoveTo ();
void ge3d_lineto ();
void ge3dLineTo ();
void ge3d_line ();
void ge3dLine ();
void ge3d_wirecube ();
void ge3dPolyLines2D ();

void ge3dCube ();
void ge3dShadedPolygon ();

/* void ge3d_polygon (); */  /* macro */
void ge3dPolygon ();
/* void ge3d_polyhedron (); */  /* macro */
void ge3dPolyhedron ();
void ge3d_wirepolyhedron ();
void ge3dFaceSet ();
void ge3dLineSet ();
void ge3dPointSet ();

void ge3dSphere ();
void ge3dCylinder ();

void ge3d_setmode ();

void ge3d_setbackgcolor ();
void ge3dBackgroundColour ();
void ge3d_setfillcolor ();
void ge3dMaterial ();
void ge3dDefaultMaterial ();
void ge3dFillColor ();
void ge3d_setlinecolor ();
void ge3dLineColor ();
void ge3d_setlinestyle ();
void ge3d_setlinewidth ();
void ge3dAntialiasing ();
int ge3dAntialiasingSupport ();

void ge3d_text ();
void ge3dText ();

void ge3d_rect ();
void ge3d_rectf ();
void ge3dFilledRect ();
void ge3d_circle ();
void ge3d_circf ();
void ge3d_arc ();

void ge3dSetLightSource ();
void ge3dLightSource ();
void ge3dSpotLight ();
void ge3d_switchlight ();

void ge3d_transform_mc_wc ();
void ge3dTransformMcWc ();
void ge3d_transformvector_mc_wc ();
void ge3dTransformVectorMcWc ();

void ge3d_setcamera ();
void ge3d_ortho_cam ();
void ge3dCamera ();

void ge3d_push_matrix ();
void ge3d_push_this_matrix ();
void ge3d_push_new_matrix ();

void ge3d_print_cur_matrix ();
void ge3d_get_and_pop_matrix ();
void ge3d_pop_matrix ();

void ge3d_rotate_axis ();
void ge3dRotate ();
void ge3d_translate ();
void ge3dTranslate ();
void ge3d_scale ();
void ge3dScale ();

int ge3dTexturingSupport ();
int ge3dCreateTexture ();
void ge3dFreeTexture ();
void ge3dDoTexturing ();
void ge3dApplyTexture ();
void ge3dTextureRepeat ();
void ge3dLoadTextureIdentity ();
void ge3dMultTextureMatrix ();
void ge3dTexturedPolygon ();

int ge3dRequestOverlay ();
void ge3dBitplanes ();
void ge3dClearOverlay ();
void ge3dMapColori ();
void ge3dMapColorRGB ();
void ge3dColorIndex ();

void ge3d_close ();


#endif



#define ge3dWireCube(p, q)  ge3d_wirecube ((p)->x, (p)->y, (p)->z, (q)->x, (q)->y, (q)->z)
#define ge3dFillColor(c)  ge3d_setfillcolor ((c)->R, (c)->G, (c)->B)
#define ge3d_polygon(vl, nv, vi, nl, nn, ni, fn)  ge3dPolygon ((vl), (nv), (vi), (nl), (nn), (ni), (fn), 0, 0, 0)
#define ge3d_polyhedron(p, v, n, f)  ge3dPolyhedron ((p), (v), 0, (n), (f))

#ifdef __cplusplus
}
#endif

#endif
