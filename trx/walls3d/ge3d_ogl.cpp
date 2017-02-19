#if 0
//<copyright>
// 
// Copyright (c) 1994,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>
#endif

#pragma warning(disable:4244)

/*
 * File:     ge3d_ogl.c
 *
 * Authors:  Michael Pichler
 *
 * Created:   5 Sep 94  (taken from GL implementation)
 *
 * Changed:  22 Feb 96  Michael Pichler
 *
 * $Id: ge3d_ogl.C,v 1.13 1996/02/22 17:40:02 mpichler Exp mpichler $
 *
 */


/*
 * Implementation of Graphics Engine 3D
 *
 * for Open GL graphics library - and workalikes
 *
 */

#include "ge3drc.h"
const char* what_ge3d_1_1_3 =
  "@(#)[HG-LIB] ge3d\t1.1.3 [3D Graphics Engine for OpenGL (and workalikes)] [Michael Pichler]";

/*
 * GE3D_PROTOTYPES may determine whether you get a stable version of this library or not.
 *
 * When compiling ge3d with C++ it is set automatically and you don't have to worry about.
 *
 * When compiling with cc in K&R mode it must not be set;
 * note that the header files of the Mesa library require Ansi-C.
 *
 * When compiling with cc in Ansi-C mode or gcc you should set it for prototyping,
 * *unless* you link the library against a C++ frontend like cfront that calls
 * a K&R C-compiler for real compilation.
 */


#if !defined(PMAX) && !defined(HPUX9)
#define GE3D_PROTOTYPES
#endif


#include "ge3d.h"



/* switch between ANSI C function prototypes and old C function declarations */


#ifdef GE3D_PROTOTYPES

#  define PARMS1(t1,p1)                                 (t1 p1)
#  define PARMS2(t1,p1,t2,p2)                           (t1 p1, t2 p2)
#  define PARMS3(t1,p1,t2,p2,t3,p3)                     (t1 p1, t2 p2, t3 p3)
#  define PARMS4(t1,p1,t2,p2,t3,p3,t4,p4)               (t1 p1, t2 p2, t3 p3, t4 p4)
#  define PARMS5(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5)         (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5)
#  define PARMS6(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6) \
   (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6)
#  define PARMS7(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7) \
   (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7)
#  define PARMS8(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7,t8,p8) \
   (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8)
#  define PARMS9(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7,t8,p8,t9,p9) \
   (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9)
#  define PARMS10(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7,t8,p8,t9,p9,t10,p10) \
   (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10)

#else

#  define PARMS1(t1,p1)                                 (p1) t1 p1;
#  define PARMS2(t1,p1,t2,p2)                           (p1, p2) t1 p1; t2 p2;
#  define PARMS3(t1,p1,t2,p2,t3,p3)                     (p1, p2, p3) t1 p1; t2 p2; t3 p3;
#  define PARMS4(t1,p1,t2,p2,t3,p3,t4,p4)               (p1, p2, p3, p4) t1 p1; t2 p2; t3 p3; t4 p4;
#  define PARMS5(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5)         (p1, p2, p3, p4, p5) t1 p1; t2 p2; t3 p3; t4 p4; t5 p5;
#  define PARMS6(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6) \
   (p1, p2, p3, p4, p5, p6) t1 p1; t2 p2; t3 p3; t4 p4; t5 p5; t6 p6;
#  define PARMS7(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7) \
   (p1, p2, p3, p4, p5, p6, p7) t1 p1; t2 p2; t3 p3; t4 p4; t5 p5; t6 p6; t7 p7;
#  define PARMS8(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7,t8,p8) \
   (p1, p2, p3, p4, p5, p6, p7, p8) t1 p1; t2 p2; t3 p3; t4 p4; t5 p5; t6 p6; t7 p7; t8 p8;
#  define PARMS9(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7,t8,p8,t9,p9) \
   (p1, p2, p3, p4, p5, p6, p7, p8, p9) t1 p1; t2 p2; t3 p3; t4 p4; t5 p5; t6 p6; t7 p7; t8 p8; t9 p9;
#  define PARMS10(t1,p1,t2,p2,t3,p3,t4,p4,t5,p5,t6,p6,t7,p7,t8,p8,t9,p9,t10,p10) \
   (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) t1 p1; t2 p2; t3 p3; t4 p4; t5 p5; t6 p6; t7 p7; t8 p8; t9 p9; t10 p10;

#endif



#ifdef __PC__
#include "stdafx.h"
#define M_PI 3.141592653589793
#endif

#include <GL/glu.h>
#include <GL/glext.h>

#include <stdio.h>
#include <math.h>
#ifndef M_PI
//#include "../values.h"
#endif
//#include <unistd.h>
#include <malloc.h>

/* internal data of ge3d library: */

static int ge3d_mode = ge3d_wireframe;   /* default drawing mode: wireframe */
/* static long ge3d_overlay_drawmode = NORMALDRAW;  / * no overlays by default */
static int ge3d_do_texturing = 0;
static int ge3d_depthbuffering = 1;  /* use Z-Buffer for hidden-line/surface-elimination */
static int ge3d_backfaceculling = 1;  /* do backface culling */
static int ge3d_lighting = 1;  /* lighting in flat/smooth shading mode */
static int ge3d_texlighting = 0;  /* combine lighting and texturing */

static float ge3d_tm [16];  /* scratch matrix data, ROW major order (see glMultMatrix) */

/* static float rgb_black [3] = { 0.0, 0.0, 0.0 }; */
/* static float rgb_gray [3] = { 0.3, 0.3, 0.3 }; */
static GLfloat fill_color [3] = { 1.0f, 1.0f, 1.0f };
static float line_color [3] = { 1.0f, 1.0f, 1.0f };
float backg_color [3] = { 0.0f, 0.0f, 0.0f };
static int samelfcolor = 1;  /* same line and fill color */
static point3D cur_position = { 0.0f, 0.0f, 0.0f };  /* for moveto/lineto */

static GLUquadricObj* gluquadobj = 0;

/* identity matrix need not be stored - glLoadIdentity loads one on the stack */

/* #define HL_OFFSET 0.0025 */
/* offset for pseudo hidden line; 0.002 to 0.003 seems fine */

#define HL_FACTOR 1.007
/* translation factor for pseudo hidden line; somewhere between 1.001 and 1.01 */


#define assigncolor( c, r, g, b ) \
(c) [0] = (r),  (c) [1] = (g),  (c) [2] = b


/* modify z translation for pseudo hidden line */
/* macro needs a float variable oldztran to save the old value */
/* TODO: consider using EXT_polygon_offset when available */
/* TODO: convert to a function! */

#define BEGINPOLYGONOFFSET(oldztran)  \
  glMatrixMode (GL_PROJECTION);  \
  glGetFloatv (GL_PROJECTION_MATRIX, ge3d_tm);  \
  oldztran = ge3d_tm [14];  \
  ge3d_tm [14] *= (float)HL_FACTOR;  \
  glLoadMatrixf (ge3d_tm);  \
  glMatrixMode (GL_MODELVIEW);

#define ENDPOLYGONOFFSET(oldztran)  \
  glMatrixMode (GL_PROJECTION);  \
  ge3d_tm [14] = oldztran;  \
  glLoadMatrixf (ge3d_tm);  \
  glMatrixMode (GL_MODELVIEW);



void ge3d_openwindow ()
{
  /*** attention: this is not part of Open GL functionality!!! ***/
  ge3d_init_ ();

} /* ge3d_openwindow */



void ge3d_init_ ()
{
  glMatrixMode (GL_MODELVIEW);  /* multi matrix mode */

  glEnable (GL_NORMALIZE);  /* otherwise on scaling wrong normals (for lighting) */

  /*glEnable (GL_DITHER);*/  /* does not prevent bad flat shading on 8bit display */

  glDepthFunc (GL_LEQUAL);  /* use "<=" instead of "<" as z-buffer function */

  glColor3fv (fill_color);  /* default color */

  ge3d_mode = ge3d_wireframe;
  /* default: no backface removal, no depth buffer, no lighting */

  glPointSize (2.0f);  /* to see anything */

/*glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, 1);*/  /* no significat difference for specular materials */

  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);  /* texture images are byte (and not word) aligned */
  glEnable(GL_MULTISAMPLE_ARB); //GL_MULTISAMPLE
  glEnable(GL_BLEND); 
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  /* "normal" blending for line anti-aliasing */
  //glEnable( GL_LINE_SMOOTH );
  //glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);  /* "best" anti-aliasing */

  /* glBlendFunc (GL_SRC_ALPHA_SATURATE, GL_ONE);  / * for polygon anti-aliasing; does NOT work with Z-buffer! */
/*  glHint (GL_POLYGON_SMOOTH_HINT, GL_NICEST);  / * "best" anti-aliasing */

/* // textures now bottom-to-top by default
  glMatrixMode (GL_TEXTURE);
  glLoadIdentity ();
  glScalef (1.0, -1.0, 1.0);
  glMatrixMode (GL_MODELVIEW);
*/
  ge3dHint (hint_texlighting, 1);  /* texturing and lighting */

  /* use linear texture filtering (weighted average of 4 texels), do mipmapping */
 //BUG? was glTexParameterf(DMcK)--
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  if (!gluquadobj)
    gluquadobj = gluNewQuadric ();

  /* use default lighting model (could change ambient light) */
} /* ge3d_init_ */



/* ge3dHint */
/* flags that allow modification of ge3d behaviour */

void ge3dHint PARMS2 (
  int, flag,
  int, value
)
{
  switch (flag)
  {
    case hint_depthbuffer:
      /* must be set before activating a drawing mode to be effective */
      ge3d_depthbuffering = value;
    break;

    case hint_backfaceculling:
      /* may be changed any time */
      if (ge3d_backfaceculling != value)
      {
        ge3d_backfaceculling = value;
        if (value && ge3d_mode != ge3d_wireframe)
        { glEnable (GL_CULL_FACE);
          glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
        }
        else
        { glDisable (GL_CULL_FACE);
          glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        }
      }
    break;

    case hint_lighting:
      /* may be changed at any time */
      if (ge3d_lighting != value)
      {
        ge3d_lighting = value;
        if (value && ge3d_mode >= ge3d_flat_shading)
          glEnable (GL_LIGHTING);
        else
          glDisable (GL_LIGHTING);
      }
    break;

    case hint_texlighting:
      if (ge3d_texlighting != value)
      { /* note: texturing+lighting not yet supported for "old style" ge3dPolyhedron (TODO) */
        ge3d_texlighting = value;
		//BUG? was glTexEnvf(DMcK) --
        glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, value ? GL_MODULATE : GL_DECAL);
        /* warning: behaviour of GL_DECAL undefined for 1 and 2 component textures */
      }
    break;
  }
} /* ge3dHint */



void ge3d_clearscreen ()
{
  glClearColor (backg_color [0], backg_color [1], backg_color [2], 0.0f);

  if ((ge3d_mode != ge3d_wireframe) && ge3d_depthbuffering)
    glClear ((GLbitfield) (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  else
    glClear (GL_COLOR_BUFFER_BIT);
}



void ge3d_swapbuffers ()
{
  /* swapping of buffers is a part of GLX and not of pure OpenGL */
}



/* ge3d_apply_material: internal function for applying the i-th one
   out of materials (compare with public ge3dMaterial) */
/* materials must not be NULL; fill_color and line_color are not changed */
/* a change occurs only when there is more than one component (it is
   assumed that the first one was activated in advance */
/* if there are too few items, they are cycled through individually */
/* is scope relevant??? (assumed front and back) */

static void ge3d_apply_material PARMS2(
  const materialsGE3D*, materials,
  int, i
)
{
  /* might try to detect whether the new material is already active for speedup */

  static float color [4];
  const colorRGB* c;

/*fprintf (stderr, "applying material %d  ", i);*/

  float A = 1.0f;  /* Alpha: 1.0 opaque, 0.0 fully transparent */
  int n = materials->num_transparency;
  if (n && ge3d_lighting)
    A = (float)(1.0 - materials->val_transparency [i % n]);
  color [3] = A;

  /* may wish to cycle base instead of diffuse */
  n = materials->num_diffuse;
  if (n > 1)
  { c = materials->rgb_diffuse + (i % n);
    if (ge3d_lighting)
    { assigncolor (color, c->R, c->G, c->B);
      glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    }
    else  /* use diffuse color only */
    {
/* // removed for efficiency, mpichler, 19950901
      if (A < 1.0)  // TODO: implement Alpha
      { assigncolor (color, c->R, c->G, c->B);
        glColor4fv (color);
      }
      else
*/
        glColor3fv ((float*) c);  /* implicit alpha of 1.0 */
      return;
    }
  }
  if (!ge3d_lighting)
    return;

  n = materials->num_ambient;
  if (n > 1)
  { c = materials->rgb_ambient + (i % n);
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, color);
  }
  n = materials->num_specular;
  if (n > 1)
  { c = materials->rgb_specular + (i % n);
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, color);
  }
  n = materials->num_emissive;
  if (n > 1)
  { c = materials->rgb_emissive + (i % n);
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, color);
  }
  n = materials->num_shininess;
  if (n > 1)
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, (float)(materials->val_shininess [i % n] * 128.0));

} /* ge3d_apply_material */



/* ge3d_wire_material: as ge3d_apply_material, but for wireframe drawings */

static void ge3d_wire_material PARMS2(
  const materialsGE3D*, materials,
  int, i
)
{
  /* use base color; no alpha wireframes */

  int n = materials->num_base;
  if (n > 1)
    glColor3fv ((float*) (materials->rgb_base + (i % n)));

} /* ge3d_wire_material */



void ge3d_moveto PARMS3(
float, x,
float, y,
float, z
)
{
  /* no pendant for moveto/lineto in Open GL - simulated */
  init3D (cur_position, x, y, z);
  /* should use ge3d_line/ge3dLine */
}


void ge3dMoveTo PARMS1(
const point3D*, p
)
{
  cur_position = *p;
}



void ge3d_lineto PARMS3(
float, x,
float, y,
float, z
)
{
  glBegin (GL_LINES);
  glVertex3fv ((float*) &cur_position);
  init3D (cur_position, x, y, z);
  glVertex3fv ((float*) &cur_position);
  glEnd ();
}


void ge3dLineTo PARMS1(
const point3D*, p
)
{
  glBegin (GL_LINES);
  glVertex3fv ((float*) &cur_position);
  cur_position = *p;
  glVertex3fv ((float*) &cur_position);
  glEnd ();
}


void ge3d_line PARMS6(  /* draw a line */
  float, x0,
  float, y0,
  float, z0,
  float, x1,
  float, y1,
  float, z1
)
{
  /* in GL lines drawn with move/draw were not subjected to lighting */
  /* in OpenGL all lines appear black while lighting is on (BUG?) */
  /* to achieve old behaviour turn off lighting while drawing lines */
  /* possibly inefficient - should think for a better solution */
  /* on the other hand I never considered it useful/necessary to shade lines */

  /* TODO: should use a variable ge3d_do_lighting instead (compare with ge3d_do_texturing) */
  register int didlighting = (ge3d_mode >= ge3d_flat_shading && ge3d_lighting);
  if (didlighting)
    glDisable (GL_LIGHTING);

  glBegin (GL_LINES);
  glVertex3f (x0, y0, z0);
  glVertex3f (x1, y1, z1);
  glEnd ();

  if (didlighting)
    glEnable (GL_LIGHTING);
}


void ge3dLine PARMS2(  /* draw a line */
  const point3D*, p,
  const point3D*, q
)
{
  register int didlighting = (ge3d_mode >= ge3d_flat_shading && ge3d_lighting);
  if (didlighting)  /* see ge3d_line */
    glDisable (GL_LIGHTING);

  glBegin (GL_LINES);
  glVertex3fv ((float*) p);
  glVertex3fv ((float*) q);
  glEnd ();

  if (didlighting)
    glEnable (GL_LIGHTING);
}


/* wirecube: wireframe of an aligned cube */
/* x0, x1 etc. need not be ordered */

void ge3d_wirecube PARMS6(
float, x0,
float, y0,
float, z0,
float, x1,
float, y1,
float, z1
)
{
  register int didlighting = (ge3d_mode >= ge3d_flat_shading && ge3d_lighting);

  if (didlighting)  /* ge3d_wirecube/ge3dWireCube should not be affected from lighting */
    glDisable (GL_LIGHTING);

  glColor3fv (line_color);
  glBegin (GL_LINE_LOOP);  /* bottom */
    glVertex3f (x0, y0, z0);
    glVertex3f (x0, y1, z0);
    glVertex3f (x1, y1, z0);
    glVertex3f (x1, y0, z0);
  glEnd ();
  glColor3fv (line_color);
  glBegin (GL_LINE_LOOP);  /* top */
    glVertex3f (x0, y0, z1);
    glVertex3f (x0, y1, z1);
    glVertex3f (x1, y1, z1);
    glVertex3f (x1, y0, z1);
  glEnd ();
  glColor3fv (line_color);
  glBegin (GL_LINES);  /* remaining edges */
    glVertex3f (x0, y0, z0);  glVertex3f (x0, y0, z1);
    glVertex3f (x0, y1, z0);  glVertex3f (x0, y1, z1);
    glVertex3f (x1, y0, z0);  glVertex3f (x1, y0, z1);
    glVertex3f (x1, y1, z0);  glVertex3f (x1, y1, z1);
  glEnd ();

  if (didlighting)
    glEnable (GL_LIGHTING);

} /* ge3d_wirecube */



/* ge3dPolyLines2D (float* p)
   draws a sequence of (open) polylines,
   i.e. arbitrary many polylines in sequence,
   terminated with (float) 0 (!);
   each polyline begins with the number of points, followed
   by that many pairs of (x, y) coordinates;
   since the polyline is not automatically closed,
   only lines with two or more points make sense;
   all line attributes apply -
   line color should be set immediately before call
*/


void ge3dPolyLines2D (float* p)
{
  register unsigned int n;
  register int didlighting = (ge3d_mode >= ge3d_flat_shading && ge3d_lighting);
  if (didlighting)  /* sorry for need of this; see ge3d_line */
    glDisable (GL_LIGHTING);

  while ((n = (unsigned int) *p++) != 0)
  {
    glBegin (GL_LINE_STRIP);
    while (n--)  /* n points */
    { glVertex2fv (p);
      p += 2;
    }
    glEnd ();  /* line is not closed */
  }

  if (didlighting)
    glEnable (GL_LIGHTING);
}



/* cube: solid cube, arguments: two opposite vertices */

void ge3dCube PARMS2(
  const point3D*, p,  /* two opposite vertices, orderd (p.x <= q.x) etc. */
  const point3D*, q   /* otherwise wrong normal vectors */
)
{
  point3D vertex [8];  /* 8 vertices */
  point3D* vptr;
  int vindex [4];  /* 4 vertices per face */
  vector3D normal;
  register unsigned int i;
  static const point2D texverts [4] =
  { { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f }
  };
  static const int tindex [4] = { 0, 1, 2, 3 };


  if (ge3d_mode == ge3d_wireframe)
  {
    ge3dWireCube (p, q);
    return;
  }

  /* compute vertices */
  for (i = 0, vptr = vertex;  i < 8;  i++, vptr++)
    init3D (*vptr, (i & 4) ? q->x : p->x, (i & 2) ? q->y : p->y, (i & 1) ? q->z : p->z);

  /* somewhat inefficient, because each polygon is drawn per se */
  /* TODO: optimization */
#define cubeface(nx, ny, nz, v0, v1, v2, v3)  \
  init3D (normal, nx, ny, nz);  \
  vindex [0] = v0,  vindex [1] = v1,  vindex [2] = v2, vindex [3] = v3;  \
  ge3dPolygon (vertex, 4, vindex, 0, 0, 0, &normal, texverts, 4, tindex)

  /* 6 faces (provide face but no vertex normals) */
  /* for each face normal vector and vertex indices */
  cubeface ( 1,  0,  0,  5, 4, 6, 7);  /* R */
  cubeface (-1,  0,  0,  0, 1, 3, 2);  /* L */
  cubeface ( 0,  1,  0,  3, 7, 6, 2);  /* T */
  cubeface ( 0, -1,  0,  0, 4, 5, 1);  /* B */
  cubeface ( 0,  0,  1,  1, 5, 7, 3);  /* front */
  cubeface ( 0,  0, -1,  4, 0, 2, 6);  /* back */
#undef cubeface
} /* ge3dCube */



/* ShadedPolygon:
   polygon with color for each vertex,
   the polygon is filled (regardless of current mode),
   fillcolor and linecolor have no influence;
   switches to gouraud shading
*/

void ge3dShadedPolygon PARMS3(
  int, nverts,
  const point3D*, vertexlist,
  const colorRGB*, colorlist
)
{
  glShadeModel (GL_SMOOTH);  /* gouraud shading */
  glBegin (GL_POLYGON);
  while (nverts--)
  { glColor3fv  ((float*) colorlist++);
    glVertex3fv ((float*) vertexlist++);
  }
  glEnd ();
}



/* polygon */

void ge3dPolygon PARMS10(
  const point3D*, vertexlist,
  int, nverts,
  const int*, vertexindexlist,
  const vector3D*, normallist,
  int, nnormals,
  const int*, normalindexlist,
  const vector3D*, f_normal,
  const point2D*, texvertlist,
  int, ntexverts,
  const int*, texvindexlist
)
{
  register int i;
  register const int *v_ptr, *n_ptr;
  int didlighting = 0;

  if (ge3d_mode == ge3d_wireframe)
  {
    glColor3fv (line_color);
    glBegin (GL_LINE_LOOP);
    v_ptr = vertexindexlist;
    i = nverts;
    while (i--)
      glVertex3fv ((float*) (vertexlist + *v_ptr++));
    glEnd ();
    return;
  }
  else if (ge3d_mode == ge3d_hidden_line)
  {
    /* OpenGL includes glPolygonMode, but there is no mode to draw
       interior and lines with a different colour, sorry. */
    glColor3fv (backg_color);
    glBegin (GL_POLYGON);
    v_ptr = vertexindexlist;
    i = nverts;
    while (i--)
      glVertex3fv ((float*) (vertexlist + *v_ptr++));
    glEnd ();
  }
  else /* if (ge3d_mode == ge3d_flat_shading, ge3d_smooth_shading, or ge3d_texturing) */
  {
    didlighting = ge3d_lighting;
    if (nverts == 3)
      glBegin (GL_TRIANGLES);
    else
      glBegin (GL_POLYGON);
    glColor3fv (fill_color);

    /* ignore texture vertices if not texturing */
    if (ge3d_mode != ge3d_texturing || !ge3d_do_texturing)
      texvertlist = 0;

    v_ptr = vertexindexlist;
    n_ptr = normalindexlist;
    if (texvertlist && ntexverts == nverts)
    {
      register const int* t_ptr = texvindexlist;

      if (f_normal)
        glNormal3fv ((float*) f_normal);
      i = nverts;
      while (i--)
      { glTexCoord2fv ((float*) (texvertlist + *t_ptr++));
        glVertex3fv ((float*) (vertexlist + *v_ptr++));
      }
    }
    else if (ge3d_mode == ge3d_flat_shading || nnormals < nverts)
    {
      if (f_normal)
        glNormal3fv ((float*) f_normal);
      i = nverts;
      while (i--)
        glVertex3fv ((float*) (vertexlist + *v_ptr++));
    }
    else  /* send vertices and normals */
    {
      i = nverts;
      while (i--)
      { glNormal3fv ((float*) (normallist + *n_ptr++));
        glVertex3fv ((float*) (vertexlist + *v_ptr++));
      }
    }
    glEnd ();
  }

  if (ge3d_mode == ge3d_hidden_line || !samelfcolor)  /* draw edges in linecolor */
  {
    float oldztran;

    BEGINPOLYGONOFFSET (oldztran)

    /* line colours are not subject to lighting (as in IRIX-GL implementation of ge3d) */
    if (didlighting)
      glDisable (GL_LIGHTING);

    glColor3fv (line_color);
    /*fprintf (stderr, "cur. line color: (%f, %f, %f)\n", line_color [0], line_color [1], line_color [2]);*/
    glBegin (GL_LINE_LOOP);
    v_ptr = vertexindexlist;
    i = nverts;
    while (i--)
      glVertex3fv ((float*) (vertexlist + *v_ptr++));
    glEnd ();

    ENDPOLYGONOFFSET (oldztran)

    if (didlighting)
      glEnable (GL_LIGHTING);
  }

} /* ge3dPolygon */



void ge3dPolyhedron PARMS5(
  point3D*, vertexlist,
  vector3D*, normallist, 
  point2D*, texvertlist,
  int, numfaces,
  face*, facelist
)
{
  register unsigned i;
  register face* faceptr;
  int didlighting = 0;

/*fprintf (stderr, "ge3dPolyhedron for %d faces; same-line-and-fill-color: %d\n", numfaces, samelfcolor);*/

  /* this loop would work always, but for efficiency drawing for each mode is done below */
/*
  for (i = 0, faceptr = facelist;  i < numfaces;  i++, faceptr++)
  {
    ge3dPolygon (
      vertexlist, faceptr->num_faceverts, faceptr->facevert,
      normallist, faceptr->num_facenormals, faceptr->facenormal, &faceptr->normal,
      texvertlist, faceptr->num_facetexverts, faceptr->facetexvert);
    );
  }
  return;
*/

  if (ge3d_mode == ge3d_wireframe)
  {
    ge3d_wirepolyhedron (vertexlist, normallist, numfaces, facelist);
    return;
  }
  else if (ge3d_mode == ge3d_hidden_line)
  {
    register unsigned j;  /* nverts */
    register int* v_ptr;

    glColor3fv (backg_color);
    for (i = numfaces, faceptr = facelist;  i--;  faceptr++)
    {
      j = faceptr->num_faceverts;
      v_ptr = faceptr->facevert;
      glBegin (GL_POLYGON);
      while (j--)
        glVertex3fv ((float*) (vertexlist + *v_ptr++));
      glEnd ();
    }
  }
  else /* if (ge3d_mode == ge3d_flat_shading || ge3d_mode == ge3d_smooth_shading) */
  {
    register unsigned j;
    register int nnormals, ntexverts, * v_ptr, * n_ptr, * t_ptr;

    didlighting = ge3d_lighting;
    glColor3fv (fill_color);

    /* ignore texture vertices if not texturing */
    if (ge3d_mode != ge3d_texturing || !ge3d_do_texturing)
      texvertlist = 0;

    for (i = numfaces, faceptr = facelist;  i--;  faceptr++)
    {
      j = faceptr->num_faceverts;  /* nverts */
      v_ptr = faceptr->facevert;
      nnormals = faceptr->num_facenormals;
      n_ptr = faceptr->facenormal;
      ntexverts = texvertlist ? faceptr->num_facetexverts : 0;
      t_ptr = faceptr->facetexvert;

      if (j == 3)  /* some OpenGL implementations are optimized on triangles */
        glBegin (GL_TRIANGLES);
      else
        glBegin (GL_POLYGON);

      if ((unsigned)ntexverts == j)  /* texturing: Texture + Vertex data */
      { /* will also have to send normals when combining texture mapping with lighting */
        /* assert: only herein if a texture is actually active */
        glNormal3fv ((float*) &faceptr->normal);  /* if assertion fails: flat shading */
        while (j--)
        { glTexCoord2fv ((float*) (texvertlist + *t_ptr++));
          /* no glNormal3fv */
          glVertex3fv ((float*) (vertexlist + *v_ptr++));
        }
      }
      else if (ge3d_mode == ge3d_flat_shading || (unsigned)nnormals < j)
      {                    /* flat shading: Vertex data only */
        glNormal3fv ((float*) &faceptr->normal);
        while (j--)
          glVertex3fv ((float*) (vertexlist + *v_ptr++));
      }
      else                 /* smooth shading: Normal + Vertex data */
      { while (j--)
        { glNormal3fv ((float*) (normallist + *n_ptr++));
          glVertex3fv ((float*) (vertexlist + *v_ptr++));
        }
      }

      glEnd ();

    }
  } /* flat and smooth shading */


  if (ge3d_mode == ge3d_hidden_line || !samelfcolor)  /* draw edges in linecolor */
  {
    float oldztran;

    BEGINPOLYGONOFFSET (oldztran)

    if (didlighting)
      glDisable (GL_LIGHTING);

    /* (no backface culling here) */
    ge3d_wirepolyhedron (vertexlist, normallist, numfaces, facelist);

    ENDPOLYGONOFFSET (oldztran)

    if (didlighting)
      glEnable (GL_LIGHTING);
  }

} /* ge3d_polyhedron */



/* ge3d_wirepolyhedron */
/* always draws a polyhedron in wireframe (independent of current mode) */

void ge3d_wirepolyhedron PARMS4(
  point3D*, vertexlist,
  vector3D*, normallist, 
  int, numfaces,
  face*, faceptr
)
{
  register int nverts, * v_ptr;

  glColor3fv (line_color);
  while (numfaces--)
  {
    nverts = faceptr->num_faceverts;
    v_ptr = faceptr->facevert;  /* vertexindexlist */
    glBegin (GL_LINE_LOOP);
    while (nverts--)
      glVertex3fv ((float*) (vertexlist + *v_ptr++));
    glEnd ();
    faceptr++;
  }

} /* ge3d_wirepolyhedron */



/* ge3dFaceSet -- under construction */
/* argument list becomes quite long; will probably introduce a type for it (like face) */

void ge3dFaceSet
#ifdef GE3D_PROTOTYPES
(
  const point3D* vertexlist,           /* array of vertices */
  int numcoordind,                     /* number of vertex indices */
  const int* coordindex,               /* indices into vertexlist, -1 for closing face */
  const materialsGE3D* materials,      /* material definitions */
  int matbinding,                      /* material binding (ge3d_matbinding_t) */
  int nummatind,                       /* number of material indices */
  const int* materindex,               /* indices into arrays of materlist */
  const vector3D* normallist,          /* array of vertex normal vectors (0 allowed) */
/*int normalbinding,*/
  int numnormalind,                    /* number of vertex normal indices (0 allowed) */
  const int* normalindex,              /* indices into normallist (corresponding to coordindex) */
  const vector3D* facenormals,         /* face normal array (one per face; necessary for shading) */
  const point2D* texvertlist,          /* array of texture vertices */
  int numtexind,                       /* number of texture vertex indices (0 allowed) */
  const int* texvertindex              /* indices into texvertlist */
)
#else
( vertexlist, numcoordind, coordindex,
  materials, matbinding, nummatind, materindex,
  normallist, numnormalind, normalindex, facenormals,
  texvertlist, numtexind, texvertindex
)
  const point3D* vertexlist;  int numcoordind;  const int* coordindex;
  const materialsGE3D* materials;  int matbinding, nummatind;  const int* materindex;
  const vector3D* normallist;  int numnormalind;  const int* normalindex;  const vector3D* facenormals;
  const point2D* texvertlist;  int numtexind;  const int* texvertindex
#endif
{
  register int cindex;

  if (!numcoordind)
    return;

  if (coordindex [numcoordind - 1] < 0)
    numcoordind--;  /* avoid empty glBegin/glEnd for last primitive (automatically closed) */

  /*fprintf (stderr, "ge3dFaceSet with %d face indices.\n", numcoordind);*/
  if (ge3d_mode == ge3d_wireframe)
  {
    /* for speed wireframes are not drawn with per vertex materials */
    glBegin (GL_LINE_LOOP);  /* closed polylines */

    if ((matbinding == matb_perpart || matbinding == matb_perface) && materials)
    {
      register int curface = 0;
      ge3d_wire_material (materials, curface++);
      while (numcoordind--)
      {
        cindex = *coordindex++;
        if (cindex < 0)
        { glEnd ();
          ge3d_wire_material (materials, curface++);
          glBegin (GL_LINE_LOOP);
        }
        else
          glVertex3fv ((float*) (vertexlist + cindex));
      }
    }
    else if ((matbinding == matb_perpartindexed || matbinding == matb_perfaceindexed)
         && materials && nummatind && materindex)
    {
      register int curface = 0;
      ge3d_wire_material (materials, materindex [curface++ % nummatind]);

      while (numcoordind--)
      {
        cindex = *coordindex++;
        if (cindex < 0)
        { glEnd ();
          ge3d_wire_material (materials, materindex [curface++ % nummatind]);
          glBegin (GL_LINE_LOOP);
        }
        else
          glVertex3fv ((float*) (vertexlist + cindex));
      }
    }
    else  /* wireframe with overall material binding */
    {
      while (numcoordind--)
      {
        cindex = *coordindex++;
        /*fprintf (stderr, "[%d] ", cindex);*/
        if (cindex < 0)
        { glEnd ();
          glBegin (GL_LINE_LOOP);
        }
        else
          glVertex3fv ((float*) (vertexlist + cindex));
      }
    }

    glEnd ();  /* end of last polyline */
    /*fprintf (stderr, "\n");*/
  }
  else if (ge3d_mode == ge3d_hidden_line)
  {
    register int ncind = numcoordind;
    register const int* crdind = coordindex;
    float oldztran;

    glColor3fv (backg_color);
    glBegin (GL_POLYGON);
    while (ncind--)
    {
      cindex = *crdind++;
      if (cindex < 0)
      { glEnd ();
        glBegin (GL_POLYGON);
      }
      else
        glVertex3fv ((float*) (vertexlist + cindex));
    }
    glEnd ();  /* end of last polygon */

    /* currently always same line and fill color for ge3dFaceSet (from VRML) */
    /* (if not do this code also on !samelfcolor at the end; take care of didlighting) */

    BEGINPOLYGONOFFSET (oldztran)

    glColor3fv (line_color);
    glBegin (GL_LINE_LOOP);

    if ((matbinding == matb_perpart || matbinding == matb_perface) && materials)
    {
      register int curface = 0;
      ge3d_wire_material (materials, curface++);

      while (numcoordind--)
      {
        cindex = *coordindex++;
        if (cindex < 0)
        { glEnd ();
          ge3d_wire_material (materials, curface++);
          glBegin (GL_LINE_LOOP);
        }
        else
          glVertex3fv ((float*) (vertexlist + cindex));
      }
    }
    else if ((matbinding == matb_perpartindexed || matbinding == matb_perfaceindexed)
         && materials && nummatind && materindex)
    {
      register int curface = 0;
      ge3d_wire_material (materials, materindex [curface++ % nummatind]);

      while (numcoordind--)
      {
        cindex = *coordindex++;
        if (cindex < 0)
        { glEnd ();
          ge3d_wire_material (materials, materindex [curface++ % nummatind]);
          glBegin (GL_LINE_LOOP);
        }
        else
          glVertex3fv ((float*) (vertexlist + cindex));
      }
    }
    else
    {
      while (numcoordind--)
      {
        cindex = *coordindex++;
        if (cindex < 0)
        { glEnd ();
          glBegin (GL_LINE_LOOP);
        }
        else
          glVertex3fv ((float*) (vertexlist + cindex));
      }
    }

    glEnd ();  /* end of last polyline */

    ENDPOLYGONOFFSET (oldztran)
  }
  else  /* ge3d_mode one of ge3d_flat_shading, ge3d_smooth_shading, ge3d_texturing */
  {
    glColor3fv (fill_color);
    glBegin (GL_POLYGON);

    if (ge3d_mode == ge3d_texturing && ge3d_do_texturing && numtexind && texvertlist)  /* texturing */
    {
      register unsigned curvert = 0;
      /* TODO: other materialbindings than overall (only relevant when lighting is on) */

      if (ge3d_texlighting && numnormalind >= numcoordind)  /* smooth shaded texture */
      { /* assume overall material binding */
        glNormal3fv ((float*) facenormals++);
        while (numcoordind--)
        {
          cindex = *coordindex++;
          if (cindex < 0)
          { normalindex++;  /* skip -1 */
            glEnd ();
            glBegin (GL_POLYGON);
            curvert++;  /* skip -1 */
          }
          else
          { glTexCoord2fv ((float*) (texvertlist + texvertindex [curvert++ % numtexind]));
            glNormal3fv ((float*) (normallist + *normalindex++));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      else if (ge3d_texlighting && facenormals)  /* flat shaded texture */
      { /* assume overall material binding */
        glNormal3fv ((float*) facenormals++);
        while (numcoordind--)
        {
          cindex = *coordindex++;
          if (cindex < 0)
          { glEnd ();
            glBegin (GL_POLYGON);
            glNormal3fv ((float*) facenormals++);
            curvert++;  /* skip -1 */
          }
          else
          { glTexCoord2fv ((float*) (texvertlist + texvertindex [curvert++ % numtexind]));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      else  /* unlit texture */
      {
        while (numcoordind--)
        {
          cindex = *coordindex++;
          if (cindex < 0)
          { glEnd ();
            glBegin (GL_POLYGON);
            curvert++;  /* skip -1 */
          }
          else
          { glTexCoord2fv ((float*) (texvertlist + texvertindex [curvert++ % numtexind]));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
    } /* texturing */
    else if (ge3d_mode == ge3d_flat_shading || numnormalind < numcoordind)
    { /* insufficient vertex normals or flat shading */
      if (facenormals)  /* flat shading */
      {
        /* will soon introduce macros for specialized rendering loops */
        if ((matbinding == matb_perpart || matbinding == matb_perface) && materials)
        {
          register int curface = 0;
          ge3d_apply_material (materials, curface++);

          glNormal3fv ((float*) facenormals++);
          while (numcoordind--)
          {
            cindex = *coordindex++;
            if (cindex < 0)
            { glEnd ();
              ge3d_apply_material (materials, curface++);
              glBegin (GL_POLYGON);
              glNormal3fv ((float*) facenormals++);
            }
            else
              glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
        else if ((matbinding == matb_perpartindexed || matbinding == matb_perfaceindexed)
             && materials && nummatind && materindex)
        {
          register int curface = 0;
          ge3d_apply_material (materials, materindex [curface++ % nummatind]);

          glNormal3fv ((float*) facenormals++);
          while (numcoordind--)
          {
            cindex = *coordindex++;
            if (cindex < 0)
            { glEnd ();
              ge3d_apply_material (materials, materindex [curface++ % nummatind]);
              glBegin (GL_POLYGON);
              glNormal3fv ((float*) facenormals++);
            }
            else
              glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
        else if (matbinding == matb_pervertex && materials)
        { /* per vertex binding only visible in smooth shading */
          register unsigned curvert = 0;

          glNormal3fv ((float*) facenormals++);

          if (ge3d_mode == ge3d_flat_shading)
          { /* flat shading: first vertex color used for whole face */
            register unsigned int firstvert = 1;

            while (numcoordind--)
            {
              cindex = *coordindex++;
              if (cindex < 0)
              { glEnd ();
                glBegin (GL_POLYGON);
                firstvert = 1;
                if (ge3d_lighting)
                  glNormal3fv ((float*) facenormals++);
              }
              else
              { if (firstvert)
                { ge3d_apply_material (materials, curvert);  firstvert = 0;
                }
                curvert++;
                glVertex3fv ((float*) (vertexlist + cindex));
              }
            }
          }
          else  /* per vertex material binding, smooth shaded, no vertex normals */
          {
            while (numcoordind--)
            {
              cindex = *coordindex++;
              if (cindex < 0)
              { glEnd ();
                glBegin (GL_POLYGON);
                if (ge3d_lighting)
                  glNormal3fv ((float*) facenormals++);
              }
              else
              { ge3d_apply_material (materials, curvert++);
                glVertex3fv ((float*) (vertexlist + cindex));
              }
            }
          }
        }
        else if (matbinding == matb_pervertexindexed && materials && nummatind && materindex)
        {
          register unsigned curvert = 0;

          /* even if (num_diffuse <= 1) materials must be applied per vertex; */
          /* other components (like emissive) may cycle */

          /* basic loop to be varied: */
          /* without lighting no normals and diffuse color only, for flat shading no materials per vertex */
          /*
          glNormal3fv ((float*) facenormals++);
          while (numcoordind--)
          {
            cindex = *coordindex++;
            if (cindex < 0)
            { glEnd ();
              glBegin (GL_POLYGON);
              glNormal3fv ((float*) facenormals++);
              curvert++;
            }
            else
            { ge3d_apply_material (materials, materindex [curvert++ % nummatind]);
              glVertex3fv ((float*) (vertexlist + cindex));
            }
          }
          */

          if (ge3d_lighting)
          {
            glNormal3fv ((float*) facenormals++);

            if (ge3d_mode == ge3d_flat_shading)
            {
              register unsigned int firstvert = 1;
              while (numcoordind--)
              {
                cindex = *coordindex++;
                if (cindex < 0)
                { glEnd ();
                  glBegin (GL_POLYGON);
                  firstvert = 1;
                  glNormal3fv ((float*) facenormals++);
                  curvert++;  /* skip -1 */
                }
                else
                { if (firstvert)
                  { ge3d_apply_material (materials, materindex [curvert % nummatind]);  firstvert = 0;
                  }
                  curvert++;
                  glVertex3fv ((float*) (vertexlist + cindex));
                }
              }
            }
            else
            {
              while (numcoordind--)
              {
                cindex = *coordindex++;
                if (cindex < 0)
                { glEnd ();
                  glBegin (GL_POLYGON);
                  glNormal3fv ((float*) facenormals++);
                  curvert++;  /* skip -1 */
                }
                else
                { ge3d_apply_material (materials, materindex [curvert++ % nummatind]);
                  glVertex3fv ((float*) (vertexlist + cindex));
                }
              }
            }
          } /* pervertexindexed with lighting */
          else  /* no lighting calculations (frequent case for pervertex material binding */
          {
            register unsigned int i;  /* num_transparency = materials->num_transparency; */
            register unsigned int num_diffuse = materials->num_diffuse;
            /* float A = 1.0;  static float color [4];  const colorRGB* c; */

            if (num_diffuse <= 1)  /* degenerate case: no need for per vertex binding */
            {
              while (numcoordind--)
              {
                cindex = *coordindex++;
                if (cindex < 0)
                { glEnd ();
                  glBegin (GL_POLYGON);
                }
                else
                  glVertex3fv ((float*) (vertexlist + cindex));
              }
            }
            else if (ge3d_mode == ge3d_flat_shading)
            {
              register unsigned int firstvert = 1;

              while (numcoordind--)
              {
                cindex = *coordindex++;
                if (cindex < 0)
                { glEnd ();
                  glBegin (GL_POLYGON);
                  firstvert = 1;
                  curvert++;  /* skip -1 */
                }
                else
                {
                  if (firstvert)
                  {
                    /* ge3d_apply_material (materials, materindex [curvert % nummatind]); */
  
                    i = materindex [curvert % nummatind];
/* // removed for efficiency, mpichler, 19950901
                    if (num_transparency && (A = 1.0 - materials->val_transparency [i % num_transparency]) < 1.0)
                    { c = materials->rgb_diffuse + (i % num_diffuse);
                      assigncolor (color, c->R, c->G, c->B);  color [3] = A;
                      glColor4fv (color);
                    }
                    else  /* Alpha 1.0 * /
*/
                      glColor3fv ((float*) (materials->rgb_diffuse + (i % num_diffuse)));
                    firstvert = 0;
                  }
                  curvert++;
                  glVertex3fv ((float*) (vertexlist + cindex));
                }
              }
            }
            else  /* smooth, no lighting */
            {
              while (numcoordind--)
              {
                cindex = *coordindex++;
                if (cindex < 0)
                { glEnd ();
                  glBegin (GL_POLYGON);
                  curvert++;  /* skip -1 */
                }
                else
                {
                  /* ge3d_apply_material (materials, materindex [curvert++ % nummatind]); */

                  i = materindex [curvert++ % nummatind];
/* // removed for efficiency, mpichler, 19950901
                  if (num_transparency && (A = 1.0 - materials->val_transparency [i % num_transparency]) < 1.0)
                  { c = materials->rgb_diffuse + (i % num_diffuse);
                    assigncolor (color, c->R, c->G, c->B);  color [3] = A;
                    glColor4fv (color);
                  }
                  else  /* Alpha 1.0 * /
*/
                    glColor3fv ((float*) (materials->rgb_diffuse + (i % num_diffuse)));
                  glVertex3fv ((float*) (vertexlist + cindex));
                }
              }
            }
          } /* pervertexindexed without lighting */
        } /* pervertexindexed */
        else  /* flat shading with overall material binding */
        {
          glNormal3fv ((float*) facenormals++);
          while (numcoordind--)
          {
            cindex = *coordindex++;
            if (cindex < 0)
            { glEnd ();
              glBegin (GL_POLYGON);
              glNormal3fv ((float*) facenormals++);
            }
            else
              glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      else  /* no face normals - will yield in incorrect flat shading */
      { /* facenormals should always be provided, this branch not maintained */
        while (numcoordind--)
        {
          cindex = *coordindex++;
          if (cindex < 0)
          { glEnd ();
            glBegin (GL_POLYGON);
          }
          else
            glVertex3fv ((float*) (vertexlist + cindex));
        }
      }
    }
    else /* smooth shading and numnormalind == numcoordind */
    { /* smooth shading: normal for each vertex */
      /* note: lighting may be on or off (then still per-vertex materials poss.) */
      if ((matbinding == matb_perpart || matbinding == matb_perface) && materials)
      {
        register int curface = 0;
        ge3d_apply_material (materials, curface++);

        while (numcoordind--)
        {
          cindex = *coordindex++;
          /*fprintf (stderr, "[%d/%d] ", cindex, *normalindex);*/
          /* nindex = *normalindex++; */
          if (cindex < 0)
          { normalindex++;  /* skip -1 */
            glEnd ();
            ge3d_apply_material (materials, curface++);
            glBegin (GL_POLYGON);
          }
          else
          { glNormal3fv ((float*) (normallist + *normalindex++));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      else if ((matbinding == matb_perpartindexed || matbinding == matb_perfaceindexed)
           && materials && nummatind && materindex)
      {
        register int curface = 0;
        ge3d_apply_material (materials, materindex [curface++ % nummatind]);

        while (numcoordind--)
        {
          cindex = *coordindex++;
          /*fprintf (stderr, "[%d/%d] ", cindex, *normalindex);*/
          /* nindex = *normalindex++; */
          if (cindex < 0)
          { normalindex++;  /* skip -1 */
            glEnd ();
            ge3d_apply_material (materials, materindex [curface++ % nummatind]);
            glBegin (GL_POLYGON);
          }
          else
          { glNormal3fv ((float*) (normallist + *normalindex++));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      else if (matbinding == matb_pervertex && materials)
      {
        register unsigned curvert = 0;
        /* TODO: if (!ge3d_lighting) normal vectors can be omitted, use diffuse component only */

        while (numcoordind--)
        {
          cindex = *coordindex++;
          /*fprintf (stderr, "[%d/%d] ", cindex, *normalindex);*/
          /* nindex = *normalindex++; */
          if (cindex < 0)
          { normalindex++;  /* skip -1 */
            glEnd ();
            glBegin (GL_POLYGON);
          }
          else
          { ge3d_apply_material (materials, curvert++);
            glNormal3fv ((float*) (normallist + *normalindex++));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      else if (matbinding == matb_pervertexindexed && materials && nummatind && materindex)
      {
        register unsigned curvert = 0;

        /* basic loop to be varied: without lighting no normals and diffuse color only */
        /*
        while (numcoordind--)
        {
          cindex = *coordindex++;
          if (cindex < 0)
          { normalindex++;
            glEnd ();
            glBegin (GL_POLYGON);
            curvert++;
          }
          else
          { ge3d_apply_material (materials, materindex [curvert++ % nummatind]);
            glNormal3fv ((float*) (normallist + *normalindex++));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
        */

        if (ge3d_lighting)
        {
          while (numcoordind--)
          {
            cindex = *coordindex++;
            /*fprintf (stderr, "[%d/%d] ", cindex, *normalindex);*/
            /* nindex = *normalindex++; */
            if (cindex < 0)
            { normalindex++;  /* skip -1 */
              glEnd ();
              glBegin (GL_POLYGON);
              curvert++;  /* skip -1 */
            }
            else
            { ge3d_apply_material (materials, materindex [curvert++ % nummatind]);
              glNormal3fv ((float*) (normallist + *normalindex++));
              glVertex3fv ((float*) (vertexlist + cindex));
            }
          }
        }
        else  /* no lighting calculations (frequent case for pervertex material binding */
        {
          register unsigned i, num_transparency = materials->num_transparency;
          register unsigned num_diffuse = materials->num_diffuse;
          float A = 1.0f;
          static float color [4];
          //const colorRGB* c;

          if (num_diffuse <= 1)  /* degenerate case: no need for per vertex binding */
          {
/* this will become macro FACESET_VERTSONLY */
            while (numcoordind--)
            {
              cindex = *coordindex++;
              if (cindex < 0)
              { glEnd ();
                glBegin (GL_POLYGON);
              }
              else
              { glVertex3fv ((float*) (vertexlist + cindex));
              }
            }
          }
          else  /* smooth, no lighting, per vertex indexed */
          {
/* this will become function SMOOTH_DIFFUSEPERVERTEXINDEXED */
            while (numcoordind--)
            {
              cindex = *coordindex++;
              /*fprintf (stderr, "[%d/%d] ", cindex, *normalindex);*/
              /* nindex = *normalindex++; */
              if (cindex < 0)
              { glEnd ();
                glBegin (GL_POLYGON);
                curvert++;  /* skip -1 */
              }
              else
              { /* ge3d_apply_material (materials, materindex [curvert++ % nummatind]); */
                i = materindex [curvert++ % nummatind];
/* // removed for efficiency, mpichler, 19950901
                if (num_transparency && (A = 1.0 - materials->val_transparency [i % num_transparency]) < 1.0)
                { c = materials->rgb_diffuse + (i % num_diffuse);
                  assigncolor (color, c->R, c->G, c->B);  color [3] = A;
                  glColor4fv (color);
                }
                else  /* Alpha 1.0 * /
*/
                  glColor3fv ((float*) (materials->rgb_diffuse + (i % num_diffuse)));
  
                /* normals irrelevant without lighting calculations */
                /* glNormal3fv ((float*) (normallist + *normalindex++)); */
                glVertex3fv ((float*) (vertexlist + cindex));
              }
            }
          }
        }
      }
      else  /* smooth shading with overall material binding */
      {
        while (numcoordind--)
        {
          cindex = *coordindex++;
          /*fprintf (stderr, "[%d/%d] ", cindex, *normalindex);*/
          /* nindex = *normalindex++; */
          if (cindex < 0)
          { normalindex++;  /* skip -1 */
            glEnd ();
            glBegin (GL_POLYGON);
          }
          else
          { glNormal3fv ((float*) (normallist + *normalindex++));
            glVertex3fv ((float*) (vertexlist + cindex));
          }
        }
      }
      /*fprintf (stderr, "\n");*/
    }

    glEnd ();  /* end of last polygon */
  } /* flat/smooth shading */

} /* ge3dFaceSet */



/* ge3dLineSet -- under construction */

void ge3dLineSet PARMS3(
  const point3D*, vertexlist,
  int, numcoordind,
  const int*, coordindex
/*,
  const void*, materlist,
  int, nummatind,
  const int*, materindex,
*/
)
{
  register int cindex;
  int didlighting = (ge3d_mode >= ge3d_flat_shading && ge3d_lighting);

  /*fprintf (stderr, "ge3dLineSet with %d polyline indices.\n", numcoordind);*/

  if (didlighting)
    glDisable (GL_LIGHTING);

  glBegin (GL_LINE_STRIP);  /* open polylines */
  while (numcoordind--)
  {
    cindex = *coordindex++;
    /*fprintf (stderr, "[%d] ", cindex);*/
    if (cindex < 0)
    { glEnd ();
      glBegin (GL_LINE_STRIP);
    }
    else
      glVertex3fv ((float*) (vertexlist + cindex));
  }
  glEnd ();  /* end of last polyline */

  if (didlighting)
    glEnable (GL_LIGHTING);
  /*fprintf (stderr, "\n");*/

} /* ge3dLineSet */



/* ge3dPointSet -- under construction */

void ge3dPointSet PARMS2(
  const point3D*, points,
  int, numpts
/*,
  const void*, materlist,
  int, nummatind,
  const int*, materindex,
*/
)
{
  int didlighting = (ge3d_mode >= ge3d_flat_shading && ge3d_lighting);

  if (didlighting)
    glDisable (GL_LIGHTING);

  glBegin (GL_POINTS);  /* points */

  while (numpts--)
    glVertex3fv ((float*) points++);

  glEnd ();  /* end of last polyline */

  if (didlighting)
    glEnable (GL_LIGHTING);

} /* ge3dPointSet */


void ge3dSphere PARMS1(
  float, radius
)
{
  glPushMatrix ();
  glRotatef (-90.0f, 1.0f, 0.0f, 0.0f);  /* y axis, not z, is vertical */

  if (ge3d_mode == ge3d_hidden_line)
  {
    float oldztran;
    glColor3fv (backg_color);
    gluQuadricDrawStyle (gluquadobj,  GLU_FILL);
    gluSphere (gluquadobj, radius, 12 /*slices*/, 6 /*stacks*/);
    gluQuadricDrawStyle (gluquadobj,  GLU_LINE);
    glColor3fv (line_color);
    BEGINPOLYGONOFFSET (oldztran)
    gluSphere (gluquadobj, radius, 12 /*slices*/, 6 /*stacks*/);
    ENDPOLYGONOFFSET (oldztran)
  }
  else
    gluSphere (gluquadobj, radius, 12 /*slices*/, 6 /*stacks*/);

  glPopMatrix ();
} /* ge3dSphere */


void ge3dCylinder PARMS5(
  float, botradius,
  float, topradius,             /* use same as botradius for a cylinder, or 0 for a cone */
  float, bottom,
  float, height,
  int, parts                    /* cyl_all or combination of cyl_sides/bottom/top */
)
{
  /* might prefer to draw a single circle instead of a gluDisk
     gluDisk with 2 rings (use same no. of slices) */
  /* be aware that a display list would depend on the drawing mode */

  if (!parts)
    return;

  glPushMatrix ();
  glRotatef (-90.0f, 1.0f, 0.0f, 0.0f);  /* y axis, not z, is vertical */
  glTranslatef (0.0f, 0.0f, bottom);  /* glu functions draw into xy-plane (z = 0) */

#define CYLINDER_BODY  \
  if (parts & cyl_sides)  \
    gluCylinder (gluquadobj, botradius, topradius, height, 12 /*slices*/, 4 /*stacks*/);  \
  if ((parts & cyl_top) && topradius)  \
  { glTranslatef (0.0f, 0.0f, height);  \
    gluDisk (gluquadobj, 0.0f, topradius, 12 /*slices*/, 2 /*rings*/);  \
    glTranslatef (0.0f, 0.0f, -height);  \
  }

  if (ge3d_mode == ge3d_hidden_line)
  {
    float oldztran;
    glColor3fv (backg_color);
    gluQuadricDrawStyle (gluquadobj,  GLU_FILL);
    CYLINDER_BODY
    gluQuadricDrawStyle (gluquadobj,  GLU_LINE);
    glColor3fv (line_color);
    BEGINPOLYGONOFFSET (oldztran)
    CYLINDER_BODY
    ENDPOLYGONOFFSET (oldztran)
  }
  else
  { CYLINDER_BODY
  }
#undef CYLINDER_BODY

  if ((parts & cyl_bottom) && botradius)
  { /* do not use negative scale: interferes lighting and does not affect normal set in gluDisk */
    switch (ge3d_mode)
    {
      case ge3d_wireframe:    /* orientation irrelevant */
        gluDisk (gluquadobj, 0, botradius, 12 /*slices*/, 2 /*rings*/);
      break;
      case ge3d_hidden_line:  /* draw interior and outline */
      {
        float oldztran;
        glColor3fv (backg_color);
        gluQuadricDrawStyle (gluquadobj,  GLU_FILL);
        gluQuadricOrientation (gluquadobj,  GLU_INSIDE);
        gluDisk (gluquadobj, 0, botradius, 12 /*slices*/, 2 /*rings*/);
        gluQuadricOrientation (gluquadobj,  GLU_OUTSIDE);
        gluQuadricDrawStyle (gluquadobj,  GLU_LINE);
        glColor3fv (line_color);
        BEGINPOLYGONOFFSET (oldztran)
        gluDisk (gluquadobj, 0, botradius, 12 /*slices*/, 2 /*rings*/);
        ENDPOLYGONOFFSET (oldztran)
      }
      break;
      case ge3d_texturing:    /* for proper texture orientation do a rotation */
        glRotatef (180.0f, 1.0f, 0.0f, 0.0f);
        gluDisk (gluquadobj, 0, botradius, 12 /*slices*/, 2 /*rings*/);
      break;
      default:  /* reverse orientation and normal vector generated by gluDisk (simpler than rotation) */
        gluQuadricOrientation (gluquadobj,  GLU_INSIDE);
        gluDisk (gluquadobj, 0, botradius, 12 /*slices*/, 2 /*rings*/);
        gluQuadricOrientation (gluquadobj,  GLU_OUTSIDE);
    }
  } /* bottom */

  glPopMatrix ();
} /* ge3dCylinder */



/* ge3d_setmode */
/* changes the drawing mode */

void ge3d_setmode PARMS1( 
  int, newmode
)
{
  /*if (ge3d_mode == newmode)*/
  /*  return;                */

  /* wireframe turns everything off */
  /* other modes enable backface culling (if ge3d_backfaceculling) */
  /* and depth test (if ge3d_depthbuffering); */
  /* depth buffer is cleared when mode was previously wireframe */

  if (!gluquadobj)
    gluquadobj = gluNewQuadric ();

  glShadeModel ((newmode >= ge3d_smooth_shading) ? GL_SMOOTH : GL_FLAT);  /* smooth shading */

  if (newmode > ge3d_wireframe && ge3d_backfaceculling)  /* backface culling if enabled */
    glEnable (GL_CULL_FACE);
  else  /* unless wireframe */
    glDisable (GL_CULL_FACE);

  if (newmode == ge3d_wireframe)
    glDisable (GL_DEPTH_TEST);
  else if (ge3d_depthbuffering && ge3d_mode == ge3d_wireframe)
  { /* activate and clear depth buffer */
    glEnable (GL_DEPTH_TEST);
    glClear (GL_DEPTH_BUFFER_BIT);
  } /* else: GL_DEPTH_TEST remains enabled */

  if (newmode >= ge3d_flat_shading && ge3d_lighting)
    glEnable (GL_LIGHTING);
  else
    glDisable (GL_LIGHTING);

  /* hidden line not supported by GLU; emulated in sphere and cone routines */
  /* normal vectors not needed for orientation in hidden line */
  gluQuadricDrawStyle (gluquadobj,  ((newmode >= ge3d_flat_shading) ? GLU_FILL : GLU_LINE));
  gluQuadricNormals (gluquadobj,
     ((newmode < ge3d_flat_shading) ? GLU_NONE :
    (newmode == ge3d_flat_shading) ? GLU_FLAT : GLU_SMOOTH));
  gluQuadricTexture (gluquadobj, newmode == ge3d_texturing);

  ge3d_mode = newmode;

} /* ge3d_setmode */



void ge3d_setlinecolor PARMS3(
float, r,
float, g,
float, b
)
{ 
  assigncolor (line_color, r, g, b);
  glColor3fv (line_color);
  samelfcolor = 0;
}


void ge3dLineColor PARMS1(
const colorRGB*, c
)
{
  assigncolor (line_color, c->R, c->G, c->B);
  glColor3fv (line_color);
  samelfcolor = 0;
}


void ge3d_setlinestyle PARMS1(
short, pattern
)
{
  if (pattern == -1)
    glDisable (GL_LINE_STIPPLE);  /* solid line */
  else
  { glLineStipple (1 /* bit repetition factor */, pattern /* 16 bit */);
    glEnable (GL_LINE_STIPPLE);
  }
}


void ge3d_setlinewidth PARMS1(
short, width
)
{
  glLineWidth (width);  /* would allow float argument */
}


void ge3dAntialiasing PARMS1(
int, flags
)
{
  if (flags)  /* blending is used for anti-aliasing */
    glEnable (GL_BLEND);

  if (flags & ge3d_aa_lines)
    glEnable (GL_LINE_SMOOTH);
  else
    glDisable (GL_LINE_SMOOTH);

  /* polygon anti-aliasing isn't that trivial; conflicts much with z-buffer at edges */
  /* another approach is to use the accumulation buffer and drawing the scene multiple times */
/*
  if (flags & ge3d_aa_polygons)
    glEnable (GL_POLYGON_SMOOTH);
  else
    glDisable (GL_POLYGON_SMOOTH);
*/
  if (!flags)
    glDisable (GL_BLEND);
}


int ge3dAntialiasingSupport ()
{
  return ge3d_aa_lines;
}


/* define NOFONT if ge3d_text/ge3dText is provided from another library */

#ifndef NOFONT
void ge3d_text PARMS4(
float, x,
float, y,
float, z,
const char*, s
)
{
/* fonts are not a core OpenGL functionality */
}


void ge3dText PARMS2(
const point3D*, p,
const char*, s
)
{
/* fonts are not a core OpenGL functionality */
}
#endif


void ge3d_rect PARMS4(  /* rectangle (outline) */
float, x1,
float, y1,
float, x2,
float, y2
)
{
  glBegin (GL_LINE_LOOP);  /* prevent filling */
  glVertex2f (x1, y1);
  glVertex2f (x2, y1);
  glVertex2f (x2, y2);
  glVertex2f (x1, y2);
  glEnd ();
}


void ge3d_rectf PARMS4(  /* filled rectangle */
float, x1,
float, y1,
float, x2,
float, y2
)
{
  glRectf (x1, y1, x2, y2);  /* have to care for filling in constant color! */
}


void ge3dFilledRect PARMS5(  /* shaded, filled rectangle */
float, x1,
float, y1,
float, x2,
float, y2,
float, z
)
{
  glBegin (GL_POLYGON);
  glNormal3f (0.0f, 0.0f, 1.0f);
  glVertex3f (x1, y1, z);
  glVertex3f (x2, y1, z);
  glVertex3f (x2, y2, z);
  glVertex3f (x1, y2, z);
  glEnd ();
}



/* internal circle drawing routine */
/* caller responsible for appropriate glBegin/glEnd pair */

static void ge3d_draw_circle PARMS3(
float, x,
float, y,
float, r
)
{
/* could use display list instead of point array */
#define POINTSONCIRCLE 32
  static int firstcall = 1;
  static float sines [POINTSONCIRCLE];
  static float cosin [POINTSONCIRCLE];

  const float* sinus;
  const float* cosinus;
  int i;

  if (firstcall)
  {
    float twopioverpoints = (float)(2.0 * M_PI / POINTSONCIRCLE);
    float angle;

    firstcall = 0;  /* calculate points only once */

    for (i = 0;  i < POINTSONCIRCLE;  i++)
    { angle = i * twopioverpoints;
      sines [i] = (float)sin(angle);
      cosin [i] = (float)cos(angle);
    }
  }

  sinus = sines;
  cosinus = cosin;
  for (i = POINTSONCIRCLE;  i;  i--)
    glVertex2f (*cosinus++ * r + x, *sinus++ * r + y);

} /* ge3d_draw_circle */



void ge3d_circle PARMS3(  /* circle (outline) */
float, x,
float, y,
float, r
)
{
  glBegin (GL_LINE_LOOP);
  ge3d_draw_circle (x, y, r);
  glEnd ();
}



void ge3d_circf PARMS3(  /* filled circle */
float, x,
float, y,
float, r
)
{
  glBegin (GL_POLYGON);  /* should turn lighting off!!! */
  ge3d_draw_circle (x, y, r);
  glEnd ();
}



void ge3d_arc PARMS5(  /* arc (outline) */
float, x,
float, y,
float, radius,
float, startangle,     /* counterclockwise from positive x-axis, */
float, endangle        /* in degrees */
)
{
/* arc (x, y, radius, startangle*10, endangle*10); */

  float angle;
  int i;

  angle = (float)(M_PI / 180.0);  /* work with radians */
  startangle *= angle;
  endangle *= angle;
  angle = (endangle - startangle) / POINTSONCIRCLE;  /* may be < 0 */

  glBegin (GL_LINE_LOOP);
  for (i = 0;  i < POINTSONCIRCLE;  i++)
  {
    glVertex2f ((GLfloat)(cos (startangle) * radius + x), (GLfloat)(sin (startangle) * radius + y));
    startangle += angle;
  }
  glEnd ();
}



void ge3d_setfillcolor PARMS3(
float, r,
float, g,
float, b
)
{ 
  float color [4];  /* RGBA */

/*fprintf (stderr, "ge3d_setfillcolor (%g, %g, %g)\n", r, g, b);*/

  assigncolor (fill_color, r, g, b);
  assigncolor (line_color, r, g, b);  /* set also linecolor */
  glColor3fv (fill_color);
  samelfcolor = 1;

  if (ge3d_mode == ge3d_wireframe || ge3d_mode == ge3d_hidden_line)
    return;

  /* many architectures only support diffuse hardware lighting */
  assigncolor (color, r, g, b);
  color [3] = 1.0f;  /* Alpha */
  glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, color);
}


void ge3dMaterial PARMS2(
  int, scope,
  materialsGE3D const*, materials
)
{
  static int glscope [] =  /* must match mat_front/back definitions */
  { GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };
  float color [4];
  const colorRGB* c;
  float A = 1.0f;  /* Alpha: 1.0 opaque, 0.0 fully transparent */
  int n;

  if (materials->num_base)
  { /* base color as fillcolor */
    const colorRGB* basecol = materials->rgb_base;
    assigncolor (fill_color, basecol->R, basecol->G, basecol->B);
    assigncolor (line_color, basecol->R, basecol->G, basecol->B);
    glColor3fv (fill_color);
    samelfcolor = 1;
  }

  if (ge3d_mode < ge3d_flat_shading || !ge3d_lighting)
    return;

  scope = glscope [scope];

  n = materials->num_transparency;
  if (n)
    A = (float)(1.0 - *materials->val_transparency);
  color [3] = A;

  if (materials->num_ambient)
  { c = materials->rgb_ambient;
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv ((GLenum) scope, GL_AMBIENT, color);
  }
  if (materials->num_diffuse)
  { c = materials->rgb_diffuse;
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv ((GLenum) scope, GL_DIFFUSE, color);
  }
  if (materials->num_specular)
  { c = materials->rgb_specular;
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv ((GLenum) scope, GL_SPECULAR, color);
  }
  if (materials->num_emissive)
  { c = materials->rgb_emissive;
    assigncolor (color, c->R, c->G, c->B);
    glMaterialfv ((GLenum) scope, GL_EMISSION, color);
  }
  if (materials->num_shininess)  /* VRML/Inventor/ge3d: 0.0 to 1.0; OpenGL: 0.0 to 128.0 */
    glMaterialf ((GLenum) scope, GL_SHININESS, (float)(*materials->val_shininess * 128.0));
} /* ge3dMaterial */


void ge3dDefaultMaterial ()
{
  /* see OpenGL manual: */
  static float def_ambient [4] = { 0.2f, 0.2f, 0.2f, 1.0f };
  static float def_diffuse [4] = { 0.8f, 0.8f, 0.8f, 1.0f };
  static float def_none [4] = { 0.0f, 0.0f, 0.0f, 1.0f };

  /* reset material to default values */
  glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, def_ambient);
  glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, def_diffuse);
  glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, def_none);
  glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, def_none);
  glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}


/* ge3d_setbackgcolor/ge3dBackgroundColor may be set at a time when gl
   routines (glClearColor) are not yet callable */

void ge3d_setbackgcolor PARMS3(
float, r,
float, g,
float, b
)
{ 
  assigncolor (backg_color, r, g, b);
}



void ge3dBackgroundColor PARMS1(
const colorRGB*, c
)
{
  assigncolor (backg_color, c->R, c->G, c->B);
}



/* ge3d_set_light_source */
/* private function that sets up a light source (positional or
   directional) in camera coordinates
*/

static void ge3d_set_light_source PARMS4(
int, index,
const colorRGB*, color,
const point3D*, pos,
float, positional
)
{
  GLfloat val [4];  /* RGBA or xyzw */

/*fprintf (stderr, "ge3d_set_light_source (%d) colour (%g, %g, %g) position (%g, %g, %g) positional (%g)\n",
         index, color->R, color->G, color->B, pos->x, pos->y, pos->z, positional);*/
/*fprintf (stderr, "doing lighting: %d", (int) glIsEnabled (GL_LIGHTING));*/

  index += GL_LIGHT0;  /* guaranteed to be continuous */

  /* fprintf (stderr, "setting light at position (%f, %f, %f) with intensities (%f, %f, %f)\n",
     pos->x, pos->y, pos->z, color->R, color->G, color->B);
  */
  assigncolor (val, color->R, color->G, color->B);
  val [3] = 1.0f;
  glLightfv ((GLenum) index, GL_DIFFUSE, val);
  glPushMatrix ();
  glLoadIdentity ();  /* prevent transformation with current matrix */

  assigncolor (val, pos->x, pos->y, pos->z);
  val [3] = positional;  /* 1.0 or 0.0 */

  glLightfv ((GLenum) index, GL_POSITION, val);
  /* turn off spot light properties for ordinary lights */
  glLightf ((GLenum) index, GL_SPOT_EXPONENT, 0.0f);
  glLightf ((GLenum) index, GL_SPOT_CUTOFF, 180.0f);
  /* GL_SPOT_DIRECTION now insignificant */

  glPopMatrix ();
} /* ge3d_set_light_source */


/* ge3dSetLightSource */
/* arguments: index of light source, light color,
   position or direction (in world coordinates),
   positional flag (1.0 = pos., 0.0 = dir.);
   directions must be given *towards* light source,
   positions and directions are relative to camera
   (use of ge3d_setlightsource is discouraged; ge3dLightSource
   has a more flexible interface) */
/* ge3d_setlightsource no longer exists; call ge3dSetLightSource with flags
   for positional and camera light source or ge3dLightSource providing a
   transformation matrix
*/


void ge3dSetLightSource PARMS5(
int, index,
const colorRGB*, color,
const point3D*, pos,
float, positional,
int, camlgt
)
{
  point3D trfpos;

  if (!camlgt)
  {
    if (positional)
      ge3dTransformMcWc (pos, &trfpos);
    else
      ge3dTransformVectorMcWc (pos, &trfpos);
    pos = &trfpos;  /* use transformed position */
  }

  ge3d_set_light_source (index, color, pos, positional);
}



/* ge3dLightSource */
/* similar to ge3dSetLightSource, but position/orientation is not given
   as point/vector, but a transformation matrix that transforms the
   default position (0, 0, 0) or orientation (-1, 0, 0).
*/
/* note light sources must be defined and switched on again
   after each change of camera */
/* for directional light sources the default direction is
   from negative x axis towards origin ('from left') - GL
   expects a vector *towards* the light source, which is
   (-1, 0, 0) for the default direction
*/

void ge3dLightSource PARMS5(
int, index,
const colorRGB*, color,
/*const*/ matrix4D, mat,  /* const sacrificed for dull compilers */
float, positional,
int, camlgt
)
{
  point3D pos;

  glPushMatrix ();

  if (camlgt)  /* light position relative to current camera */
    glLoadMatrixf ((float*) mat);
  else         /* light position in world coordinates */
    glMultMatrixf ((float*) mat);  /* necessary for correct lighting */

  glGetFloatv (GL_MODELVIEW_MATRIX, ge3d_tm);

  if (positional)
  { /* ge3d_transform_mc_wc ((float) 0, (float) 0, (float) 0, &pos.x, &pos.y, &pos.z); */
    init3D (pos, ge3d_tm [12], ge3d_tm [13], ge3d_tm [14]);
  }
  else
  { /* ge3d_transformvector_mc_wc ((float) -1, (float) 0, (float) 0, &pos.x, &pos.y, &pos.z); */
    init3D (pos, - ge3d_tm [0], - ge3d_tm [1], - ge3d_tm [2]);
  }

  glPopMatrix ();

  ge3d_set_light_source (index, color, &pos, positional);
  /* already transformed to camera coordinates */
} /* ge3dLightSource */


void ge3dSpotLight PARMS6(
  int, index,                           /* light index */
  const colorRGB*, color,               /* color */
  const point3D*, pos,                  /* light position (in object coordinates) */
  const vector3D*, dir,                 /* spot direction - along light emission */
  float, dropoff,                       /* exponent for angular dropoff */
  float, cutoffangle                    /* spread angle from center, degrees, range [0, 90] or 180 */
)
{
  GLfloat val [4];  /* RGBA or xyzw */

  index += GL_LIGHT0;  /* guaranteed to be continuous */

  assigncolor (val, color->R, color->G, color->B);  val [3] = 1.0f;
  glLightfv ((GLenum) index, GL_DIFFUSE, val);

  assigncolor (val, pos->x, pos->y, pos->z);  val [3] = 1.0f;  /* positional */
  glLightfv ((GLenum) index, GL_POSITION, val);

  assigncolor (val, dir->x, dir->y, dir->z);  /* val [3] ignored */
  glLightfv ((GLenum) index, GL_SPOT_DIRECTION, val);

  /* dropOffRate: VRML/Inventor/ge3d: 0..1; OpenGL: 0..128 */
  glLightf ((GLenum) index, GL_SPOT_EXPONENT, (float)(dropoff * 128.0));

  glLightf ((GLenum) index, GL_SPOT_CUTOFF, cutoffangle);

} /* ge3dSpotLight */


void ge3d_switchlight PARMS2(
int, index,
int, state
)
{ 
  if (index > 7)  /* 0..7 allowed */
  { fprintf (stderr, "ge3d. error: invalid light index %d.\n", index);
    return;
  }
/*
  pushmatrix ();
  loadmatrix (ge3d_identitymat);
  lmbind (lighttarget [index], state ? index : 0);
  popmatrix ();  
*/
  index += GL_LIGHT0;  /* guaranteed to be continuous */
  if (state)
    glEnable ((GLenum) index);
  else
    glDisable ((GLenum) index);
}



void ge3d_transform_mc_wc PARMS6(
float, inx,
float, iny,
float, inz,
float*, outx,
float*, outy,
float*, outz
)
{
  glGetFloatv (GL_MODELVIEW_MATRIX, ge3d_tm);
  *outx = inx * ge3d_tm [0] + iny * ge3d_tm [4] + inz * ge3d_tm [8] + ge3d_tm [12];
  *outy = inx * ge3d_tm [1] + iny * ge3d_tm [5] + inz * ge3d_tm [9] + ge3d_tm [13];
  *outz = inx * ge3d_tm [2] + iny * ge3d_tm [6] + inz * ge3d_tm[10] + ge3d_tm [14];
}


void ge3dTransformMcWc PARMS2(
const point3D*, in,
point3D*, out
)
{
  float x = in->x, y = in->y, z = in->z;  /* beware of transform (p, p)! */
  glGetFloatv (GL_MODELVIEW_MATRIX, ge3d_tm);
  out->x = x * ge3d_tm [0] + y * ge3d_tm [4] + z * ge3d_tm [8] + ge3d_tm [12];
  out->y = x * ge3d_tm [1] + y * ge3d_tm [5] + z * ge3d_tm [9] + ge3d_tm [13];
  out->z = x * ge3d_tm [2] + y * ge3d_tm [6] + z * ge3d_tm[10] + ge3d_tm [14];
}


void ge3d_transformvector_mc_wc PARMS6(
float, inx,
float, iny,
float, inz,
float*, outx,
float*, outy,
float*, outz
)
{
  glGetFloatv (GL_MODELVIEW_MATRIX, ge3d_tm);
  *outx = inx * ge3d_tm [0] + iny * ge3d_tm [4] + inz * ge3d_tm [8];
  *outy = inx * ge3d_tm [1] + iny * ge3d_tm [5] + inz * ge3d_tm [9];
  *outz = inx * ge3d_tm [2] + iny * ge3d_tm [6] + inz * ge3d_tm[10];
}


void ge3dTransformVectorMcWc PARMS2(
const vector3D*, in,
vector3D*, out
)
{
  float x = in->x, y = in->y, z = in->z;  /* beware of transform (v, v)! */
  glGetFloatv (GL_MODELVIEW_MATRIX, ge3d_tm);

  out->x = x * ge3d_tm [0] + y * ge3d_tm [4] + z * ge3d_tm [8];
  out->y = x * ge3d_tm [1] + y * ge3d_tm [5] + z * ge3d_tm [9];
  out->z = x * ge3d_tm [2] + y * ge3d_tm [6] + z * ge3d_tm[10];
}



void ge3d_setcamera PARMS8(     /* set up a perspective camera */
const point3D*, eye,            /* camera position */
const point3D*, ref,            /* reference (lookat) point */
const vector3D*, up,            /* up vector (defaults to (0, 1, 0) on NULL) */
float, aper,                    /* window height ... */
float, focal,                   /* ... at distance focal */
float, aspect,                  /* ratio width / height */
float, hither,                  /* near and ... */
float, yon                      /* ... far clipping plane (both from eye; positive) */
)
{
  float alpha;

  alpha = (float)(2 * atan2 (aper, 2 * focal));  /* in radians */

  glMatrixMode (GL_PROJECTION);
  /* fprintf (stderr, "calling gluPerspective (%f, %f, %f, %f)\n",
     (float) (alpha * 180 / M_PI), aspect, hither, yon); */
  glLoadIdentity ();  /* replace old projection matrix */
  gluPerspective (alpha * 180 / M_PI, aspect, hither, yon);

  glMatrixMode (GL_MODELVIEW);  /* load matrix of lookat on viewing matrix stack - otherwise lighting is incorrect */
  glLoadIdentity ();  /* replace old camera transformation */
  if (up)
    gluLookAt (eye->x, eye->y, eye->z, ref->x, ref->y, ref->z, up->x, up->y, up->z);  /* use up vector */
  else
    gluLookAt (eye->x, eye->y, eye->z, ref->x, ref->y, ref->z, 0.0, 1.0, 0.0);  /* y axis as up direction */
  /* fprintf (stderr, "lookat matrix\n"); ge3d_print_cur_matrix (); */
  /* GL_MODELVIEW */
}


void ge3dCamera PARMS8(                 /* camera (perspective/orthographic; new style) */
  int, type,                            /* cam_perspective/orthographic | cam_absolute/relative */
  const point3D*, pos,                  /* camera position */
  float, rotangle,                      /* angle of rotation (degrees) ... */
  const vector3D*, rotaxis,             /* ... around axis */
  float, height,        /* perspective: total vertical viewing angle, degrees; orthographic: window height */
  float, aspect,                        /* ratio width / height */
  float, hither,                        /* near and ... */
  float, yon                            /* ... far clipping plane (both from eye; positive) */
)                                       /* perspective: both clipping planes must be positive */
{
  /* only perspective part of camera transformation has to be on projection matrix */
  /* otherwise incorrect lighting */

  if (type & cam_perspective)
  {
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();  /* replace old projection matrix */
    gluPerspective (height/*angle*/, aspect, hither, yon);
  }
  else if (type & cam_orthographic)
  {
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();  /* replace old projection matrix */
    height /= 2.0f;     /* half height */
    aspect *= height;  /* half width */
    glOrtho (-aspect, aspect, -height, height, hither, yon);
  } /* may have to place gluOrtho onto modelview matrix stack for correct lighting (?) */
  /* otherwise no change of projection matrix */

  /* we have to translate, then rotate the camera, */
  /* so apply the inverse rotation and translation to the scene */
  glMatrixMode (GL_MODELVIEW);
  if (type & cam_absolute)
    glLoadIdentity ();  /* replace old camera transformation */
  if (type & (cam_absolute | cam_relative))
  { glRotatef (- rotangle, rotaxis->x, rotaxis->y, rotaxis->z);
    glTranslatef (- pos->x, - pos->y, - pos->z);
  }
  /* otherwise no change of modelview matrix */
  /* GL_MODELVIEW */
}


void ge3d_ortho_cam PARMS7(     /* set up an orthogonal camera */
const point3D*, eye,            /* camera position */
const point3D*, ref,            /* reference (lookat) point */
const vector3D*, up,            /* up vector (defaults to (0, 1, 0) on NULL) */
float, width,                   /* window width */
float, height,                  /* window height */
float, hither,                  /* near and ... */
float, yon                      /* ... far clipping plane (both from eye; neg. allowed) */
)
{
  width /= 2;
  height /= 2;
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (-width, width, -height, height, hither, yon);

  glMatrixMode (GL_MODELVIEW);  /* load matrix of lookat on viewing matrix stack - otherwise lighting is incorrect */
  glLoadIdentity ();  /* replace old camera transformation */
  if (up)
    gluLookAt (eye->x, eye->y, eye->z, ref->x, ref->y, ref->z, up->x, up->y, up->z);  /* use up vector */
  else
    gluLookAt (eye->x, eye->y, eye->z, ref->x, ref->y, ref->z, 0.0, 1.0, 0.0);  /* y axis as up direction */
  /* GL_MODELVIEW */
}



/* matrix operations */
/* be aware that OpenGL uses COLUMNmajor matrices, multiplication with
   COLUMNvectors and POSTconcatenation whereas IRIX GL uses ROWmajor
   matrices, multiplication with ROWvectors and PREconcatenation.
*/

void ge3d_push_matrix ()
{
  glPushMatrix ();  
}


void ge3dPushIdentity ()
{
  /* push identity matrix onto the stack */
  glPushMatrix ();
  glLoadIdentity ();
}


void ge3dLoadIdentity ()
{
  /* replace current transformation matrix with identity matrix */
  glLoadIdentity ();
}


void ge3d_push_this_matrix
#ifdef GE3D_PROTOTYPES
(const float mat [4][4])
#else
(mat)  const float mat [4][4];
#endif
{
  glPushMatrix ();
  glMultMatrixf ((float*) mat);  /* concatenate */
}


void ge3d_push_new_matrix
#ifdef GE3D_PROTOTYPES
(const float mat [4][4])
#else
(mat)  const float mat [4][4];
#endif
{
  glPushMatrix ();
  glLoadMatrixf ((float*) mat);  /* new */
}


void ge3dMultMatrix
#ifdef GE3D_PROTOTYPES
(const float mat [4][4])
#else
(mat)  const float mat [4][4];
#endif
{
  glMultMatrixf ((float*) mat);  /* concatenate */
  /* OpenGL wants column major matrices to multiply with column vectors */
  /* IrisGL wants row major matrices to multiply with row vectors */
  /* watch the float array as you like - its the same */
  /* e.g. elements [12, 13, 14] do the translation part */
}


void ge3d_print_cur_matrix ()
{
  int i;

  glGetFloatv (GL_MODELVIEW_MATRIX, ge3d_tm);
  for (i = 0;  i < 4;  i++)
    printf ("%13f %12f %12f %12f\n", ge3d_tm [i], ge3d_tm [i+4], ge3d_tm [i+8], ge3d_tm [i+12]);
}


void ge3d_get_and_pop_matrix PARMS1(
matrix4D, mat
)
{
  /* note that IRIX/Open GL returns a row/column major matrix */
  glGetFloatv (GL_MODELVIEW_MATRIX, (float*) mat);
  glPopMatrix ();
}



void ge3d_pop_matrix ()
{
  glPopMatrix ();
}



void ge3d_rotate_axis PARMS2(
char, axis,
float, angle  /* in degrees */
)
{
  /* it's a bit cumbersome for OpenGL to do elementary rotations - */
  /* so build the rotation matrices by hand (also to avoid rounding errors) */

  static float matxrot [16] = { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f };
  static float matyrot [16] = { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f };
  static float matzrot [16] = { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f };

  float s = (float)sin (angle * (M_PI / 180));
  float c = (float)cos (angle * (M_PI / 180));

  switch (axis)
  {
    case 'x':
    case 'X':
      /*glRotatef (angle, 1, 0, 0);*/
      matxrot [5] = matxrot [10] = c;  /* [1][1], [2][2] */
      matxrot [9] = - (matxrot [6] = s);
      glMultMatrixf (matxrot);
    break;
    case 'y':
    case 'Y':
      /*glRotatef (angle, 0, 1, 0);*/
      matyrot [0] = matyrot [10] = c;  /* [0][0], [2][2] */
      matyrot [2] = - (matyrot [8] = s);
      glMultMatrixf (matyrot);
    break;
    case 'z':
    case 'Z':
      /*glRotatef (angle, 0, 0, 1);*/
      matzrot [0] = matzrot [5] = c;  /* [0][0], [1][1] */
      matzrot [4] = - (matzrot [1] = s);
      glMultMatrixf (matzrot);
    break;
  }
} /* ge3d_rotate_axis */


void ge3dRotate PARMS2(
  const vector3D*, axis,  /* rotation axis vector */
  float, angle            /* in radians, righthand */
)
{
  glRotatef (angle * (float)(180 / M_PI), axis->x, axis->y, axis->z);
}


void ge3d_translate PARMS3(
float, x,
float, y,
float, z
)
{
  glTranslatef (x, y, z);
}


void ge3dTranslate PARMS1(
const vector3D*, v
)
{
  glTranslatef (v->x, v->y, v->z);
}



void ge3d_scale PARMS4(
float, x,
float, y,
float, z,
float, all
)
{
  glScalef (x * all, y * all, z * all);
}


void ge3dScale PARMS1(
const float*, s    /* scaling by a point or vector makes no sense; just 3 float values */
)
{
  glScalef (s [0], s [1], s [2]);
}


/* texture mapping routines */

int ge3dTexturingSupport ()
{
  return 1;  /* texture mapping supported in OpenGL */
}


/* flip_image
   little helper for ge3dCreateTexture that converts top-to-bottom images to bottom-to-top
   the pointer returned must be deleted with free
*/

static void* flip_image (int bytesperrow, int height, const void* data)
{
  unsigned char* imgdata = (unsigned char*) malloc (height * bytesperrow);
  if (!imgdata)
    return 0;

  const unsigned char* src;
  unsigned char* dst = imgdata;
  register int j;

  while (height--)
  { /* copy source row [height] onto next destination row */
    src = ((const unsigned char*) data) + height * bytesperrow;
    j = bytesperrow;
    while (j--)
      *dst++ = *src++;
  }
  return imgdata;
}


/* ge3dCreateTexture

   creates a texture from a data array
   width and height specify the size (need not be powers of 2),
   data is the data array as specified by bmpformat argument:
   ge3d_ubyte_RGB_TB ... triples of unsigned byte, no filling byte, top-to-bottom
   (this is currently the only format supported).

   The function returns a handle (actually a display list number)
   which is to be used for later calls of ge3dApplyTexture and
   finally for distroying with ge3dFreeTexture.
   In case of an error, 0 will be returned.
*/

int ge3dCreateTexture PARMS4(
  int, width,
  int, height,
  const void*, data,
  int, bmpformat
)
{
  int index;

  index = glGenLists (1);  /* get an unused list index */
  if (!index)
    fprintf (stderr, "ge3dCreateTexture. error: could not create display list\n");
/*
else fprintf (stderr, "ge3d: created texture display list with index %d; texture size: %d x %d\n", index, width, height);
*/

  glNewList (index, GL_COMPILE);

/*if (bmpformat == ge3d_ubyte_RGB_TB) */
/*  // single texture image, must be power of 2 size in each direction
    glTexImage2D (GL_TEXTURE_2D, 0, 3, / * texture, level of detail, num RGB components * /
                  width, height, 0,  / * width, height, border (0/1) * /
                  GL_RGB, GL_UNSIGNED_BYTE, data / * format, type, data * /
    );
    / * fprintf (stderr, "glTex error: %s\n", gluErrorString (glGetError ())); * /
*/

  switch (bmpformat)
  {
    case ge3d_ubyte_RGB_TB:
    {
      void* imgdata = flip_image (3 * width, height, data);
      if (imgdata)
      { /* series of mipmapped texture images, automatically scaled to power of 2 */
        gluBuild2DMipmaps (GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, imgdata);
        free (imgdata);
      }
    }
    break;

    case ge3d_ubyte_I_BT:
      gluBuild2DMipmaps (GL_TEXTURE_2D, 1, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    break;
    case ge3d_ubyte_IA_BT:
      gluBuild2DMipmaps (GL_TEXTURE_2D, 2, width, height, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
    break;
    case ge3d_ubyte_RGB_BT:
      gluBuild2DMipmaps (GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    break;
    case ge3d_ubyte_RGBA_BT:
      gluBuild2DMipmaps (GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    break;

    default:
      fprintf (stderr, "ge3dCreateTexture: error: invalid enumerator %d for bitmap format\n", (int) bmpformat);
  }

  glEndList ();

  return index;
} /* ge3dCreateTexture */


/* ge3dFreeTexture
   frees a texture (actually a display list) associated with its handle
*/

void ge3dFreeTexture PARMS1(
  int, index
)
{
/*fprintf (stderr, "ge3d: freeing texture display list %d\n", index);*/
  glDeleteLists (index, 1);
}


/* ge3dDoTexturing - turn on or off texture mapping */
/* may be done implicitly in ge3dApplyTexture (argument 0 to turn off) */

void ge3dDoTexturing PARMS1(
  int, on
)
{
  ge3d_do_texturing = on;

  /* fprintf (stderr, "ge3d: %s texture mapping\n", on ? "enabling" : "disabling"); */
  if (on)
    glEnable (GL_TEXTURE_2D);
  else
    glDisable (GL_TEXTURE_2D);
}


/* ge3dApplyTexture
   apply a texture associated with its handle (actually call the display list)
*/

void ge3dApplyTexture PARMS1(
  int, index
)
{
  /* fprintf (stderr, "ge3d: calling texture display list with index %d\n", index); */
  glCallList (index);  /* will be ignored on invalid handles */
}


/* ge3dTextureRepeat
   set repeat flags for s and t direction
*/

void ge3dTextureRepeat PARMS2(
  int, s,
  int, t
)
{
  //BUG? (DMcK)
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s ? GL_REPEAT : GL_CLAMP);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t ? GL_REPEAT : GL_CLAMP);
}


/* ge3dLoadTextureIdentity
   set the texture trf. matrix to an identity matrix
*/

void ge3dLoadTextureIdentity ()
{
  glMatrixMode (GL_TEXTURE);
  glLoadIdentity ();
  glMatrixMode (GL_MODELVIEW);
}


/* ge3dMultTextureMatrix
   multiply the current texture trf. matrix with given matrix
*/

void ge3dMultTextureMatrix
#ifdef GE3D_PROTOTYPES
(const float mat [4][4])
#else
(mat)  const float mat [4][4];
#endif
{
  glMatrixMode (GL_TEXTURE);
  glMultMatrixf ((float*) mat);  /* concatenate */
  glMatrixMode (GL_MODELVIEW);
}


/* ge3dTexturedPolygon
   polygon with texture mapped on it,
   the polygon is textured and flat coloured (regardless of current mode),
   fillcolor and linecolor have no influence;
   contains implicit ge3dDoTexturing (1) and ge3dApplyTexture (handle)
*/

void ge3dTexturedPolygon PARMS4(
  int, nverts,
  const point3D*, vertexlist,
  const point2D*, texturelist,
  int, index
)
{
  glEnable (GL_TEXTURE_2D);
  glCallList (index);

  glBegin (GL_POLYGON);
  while (nverts--)
  { glTexCoord2fv ((float*) texturelist++);
    glVertex3fv ((float*) vertexlist++);
  }
  glEnd ();
}



#if 0
// /* overlay routines */

// /* the following routines are NOT allowed for mixed-model programs:
//    ge3dRequestOverlay, ge3dBitplanes, ge3dClearOverlay

//    the according functionality must be obtained with X-routines.
// */

// /* ge3dRequestOverlay ();

//    call this function once right after initialization to request
//    overlay planes.
//    return value: no. of bitplanes got (currently at least 2)

//    when true overlay bitplanes are not available (only on 8bit entry
//    systems), use pop up planes (which should normally be reserved to
//    the window manager).
// */

// int ge3dRequestOverlay ()
// {
//   int avail = getgdesc (GD_BITS_OVER_SNG_CMODE);

//   if (avail < 2)  /* no overlay planes - use popup planes */
//   { ge3d_overlay_drawmode = PUPDRAW;
//     return 2;     /* popup planes need not be configured */
//   }

//   /* overlay planes available */
//   ge3d_overlay_drawmode = OVERDRAW;
//   overlay (avail);
//   gconfig ();
//   return avail;
// }

// /* ge3dBitplanes (mode);

//    determines active bitplanes that are affected by following
//    commands.
//    argument: (currently) either ge3d_normal_planes or ge3d_overlay_planes
// */

// void ge3dBitplanes (int mode)
// {
//   if (mode == ge3d_overlay_planes)
//     drawmode (ge3d_overlay_drawmode);  /* usually OVERDRAW */
//   else
//     drawmode (NORMALDRAW);
// }

// /* ge3dClearOverlay ();

//    clears the overlay bitplanes (to the invisible color);
//    also activates overlay bitplanes for following commands.
// */

// void ge3dClearOverlay ()
// {
//   drawmode (ge3d_overlay_drawmode);
//   color (0);
//   clear ();
//   /* overlay bitplanes remain active at this point */
// }
#endif


/* ge3dMapColori (int index, short r, short g, short b);
   ge3dMapColorRGB (int index, const RGBColor*);

   define a color map entry. (r, g, b) are in range 0 to 255,
   RGB color components are defined in range 0.0 to 1.0.
   With n bitplanes color indices 0 to (1<<n)-1 are available.
   For overlay planes index 0 is reserved for "invisible".

   Note that currently normal bitplanes are always in RGB mode and
   overlay bitplanes olways in colormap (color indexed) mode. Note
   also that the functions ge3d_polygon, ge3d_polyhedron, and
   ge3dShadedPolygon may not be called in in colormap mode.
*/

void ge3dMapColori PARMS4(
int, index,
short, r,
short, g,
short, b
)
{
/*mapcolor (index, r, g, b);*/
}


void ge3dMapColorRGB PARMS2(
int, index,
const colorRGB*, c
)
{
/*mapcolor (index, c->R * 255.0, c->G * 255.0, c->B * 255.0);*/
}


/* ge3dColorIndex (int index);

   activate a color from the colormap. Only allowed in colormap
   mode (i.e. currently only for overlay bitplanes).
*/

void ge3dColorIndex PARMS1(
int, index
)
{
/*color (index);*/
}


/* ge3d_close */

void ge3d_close ()
{
}
