#ifndef _POLYTEST_H
#define _POLYTEST_H

#ifndef _FLTRECT_H
#include "FltRect.h"
#endif

#pragma pack(1)

struct SHP_POLY {
	int part;
	CFltPoint point[1];
};

struct SHP_PT_POLY {
	LPSTR pszName;
	int numPoints;
	SHP_POLY *poly; //==1
};

typedef std::vector<SHP_PT_POLY> V_SHP_PT_POLY;

typedef struct {
	double xmin,ymin,xmax,ymax;
} SHP_BOX;

typedef struct {
	long filecode;		//9994 (big endian)
	long unused[5];
	long filelength;	//file length in 16-bit words (big endian)
	long version;		//1000
	long shapetype;		//1-Point, 3-PolyLine, 5-Polygon, etc.
	SHP_BOX xy_box;		//extent of all (X,Y) points in file
	double zmin,zmax;	//extent of all Z
	double mmin,mmax;   //extent of all M
} SHP_MAIN_HDR;

struct SHP_POLY_HDR {
	int recno;  //be
	int length; //be
	int typ;	//le
	CFltRect extent; //le
	int numParts; //le
	int numPoints; //le
};

#pragma pack()

__inline __int32 FlipBytes(__int32 nData)
{
	__asm mov eax, [nData];
	__asm bswap eax;
	__asm mov [nData], eax; // you can take this out but leave it just to be sure
	return(nData);
}

#endif