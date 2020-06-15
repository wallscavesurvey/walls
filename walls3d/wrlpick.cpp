//<copyright>
// 
// Copyright (c) 1995,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        wrlpick.C
//
// Purpose:     picking routines of VRML nodes
//
// Created:     29 Sep 95   Michael Pichler
//
// Changed:     16 Jan 96   Michael Pichler
//
// $Id: wrlpick.C,v 1.13 1996/03/12 09:58:15 mpichler Exp mpichler $
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

#pragma warning(disable:4305)

// the traversal state during picking is the ray in object coordinates
// (changed on transformation nodes, restored on separator-like nodes)

// for computing the normal vector we need the inverse transposed
// transformation matrix, which is kept track with ge3d

static point3D ray_A;                   // ray start point (object coordinates)
static vector3D ray_b;                  // ray direction   ("      "          )
static float hit_tnear;                 // near clipping plane
static float hit_tmin;                  // current hit; updated during picking
static QvNode* hit_node = 0;            // node hit on picking
static QvWWWAnchor* hit_anchor = 0;     // anchor containg node hit
static float hit_tanch;                 // hit time of anchor
static vector3D* hit_normal = 0;        // if not NULL, filled out with normal vector in hit point

// other traversal states (e.g. twosided rendering) that are handled
// by ge3d during draw have to be tracked here.

static int pick_2side = 0;



/***** pickObject *****/

// picking of a ray from A in direction b
// returns the node hit and optionally the 3D hit point, the surface
// normal vector, and the hittime

void* VRMLScene::pickObject(
	const point3D& A,
	const vector3D& b,
	float tnear,
	float tfar,
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

	if (!root_)
		return 0;

	// copy to working vars
	ray_A = A;
	ray_b = b;
	hit_tnear = tnear;
	hit_tmin = tfar;

	point3D world_A = ray_A;   // save ray
	vector3D world_b = ray_b;  // in world coordinates

	hit_node = 0;  // node hit
	hit_anchor = 0;  // anchor hit
	hit_tanch = hit_tmin;  // anchor hit time
	hit_normal = normal;  // where to store normal vector
	if (normal)
	{
		init3D(*normal, 0, 0, 0);  // just to go safe
		ge3dPushIdentity();
	}

	pick_2side = scene_->twosidedpolys() != Scene3D::twosided_never;

	if (groups)
		*groups = 0;  // no group hit

	root_->pick();  // perform picking
	// bounding box test will be performed by each node
	// ray_A, ray_b may be changed on return; use unchanged world ray

	if (normal)
		ge3d_pop_matrix();

	if (hit_node && hitpoint)  // hitpoint unchanged if no object hit
	{
		pol3D(world_A, hit_tmin, world_b, *hitpoint);  // hitpoint = A + b*tmin
	   // cerr << "hit point: " << *hitpoint << endl;
	}

	if (hit_node && hittime)  // hittime undefined if no object hit
		*hittime = hit_tmin;

	// normal vector already in world coordinates

	if (hit_node && hit_normal)  // normal undefined if no object hit
	{ // might need double precision and comparision against small positive number here
		float norm2 = dot3D(*hit_normal, *hit_normal);
		if (norm2 > 0.0)  // normalize
		{
			norm2 = 1 / sqrt(norm2);
			scl3D(*hit_normal, norm2);
		}
	}

	if (node)
		*node = hit_node;
	if (anchor && hit_tmin == hit_tanch)  // anchor hit first
		*anchor = hit_anchor;

	return (void*)hit_node;

} // pickObject



/***** boundingbox test *****/

// boundingbox test: possible hit when ray hits bounding box
// (at t: 0 < t < tmin) or the camera is inside the box
// done in object coordinates, because wmin_/wmax_ are invalid after build
// on multiple instances (omin_/omax_ independent from traversal state)

// #define BBOXTEST  /* test: no bounding box test */
#define BBOXTEST   \
if (! (hasextent_  \
       && (rayhitscube (ray_A, ray_b, 0, hit_tmin, omin_, omax_)  \
           || pointinsidecube (ray_A, omin_, omax_))))  \
  return;



/***** groups *****/

void QvGroup::pick()
{
	// as a group may change the global transformation,
	// we cannot exit here if the bounding box is not hit

	int n = getNumChildren();
	for (int i = 0; i < n; i++)
		getChild(i)->pick();
}


void QvLOD::pick()
{
	// no bounding box test (see Group)

	if (lastdrawn_ >= 0)
		getChild(lastdrawn_)->pick();
}


void QvSeparator::pick()
{
	// can ignore all children, when bounding box is not hit
	BBOXTEST

		// save ray in current coordinate system
		point3D A = ray_A;
	vector3D b = ray_b;
	if (hit_normal)
		ge3d_push_matrix();

	int n = getNumChildren();
	for (int i = 0; i < n; i++)
		getChild(i)->pick();

	if (hit_normal)
		ge3d_pop_matrix();
	// restore ray
	ray_A = A;
	ray_b = b;
}


void QvSwitch::pick()
{
	// Switch behaves like Group; no boundingbox test
	int which = whichChild.value;

	if (which == QV_SWITCH_NONE)
		return;
	if (which == QV_SWITCH_ALL)
		QvGroup::pick();
	else if (which < getNumChildren())
		getChild(which)->pick();
}


void QvTransformSeparator::pick()
{
	// like Separator
	BBOXTEST

		// save ray in current coordinate system
		point3D A = ray_A;
	vector3D b = ray_b;
	if (hit_normal)
		ge3d_push_matrix();

	int n = getNumChildren();
	for (int i = 0; i < n; i++)
		getChild(i)->pick();

	if (hit_normal)
		ge3d_pop_matrix();
	// restore ray
	ray_A = A;
	ray_b = b;
}



#define PICK(className)  \
void className::pick ()  { }



/***** coordinates *****/

// already handled in build
PICK(QvCoordinate3)
PICK(QvNormal)
PICK(QvTextureCoordinate2)



/***** properties *****/


PICK(QvFontStyle)
PICK(QvMaterial)
PICK(QvMaterialBinding)
PICK(QvNormalBinding)

void QvShapeHints::pick()
{
	if (scene_->twosidedpolys() == Scene3D::twosided_auto)
		pick_2side = !backfaceculling_;
}

PICK(QvTexture2)
PICK(QvTexture2Transform)



/***** lights *****/


PICK(QvDirectionalLight)
PICK(QvPointLight)
PICK(QvSpotLight)



/***** cameras *****/


void QvOrthographicCamera::pick()
{
	// apply inverse transformation (orthographic projection and clipping planes unchanged)
	multiplyPoint3DMatrix4D(ray_A, invmat_, ray_A);
	multiplyVector3DMatrix4D(ray_b, invmat_, ray_b);

	if (hit_normal)  // inverse transposed is original rotation
		ge3dRotate(rotaxis_, -rotangle_);  // radians
		// translation has no effect on normal vector
}


void QvPerspectiveCamera::pick()
{
	// apply inverse transformation (perspective projection and clipping planes unchanged)
	multiplyPoint3DMatrix4D(ray_A, invmat_, ray_A);
	multiplyVector3DMatrix4D(ray_b, invmat_, ray_b);

	if (hit_normal)  // inverse transposed is original rotation
		ge3dRotate(rotaxis_, -rotangle_);  // radians
		// translation has no effect on normal vector
}


/***** transformations *****/

// TODO: may calculate overall transformations right after loading

// instead of transforming primitives, the ray is transformed into
// object coordinates by applying the inverse transformation;
// with ge3d we keep track of the accumulated inverse transposed
// for transforming the normal vector

void QvTransform::pick()
{
	// apply inverse transformation matrix
	multiplyPoint3DMatrix4D(ray_A, invmat_, ray_A);
	multiplyVector3DMatrix4D(ray_b, invmat_, ray_b);

	if (hit_normal)  // put transposed inverse transformation onto the stack
		ge3dMultMatrix((const float(*)[4]) invtranspmat3D_);
}


void QvRotation::pick()
{
	// apply inverse transformation matrix
	multiplyPoint3DMatrix4D(ray_A, invmat_, ray_A);
	multiplyVector3DMatrix4D(ray_b, invmat_, ray_b);

	if (hit_normal)  // inverse transposed is the original matrix
		ge3dMultMatrix((const float(*)[4]) mat_);
}


void QvMatrixTransform::pick()
{
	// apply inverse transformation matrix
	multiplyPoint3DMatrix4D(ray_A, invmat_, ray_A);
	multiplyVector3DMatrix4D(ray_b, invmat_, ray_b);

	if (hit_normal)  // put transposed inverse transformation onto the stack
		ge3dMultMatrix((const float(*)[4]) invtranspmat3D_);
}


void QvTranslation::pick()
{
	// apply inverse translation
	dec3D(ray_A, *trans_);  // A -= trans
	// vectors are not translated, of course
	// translation has no effect on normal vector
}


void QvScale::pick()
{
	// apply inverse scaling
	const float* is = invscale_;

	ray_A.x *= *is;  ray_b.x *= *is;  is++;
	ray_A.y *= *is;  ray_b.y *= *is;  is++;
	ray_A.z *= *is;  ray_b.z *= *is;

	// inverse transposed same as inverse
	if (hit_normal)
		ge3dScale(invscale_);
}



/***** shapes *****/


// HITENCOUNTERED ... report a hit
// arguments: hit time, hit node, and
// statement for calculating the normal vector in object coordinates

#define HITENCOUNTERED(hit,node,stmt)  \
{  \
  hit_tmin = hit;  \
  hit_node = node;  \
  if (hit_normal)  \
  {  \
    stmt;  \
    ge3dTransformVectorMcWc (hit_normal, hit_normal);  \
  }  \
}


// Intersection test between a line from P towards infinity +y and and
// edge from A to B; lines passing through a vertex point tangentially
// do not change the intersection count
// projection axis paxis: which coordinate to discard (0, 1, 2)
// Author: Keith Andrews <kandrews@iicm>
// adaption for projections of 3D points: Michael Pichler

static int Intersection(
	const point3D& p,
	const point3D& a,
	const point3D& b,
	int paxis
)
{
	//register 
	point2D P, A, B;  // test point, leftmost, and rightmost point
	double d;
	//cerr << "Inter[" << p << a << b << "]";

#define ASSIGN(u,v)  \
{  \
  init2D (P, p.u, p.v);  \
  if (a.u < b.u)  \
    init2D (A, a.u, a.v), init2D (B, b.u, b.v);  \
  else  \
    init2D (A, b.u, b.v), init2D (B, a.u, a.v);  \
} /* now A.u <= B.u */

	if (!paxis)           // discard x
		ASSIGN(y, z)
	else if (paxis == 1)  // discard y
		ASSIGN(x, z)
	else                  // discard z
		ASSIGN(x, y)

#undef ASSIGN

		//cerr << "Inter<" << P << A << B << ">";

		  // check intersection

		if (A.x >= P.x || B.x < P.x) return 0;

	if (A.y > P.y && B.y > P.y) return 1;

	//An intermediate double is required to prevent internal compiler error! (DMcK)
	d = A.y + (P.x - A.x)*((double)(B.y - A.y) / (B.x - A.x));

	if (d > (double)P.y) return 1;

	/*
	  if(A.x < P.x && B.x >= P.x) {                            // cases 1, 3, 5, 6
		//---------------------------
		if(A.y <= P.y) {
		  if(A.y + ((B.y-A.y)/(B.x-A.x))*(P.x-A.x) > P.y) return 1;
		}
		else {
		  if(B.y > P.y) return 1;
		  if(A.y + ((B.y-A.y)/(B.x-A.x))*(P.x-A.x) > P.y) return 1;
		}
		//---------------------------
		if ( A.y > P.y ) {                                        //   cases 1, 6
		  if ( (B.y > P.y) ||                                     //     case 1 or
			   (A.y + ((B.y-A.y)/(B.x-A.x))*(P.x-A.x) > P.y) )    //     case 6a
			return 1 ;                                            //       count intersection
		}
		else {                                                    //   cases 3, 5
		  if ( A.y + ((B.y-A.y)/(B.x-A.x))*(P.x-A.x) > P.y )      //     case 5a
			return 1 ;                                            //       count intersection
		}
		//---------------------------
	  }
	*/

	return 0;
} // Intersection



/***** shapes *****/


PICK(QvAsciiText)  // TODO


void QvCube::pick()
{
	// no bboxtest necessary

	float hit = rayhitscube(ray_A, ray_b, hit_tnear, hit_tmin, omin_, omax_, hit_normal);
	// cerr << "QvCube::pick: rayhitscube: " << hit << endl;

	if (hit)
	{
		HITENCOUNTERED(hit, this, 0)
			return;
	}

	if (pick_2side)
	{ // pick the inner side of the cube
		hit = rayhitscube(ray_A, ray_b, hit_tnear, hit_tmin, omin_, omax_, hit_normal, 1);
		if (hit)
			HITENCOUNTERED(hit, this, 0)
	}
} // QvCube


void QvCone::pick()
{
	BBOXTEST

		const float r = bottomRadius.value;
	const float r2 = r * r;  // r^2
	const float h = height.value;
	const float h2 = h / 2.0;  // half height
	// sides: x^2 + z^2 == R^2, R = (r/2 - r/h*y)
	// bottom: x^2 + z^2 <= r^2, y == -h2

	if ((parts_ & cyl_bottom) && ray_b.y && (ray_b.y > 0 || pick_2side))  // bottom: same as for cylinder
	{
		float hit = (-h2 - ray_A.y) / ray_b.y;
		if (hit_tnear < hit && hit < hit_tmin)
		{
			point3D hitpoint;
			pol3D(ray_A, hit, ray_b, hitpoint);

			if (hitpoint.x * hitpoint.x + hitpoint.z * hitpoint.z <= r2)
				HITENCOUNTERED(hit, this, init3D(*hit_normal, 0.0, (ray_b.y > 0) ? -1.0 : 1.0, 0.0))
		}
	}

	if (parts_ & cyl_sides)
	{
		// coefficients of quadratic equation
		float a = ray_b.x * ray_b.x + ray_b.z * ray_b.z - r2 * ray_b.y * ray_b.y / (h * h);
		float alpha = r / 2 - r * ray_A.y / h;
		float b = ray_A.x * ray_b.x + ray_A.z * ray_b.z + alpha * r * ray_b.y / h;
		float c = ray_A.x * ray_A.x + ray_A.z * ray_A.z - alpha * alpha;
		float d = b * b - a * c;  // discriminant

		if (a && d >= 0)  // otherwise no hit (parallel or no intersection)
		{
			d = sqrt(d) / a;
			a = -b / a;
			// ad normal vector: slope of sides is h/r, so slope of normal must be r/h
			// horicontal length is R, so height must be r*R/h

			// 2 possible hits at a +- d
			float hit = a - d;
			float hit_y = ray_A.y + hit * ray_b.y;

			if (-h2 < hit_y && hit_y < h2 && hit_tnear < hit && hit < hit_tmin)
				HITENCOUNTERED(hit, this, (
					hit_normal->x = ray_A.x + hit * ray_b.x,
					hit_normal->z = ray_A.z + hit * ray_b.z,
					hit_normal->y = r2 * (.5 / h - hit_y / (h * h)),
					(!hit_normal->y ? (init3D(*hit_normal, 0, 1, 0)) : 0)
					))
			else if (pick_2side)
			{
				hit = a + d;
				hit_y = ray_A.y + hit * ray_b.y;
				if (-h2 < hit_y && hit_y < h2 && hit_tnear < hit && hit < hit_tmin)
					HITENCOUNTERED(hit, this, (
						hit_normal->x = -ray_A.x - hit * ray_b.x,
						hit_normal->z = -ray_A.z - hit * ray_b.z,
						hit_normal->y = -r2 * (.5 / h - hit_y / (h * h)),
						(!hit_normal->y ? (init3D(*hit_normal, 0, -1, 0)) : 0)
						))
			}
		}
	} // cyl_sides

} // QvCone


void QvCylinder::pick()
{
	BBOXTEST

		float r2 = radius.value * radius.value;  // r^2
	float h2 = height.value / 2.0;  // half height
	// sides: x^2 + z^2 == r^2, y within +-h2
	// bottom/top: x^2 + z^2 <= r^2, y == +-h2

	if (ray_b.y)  // otherwise horicontal ray
	{
		if ((parts_ & cyl_bottom) && ray_b.y && (ray_b.y > 0 || pick_2side))
		{
			float hit = (-h2 - ray_A.y) / ray_b.y;
			if (hit_tnear < hit && hit < hit_tmin)
			{
				point3D hitpoint;
				pol3D(ray_A, hit, ray_b, hitpoint);

				if (hitpoint.x * hitpoint.x + hitpoint.z * hitpoint.z <= r2)
					HITENCOUNTERED(hit, this, init3D(*hit_normal, 0.0, (ray_b.y > 0) ? -1.0 : 1.0, 0.0))
			}
		}

		if ((parts_ & cyl_top) && ray_b.y && (ray_b.y < 0 || pick_2side))
		{
			float hit = (h2 - ray_A.y) / ray_b.y;
			if (hit_tnear < hit && hit < hit_tmin)
			{
				point3D hitpoint;
				pol3D(ray_A, hit, ray_b, hitpoint);

				if (hitpoint.x * hitpoint.x + hitpoint.z * hitpoint.z <= r2)
					HITENCOUNTERED(hit, this, init3D(*hit_normal, 0.0, (ray_b.y > 0) ? -1.0 : 1.0, 0.0))
			}
		}
	} // top/bottom

	if (parts_ & cyl_sides)
	{
		// coefficients of quadratic equation
		float a = ray_b.x * ray_b.x + ray_b.z * ray_b.z;
		float b = ray_A.x * ray_b.x + ray_A.z * ray_b.z;
		float c = ray_A.x * ray_A.x + ray_A.z * ray_A.z - r2;
		float d = b * b - a * c;  // discriminant

		if (a && d >= 0)  // otherwise no hit (parallel or no intersection)
		{
			d = sqrt(d) / a;
			a = -b / a;

			// 2 possible hits at a +- d
			float hit = a - d;
			float hit_y = ray_A.y + hit * ray_b.y;

			if (-h2 < hit_y && hit_y < h2 && hit_tnear < hit && hit < hit_tmin)
				HITENCOUNTERED(hit, this, init3D(*hit_normal, ray_A.x + hit * ray_b.x, 0.0, ray_A.z + hit * ray_b.z))
			else if (pick_2side)
			{
				hit = a + d;
				hit_y = ray_A.y + hit * ray_b.y;

				if (-h2 < hit_y && hit_y < h2 && hit_tnear < hit && hit < hit_tmin)
					HITENCOUNTERED(hit, this, init3D(*hit_normal, -ray_A.x - hit * ray_b.x, 0.0, -ray_A.z - hit * ray_b.z))
			}
		}
	} // cyl_sides

} // QvCylinder


void QvSphere::pick()
{
	BBOXTEST

		float r = radius.value;
	// sphere: (A + b*t)^2 == r^2

	// coefficients of quadratic equation
	float a = dot3D(ray_b, ray_b);
	float b = dot3D(ray_A, ray_b);
	float c = dot3D(ray_A, ray_A) - r * r;
	float d = b * b - a * c;  // discriminant

	if (!a || d < 0)  // no hit
		return;

	d = sqrt(d) / a;
	a = -b / a;

	// 2 possible hits at a +- d
	float hit = a - d;
	if (hit_tnear < hit && hit < hit_tmin)
	{
		HITENCOUNTERED(hit, this, pol3D(ray_A, hit, ray_b, *hit_normal))
			return;
	}

	hit = a + d;
	if (pick_2side && hit_tnear < hit && hit < hit_tmin)
		HITENCOUNTERED(hit, this, (pol3D(ray_A, hit, ray_b, *hit_normal), neg3D(*hit_normal)))  // normal points inside

} // QvSphere


// the FaceSet hit test used here projects each face to one of the
// three planes formed by the coordinate axis (by throwing away the
// largest absolute coordinate in the normal; see Foley/v.Dam p. 704)
// and does a 2D intersection test. As opposed to the 3D inclusion
// test implemented in scenepick.C this test works for arbitrary
// vertex order and non-convex polygons too (and is even more
// efficient)

void QvIndexedFaceSet::pick()
{
	BBOXTEST

		const int* cind = vertindices_;
	unsigned nv = numvertinds_;
	const vector3D* fn = facenormals_;
	point3D hitpoint;
	const point3D *p, *q;
	float hit, denom;


	while (nv)
	{
		// here at the beginning of a new face
		int v0 = cind[0];
		int v1 = cind[1];
		int v2 = cind[2];

		if (v0 >= 0 && v1 >= 0 && v2 >= 0 && nv > 2)
		{
			// cerr << "picking face " << v0 << ", " << v1 << ", " << v2 << ", ...: ";
			// *fn ... curent face normal
			if ((denom = dot3D(*fn, ray_b)) < 0 || pick_2side && denom)  // entering ray or twosided
			{
				// hit time: <n.(P-A)>/<n.b>
				p = vertexlist_ + v0;
				sub3D(*p, ray_A, hitpoint);  // not yet the hitpoint
				hit = dot3D(*fn, hitpoint) / denom;

				if (hit_tnear < hit && hit < hit_tmin)  // this face plane hit first
				{
					unsigned intersect = 0;  // odd-even intersection test

					// compute the hit point and check wheter it lies within the face
					pol3D(ray_A, hit, ray_b, hitpoint);

					// projection direction: largest abs. value of face normal
					// might precompute this during build
					int paxis = 0;  // x
					{
						float nmax = fabs(fn->x);
						if (fabs(fn->y) > nmax)
						{
							paxis = 1;  // y
							nmax = fabs(fn->y);
						}
						if (fabs(fn->z) > nmax)
							paxis = 2;  // z
					}
					// cerr << '+' << paxis << ' ';

					q = vertexlist_ + v0;
					while (nv > 1 && cind[1] >= 0)
					{ // test edge cind[0] to cind[1] (well defined when nv > 1)
						p = q;
						// cerr << '{' << cind[0] << '/' << cind[1] << '}';
						q = vertexlist_ + *++cind;
						nv--;
						intersect += Intersection(hitpoint, *p, *q, paxis);
						// int val = Intersection (hitpoint, *p, *q, paxis);  intersect += val;  cerr << '=' << val << ' ';
					}

					p = q;  // last edge (back to v0)
					q = vertexlist_ + v0;
					// cerr << '{' << cind[0] << '/' << v0 << '}';
					intersect += Intersection(hitpoint, *p, *q, paxis);
					// int val = Intersection (hitpoint, *p, *q, paxis);  intersect += val;  cerr << '=' << val << ' ';

					// cerr << 'i' << intersect << endl;
					if (intersect & 0x1)  // hitpoint found
					{ /* HITENCOUNTERED */
						hit_tmin = hit;
						hit_node = this;
						if (hit_normal)  // transformed to world coordinates at end of procedure
							*hit_normal = *fn;
					}

				} // nearest face
			} // entering ray or twosided
		} // face with at least 3 vertices

		// faces with less than 3 vertices are not drawn and therefore not picked
		// (see OpenGL Programming Guide, p. 36.: GL_POLYGON)

		// goto next face
		fn++;
		while (*cind >= 0 && nv)
			cind++, nv--;
		if (nv)  // skip index -1 (face separator)
			cind++, nv--;
	} // for all faces

	// transform normal if hit encountered
	if (hit_normal && hit_node == this)
	{
		if (dot3D(*hit_normal, ray_b) > 0)  // hit back side
			neg3D(*hit_normal);  // reverse normal vector
		ge3dTransformVectorMcWc(hit_normal, hit_normal);
	}

} // QvIndexedFaceSet


void QvIndexedLineSet::pick()
{
	BBOXTEST

		const int* cind = vertindices_;
	unsigned nv = numvertinds_;
	vector3D v;  // line direction
	int v0, v1 = -1;
	const float eps2 = epsilon_ * epsilon_;
	// weakness: fix distance tolerance in object coordinates,
	// independent of current transformation and scene size

	while (nv--)
	{
		// next edge
		v0 = v1;
		v1 = *cind++;

		if (v0 < 0 || v1 < 0)
			continue;
		// cerr << "picking edge (" << v0 << ", " << v1 << ")" << endl;

		const point3D& P = vertexlist_[v0];
		const point3D& Q = vertexlist_[v1];
		sub3D(Q, P, v);

		// check distance between lines (A + bt) and (P + kv)
		float hit = rayhitsline(ray_A, ray_b, hit_tnear, hit_tmin, P, v, eps2);

		if (hit)
		{ /* HITENCOUNTERED */
			hit_tmin = hit;
			hit_node = this;
			if (hit_normal)  // properly oriented at end of procedure
				*hit_normal = v;
		}

	} // for all edges

	// orient and transform normal if hit encountered
	if (hit_normal && hit_node == this)
	{ // here hit_normal is the direction of the line
		vector3D up;

		v = *hit_normal;
		crp3D(v, ray_b, up);
		crp3D(v, up, *hit_normal);
		// now hit_normal is perpendicular to the line and looks to -ray_b AFAP
		ge3dTransformVectorMcWc(hit_normal, hit_normal);
	}

} // QvIndexedLineSet


void QvPointSet::pick()
{
	BBOXTEST

		int n = num_;
	float hit;
	const point3D* pnt = points_;
	const float eps2 = epsilon_ * epsilon_;  // see remarks in IndexedLineSet

	while (n--)
	{
		hit = rayhitspoint(ray_A, ray_b, hit_tnear, hit_tmin, *pnt++, eps2);
		if (hit)
		{ /* HITENCOUNTERED */
			hit_tmin = hit;
			hit_node = this;
		} // normal defined at end of procedure

	} // for all points

	if (hit_normal && hit_node == this)
	{
		*hit_normal = ray_b;
		neg3D(*hit_normal);  // use inverse ray direction as point normal
		ge3dTransformVectorMcWc(hit_normal, hit_normal);
	}

} // QvPointSet



/***** WWW *****/


void QvWWWAnchor::pick()
{
	// like Separator
	BBOXTEST

		// save ray in current coordinate system
		point3D A = ray_A;
	vector3D b = ray_b;
	if (hit_normal)
		ge3d_push_matrix();

	float curhittime = hit_tmin;

	int n = getNumChildren();
	for (int i = 0; i < n; i++)
		getChild(i)->pick();

	if (hit_tmin < curhittime)  // hit a child of this anchor
		if (hit_tmin < hit_tanch)  // select innermost anchor
		{
			hit_tanch = hit_tmin;
			hit_anchor = this;

			if (map.value == POINT)  // save hitpoint in object coordinates
				pol3D(ray_A, hit_tmin, ray_b, hitpoint_);
		}

	if (hit_normal)
		ge3d_pop_matrix();
	// restore ray
	ray_A = A;
	ray_b = b;
} // QvWWWAnchor


void QvWWWInline::pick()
{
	// pick bounding box (lines only, not interior) until scene fetched
	if (hasextent_ && state_ != s_completed)
	{
		point3D A[12];
		vector3D v[12];
		point3D& min = omin_;
		point3D& max = omax_;

		point3D* aptr = A;
		point3D* vptr = v;
		vector3D up;

#define EDGE(X,Y,Z,C)  \
    init3D (*aptr, X ? max.x : min.x, Y ? max.y : min.y, Z ? max.z : min.z),  \
    init3D (*vptr, 0, 0, 0),  vptr->C = max.C - min.C,  \
    aptr++,  vptr++

		EDGE(0, 0, 0, x);
		EDGE(0, 0, 1, x);
		EDGE(0, 1, 0, x);
		EDGE(0, 1, 1, x);
		EDGE(0, 0, 0, y);
		EDGE(0, 0, 1, y);
		EDGE(1, 0, 0, y);
		EDGE(1, 0, 1, y);
		EDGE(0, 0, 0, z);
		EDGE(0, 1, 0, z);
		EDGE(1, 0, 0, z);
		EDGE(1, 1, 0, z);
#undef EDGE

		aptr = A;
		vptr = v;
		for (int i = 12; i--; aptr++, vptr++)
		{
			float hit = rayhitsline(ray_A, ray_b, hit_tnear, hit_tmin, *aptr, *vptr, 0.0025);
			if (hit)
				HITENCOUNTERED(hit, this, (crp3D(*vptr, ray_b, up), crp3D(*vptr, up, *hit_normal)));

		} // for all 12 bounding cube edges

	}

	QvGroup::pick();
} // QvWWWInline



/***** misc *****/


PICK(QvInfo)

void QvUnknownNode::pick()
{
	QvGroup::pick();
}



/***** extensions *****/


PICK(QvLabel)
PICK(QvLightModel)
