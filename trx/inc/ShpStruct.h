#ifndef _SHPSTRUCT_H
#define _SHPSTRUCT_H

#ifndef _FLTPOINT_H
#include "FltPoint.h"
#endif

#define SHP_FILECODE 9994

#pragma pack(1)
struct SHP_MAIN_HDR {		//100 bytes
	__int32 fileCode;		//be (9994)
	__int32 unused[5];
	__int32 fileLength;		//be
	__int32 version;		//le (1000)
	__int32 shpType;		//le
	double Xmin;			//le
	double Ymin;			//le
	double Xmax;			//le
	double Ymax;			//le
	double Zmin;			//le
	double Zmax;			//le
	double Mmin;			//le
	double Mmax;			//le
};

struct  SHX_REC {
	long offset;		//word offset of corrsponding rec in SHP file (big endian)
	long reclen;		//reclen in SHP file (big endian)
};

struct SHP_POINT_REC { //28 bytes
	int recNumber; //big-endian (not counted in content length)
	int length; //content length in words, big-endian (28-4)
	int shpType;
	CFltPoint fpt;
};

struct SHP_POINTM_REC { //36 bytes
	int recNumber;
	int length; //36-4
	int shpType;
	CFltPoint fpt;
	double m;
};

struct SHP_POINTZ_REC { //44 bytes
	int recNumber;
	int length; //44-4
	int shpType;
	CFltPoint fpt;
	double z;
	double m;
};

struct  SHP_POINT_XY{
	long typ;			//SHP_POINT==1 or SHP_NULL==0
	CFltPoint fpt;
};

struct SHP_POINTZM_REC {
	int recNumber;
	int length;
	int shpType;
	CFltPoint fpt;
	CFltPoint zm;
};

//#define SHP_ZM_FILEOFF(rec) (100+(rec)*sizeof(SHP_POINTZ_REC)-sizeof(CFltPoint))

struct  SHP_POINTZ_XY{
	long typ;			//SHP_POINT==1 or SHP_NULL==0
	CFltPoint xy;
	CFltPoint zm;
};

struct SHP_POLY_REC {
	int recno;  //be
	int len; //be (content length)
	int typ;	//le
	double Xmin; //le
	double Ymin;
	double Xmax;
	double Ymax;
	int numParts; //le  (ppoly->len is record length staring at ppoly->off==&rec.numParts)
	int numPoints; //le
};
#define SHP_POLY_SIZCONTENT(ppoly_len) ((ppoly_len)+sizeof(SHP_POLY_REC)-4*sizeof(int))

struct SHP_POLY_NUMP {
	int numParts; //le  (ppoly->len is record length staring at ppoly->off==&rec.numParts)
	int numPoints; //le
};

struct SHP_POLYLINE1_REC {
	int recno;  //be
	int len; //be (content length)
	int typ;	//=SHP_POLYLINEZ
	double Xmin; //le
	double Ymin;
	double Xmax;
	double Ymax;
	int numParts;  //=1
	int numPoints; //=2
	int Parts[1];
	CFltPoint Points[2];
	double Zmin,Zmax;
	double Z[2];
	double Mmin,Mmax;
	double M[2];
};
#define SHP_POLYLINE1_SIZCONTENT (sizeof(SHP_POLYLINE1_REC)-2*sizeof(int))

#pragma pack()
#endif
