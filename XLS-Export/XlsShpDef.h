#pragma once

#ifndef __DBFILE_H
#include <dbfile.h>
#endif

enum {LF_LAT,LF_LON,LF_EAST,LF_NORTH,LF_ZONE,LF_DATUM,LF_UPDATED,LF_CREATED,NUM_LOCFLDS};

typedef std::vector<CString> V_CSTRING;
typedef V_CSTRING::iterator IT_CSTRING;
typedef std::vector<DBF_FLDDEF> V_FLDDEF;
typedef std::vector<int> V_INT;
typedef V_INT::iterator IT_INT;
typedef std::vector<double> V_DOUBLE;

class CShpDBF;
class CShpLayer;

enum {SHPD_UTMEAST=1,SHPD_UTMNORTH=2,SHPD_UTMZONE=4,SHPD_UTMFLAGS=7};

class CShpDef
{
public:
	CShpDef(void) : numFlds(0), numSrcFlds(0), numMemoFlds(0),
	iTypXY(-1), iFldZone(-1), iFldX(-1), iFldY(-1), iFldKey(-1), iDatum(-1), uFlags(0),
	latMax(90),latMin(-90),lonMax(180),lonMin(-180),latDflt(0),latErr(0),
	lonDflt(0),lonErr(0),zoneMin(1),zoneMax(60),iZoneDflt(0) { sFldSep[0]=0;}
	~CShpDef(void);

	BOOL Process(CDaoRecordset *pRS, LPCSTR pathName);
	static int GetLocFldTyp(LPCSTR pNam);

	V_DOUBLE  v_fldScale;
	V_FLDDEF  v_fldDef;
	V_INT     v_srcIdx;  //source field number if not negative;
	V_CSTRING v_srcDflt; //default or assigned constant field value
	V_CSTRING v_srcNames; //names of fields in csv
	double latMax,lonMin,latMin,lonMax; //TL and BR coordinates of allowed region
	double latDflt,lonDflt;
	double latErr,lonErr;
	char sFldSep[4];      // single char or "|", ",", etc.
	int zoneMin,zoneMax;
	int iDatum;
	int iTypXY;          //0 if lat/lon, 1 if UTM
	int iFldX,iFldY;     //Lon,Lat or easting,northing indexes in v_fldDef[]
	int iFldZone;        //Zone field index in vFldDef (or -1 if iTypXY==0)
	int iZoneDflt;
	int iFldDatum;       //fld where datum is specified via a constant (index in v_xc[])
	int iFldKey;         //fld to list in log file to allow searching
	int numFlds;
	int numSrcFlds;      //src fields actually used (length og v_src*[])
	int numMemoFlds;
	int numDbFlds;       //total no. fields in source DB
	UINT uFlags;
};
