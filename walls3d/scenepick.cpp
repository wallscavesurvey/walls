//<copyright>
// 
// Copyright (c) 1993,94,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        scenepick.C
//
// Purpose:     implementation of SDFScene::pickObject
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     16 Jan 96   Michael Pichler
//
// $Id: scenepick.C,v 1.4 1996/01/30 12:31:07 mpichler Exp $
//
//</file>


#include "scene3d.h"
#include "memleak.h"


#include "geomobj.h"
#include "sdfcam.h"
#include "vecutil.h"

#include "ge3d.h"

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <math.h>



// pickObject
// picking of a ray from A in direction b
// return value: a pointer to the first geometric object hit
// by the ray A + t * b or NULL if no object is hit.
// output parameters: hitpoint, normal vector, groups hit, hit time (unless pointers are NULL)
// change on 19930728: parameter linksonly discarded (always consider all objects on picking)
// TODO: care for twosided polygons

void* SDFScene::pickObject(
	const point3D& A,
	const vector3D& b,
	float t_near,
	float t_min,
	GeometricObject** gobj,
	QvNode** node,
	QvWWWAnchor** anchor,
	point3D* hitpoint, vector3D* normal,
	const StringArray** groups,
	float* hittime
)
{
	if (gobj)
		*gobj = 0;
	if (node)
		*node = 0;
	if (anchor)
		*anchor = 0;

	if (!activecam_)
		return 0;  // no scene

	  // in case there are there still some compilers that cannot make a float& out of a float argument
	float tnear = t_near;
	float tmin = t_min;

	GeometricObject* hitobj = NULL;

	/*
	  // display ray for testing (only visible without double buffering)
	  ge3d_setlinecolor (1.0, 0.0, 0.0);
	  ge3d_moveto (vppoint.x, vppoint.y, vppoint.z);  // line from eye to vppoint
	  ge3d_lineto (A.x, A.y, A.z);  // projects to a single point on screen (hit point)
	*/
	//cerr << "ray from " << A << " to " << vppoint << endl;

	if (groups)
		*groups = 0;  // no group hit

	  // go through all objects
	for (GeometricObject* gobjptr = (GeometricObject*)gobjlist_.first();  gobjptr;
		gobjptr = (GeometricObject*)gobjlist_.next())
	{ // if (!linksonly || gobjptr->pickable ())  // always all objects are considered
		if (gobjptr->ray_hits(A, b, tnear, tmin, normal, groups))  // updates tmin, computes normal (obj. coord.)
			hitobj = gobjptr;
	} // for all geometric objects

	// fill in the results
	if (hitobj && hitpoint)  // hitpoint unchanged if no object hit
		pol3D(A, tmin, b, *hitpoint);  // hitpoint = A + b*tmin

	if (hitobj && hittime)  // hittime undefined if no object hit
		*hittime = tmin;

	// transform normal from object to world coordinates (transposed inverse trf.mat.)
	if (hitobj && normal)
	{
		const matrix4D* im = hitobj->getInvTrfMat();
		vector3D w_n;
		w_n.x = (*im)[0][0] * normal->x + (*im)[0][1] * normal->y + (*im)[0][2] * normal->z;
		w_n.y = (*im)[1][0] * normal->x + (*im)[1][1] * normal->y + (*im)[1][2] * normal->z;
		w_n.z = (*im)[2][0] * normal->x + (*im)[2][1] * normal->y + (*im)[2][2] * normal->z;
		float norm2 = dot3D(w_n, w_n);
		if (norm2 > 0.0)  // normalize
		{
			norm2 = 1 / sqrt(norm2);
			scl3D(w_n, norm2);
		}
		*normal = w_n;
	}

	if (gobj)
		*gobj = hitobj;

	return (void*)hitobj;

}  // SDFScene::pickObject
