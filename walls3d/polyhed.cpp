//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        polyhed.C
//
// Purpose:     implementation of class Polyhedron
//
// Created:      1 Apr 92   Michael Hofer and Michael Pichler
//
// Changed:     26 Apr 95   Michael Pichler (P. Mikl in May 95)
//
// Changed:     27 Jul 95   Michael Pichler
//
//
//</file>



#include "polyhed.h"
#include "memleak.h"

#include "srcanch.h"
#include "material.h"
#include "slist.h"
#include "vecutil.h"
#include "strarray.h"
#include "sdfscene.h"

#include "ge3d.h"

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <math.h>

#pragma warning(disable:4305)

// MAXFLOAT ((float)3.4e38) already defined in vecutil.h

// #ifdef SGI  /* to get rid of redefinition warnings in <values.h> */
// # undef M_LN2
// # undef M_PI
// # undef M_SQRT2
// #endif


#define FILE_SENTINEL '@'
// marker between logical file parts



// *** class MaterialGroup ***

class MaterialGroup
{
public:
	MaterialGroup()
	{
		material_ = 0;
		first_ = count_ = 0;
	}

	// NO delete material_, because materials are shared (managed by Scene)

	const Material* material() const
	{
		return material_;
	}

	int first() const
	{
		return first_;
	}

	int count() const
	{
		return count_;
	}

	void print() const;

private:
	Material* material_;
	int first_,                         // index into facelist_ for first face
		count_;                         // number of faces of this group
	// i. e. material_ is valid for faces face [first_] to face_ [first_+count_-1]

	friend int Polyhedron::readObjFile(FILE*, SDFScene*);
};


void MaterialGroup::print() const
{
	cout << "Material '" << material_->name()
		<< "' for " << count_ << " faces from " << first_ << "th." << endl;
}



// *** class FaceGroup ***
// each face can logically belong to several groups
// this class manages a group of faces which belong to the
// same set of groups (separated by blanks)
// function isInGroup tells whether a face group name is con-
// tained in this FaceGroup (e.g. "chair" isInGroup "default chair")
// be aware that other faces may also belong to some groups
// contained in this set of groups

class FaceGroup : public StringArray
{
public:
	FaceGroup(const char* gname)
		: StringArray(gname)
	{
		first_ = count_ = 0;
	}

	int first() const
	{
		return first_;
	}

	int count() const
	{
		return count_;
	}

	// members item and contains derived from StringArray

	void print() const;

private:
	int first_,                         // index into facelist_ for first face
		count_;                         // number of faces of this group
	// i. e. faces face [first_] to face_ [first_+count_-1] belong to this groups

	friend int Polyhedron::readObjFile(FILE*, SDFScene*);
};


void FaceGroup::print() const
{
	cout << "Face group(s) for " << count_ << " faces from "
		<< first_ << "th: " << *this << endl;
}



// *** implementation of class Polyhedron ***


Polyhedron::Polyhedron(int obj_n, int par, const char* name, const Polyhedron* copy)
	: GeometricObject(obj_n, par, name)
{
	copy_ = copy;
	num_verts_ = num_normals_ = num_texverts_ = num_faces_ = 0;
	vertexlist_ = 0;
	normallist_ = 0;
	texvertlist_ = 0;
	facelist_ = 0;
	facepickable_ = 0;
	faceselected_ = 0;
	matgrouplist_ = 0;
	facegrouplist_ = 0;
}



Polyhedron::~Polyhedron()
{
	delete[] facepickable_;  // not shared
	delete[] faceselected_;  // not shared

	if (copy_)  // slave
		return;   // all shared data deleted by master

	delete[] vertexlist_;
	delete[] normallist_;
	delete[] texvertlist_;

	int nf = num_faces_;
	face* faceptr = facelist_;
	while (nf--)
	{
		// cerr << "delete (" << faceptr->facevert << ")" << endl;
		delete[] faceptr->facevert;
		delete[] faceptr->facetexvert;
		delete[] faceptr->facenormal;
		faceptr++;
	}
	delete[] facelist_;


	// delete material groups in material list
	if (matgrouplist_)
		for (MaterialGroup* matgrptr = (MaterialGroup*)matgrouplist_->first();  matgrptr;
			matgrptr = (MaterialGroup*)matgrouplist_->next())
	{
		delete matgrptr;
	}

	delete matgrouplist_;  // clear list

	// delete face groups in face group list
	if (facegrouplist_)
		for (FaceGroup* fgrptr = (FaceGroup*)facegrouplist_->first();  fgrptr;
			fgrptr = (FaceGroup*)facegrouplist_->next())
	{
		delete fgrptr;
	}

	delete facegrouplist_;  // clear list
}


// buffer sizes for vertices, normals, and materials
#define VERTBUFSIZE 1024
#define FACEBUFSIZE 1024

// maximal no. of vertices per face
// note: graphics hardware may set a lower limit (e.g. 255)!
#define MAXVERTPERFACE 1024


// this implementation would cry for a template!
// ... or at least a class with preprocessor macros!

typedef struct
{
	int nvertsinbuffer;  // 0..VERTBUFSIZE-1
	point3D vertexdat[VERTBUFSIZE];
} vertexbuffer;


typedef struct
{
	int nnormalsinbuffer;  // 0..VERTBUFSIZE-1
	vector3D normaldat[VERTBUFSIZE];
} normalbuffer;


typedef struct
{
	int ntexvertsinbuffer;  // 0..VERTBUFSIZE-1
	point2D texvertdat[VERTBUFSIZE];
} texvertbuffer;


typedef struct
{
	int nfacesinbuffer;  // 0..FACEBUFSIZE-1
	face facedat[FACEBUFSIZE];
} facebuffer;



// readobjfile:
// reads in the object data, i. e. vertex-, normal-, texvert- and facelist_ for a polygon
// and computes the bounding box minbound_oc - maxbound_oc (object coordinates)
// returns 0 if no error
// errorcode otherwise (>= 20)
// when the polyhedron is a copy of another one, the data of the master copy_
// are copied, and nothing is read from file

int Polyhedron::readObjFile(FILE* file, SDFScene* scene)
{
	/*
	  if (copy_)
		cerr << "going to copy polyhedron data from " << copy_->name () << " to " << name () << endl;
	  else
		cerr << "going to read polyhedron '" << name () << "'." << endl;
	*/

	if (copy_)  // copy data (pointers) from copy_ to this
	{
		num_verts_ = copy_->num_verts_;
		vertexlist_ = copy_->vertexlist_;
		num_normals_ = copy_->num_normals_;
		normallist_ = copy_->normallist_;
		num_texverts_ = copy_->num_texverts_;
		texvertlist_ = copy_->texvertlist_;
		num_faces_ = copy_->num_faces_;
		facelist_ = copy_->facelist_;
		// facepickable_ and faceselected_ not shared!
		matgrouplist_ = copy_->matgrouplist_;
		facegrouplist_ = copy_->facegrouplist_;

		minbound_oc_ = copy_->minbound_oc_;
		maxbound_oc_ = copy_->maxbound_oc_;

		return 0;
	}

	slist tmpvertlist,     // temporary vertexlist   (vertexbuffer*)
		tmpnormallist,   // temporary normallist   (normalbuffer*)
		tmptexvertlist,  // temporary texvertlist  (texvertbuffer*)
		tmpfacelist;     // temporary facelist     (facebuffer*)

	matgrouplist_ = New slist;
	facegrouplist_ = New slist;

	// provide initial buffers  // (of course, also cleaned up when nothing read)
	tmpvertlist.put((void*)New vertexbuffer);
	tmpnormallist.put((void*)New normalbuffer);
	tmptexvertlist.put((void*)New texvertbuffer);
	tmpfacelist.put((void*)New facebuffer);
	// material and face groups are created on reading

	// number of vertices/normals/faces in current buffer
	int nvertsinlastbuf = 0,
		nnormalsinlastbuf = 0,
		ntexvertsinlastbuf = 0,
		nfacesinlastbuf = 0;

	// total number of vertices/normals/faces read up to now
	int nvertsinbuf = 0,
		nnormalsinbuf = 0,
		ntexvertsinbuf = 0,
		nfacesinbuf = 0;

	((vertexbuffer*)tmpvertlist.first())->nvertsinbuffer = 0;
	((normalbuffer*)tmpnormallist.first())->nnormalsinbuffer = 0;
	((texvertbuffer*)tmptexvertlist.first())->ntexvertsinbuffer = 0;
	((facebuffer*)tmpfacelist.first())->nfacesinbuffer = 0;

	int tmpfacevert[MAXVERTPERFACE],  // vertex indices of a face
		tmpfacetexvert[MAXVERTPERFACE],  // texture vertex indices of a face
		tmpfacenormal[MAXVERTPERFACE];  // normal indices of a face

	int tmpnumfaceverts,    // number of vertices read for a face
		tmpnumfacetexvs,    // number of texture vertices read for a face
		tmpnumfacenormals;  // number of normals read for a face

	face* ptrfacedat;

	int i;
	int c;  // char or EOF
	point3D avertex;
	vector3D anormal;
	point2D atexvert;
	char materialname[64];    *materialname = '\0';

	init3D(minbound_oc_, MAXFLOAT, MAXFLOAT, MAXFLOAT);
	init3D(maxbound_oc_, -MAXFLOAT, -MAXFLOAT, -MAXFLOAT);


	/* read in each line of the object file */
	int lineno = 0;

	while ((c = fgetc(file)) != EOF && c != FILE_SENTINEL)  // until EOF or next file part
	{
		lineno++;

		switch ((char)c)
		{
			// mtllib LIBRARYNAME is ignored; assuming that all relevant materials have been read in

		case 'u':
			if (fscanf(file, "semtl %s\n", materialname) == 1)  // usemtl
			{
				Material* materialptr = scene->findMaterial(materialname);

				if (materialptr)
				{
					// make a materialgroup and put it to the matgrouplist
					MaterialGroup* matgroup = New MaterialGroup;
					matgroup->material_ = materialptr;
					matgroup->first_ = nfacesinbuf;  // no. of faces read up to now
					// count_ is set after the loop (don't know now when group changes)

					matgrouplist_->put((void*)matgroup);
				}
				else
					cerr << "hg3d. error on reading polyhedron '" << name() << "': material "
					<< materialname << " not found." << endl;
			}
			break;  // u

		case 'g':
			switch (fgetc(file))
			{
			case ' ':   // face group
			case '\t':  // ungrouping (only g in one line) ignored (use dummy group)
			{ char fgroupname[256];
			fgets(fgroupname, 256, file);
			// make a face group and put it to the facegrouplist
			FaceGroup* fgroup = new FaceGroup(fgroupname);
			fgroup->first_ = nfacesinbuf;  // no. of faces read up to now
			// count_ is set after the loop (don't know now when group changes)

			facegrouplist_->put((void*)fgroup);
			}
			break;
			}
			break;  // g

		case 'v':
			switch (fgetc(file))
			{
			case ' ':
			case '\t':  // vertex
				if (nvertsinlastbuf == VERTBUFSIZE)
				{
					((vertexbuffer*)tmpvertlist.last())->nvertsinbuffer =
						VERTBUFSIZE;
					tmpvertlist.put((void*)New vertexbuffer);  // next buffer
					nvertsinlastbuf = 0;
				}
				if (fscanf(file, "%g %g %g\n", &avertex.x, &avertex.y, &avertex.z) == 3)
				{ // found a vertex!
					((vertexbuffer*)tmpvertlist.last())->vertexdat[nvertsinlastbuf] = avertex;
					nvertsinlastbuf++;
					nvertsinbuf++;
					// update bounding box minbound_oc_ - maxbound_oc_
					float x = avertex.x, y = avertex.y, z = avertex.z;
					if (x < minbound_oc_.x)  minbound_oc_.x = x;
					if (y < minbound_oc_.y)  minbound_oc_.y = y;
					if (z < minbound_oc_.z)  minbound_oc_.z = z;
					if (x > maxbound_oc_.x)  maxbound_oc_.x = x;
					if (y > maxbound_oc_.y)  maxbound_oc_.y = y;
					if (z > maxbound_oc_.z)  maxbound_oc_.z = z;
				}
				break;

			case 'n':  // vertexnormal
				switch (fgetc(file))
				{
				case ' ':
				case '\t':

					if (nnormalsinlastbuf == VERTBUFSIZE)
					{
						((normalbuffer*)tmpnormallist.last())->nnormalsinbuffer =
							VERTBUFSIZE;
						tmpnormallist.put((void*)New normalbuffer);  // next buffer
						nnormalsinlastbuf = 0;
					}

					if (fscanf(file, "%g %g %g\n", &anormal.x, &anormal.y, &anormal.z) == 3)
					{
						((normalbuffer*)tmpnormallist.last())->normaldat[nnormalsinlastbuf] = anormal;
						nnormalsinlastbuf++;
						nnormalsinbuf++;
					}

					break;
				}  // switch
				break;  // case vn

			case 't':  // texturevertex
				switch (fgetc(file))
				{
				case ' ':
				case '\t':

					if (ntexvertsinlastbuf == VERTBUFSIZE)
					{
						((texvertbuffer*)tmptexvertlist.last())->ntexvertsinbuffer =
							VERTBUFSIZE;
						tmptexvertlist.put((void*)New texvertbuffer);  // next buffer
						ntexvertsinlastbuf = 0;
					}

					if (fscanf(file, "%g %g\n", &atexvert.x, &atexvert.y) == 2)
					{
						((texvertbuffer*)tmptexvertlist.last())->texvertdat[ntexvertsinlastbuf] = atexvert;
						ntexvertsinlastbuf++;
						ntexvertsinbuf++;
					}

					break;
				}  // switch
				break;  // case vt

			} // switch v?
			break;  // v

		case 'f':
			switch (fgetc(file))
			{
			case ' ':
			case '\t':  // face

				if (nfacesinlastbuf == FACEBUFSIZE)
				{
					((facebuffer*)tmpfacelist.last())->nfacesinbuffer =
						FACEBUFSIZE;
					tmpfacelist.put((void*)New facebuffer);  // next buffer
					nfacesinlastbuf = 0;
				}
				tmpnumfaceverts = 0;
				tmpnumfacetexvs = 0;
				tmpnumfacenormals = 0;
				int tmpfacevertindex;

				// read the vertex [and texture] [and normal] indices of this face
				// possible: V or V/T or V//N or V/T/N (texture vertex indices ignored)
				while (tmpnumfaceverts < MAXVERTPERFACE &&
					fscanf(file, "%d", &tmpfacevertindex) == 1)
				{
					// range check, allowed: 1..n and -n..-1 (for n read vertices)
					// internal indices run from 0 (mapped to 0..n-1)
					if (tmpfacevertindex > 0 && tmpfacevertindex <= nvertsinbuf)
						tmpfacevertindex--;
					else if (tmpfacevertindex < 0 && tmpfacevertindex >= -nvertsinbuf)
						tmpfacevertindex += nvertsinbuf;
					else
					{
						cerr << "error in face " << nfacesinbuf + 1 << " of " << name()
							<< ": vertex index " << tmpfacevertindex << " out of range." << endl;
						tmpfacevertindex = 0;
					}
					tmpfacevert[tmpnumfaceverts++] = tmpfacevertindex;

					int nextchar = fgetc(file);
					if (nextchar == '/')                // texture and/or normal index
					{
						if (fscanf(file, "%d", &tmpfacevertindex) == 1)   // texture vertex index
						{ // range checking
						  // cerr << "got face texture index " << tmpfacevertindex << endl;
							if (tmpfacevertindex > 0 && tmpfacevertindex >= -ntexvertsinbuf)
								tmpfacevertindex--;
							else if (tmpfacevertindex < 0 && tmpfacevertindex >= -ntexvertsinbuf)
								tmpfacevertindex += ntexvertsinbuf;
							else
							{
								cerr << "error in face " << nfacesinbuf + 1 << " of " << name()
									<< ": texturevert index " << tmpfacevertindex << " out of range." << endl;
								tmpfacevertindex = 0;
							}
							tmpfacetexvert[tmpnumfacetexvs++] = tmpfacevertindex;
						}

						nextchar = fgetc(file);
						if (nextchar == '/')              // normal index expected
						{
							if (fscanf(file, "%d", &tmpfacevertindex) == 1)
							{ // range checking (like for vertices above)
							  // cerr << "got face normal index " << tmpfacevertindex << endl;
								if (tmpfacevertindex > 0 && tmpfacevertindex <= nnormalsinbuf)
									tmpfacevertindex--;
								else if (tmpfacevertindex < 0 && tmpfacevertindex >= -nnormalsinbuf)
									tmpfacevertindex += nnormalsinbuf;
								else
								{
									cerr << "error in face " << nfacesinbuf + 1 << " of " << name()
										<< ": normal index " << tmpfacevertindex << " out of range" << endl;
									tmpfacevertindex = 0;
								}
								tmpfacenormal[tmpnumfacenormals++] = tmpfacevertindex;
							}
						}
						else  ungetc(nextchar, file);
					}
					else  ungetc(nextchar, file);
				} // while vertex index found

				ptrfacedat = ((facebuffer*)tmpfacelist.last())->facedat +
					nfacesinlastbuf;
				ptrfacedat->num_faceverts = tmpnumfaceverts;
				ptrfacedat->num_facetexverts = tmpnumfacetexvs;
				ptrfacedat->num_facenormals = tmpnumfacenormals;
				ptrfacedat->facevert = New int[tmpnumfaceverts];
				// cerr << "new int [" << tmpnumfaceverts << "] (" << ptrfacedat->facevert << ")" << endl;
				ptrfacedat->facetexvert =
					tmpnumfacetexvs ? New int[tmpnumfacetexvs] : 0;
				ptrfacedat->facenormal =
					tmpnumfacenormals ? New int[tmpnumfacenormals] : 0;

				int *ptrdst,
					*ptrsrc;

				// copy data into face index arrays
				ptrdst = ptrfacedat->facevert;
				ptrsrc = tmpfacevert;
				for (i = 0; i < tmpnumfaceverts; i++)  // vertex indices
					*ptrdst++ = *ptrsrc++;
				ptrdst = ptrfacedat->facetexvert;
				ptrsrc = tmpfacetexvert;
				for (i = 0; i < tmpnumfacetexvs; i++)  // texture vertex indices
					*ptrdst++ = *ptrsrc++;
				ptrdst = ptrfacedat->facenormal;
				ptrsrc = tmpfacenormal;
				for (i = 0; i < tmpnumfacenormals; i++)  // normal indices
					*ptrdst++ = *ptrsrc++;

				nfacesinlastbuf++;
				nfacesinbuf++;
				break;  // face
			}  // f
			break;

		case '\n':  // empty line
			break;

		default: // ignore [rest of] line
			while ((c = fgetc(file)) != EOF && (char)c != '\n');
			break;
		} // switch (c)

	} // while (!EOF && c != FILE_SENTINEL)


	if (c == FILE_SENTINEL)
		fgetc(file);  // eat '\n'


	((vertexbuffer*)tmpvertlist.last())->nvertsinbuffer =
		nvertsinlastbuf;
	((normalbuffer*)tmpnormallist.last())->nnormalsinbuffer =
		nnormalsinlastbuf;
	((texvertbuffer*)tmptexvertlist.last())->ntexvertsinbuffer =
		ntexvertsinlastbuf;
	((facebuffer*)tmpfacelist.last())->nfacesinbuffer =
		nfacesinlastbuf;

	// copy vertexlist, normallist, texvertlist, facelist; set num_*

	vertexlist_ = New point3D[nvertsinbuf];
	normallist_ = New vector3D[nnormalsinbuf];
	texvertlist_ = New point2D[ntexvertsinbuf];
	facelist_ = New face[nfacesinbuf];

	// copy the vertices

	vertexbuffer* vbufptr;
	point3D *vptrsrc,
		*vptrdst;
	vptrdst = vertexlist_;
	for (vbufptr = (vertexbuffer*)tmpvertlist.first();  vbufptr;
		vbufptr = (vertexbuffer*)tmpvertlist.next())
		for (vptrsrc = vbufptr->vertexdat, i = 0;  i < vbufptr->nvertsinbuffer;
			i++)
			*vptrdst++ = *vptrsrc++;

	// copy the normals

	normalbuffer* vnbufptr;
	vector3D *vnptrsrc,
		*vnptrdst;
	vnptrdst = normallist_;
	for (vnbufptr = (normalbuffer*)tmpnormallist.first();  vnbufptr;
		vnbufptr = (normalbuffer*)tmpnormallist.next())
		for (vnptrsrc = vnbufptr->normaldat, i = 0;  i < vnbufptr->nnormalsinbuffer;
			i++)
			*vnptrdst++ = *vnptrsrc++;

	// copy the texverts

	texvertbuffer* vtbufptr;
	point2D *vtptrsrc,
		*vtptrdst;
	vtptrdst = texvertlist_;
	for (vtbufptr = (texvertbuffer*)tmptexvertlist.first();  vtbufptr;
		vtbufptr = (texvertbuffer*)tmptexvertlist.next())
		for (vtptrsrc = vtbufptr->texvertdat, i = 0;  i < vtbufptr->ntexvertsinbuffer;
			i++)
			*vtptrdst++ = *vtptrsrc++;

	// copy the faces

	facebuffer* fbufptr;
	face *fptrsrc,
		*fptrdst;
	fptrdst = facelist_;
	for (fbufptr = (facebuffer*)tmpfacelist.first();  fbufptr;
		fbufptr = (facebuffer*)tmpfacelist.next())
		for (fptrsrc = fbufptr->facedat, i = 0;  i < fbufptr->nfacesinbuffer;
			i++)
			*fptrdst++ = *fptrsrc++;

	// set count_ for the material groups

	MaterialGroup* matgroupptr = (MaterialGroup*)matgrouplist_->first();
	if (!matgroupptr)
		cerr << "warning: polyhedron '" << name() << "' has no defined material (color)." << endl;
	else if (matgroupptr->first_ > 0)
		cerr << "warning: first " << matgroupptr->first_ << " faces of polyhedron " << name()
		<< " have no defined material (color)." << endl;

	// matgroupptr = matgrouplist_->first ()
	while (matgroupptr)
	{
		MaterialGroup* nextmatgroupptr = (MaterialGroup*)matgrouplist_->next();

		int next = (nextmatgroupptr) ? nextmatgroupptr->first_ : nfacesinbuf;
		matgroupptr->count_ = next - matgroupptr->first_;

		// cerr << "# Obj. " << name () << ": mat. group for " << matgroupptr->count_ << " faces from no. "
		//      << matgroupptr->first_ << " (to no. " << matgroupptr->first_+matgroupptr->count_-1 << ")." << endl;

		matgroupptr = nextmatgroupptr;
	} // matgroupptr = (MaterialGroup*) matgrouplist_->next ()

	// set count_ for the face groups

	FaceGroup* facegroupptr = (FaceGroup*)facegrouplist_->first();
	while (facegroupptr)
	{
		FaceGroup* nextfacegroup = (FaceGroup*)facegrouplist_->next();

		int next = (nextfacegroup) ? nextfacegroup->first_ : nfacesinbuf;
		facegroupptr->count_ = next - facegroupptr->first_;

		// cerr << "# Obj. " << name () << ": face group for " << facegroupptr->count_ << " faces from no. "
		//      << facegroupptr->first_ << " (to no. " << facegroupptr->first_ + facegroupptr->count_ - 1 << "): ";
		// facegroupptr->printItems (cerr);
		// cerr << endl;

		facegroupptr = nextfacegroup;
	} // for all face groups

	// set number of vertices, normals, faces

	num_verts_ = nvertsinbuf;
	num_normals_ = nnormalsinbuf;
	num_texverts_ = ntexvertsinbuf;
	num_faces_ = nfacesinbuf;

	// free the store of temporary lists

	for (vbufptr = (vertexbuffer*)tmpvertlist.first();  vbufptr;
		vbufptr = (vertexbuffer*)tmpvertlist.next())
		delete vbufptr;
	for (vnbufptr = (normalbuffer*)tmpnormallist.first();  vnbufptr;
		vnbufptr = (normalbuffer*)tmpnormallist.next())
		delete vnbufptr;
	for (vtbufptr = (texvertbuffer*)tmptexvertlist.first();  vtbufptr;
		vtbufptr = (texvertbuffer*)tmptexvertlist.next())
		delete vtbufptr;
	for (fbufptr = (facebuffer*)tmpfacelist.first();  fbufptr;
		fbufptr = (facebuffer*)tmpfacelist.next())
		delete fbufptr;

	return 0;

} // Polyhedron::readobjfile



void Polyhedron::print(int all)
{
	cout << "Polyhedron. ";
	printobj();

	cout << "  "
		<< num_verts_ << " vertices, "
		<< num_normals_ << " normals, "
		<< num_faces_ << " faces." << endl;

	cout << endl;

	if (!all)
		return;

	int i, j;

	// vertexlist
	for (i = 0; i < num_verts_; i++)
		cout << "Vertex " << i << ": " << vertexlist_[i] << endl;
	cout << endl;

	// normallist
	for (i = 0; i < num_normals_; i++)
		cout << "Normal " << i << ": " << normallist_[i] << endl;
	cout << endl;

	// texturevertexlist
	for (i = 0; i < num_texverts_; i++)
		cout << "TextureVertex " << i << ": " << texvertlist_[i] << endl;
	cout << endl;

	// material groups
	if (matgrouplist_)
		for (MaterialGroup* matgroupptr = (MaterialGroup*)matgrouplist_->first();  matgroupptr;
			matgroupptr = (MaterialGroup*)matgrouplist_->next())
			matgroupptr->print();
	cout << endl;

	// face groups
	if (facegrouplist_)
		for (FaceGroup* fgroupptr = (FaceGroup*)facegrouplist_->first();  fgroupptr;
			fgroupptr = (FaceGroup*)facegrouplist_->next())
			fgroupptr->print();
	cout << endl;

	// facelist
	face* face_i;
	for (i = 0, face_i = facelist_; i < num_faces_; i++, face_i++)
	{
		cout << "Face " << i << ": "
			<< face_i->num_faceverts << " vertices & "
			<< face_i->num_facenormals << " normals + "
			<< face_i->num_facetexverts << " textureverts:";

		for (j = 0; j < face_i->num_faceverts; j++)
			cout << ' ' << face_i->facevert[j];

		cout << " &";
		for (j = 0; j < face_i->num_facenormals; j++)
			cout << ' ' << face_i->facenormal[j];

		cout << " +";
		for (j = 0; j < face_i->num_facetexverts; j++)
			cout << ' ' << face_i->facetexvert[j];

		cout << endl;
	}

	cout << "*** end of polyhedron '" << name() << "' (# " << obj_num_ << ") ***" << endl << endl;

} // print


void Polyhedron::writeData(ostream& os) const
{
	// caller may test whether this object has a master
	// to avoid multiple output of same data
	if (!os)
		return;

	os << "# " << name() << '\n';

	// vertexlist
	os << "g\n";  // anonymous vertices
	int n = num_verts_;
	const point3D* vertex = vertexlist_;
	while (n--)
	{
		os << "v " << vertex->x << ' ' << vertex->y << ' ' << vertex->z << '\n';
		vertex++;
	}
	os << "# " << num_verts_ << " vertices" << endl;

	// texturevertexlist
	n = num_texverts_;
	const point2D* texvert = texvertlist_;
	while (n--)
	{
		os << "vt " << texvert->x << ' ' << texvert->y << '\n';
		texvert++;
	}
	os << "# " << num_texverts_ << " texture vertices" << endl;

	// normallist
	n = num_normals_;
	const vector3D* normal = normallist_;
	while (n--)
	{
		os << "vn " << normal->x << ' ' << normal->y << ' ' << normal->z << '\n';
		normal++;
	}
	os << "# " << num_normals_ << " normals" << endl;

	const MaterialGroup* curmatgroup = matgrouplist_ ? (MaterialGroup*)matgrouplist_->first() : nil;
	const FaceGroup* curfacegroup = facegrouplist_ ? (FaceGroup*)facegrouplist_->first() : nil;

	// facelist
	n = num_faces_;
	const face* aface = facelist_;
	for (int i = 0; i < n; i++)
	{
		// see whether material or face group changes
		if (curmatgroup && i == curmatgroup->first())
		{
			os << "usemtl " << curmatgroup->material()->name() << '\n';
			curmatgroup = (MaterialGroup*)matgrouplist_->next();
		}
		if (curfacegroup && i == curfacegroup->first())
		{
			os << "g " << *curfacegroup << '\n';
			curfacegroup = (FaceGroup*)facegrouplist_->next();
		}

		os << 'f';
		int nverts = aface->num_faceverts;
		int ntexverts = aface->num_facetexverts;
		int nnormals = aface->num_facenormals;

		if (ntexverts != nverts)
			ntexverts = 0;
		if (nnormals != nverts)
			nnormals = 0;

		// formats: v, v//n, v/t, v/t/n
		for (int j = 0; j < nverts; j++)
		{
			os << ' ' << aface->facevert[j] + 1;
			if (ntexverts)
				os << '/' << aface->facetexvert[j] + 1;
			if (nnormals)
				os << '/' << aface->facenormal[j] + 1;
		}

		os << '\n';
		aface++;
	}
	os << "# " << num_faces_ << " elements" << endl;

} // writeData



// writeVRML
// output polyhedron data in VRML format

void Polyhedron::writeVRML(ostream& os) const
{
	if (copy_)  // share geometry information
	{
		os << "USE Obj_" << copy_->getobj_num() << endl;
		return;
	}

	os << "# " << name() << endl;
	os << "DEF Obj_" << getobj_num() << " Group {" << endl;

	// Coordinate3
	os << "Coordinate3 { point [\n";

	int n = num_verts_;
	const point3D* vertex = vertexlist_;
	while (n--)
	{
		os << '\t';
		os << vertex->x << ' ' << vertex->y << ' ' << vertex->z << (n ? ',' : ']') << '\n';
		vertex++;
	}
	os << "} # " << num_verts_ << " vertices" << endl;  // Coordinate3

	// Normal (TODO)

	// TextureCoordinate2 (TODO)

	// IndexedFaceSet (one per material group; face group information gets lost)

	MaterialGroup* matgrptr = (MaterialGroup*)matgrouplist_->first();

	// writeFacesVRML (os, facelist_, num_faces_, 0);

	if (!matgrptr)  // no materials defined
		writeFacesVRML(os, facelist_, num_faces_, 0);
	else if (matgrptr->first() > 0)  // no material for some faces
		writeFacesVRML(os, facelist_, matgrptr->first(), 0);

	for (; matgrptr; matgrptr = (MaterialGroup*)matgrouplist_->next())
		writeFacesVRML(os, facelist_ + matgrptr->first(), matgrptr->count(), matgrptr->material());

	os << "} # Obj_" << getobj_num() << ": " << num_faces_ << " elements" << endl;  // geometry group

} // writeVRML



// writeFacesVRML
// output face group in VRML format

void Polyhedron::writeFacesVRML(ostream& os, const face* aface, int n, const Material* mtl) const
{
	if (!n)
		return;

	if (mtl)
		os << "USE Mat_" << mtl->name() << endl;

	os << "IndexedFaceSet { coordIndex [\n";

	while (n--)
	{
		int nverts = aface->num_faceverts;
		os << '\t';

		for (int j = 0; j < nverts; j++)
			os << aface->facevert[j] << ',';

		os << "-1" << (n ? ',' : ']') << '\n';
		aface++;
	}
	os << "}\n";  // IndexedFaceSet

} // writeFacesVRML



void Polyhedron::setupNormals()
{
	// compute normalized outward normal of each face

	int i;
	face* fptr;

	for (fptr = facelist_, i = 0; i < num_faces_; i++, fptr++)
	{
		if (fptr->num_faceverts > 2)
		{
			vector3D& n = fptr->normal;         // the normal to be computed
			const int* v_ptr = fptr->facevert;  // from first three vertices

			::computeNormal(vertexlist_[*v_ptr], vertexlist_[v_ptr[1]], vertexlist_[v_ptr[2]], n);
			// never, *never*, NEVER, *NEVER* do a call like foo (ptr, ptr++, ptr++)!
			// cerr << "normal vector of face " << i << ": " << n;

			float norm2 = dot3D(n, n);
			if (norm2 > 0.0)  // normalize
			{
				norm2 = 1 / sqrt(norm2);
				scl3D(n, norm2);
			}
			// cerr << "; normalized: " << n << endl;;

		}
		else
			init3D(fptr->normal, 0, 0, 0);

	} // for all faces

} // setupnormals



void Polyhedron::groupAnchorsChanged()
{
	// on changing group anchors facepickable flags have to be recalculated
	delete[] facepickable_;
	facepickable_ = 0;
}



void Polyhedron::groupSelectionChanged()
{
	// on changing group selections faceselected flags have to be recalculated
	delete[] faceselected_;
	faceselected_ = 0;
}



// definePickableFaces
// define array of flags facepickable_
// flag is set if face belongs to an anchor group
// facepickable_ is allocated with new (should be nil or deleted first)
// facegrouplist_ is assumed to exist (otherwise cannot highlight groups)

void Polyhedron::definePickableFaces()
{
	//cerr << "set up new facepickable flags" << endl;

	facepickable_ = new int[num_faces_];

	int* fp = facepickable_;
	int i = num_faces_;
	while (i--)
		*fp++ = 0;

	// scan through group anchor list
	for (SourceAnchor* anch = (SourceAnchor*)anchorlist_.first();  anch;
		anch = (SourceAnchor*)anchorlist_.next())
	{
		const char* groupname = anch->groupName();  // name of group anchor

		// all face groups that contain a group anchor must be highlighted
		if (groupname)
		{
			for (FaceGroup* fgrptr = (FaceGroup*)facegrouplist_->first();  fgrptr;
				fgrptr = (FaceGroup*)facegrouplist_->next())
				if (fgrptr->contains(groupname))
				{
					fp = facepickable_ + fgrptr->first();
					i = fgrptr->count();
					// cerr << "highlighting " << i << " faces for group anchor " << ganame << endl;
					while (i--)
						*fp++ = 1;  // could use increment to show overlapping anchors
				}
		} // group anchor
	} // for all anchors

} // definePickableFaces



// defineSelectedFaces
// define array of flags faceselected_
// flag is set if face belongs to the selected group
// faceselected_ is allocated with new (should be nil or deleted first)
// facegrouplist_ is assumed to exist (otherwise cannot highlight groups)

void Polyhedron::defineSelectedFaces()
{
	faceselected_ = new int[num_faces_];

	int* fs = faceselected_;
	int i = num_faces_;
	while (i--)
		*fs++ = 0;

	const char* sgname = selectedGroup();  // name of selected group

	if (!sgname)
	{
		cerr << "hg3d. warning (internal): useless call of defineSelectedFaces (no selected group)" << endl;
		return;
	}

	for (FaceGroup* fgrptr = (FaceGroup*)facegrouplist_->first();  fgrptr;
		fgrptr = (FaceGroup*)facegrouplist_->next())
		if (fgrptr->contains(sgname))
		{
			fs = faceselected_ + fgrptr->first();
			i = fgrptr->count();
			while (i--)
				*fs++ = 1;
		}

} // defineSelectedFaces



void Polyhedron::draw_(int hilitindex, int texturing,
	const colorRGB* col_anchorface, const colorRGB* col_anchoredge
)
{
	// default color
	ge3d_setfillcolor(0.5, 0.5, 0.5);

	MaterialGroup* matgrptr = (MaterialGroup*)matgrouplist_->first();

	if (!matgrptr)  // no materials defined -- draw whole polyhedron in default color
		ge3dPolyhedron(vertexlist_, normallist_, texvertlist_, num_faces_, facelist_);
	else if (matgrptr->first() > 0)  // no material defined for some faces
		ge3dPolyhedron(vertexlist_, normallist_, texvertlist_, matgrptr->first(), facelist_);

	int hilitgroupanchors = hilitindex && hasgroupanchors_ && !hasobjectanchors_;
	const char* selectedgroup = selectedGroup();

	if ((hilitgroupanchors || selectedgroup) && facegrouplist_)
	{
		// different highlighting of single faces (group anchors or selection)

		if (hilitgroupanchors && !facepickable_)  // set up array of pickable flags once
			definePickableFaces();

		if (selectedgroup && !faceselected_)  // set up array of group selection flags once
			defineSelectedFaces();

		// one of hilitgroupanchors and selectedgroup - or both - are set

		// go through material groups, but with respect to facepickable/faceselected
		// matgrptr = matgrptr->first ()
		for (; matgrptr; matgrptr = (MaterialGroup*)matgrouplist_->next())
		{
			const Material* mat = matgrptr->material();
			int matcount = matgrptr->count();

			if (mat)
			{
				int i = matgrptr->first();
				face* faceptr = facelist_ + i;
				int* fpick = hilitgroupanchors ? (facepickable_ + i) : 0;
				int* fsel = selectedgroup ? (faceselected_ + i) : 0;

				int anchhilit;  // currently doing anchor highlighting
				int selhilit;   // currently doing higlhighting of a selection

				i = 0;  // no. of faces drawn
				while (i < matcount)
				{
					anchhilit = hilitgroupanchors && *fpick;
					mat->setge3d(hilitindex, texturing, hasobjectanchors_ || anchhilit, col_anchorface, col_anchoredge);
					selhilit = selectedgroup && *fsel;
					if (selhilit)  // sorry, cannot draw twocolored polygon edges
					{
						const short l_pat = (short)0xF00F;  // 16 bit
						ge3d_setlinestyle(l_pat);
						ge3dLineColor(col_anchoredge);
					}
					else
						ge3d_setlinestyle(-1);

					int j = 1;  // no. of faces with same hilit
					int jmax = matcount - i;

					while (j < jmax)
					{
						int ok = 1;
						if (hilitgroupanchors)
							ok = (*++fpick == anchhilit);
						if (selectedgroup)
							ok &= (*++fsel == selhilit);
						if (!ok)  // ensure pointers fpick and fsel are running synchron
							break;
						j++;
					}

					ge3dPolyhedron(vertexlist_, normallist_, texvertlist_, j, faceptr);

					i += j;
					faceptr += j;
					// fpick already points to next face
				}

			}
			else
				ge3dPolyhedron(vertexlist_, normallist_, texvertlist_, matcount, facelist_ + matgrptr->first());

		} // for all material groups

		ge3d_setlinestyle(-1);

	}
	else  // whole object is anchor or no part of it
	{
		// matgrptr = matgrptr->first ()
		for (; matgrptr; matgrptr = (MaterialGroup*)matgrouplist_->next())
		{
			const Material* mat = matgrptr->material();
			if (mat)
				mat->setge3d(hilitindex, texturing, hasAnchors(), col_anchorface, col_anchoredge);

			ge3dPolyhedron(vertexlist_, normallist_, texvertlist_, matgrptr->count(), facelist_ + matgrptr->first());
		}
	}

}  // draw_



// isleftof
// returns not zero if pt lies left of or on the line from p to q
// with respect to the plane given by outward normal n
// in other words: determines wheter (p, q, pt) describe a CCW-oriented triangle
// on a plane with upward normal n

inline int isleftof(const point3D& p, const point3D& q, const point3D& pt, const vector3D& n)
{
	const float eps = 1e-10;

	vector3D q_p, pt_p;
	vector3D crp1;

	sub3D(q, p, q_p);
	sub3D(pt, p, pt_p);

	crp3D(q_p, pt_p, crp1);

	return (dot3D(crp1, n) > -eps);  // >= 0

}  // isleftof



// rayhits_
// returns 1 if ray A_in + b_in.t (world coord.) hits the polyhedron at a value of t,
// tnear < t < tmin; in this case:
// - tmin is set to t,
// - *normal (if not nil) is set to the outward normal at the hitpoint (in object coord.)
// - *groups (if not nil) is set to the set of groups hit (const StringArray*)
// returns 0 otherwise
// the hit point itself calculates as A_in + tmin*b_in;

int Polyhedron::rayhits_(const point3D& A_in, const vector3D& b_in, float tnear,
	float& tmin, vector3D* normal, const StringArray** groups)
{
	float thit, denom;
	int i;
	face* fptr;
	int* v_ptr;
	point3D A;   // start and
	vector3D b;  // direction of ray in object (modelling) coordinates
	point3D *p, *q, hp;
	int ray_hits = 0;  // return value
	int face_hit = -1;

	//cerr << "rayhits called for object " << name () << endl;

	  // transform ray into modelling coordinates
	ge3d_push_new_matrix((const float(*)[4]) invtrfmat_);
	ge3dTransformMcWc(&A_in, &A);
	ge3dTransformVectorMcWc(&b_in, &b);
	ge3d_pop_matrix();

	for (fptr = facelist_, i = 0; i < num_faces_; i++, fptr++)
	{
		int num_fv = fptr->num_faceverts;
		if (num_fv < 3)
			continue;  // ignore faces with less than 3 vertices

		const vector3D& n = fptr->normal;

		if ((denom = dot3D(n, b)) < 0)  // ray enters face plane
		{
			// hit time: (<n.p>-<n.A>)/<n.b>
			p = vertexlist_ + *fptr->facevert;
			sub3D(*p, A, hp);
			thit = dot3D(n, hp) / denom;

			if (thit > tnear && thit < tmin)  // this face plane hit first
			{
				// check if hit point hp = A + thit * b  lies within face
				// (this fast algorithm only works for convex faces)

				pol3D(A, thit, b, hp);

				v_ptr = fptr->facevert;
				p = vertexlist_ + v_ptr[num_fv - 1];
				q = vertexlist_ + *v_ptr++;

				int inface = isleftof(*p, *q, hp, n);  // test edge n-1_0

				// test edges 0_1, 1_2 .. n-2_n-1

				// *v_ptr is fptr->facevert [1], q is vertex no. 0
				for (register int j = 1; inface && j < num_fv; j++)
				{
					p = q;                       // now p is vertex j-1
					q = vertexlist_ + *v_ptr++;  // and q is vertex j

					inface = isleftof(*p, *q, hp, n);  // test whether hp is "left of" line p to q
				}  // for j

				if (inface)
				{
					ray_hits = 1;
					face_hit = i;  // hit face i
					tmin = thit;
					if (normal)
						*normal = n;
					// don't exit here - might find less value of t for another face
					// cerr << '{' << obj_num << ':' << i << '}';
				}

			}  // if face plane hit first (tnear < thit < tmin)
		}  // if entering ray

	} // for all faces


	if (groups && ray_hits && facegrouplist_)  // set *groups to groups hit
	{
		*groups = 0;  // possibly there are faces that do not belong to any group

		for (FaceGroup* fgrptr = (FaceGroup*)facegrouplist_->first();  fgrptr;
			fgrptr = (FaceGroup*)facegrouplist_->next())
		{
			int first = fgrptr->first();
			int behindlast = first + fgrptr->count();

			if (first <= face_hit && face_hit < behindlast)
			{
				*groups = fgrptr;
				break;  // for
			}
		} // for all face groups
	}

	return (ray_hits);

}  // Polyhedron::rayhits
