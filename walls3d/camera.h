// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993,94,95,96
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        camera.h
//
// Purpose:     interface to Camera base object
//
// Created:      4 May 95   Michael Pichler (extracted from SDFCamera)
//
// Changed:     18 Jan 96   Michael Pichler
//
// $Id: camera.h,v 1.8 1996/02/14 17:02:19 mpichler Exp $
//
//</file>



#ifndef harmony_scene_camera_h
#define harmony_scene_camera_h

#include "vectors.h"

class Scene3D;


class Camera
{
public:
	Camera();
	virtual ~Camera();

	void reset();  // set default position and orientation

	void setCamera(const Scene3D*);    // set ge3d's camera

	virtual void print();  // write camera information

	// *** movement of the scene/camera ***
	// translate and zooming includes collision detection (if enabled)
	void translate(const vector3D&, Scene3D*, int slide, int respnear);  // translate along vector
	void translate(float x, float y, float winasp, float flen, Scene3D*);
	// translate parallel to viewplane
	void zoom_in(float, Scene3D*);     // zoom in
	void rotate_camera_right(float);   // rotate left to right (around position)
	void rotate_camera_up(float);      // rotate bottom to top ("      "       )

	void rotate_camera(                // rotate camera around arbitrary center
		float l2r,                        //   horicontal angle (left-to-right; rad)
		float b2t,                        //   vertical angle (bottom-to-top; rad)
		const point3D& center             //   center of rotation
	);

	void rotate_axis(                  // rotate around arbitrary center and axis
		const point3D& center,            //   center of rotation
		const vector3D& axis,             //   axis of rotation
		float angle                       //   rotation angle (rad)
	);

	void rotate_caxis(                 // rotation about coordinate axis
		float,                            //   angle in radians
		const point3D& center,            //   center of coordinate system
		char axis                         //   'x', 'y', 'z'
	);                                  // (anachronism)

	void makeHoricontal();             // level view (includes untilt)
	void untilt();                     // untilted view

	void viewingRay(                   // get viewing ray from point on window
		float fx,                         //   relative x coordinate (0=left, 1=right)
		float fy,                         //   relative y coordinate (0=bottom, 1=top)
		point3D& eye,                     //   camera position
		vector3D& look,                   //   line of sight
		float& tnear,                     //   near clipping plane: eye + tnear * look
		float& tfar                       //   far clipping plane: eye + tfar * look
	);

	// *** set camera parameters ***
	void setposlookup(const point3D&, const point3D&, const vector3D* = 0);
	void setposition(const point3D& pos) { position_ = pos; }
	void setlookat(const point3D& lkat) { lookat_ = lkat; }

	// perspective/orthographic projections
	void perspectiveCam(float yangle);  // angle in rad
	void orthographicCam(float height);

	// *** get camera parameters ***
	void getposition(point3D& pos) const { pos = position_; }
	void getlookat(point3D& lkat) const { lkat = lookat_; }
	void getupvector(vector3D& upv) const { upv = up_; }

	// *** get/set individual parameters ***
	float getfocallen() const          // distance from camera to viewplane
	{
		return focallen_;
	}
	float getaper() const              // height of the camera window (on viewplane)
	{
		return aper_;
	}
	float getfovy() const;             // vertical field of view (radians)
	float getaspect() const            // ratio width/height of camera window
	{
		return aspect_;
	}                 // (should always match window aspect)
	float gethither() const            // near clipping plane
	{
		return hither_;
	}
	float getyon() const               // far clipping plane
	{
		return yon_;
	}
	void sethither(float value)
	{
		hither_ = value;
	}
	void setyon(float value)
	{
		yon_ = value;
	}

	static float collisionDistance_;    // (only relevant if > 1.1 * hither)

protected:
	point3D position_, lookat_;
	vector3D up_;
	int useupvec_;   // flag for up vector usage
	char projtype_;  // 'O' or 'P'
	float focallen_,
		aper_,     // viewport height
		aspect_,
		left_, top_, right_, bottom_,
		hither_, yon_;
	int pos_look_set_;

}; // Camera


inline void Camera::setposlookup(const point3D& pos, const point3D& look, const vector3D* up)
{
	position_ = pos;
	lookat_ = look;
	if (up)
		up_ = *up;
	pos_look_set_ = 1;
}


#endif
