//<copyright>
// 
// Copyright (c) 1993,94,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        camera.C
//
// Purpose:     implementation of class Camera
//
// Created:      4 May 95   Michael Pichler (extracted from SDFCamera)
//
// Changed:     14 Feb 96   Michael Pichler
//
// $Id: camera.C,v 1.9 1996/02/14 17:02:19 mpichler Exp $
//
//</file>


#include "camera.h"
#include "scene3d.h"

#include "vecutil.h"
#include "ge3d.h"

#include <math.h>
#include <iostream>

#pragma warning(disable:4305)

float Camera::collisionDistance_ = 1.5;


Camera::Camera ()
{
  reset ();  // default view

  useupvec_ = 1;
  projtype_ = 'P';
  focallen_ = 1.0;
  aper_ = 1.0;  // to check
  aspect_ = 4.0/3.0;  // ignored (window aspect ratio used instead)
  hither_ = 0.1;
  yon_ = 1000.0;

  pos_look_set_ = 0;  // only relevant for SDFCamera:
  // use channel values for camera position/lookat by default
}


Camera::~Camera ()
{ // some compilers confused with virtual inline
}


void Camera::reset ()
{
  // default view (choosen to produce an identity view transformation)
  init3D (position_, 0, 0, 0);
  init3D (lookat_, 0, 0, -1);
  init3D (up_, 0, 1, 0);  // untilted
  // perspective parameters unchanged
}


void Camera::setCamera (const Scene3D* scene)
{
  if (scene)
  { aspect_ = scene->getWinAspect ();
    useupvec_ = scene->arbitraryRotations ();
  }

  // TODO: possibly change up vector
  if (projtype_ == 'O')
    ge3d_ortho_cam (&position_, &lookat_, useupvec_ ? &up_ : NULL, aper_ * aspect_, aper_, 0.0, yon_);
  else
    ge3d_setcamera (&position_, &lookat_, useupvec_ ? &up_ : NULL, aper_, focallen_, aspect_, hither_, yon_);
}



void Camera::print ()
{
  // print information about the camera
  cout << "  projtype: " << projtype_ << " focallen: " << focallen_
       << " aper: " << aper_ << " aspect: " << aspect_ << endl;
  cout << "  left: " << left_ << " top: " << top_ << " right: " << right_ << " bottom: " << bottom_
       << " hither: " << hither_ << " yon: " << yon_ << endl;

  cout << endl;
}


float Camera::getfovy () const
{
  // vertical field of view (total; radians)
  return 2 * atan2 (aper_, 2 * focallen_);
}


// translate (const point3D&, ...)
// move whole camera by a vector
// scene must be known for collision detection,
// slide flag tells whether to "slide" along surface when colliding (otherwise stopping)
// respnear flat tells wheter to respect near clipping plane on (primary) collision test

void Camera::translate (const vector3D& tran, Scene3D* scene, int slide, int respnear)
{
  if (!scene || !scene->collisionDetection ())
  { inc3D (position_, tran);
    inc3D (lookat_, tran);
    // upvector unchanged
    // cerr << "-";
    return;
  }

  const float norm = norm3D (tran);  // also used for sliding
  if (!norm)  // no translation
    return;

  vector3D ntrans = tran;
  float temp = 1.0 / norm;
  scl3D (ntrans, temp);  // normalized translation

  // check whether position_ + tran (i.e. position_ + norm * ntrans)
  // is still colltime away from next object (along ray ntrans)

  point3D hitpoint;
  vector3D normal;
  float hittime;

  int hit = (scene->pickObject (
    position_, ntrans, respnear ? hither_ : 0, yon_,
    hitpoint, normal, &hittime) != 0);

  float colltime = 1.1 * hither_;  // collisionDistance_, at least 1.1 times hither
  if (collisionDistance_ > colltime)
    colltime = collisionDistance_;

  if (!hit)  // no ray intersection
  { inc3D (position_, tran);
    inc3D (lookat_, tran);
    // cerr << "_"; // no intersection
    return;
  }

  vector3D delta = position_;  // pointing from to hitpoint to position
  dec3D (delta, hitpoint);

  float ndist = dot3D (delta, normal);  // perpendicular distance of position from object hit
  // moving forward  x * ntrans  the distance becomes  (hittime - x) / hittime * ndist

  if (hittime && ndist && (hittime - norm) / hittime * ndist < colltime)  // collision
  {
    // might move onto collision plane here

    if (slide)
    {
      // slide along a vector perpendicular to normal, in direction "near" ntrans
      vector3D nslidev, slidevec;
      crp3D (ntrans, normal, nslidev);  // perp. help vector
      crp3D (normal, nslidev, slidevec);

      temp = norm3D (slidevec);
      if (!temp)
        return;
      nslidev = slidevec;
      temp = 1.0 / temp;
      scl3D (nslidev, temp);  // normalized

      // float snorm = norm;  // to slide same distance as would have walked forward
      // slide distance: length of projection of tran onto slidevec
      float snorm = dot3D (tran, slidevec) * temp * temp;  // (tran . slidevec) / (slidevec . slidevec)

      // check we are not colliding (hitpoint and normal vector not needed here)
      hit = (scene->pickObject (position_, nslidev, 0, yon_, 0, 0, 0, 0, 0, 0, &hittime) != 0);

      if (!hit || hittime > snorm + colltime)  // no coll. on sliding
      { scl3D (nslidev, snorm);
        inc3D (position_, nslidev);
        inc3D (lookat_, nslidev);
        // cerr << "*";  // sliding
      }
      // else cerr << "X";  // stopping

    } // sliding
  }
  else
  { inc3D (position_, tran);
    inc3D (lookat_, tran);
    // cerr << "+";  // no collision, moving forward
  }

} // translate



// translate (float x, float y, ...)
// move whole camera parallel to the viewplane
// x and y are fractions of window width and height
// corresponds to movement of plane at distance flen

void Camera::translate (float x, float y, float winasp, float flen, Scene3D* scene)
{
  vector3D look_pos, u, v;  // pointing to the right/upwards on the viewplane

  sub3D (lookat_, position_, look_pos);  // lookat - position
  crp3D (look_pos, up_, u);  // points to right on viewplane
  crp3D (u, look_pos, v);   // points upwards on viewplane

  // work with a window at distance flen instead of focallen
  // focallen is most often not related to the size of the scene

  float winheight = aper_ / focallen_;
  if (projtype_ != 'O')
    winheight *= flen;

  x *= winasp * winheight / norm3D (u);  // x *= windowwidth / norm (u)
  y *= winheight / norm3D (v);  // y *= windowheight / norm (v)

  vector3D tran;  // translation
  lcm3D (x, u, y, v, tran);  

  translate (tran, scene, 0, 0);  // collision detection, but no sliding, near clip irrelevant
} // translate



// zoom_in
// move camera along straight line from position towards lookat
// (distance dist in world coordinates)

void Camera::zoom_in (float dist, Scene3D* scene)
{
  vector3D look_pos;
  sub3D (lookat_, position_, look_pos);  // lookat - position

  float factor = dist / norm3D (look_pos);

  scl3D (look_pos, factor);

  translate (look_pos, scene, 1, 1);  // collision detection with sliding, respect near clip
} // zoom_in



/* common to all rotations:
   to avoid accumulation of rounding errors
   distance of lookat from position is normalized to 1.0
*/


// rotate_camera_right
// rotate camera left to right (around position, axis up_)
// (angle phi in radians)
// name grown historically, rotate_horicontal would be better

void Camera::rotate_camera_right (float phi)
{
  double c = cos (phi), s = sin (phi);

  vector3D look_pos, u;

  sub3D (lookat_, position_, look_pos);  // lookat - position
  crp3D (look_pos, up_, u);  // points to right on viewplane
  c /= norm3D (look_pos);  // normalize
  s /= norm3D (u);
  lcm3D (c, look_pos, s, u, look_pos);

  double factor = 1 / norm3D (look_pos);
  // set distance between position and lookat to 1

  pol3D (position_, factor, look_pos, lookat_);  // set new lookat

} // rotate_camera_right



// rotate_camera_up
// rotate camera upwards (around position, axis look_pos X up_)
// (angle phi in radians)
// name grown historically, rotate_vertical would be better

void Camera::rotate_camera_up (float phi)
{
  double c = cos (phi), s = sin (phi);

  vector3D look_pos, u, v;

  // look-pos and v must be normalized!
  sub3D (lookat_, position_, look_pos);  // lookat - position
  crp3D (look_pos, up_, u);  // points to right on viewplane
  crp3D (u, look_pos, v);   // points upwards on viewplane
  c /= norm3D (look_pos);  // normalize
  s /= norm3D (v);
  lcm3D (c, look_pos, s, v, look_pos);
  // new up vector
  crp3D (u, look_pos, up_);

  double factor = 1 / norm3D (look_pos);
  // set distance between position and lookat to 1
  pol3D (position_, factor, look_pos, lookat_);  // set new lookat

  factor = 1 / norm3D (up_);
  scl3D (up_, factor);  // normalize up vector

} // rotate_camera_up



// rotate_camera
// rotate camera around an arbitrary center
// angle l2r left-to-right, b2t bottom-to-top,
// both in radians as seen on the viewplane

void Camera::rotate_camera (float l2r, float b2t, const point3D& center)
{
  // ignore anything on matrix stack
  ge3dPushIdentity ();

  // compute u and v
  vector3D look_pos, u, v;
  sub3D (lookat_, position_, look_pos);  // lookat - position
  crp3D (look_pos, up_, u);  // points to right on viewplane
  crp3D (u, look_pos, v);   // points upwards on viewplane

  point3D uppoint;  // transform up point instead of vector
  add3D (up_, position_, uppoint);

  // move to origin
  dec3D (position_, center);
  dec3D (lookat_, center);
  dec3D (uppoint, center);

  if (l2r)
    ge3dRotate (&v, l2r);  // horicontal rotation
  if (b2t)
    ge3dRotate (&u, -b2t);  // vertical rotation (-u, b2t)

  // apply transformation
  ge3dTransformVectorMcWc (&position_, &position_);
  ge3dTransformVectorMcWc (&lookat_, &lookat_);
  ge3dTransformVectorMcWc (&uppoint, &uppoint);

  // back to original coordinate system
  inc3D (position_, center);
  inc3D (lookat_, center);
  inc3D (uppoint, center);

  // restore upvector
  sub3D (uppoint, position_, up_);

  // set distance between position and lookat to 1
  vector3D pt_center;
  sub3D (lookat_, position_, pt_center);  // lookat - position
  double factor = 1 / norm3D (pt_center);
  pol3D (position_, factor, pt_center, lookat_);  // set new lookat

  ge3d_pop_matrix ();

  factor = 1 / norm3D (up_);
  scl3D (up_, factor);  // normalize up vector

} // rotate_camera (around center)



// rotate_axis
// rotation about arbitrary center and axis
// contributed by drg@spacetec.com
// angle in radians, CCW (one might use norm3D (axis) as rotation angle)

void Camera::rotate_axis (const point3D& center, const vector3D& axis, float angle)
{
  ge3dPushIdentity ();

  // compute u and v
  vector3D look_pos, u, v;
  sub3D (lookat_, position_, look_pos);  // lookat - position
  crp3D (look_pos, up_, u);  // points to right on viewplane
  crp3D (u, look_pos, v);   // points upwards on viewplane

  point3D uppoint;  // transform up point instead of vector
  add3D (up_, position_, uppoint);

  // move to origin
  dec3D (position_, center);
  dec3D (lookat_, center);
  dec3D (uppoint, center);

  ge3dRotate (&axis, angle);

  // apply transformation
  ge3dTransformVectorMcWc (&position_, &position_);
  ge3dTransformVectorMcWc (&lookat_, &lookat_);
  ge3dTransformVectorMcWc (&uppoint, &uppoint);

  // back to original coordinate system
  inc3D (position_, center);
  inc3D (lookat_, center);
  inc3D (uppoint, center);

  // restore upvector
  sub3D (uppoint, position_, up_);

  // set distance between position and lookat to 1
  vector3D pt_center;
  sub3D (lookat_, position_, pt_center);  // lookat - position
  double factor = 1 / norm3D (pt_center);
  pol3D (position_, factor, pt_center, lookat_);  // set new lookat

  ge3d_pop_matrix ();

  factor = 1 / norm3D (up_);
  scl3D (up_, factor);  // normalize up vector

} // rotate_axis



// rotate_
// rotate a vector vec around 'axis'
// by angle c = cos (phi), s = sin (phi), CCW


static void rotate_ (vector3D& vec, double c, double s, char axis)
{
  float* from,
       * to;

  switch (axis)
  { case 'x':  case 'X':
      from = &vec.y;  to = &vec.z;
    break;
    case 'y':  case 'Y':
      from = &vec.z;  to = &vec.x;
    break;
    default:  // case 'z':  case 'Z':
      from = &vec.x;  to = &vec.y;
    break;
  }

  float oldval = *from;

  *from = c * oldval - s * *to;
  *to   = c * *to    + s * oldval;

} // rotate_



// rotate_caxis
// rotate whole camera around point center
// along an axis, CCW

void Camera::rotate_caxis (float phi, const point3D& center, char axis)
{
  vector3D vec;
  double c = cos (phi),
         s = sin (phi);

  // rotate position
  sub3D (position_, center, vec);  // vec = position - center
  rotate_ (vec, c, s, axis);
  add3D (vec, center, position_);

  // rotate lookat
  sub3D (lookat_, center, vec);  // vec = lookat - center
  rotate_ (vec, c, s, axis);
  add3D (vec, center, lookat_);

  point3D uppoint;  // transform up point instead of vector
  add3D (up_, position_, uppoint);
  // rotate uppoint
  sub3D (uppoint, center, vec);  // vec = lookat - center
  rotate_ (vec, c, s, axis);
  add3D (vec, center, uppoint);
  // restore upvector
  sub3D (uppoint, position_, up_);

} // rotate_caxis



// makeHoricontal
// rotate around position such that position and lookat are on same level

void Camera::makeHoricontal ()
{
  vector3D look_pos, new_look_pos;

  sub3D (lookat_, position_, look_pos);  // lookat - position

  new_look_pos = look_pos;
  new_look_pos.y = 0;

  double newlensquare = dot3D (new_look_pos, new_look_pos);

  if (newlensquare > 0.0)
  { double factor = sqrt (dot3D (look_pos, look_pos) / newlensquare);
    // maintain distance between position and lookat
    // set new lookat
    pol3D (position_, factor, new_look_pos, lookat_);
  }
  // else: line of sight was parallel to y-axis - no change

  init3D (up_, 0, 1, 0);  // untilt as well

} // makehoricontal


void Camera::untilt ()
{
  init3D (up_, 0, 1, 0);  // default up vector
}


// viewingRay
// build a ray (eye + t * look) from a point on the viewing window

void Camera::viewingRay (
  float fx, float fy,
  point3D& eye, vector3D& look,
  float& tnear, float& tfar
)
{
  eye = position_;
  look = lookat_;
  vector3D up;
  if (useupvec_)
    up = up_;
  else
    init3D (up, 0, 1, 0);

//cerr << "eye = " << eye << ", lookat = " << look << endl;

  vector3D u, v, n;     // camera coordinate system (orthonormal)
  point3D vppoint,      // point of interest in world coordinates
          ref;          // view reference point (midpoint of viewplane)
  float temp;

  // setup camera coordinate system (normalize later)
  sub3D (look, eye, n);                 // n = look - pos; from eye to lookat, normal to viewplane
  crp3D (n, up, u);                     // u = n X up;     "to the right" on viewplane
  crp3D (u, n, v);                      // v = u X n;      "upwards" on viewplane

//cerr << "u = " << u << ", v = " << v << ", n = " << n << endl;

  if (projtype_ == 'O')  // orthographic camera
  {
//position, lookat, aper*aspect, aper, 0.0, yon
    temp = (fx - 0.5) * aper_ * aspect_ / norm3D (u);
    scl3D (u, temp);
    temp = (fy - 0.5) * aper_ / norm3D (v);
    pol3D (u, temp, v, u);
    // cerr << "translation: " << u << endl;
    inc3D (eye, u);   // eye += u
    inc3D (look, u);  // look += u

    dec3D (look, eye);  // ray direction: look - eye
    tnear = 0;  // see setCamera
    tfar = yon_ / norm3D (n);
    return;
  } // orthographic camera

  // perspective camera

  // compute view reference point (midpoint of viewplane) 
  temp = focallen_ / norm3D (n);
  pol3D (eye, temp, n, ref);

  // compute vppoint (point on viewplane)
  temp = (fx - 0.5) * aper_ * aspect_ / norm3D (u);
  pol3D (ref, temp, u, vppoint);
  temp = (fy - 0.5) * aper_ / norm3D (v);
  pol3D (vppoint, temp, v, vppoint);

  // eye = position_;
  sub3D (vppoint, eye, look);   // ray direction: vppoint - eye
  tnear = hither_ / focallen_;  // ray reaches near clipping plane
  tfar = yon_ / focallen_;      // ray reaches

} // viewingRay


// perspectiveCam
// turn camera into a perspective one

void Camera::perspectiveCam (float yangle)  // height angle (radians)
{
  reset ();
  projtype_ = 'P';
  focallen_ = 1.0;  // not relevant for camera
  aper_ = 2.0 * tan (yangle / 2.0);
  // hither, yon unchanged
}


// orthographicCam
// turn camera into an orthographic one

void Camera::orthographicCam (float height)
{
  reset ();
  projtype_ = 'O';
  focallen_ = 1.0;  // not relevant for camera
  aper_ = height;
  // hither, yon unchanged; although orthographic camera uses hither of 0.0
}
