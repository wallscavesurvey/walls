// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        object3d.h
//
// Purpose:     interface to 3D object
//
// Created:     12 Mar 92   Michael Hofer and Michael Pichler
//
// Changed:     26 Jul 95   Michael Pichler
//
//
//</file>



#ifndef harmony_scene_object3d_h
#define harmony_scene_object3d_h


#include "sdfscene.h"
struct objindexstruct;

#include "vectors.h"


class Object3D
{
public:
	Object3D(int obj_n, int par = 0, const char* name = 0);
	virtual ~Object3D() { }

	int getobj_num() const
	{
		return obj_num_;
	}
	int getparent() const
	{
		return parent_;
	}
	const char* name() const
	{
		return name_;
	}

	enum
	{
		ch_xtran, ch_ytran, ch_ztran, ch_xrot, ch_yrot, ch_zrot,
		ch_xscale, ch_yscale, ch_zscale, ch_scale,
		num_channels  // number of channels
	};

	static const char* channelName(int);
	float getChannel(int i) const { return channel_[i]; }

	const matrix4D* getTrfMat() const;
	const matrix4D* getInvTrfMat() const;

	const char* getTrfPrior() const { return transfprior_; }
	const char* getRotPrior() const { return rotprior_; }

	virtual const char* type() const  // type of object
	{
		return "obj";
	}                  // NEVER used to cast down a pointer

	virtual int sharedInfo() const  // filled if shared info (polyhedron)
	{
		return 0;
	}

	void writeVRMLTransformation(ostream&) const;

protected:
	int obj_num_,  // number of object
		parent_;   // number of parent (no parent: 0)
	char name_[32];

	float channel_[num_channels];  // 10 channels
	int no_of_channels_;  // in pos file

	matrix4D trfmat_,
		invtrfmat_;

	char transfprior_[4],
		rotprior_[4];

	void printobj();


	friend int SDFScene::readActFile(FILE*, int&);  // transfprior, rotprior
	friend int SDFScene::readPosFile(FILE*, int);  // channels
	friend void SDFScene::buildMatrix(objindexstruct*, int);  // transformation matrix
};


inline const matrix4D* Object3D::getTrfMat() const
{
	return (const matrix4D*)&trfmat_;
}

inline const matrix4D* Object3D::getInvTrfMat() const
{
	return (const matrix4D*)&invtrfmat_;
}


#endif
