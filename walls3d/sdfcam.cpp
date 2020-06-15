//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        sdfcam.C
//
// Purpose:     implementation of class SDFCamera
//
// Created:     18 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:      4 May 95   Michael Pichler
//
//
//</file>


#include "sdfcam.h"

#include "ge3d.h"

#include <iostream>



SDFCamera::SDFCamera(int obj_n, int par, const char* name)
	: Object3D(obj_n, par, name), Camera()
{
	dispstat_ = 0;  // set on reading
}


void SDFCamera::print()
{
	cout << "Camera. ";

	Object3D::printobj();  // name, number, channels

	Camera::print();
}


void SDFCamera::computeParams()
{
	if (pos_look_set_)  // position and lookat were set externally, channel values are ignored
		return;

	ge3d_push_new_matrix((const float(*)[4]) trfmat_);

	ge3d_transform_mc_wc(0, 0, 0, &position_.x, &position_.y, &position_.z);
	ge3d_transform_mc_wc(0, 0, -1, &lookat_.x, &lookat_.y, &lookat_.z);
	// ray from position to lookat determins line of sight

	ge3d_pop_matrix();

} // computeParams
