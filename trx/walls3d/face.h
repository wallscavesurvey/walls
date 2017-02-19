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
 * File:     face.h
 *
 * Author:   Michael Hofer (until May 92) and Michael Pichler
 *
 * Created:   2 Apr 92
 *
 * Changed:  30 Jan 95
 *
 */


#ifndef face_h
#define face_h


#include "vectors.h"


typedef struct
{
  int num_faceverts,     /* number of vertices in face */
      num_facenormals,   /* number of normals in face */
      num_facetexverts;  /* number of texture verts in face */
  int *facevert,         /* array of indices of the vertices, ... */
      *facenormal,       /* ... and of the normals, ... */
      *facetexvert;      /* ... and texture vertices of the face */
  vector3D normal;       /* normalized, outward face normal */
} face;


#endif
