/*
 * File:     hiertypes.h
 *
 * Author:   Michael Hofer and Michael Pichler
 *
 * Created:  04 May 92
 *
 * Changed:  11 Jan 94
 *
 */


#ifndef hiertypes_h
#define hiertypes_h


class Object3D;

struct objindexstruct
{
	Object3D* obj;  // pointer to object i
	int flag;       // matrix for object i computed?
};


#endif
