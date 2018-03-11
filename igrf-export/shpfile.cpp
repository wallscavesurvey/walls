// wall-shp.cpp : CSegView::ExportSHP() implementation
//

#include "stdafx.h"
#include "resource.h"
#include <trx_str.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys\stat.h>

#include "shpfile.h"

//=======================================================

//Shapefile structures --

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)),SWAPWORD(LOWORD(x)))

#define SHP_FILECODE 9994
#define SHP_TYPE_POINT 1
#define SHP_TYPE_POINTZ 11
#define SHP_TYPE_POLYLINE 3
#define SHP_TYPE_POLYLINEZ 13
#define SHP_TYPE_POLYGON 5

#ifdef _USE_LLOYD
#pragma pack(1)
typedef struct {
	char Flag;
	char Name[SHP_LEN_NAME];
	char Alt_Names[SHP_LEN_ALT_NAMES];
	char Municipio[SHP_LEN_MUNICIPIO];
	char State[SHP_LEN_STATE];
	char Elev[SHP_LEN_ELEV];
	char Topo[SHP_LEN_TOPO];
	char Length[SHP_LEN_LENGTH];
	char Depth[SHP_LEN_DEPTH];
	char Location[SHP_LEN_LOCATION];
	char Comments[SHP_LEN_COMMENTS];
	char Site_ID[SHP_LEN_SITE_ID];
} DBF_SHP_REC;
#pragma pack()

static DBF_FLDDEF n_fld[]={
	{"NAME"			,'C', SHP_LEN_NAME,0},
	{"ALT_NAMES"	,'C', SHP_LEN_ALT_NAMES,0},
	{"MUNICIPIO"	,'C', SHP_LEN_MUNICIPIO,0},
	{"STATE"		,'C', SHP_LEN_STATE,0},
	{"ELEV"			,'N', SHP_LEN_ELEV,0},
	{"TOPO"			,'C', SHP_LEN_TOPO,0},
	{"LENGTH"       ,'N', SHP_LEN_LENGTH,0},
	{"DEPTH"		,'N', SHP_LEN_DEPTH,0},
	{"LOCATION"     ,'C', SHP_LEN_LOCATION,0},
	{"COMMENTS"		,'M', SHP_LEN_COMMENTS,0},
	{"SITE_ID"		,'C', SHP_LEN_SITE_ID,0}
};
#endif

#ifdef _USE_GUA
#pragma pack(1)
typedef struct {
	char Flag;
	char Name[SHP_LEN_NAME];
	char ID[SHP_LEN_ID];
	char Alt_Names[SHP_LEN_ALT_NAMES];
	char Type[SHP_LEN_TYPE];
	char Exp_Status[SHP_LEN_EXP_STATUS];
	char Accuracy[SHP_LEN_ACCURACY];
	char Elev[SHP_LEN_ELEV];
	char Quad[SHP_LEN_QUAD];
	char Location[SHP_LEN_LOCATION];
	char Department[SHP_LEN_DEPARTMENT];
	char Desc[SHP_LEN_DESC];
	char Map_Year[SHP_LEN_MAP_YEAR];
	char Map_Status[SHP_LEN_MAP_STATUS];
	char Length[SHP_LEN_LENGTH];
	char Depth[SHP_LEN_DEPTH];
	char Map_Links[SHP_LEN_MAP_LINKS];
	char Discovery[SHP_LEN_DISCOVERY];
	char Mapping[SHP_LEN_MAPPING];
	char Project[SHP_LEN_PROJECT];
	char Owner[SHP_LEN_OWNER];
	char Gear[SHP_LEN_GEAR];
	char History[SHP_LEN_HISTORY];
	char Year_Found[SHP_LEN_YEAR_FOUND];
	char References[SHP_LEN_REFERENCES];
} DBF_SHP_REC;
#pragma pack()

static DBF_FLDDEF n_fld[]={
	{"NAME"     ,'C', SHP_LEN_NAME,0},
	{"ID"     ,'C', SHP_LEN_ID,0},
	{"ALT_NAMES"     ,'C', SHP_LEN_ALT_NAMES,0},
	{"TYPE"     ,'C', SHP_LEN_TYPE,0},
	{"EXP_STATUS",'C', SHP_LEN_EXP_STATUS,0},
	{"ACCURACY"     ,'C', SHP_LEN_ACCURACY,0},
	{"ELEV"     ,'C', SHP_LEN_ELEV,0},
	{"QUAD"     ,'C', SHP_LEN_QUAD,0},
	{"LOCATION"     ,'C', SHP_LEN_LOCATION,0},
	{"DEPARTMENT"     ,'C', SHP_LEN_DEPARTMENT,0},
	{"DESC"     ,'C', SHP_LEN_DESC,0},
	{"MAP_YEAR"     ,'C', SHP_LEN_MAP_YEAR,0},
	{"MAP_STATUS"     ,'C', SHP_LEN_MAP_STATUS,0},
	{"LENGTH"     ,'C', SHP_LEN_LENGTH,0},
	{"DEPTH"     ,'C', SHP_LEN_DEPTH,0},
	{"MAP_LINKS"     ,'C', SHP_LEN_MAP_LINKS,0},
	{"DISCOVERY"     ,'C', SHP_LEN_DISCOVERY,0},
	{"MAPPING"     ,'C', SHP_LEN_MAPPING,0},
	{"PRJ_LEADER"     ,'C', SHP_LEN_PROJECT,0},
	{"OWNER"     ,'C', SHP_LEN_OWNER,0},
	{"GEAR"     ,'C', SHP_LEN_GEAR,0},
	{"HISTORY"     ,'C', SHP_LEN_HISTORY,0},
	{"YEAR_FOUND"     ,'C', SHP_LEN_YEAR_FOUND,0},
	{"REFERENCES"     ,'C', SHP_LEN_REFERENCES,0}
};
#endif

#ifdef _USE_RUSSELL
#pragma pack(1)
typedef struct {
	char Flag;
	char Name[SHP_LEN_NAME];
	char Alt_Names[SHP_LEN_ALT];
	char TssID[SHP_LEN_TSSID];
	char Type[SHP_LEN_TYPE];
	char Length[SHP_LEN_LENGTH];
	char Depth[SHP_LEN_DEPTH];
	char Elev[SHP_LEN_ELEV];
	char Volume[SHP_LEN_VOLUME];
	char Area[SHP_LEN_AREA];
	char Entrance[SHP_LEN_ENTRANCE];
	char Owner[SHP_LEN_OWNER];
	char Synopsis[SHP_LEN_SYNOPSIS];
	char Report[SHP_LEN_MEMO];
	char Maps[SHP_LEN_MEMO];
	char Photos[SHP_LEN_MEMO];
	char TexBio[SHP_LEN_MEMO];
	char Quad[SHP_LEN_QUAD];
} DBF_SHP_REC;
#pragma pack()

static DBF_FLDDEF n_fld[]={
    {"NAME"     ,'C', SHP_LEN_NAME,0},
    {"ALT_NAMES",'C', SHP_LEN_ALT,0},
	{"TSSID"	,'C', SHP_LEN_TSSID,0},
	{"TYPE"		,'C', SHP_LEN_TYPE,0},
	{"LENGTH"	,'N', SHP_LEN_LENGTH,0},
	{"DEPTH"	,'N', SHP_LEN_DEPTH,0},
	{"ELEV_FT"	,'N', SHP_LEN_ELEV,0},
	{"VOLUME"	,'N', SHP_LEN_VOLUME,0},
	{"AREA"		,'C', SHP_LEN_AREA,0},
	{"ENTRANCE" ,'C', SHP_LEN_ENTRANCE,0},
	{"OWNER"	,'C', SHP_LEN_OWNER,0},
	{"SYNOPSIS" ,'C', SHP_LEN_SYNOPSIS,0},
	{"REPORT"	,'M', SHP_LEN_MEMO,0},
	{"MAPS"		,'M', SHP_LEN_MEMO,0},
	{"PHOTOS"	,'M', SHP_LEN_MEMO,0},
	{"TEXBIO"	,'M', SHP_LEN_MEMO,0},
	{"QUAD"		,'C', SHP_LEN_QUAD,0}
};
#endif
#ifdef _USE_COA
#pragma pack(1)
typedef struct {
	char Flag;
	char kde_no[SHP_LEN_KDE_NO];
	char faunal_srv[SHP_LEN_FAUNAL_SRV];	
	char txssid[SHP_LEN_TXSSID];
	char name[SHP_LEN_NAME];
	char type[SHP_LEN_TYPE];
	char aquifer[SHP_LEN_AQUIFER];
	char member[SHP_LEN_MEMBER];
	char catch_area[SHP_LEN_CATCH_AREA];
	char state83_x[SHP_LEN_STATE83_X];
	char state83_y[SHP_LEN_STATE83_Y];
	char feat_srv[SHP_LEN_FEAT_SRV];
	char catch_srv[SHP_LEN_CATCH_SRV];
	char kdb_status[SHP_LEN_KDB_STATUS];
	char orig_62[SHP_LEN_ORIG_62];
} DBF_SHP_REC;
#pragma pack()

static DBF_FLDDEF n_fld[]={
	{"KDE_NO" ,'N', SHP_LEN_KDE_NO,0},
	{"FAUNAL_SRV",'L', SHP_LEN_FAUNAL_SRV,0},	
	{"TXSSID" ,'C', SHP_LEN_TXSSID,0},
	{"NAME" ,'C', SHP_LEN_NAME,0},
	{"TYPE" ,'C', SHP_LEN_TYPE,0},
	{"AQUIFER" ,'C', SHP_LEN_AQUIFER,0},
	{"MEMBER" ,'C', SHP_LEN_MEMBER,0},
	{"CATCH_AREA" ,'N', SHP_LEN_CATCH_AREA,2},
	{"STATE83_X" ,'N', SHP_LEN_STATE83_X,2},
	{"STATE83_Y" ,'N', SHP_LEN_STATE83_Y,2},
	{"FEAT_SRV" ,'C', SHP_LEN_FEAT_SRV,0},
	{"CATCH_SRV" ,'C', SHP_LEN_CATCH_SRV,0},
	{"KDB_STATUS" ,'C', SHP_LEN_KDB_STATUS,0},
	{"ORIG_62" ,'L', SHP_LEN_ORIG_62,0}
};
#endif

#ifdef _USE_WHE
#pragma pack(1)
typedef struct {
	char Flag;
	char name[SHP_LEN_NAME];
	char type[SHP_LEN_TYPE];
	char exc_recom[SHP_LEN_EXC_RECOM];
	char eurycea[SHP_LEN_EURYCEA];
	char geo_eval[SHP_LEN_GEO_EVAL];
	char comment[SHP_LEN_COMMENT];
	char epe[SHP_LEN_EPE];
} DBF_SHP_REC;
#pragma pack()

static DBF_FLDDEF n_fld[]={
	{"NAME" ,'C', SHP_LEN_NAME,0},
	{"TYPE" ,'C', SHP_LEN_TYPE,0},
	{"ESC_RECOM" ,'C', SHP_LEN_EXC_RECOM,0},
	{"EURYCEA" ,'L', SHP_LEN_EURYCEA,0},
	{"GEO_EVAL" ,'L', SHP_LEN_GEO_EVAL,0},
	{"COMMENT" ,'C', SHP_LEN_COMMENT,0},
	{"EPE" ,'N', SHP_LEN_EPE,0}
};
#endif

#ifdef _USE_BIO
#pragma pack(1)
typedef struct {
	char Flag;
	char name[SHP_LEN_NAME];
	char date[SHP_LEN_DATE];
} DBF_SHP_REC;
#pragma pack()

static DBF_FLDDEF n_fld[]={
	{"NAME" ,'C', SHP_LEN_NAME,0},
	{"DATE" ,'C', SHP_LEN_DATE,0}
};
#endif


#define N_NUMFLDS (sizeof(n_fld)/sizeof(DBF_FLDDEF))

#define SHP_DBF_BUFRECS 20
#define SHP_DBF_NUMBUFS 5

#pragma pack(1)

typedef struct {
	double x,y;
} DPOINT;

typedef struct {
	long typ;			//always SHP_TYPE_POINT==3
	double x;
	double y;
} SHP_POINT_XY;

typedef struct {
	long typ;			//always SHP_TYPE_POINTZ==11
	double x;
	double y;
	double z;
	double m;
} SHP_POINT_XYZ;

typedef struct {
	double r;
	double a;
	double v;
} SHP_POINT_RAV;

typedef struct {
	double xmin,ymin,xmax,ymax;
} SHP_BOX;

typedef struct {
	long typ;			//always SHP_TYPE_POLYLINE==3
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	DPOINT points[2];
} SHP_POLY1;

typedef struct {
	long typ;			//always SHP_TYPE_POLYLINEZ==13
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	DPOINT points[2];
	double zrange[2];
	double zarray[2];
	double mrange[2];
	double marray[2];
} SHP_POLYZ1;

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

typedef struct {
	long offset;		//word offset of corrsponding rec in SHP file (big endian)
	long reclen;		//reclen in SHP file (big endian)
} SHX_REC;

typedef struct {
	long recno;			//record number in associated DBF (1-based, big endian)
	long reclen;        //sizeof(SHP_POINT_XY)/2 (big endian)
	long typ;			//always SHP_TYPE_POINT==1
	double x;
	double y;
} SHP_PT_REC;

typedef struct {
	long recno;			//record number in associated DBF (1-based, big endian)
	long reclen;        //sizeof(SHP_POLY1)/2 (big endian)
	long typ;			//always SHP_TYPE_POLYLINE==3
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	double point0[2];
	double point1[2];
} SHP_VEC_REC;

typedef struct {
	long recno;			//record number in associated DBF (1-based, big endian)
	long reclen;        //sizeof(SHP_POINT_XY)/2 (big endian)
	long typ;			//always SHP_TYPE_POINTZ==11
	double x;
	double y;
	double z;
    double m;
} SHP_PTZ_REC;

typedef struct {
	long recno;			//record number in associated DBF (1-based, big endian)
	long reclen;        //sizeof(SHP_POLY1)/2 (big endian)
	long typ;			//always SHP_TYPE_POLYLINEZ==13
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	double point0[2];
	double point1[2];
	double zmin;
	double zmax;
	double z[2];
	double m[4];
} SHP_VECZ_REC;

typedef struct {
	long recno;			//record number in associated DBF (1-based, big endian)
	long reclen;        //size of record in words beginning with typ (big endian)
	long typ;			//always SHP_TYPE_POLYGON==5
	SHP_BOX xy_box;
	long numparts;		//variable
	long numpoints;		//variable
	long parts[1];		//first element always 0 (0-based index of first point in point array)
	//point array follows parts[]
} SHP_POLY_REC;

#pragma pack()
//=======================================================
//Structures returned from Walls --

#ifdef _USE_BIO
static FILE *fpS[NUM_SPECIES],*fpX[NUM_SPECIES];
static DBF_NO db[NUM_SPECIES];
static bool db_created[NUM_SPECIES],db_opened[NUM_SPECIES],s_opened[NUM_SPECIES],x_opened[NUM_SPECIES];
static long numrecs[NUM_SPECIES],offset[NUM_SPECIES];
static SHP_MAIN_HDR main_hdr[NUM_SPECIES];
#else
static FILE *fpS,*fpX;
static DBF_NO db;
static bool db_created,db_opened,s_opened,x_opened;
static long numrecs,offset;
static SHP_MAIN_HDR main_hdr;
#endif
static SHX_REC shx_rec;
static SHP_PTZ_REC pt_rec;
static DBF_SHP_REC dbf_shp_rec;
static SHP_POINT_RAV rav;

enum e_err {ERR_SHP=1,ERR_SHX=2,ERR_DBF=3,ERR_DBT=4};

static CFile dbt;
static UINT dbt_nextRec;
static BYTE dbt_hdr[512];

static BOOL dbt_WriteHdr(void)
{
	//Must be called prior DBT close.
	//Assumes dbt_nextRec stores number of 512-byte blocks including header block.
	memset(dbt_hdr,0,512);
	*(UINT *)dbt_hdr=dbt_nextRec;
	dbt_hdr[16]=0x03;
	try {
		dbt.SeekToBegin();
		dbt.Write(dbt_hdr,512);
	}
	catch(...) {
		return FALSE;
	}
	return TRUE;
}

static int dbt_RecNo(LPCSTR p10)
{
	if(!p10) return 0;
	char buf[12];
	int n=10;
	for(;n && (*p10==' ' || *p10=='0');n--,p10++);
	if(n) {
		memcpy(buf,p10,n);
		buf[n]=0;
		n=atoi(buf);
	}
	return n;
}

UINT dbt_AppendRec(LPCSTR pData)
{
	//Assumes dbt_nextRec stores current number of 512-byte blocks including header block.
	UINT len=strlen(pData)+2;
	UINT recPos=dbt_nextRec;
	try {
		dbt.SeekToEnd(); //should be unnecessary
		while(len) {
			memset(dbt_hdr,0,512);
			if(len>2) {
				UINT sizData=len-2;
				if(sizData>512) sizData=512;
				memcpy(dbt_hdr,pData,sizData);
				pData+=sizData;
				len-=sizData;
				while(len && sizData<512) {
					dbt_hdr[sizData++]=0x1A;
					len--;
				}
			}
			else {
				while(len) dbt_hdr[--len]=0x1A;
			}
			dbt.Write(dbt_hdr,512);
			dbt_nextRec++;
		}
	}
	catch(...) {
		return 0;
	}
	return recPos;
}

UINT dbt_AppendCopy(CFile &dbtBio,LPCSTR p10)
{
	BYTE buf[512];
	UINT recNo=dbt_RecNo(p10);
	if(recNo) {
		try {
			dbt.Seek(dbt_nextRec*512,CFile::begin);
			dbtBio.Seek(512*recNo,CFile::begin);
			recNo=dbt_nextRec;
			do {
				if(dbtBio.Read(buf,512)!=512)
					throw 0;
				dbt.Write(buf,512);
				dbt_nextRec++;
			}
			while(!memchr(buf,0x1A,512));
		}
		catch(...) {
			return (UINT)-1;
		}
	}
	return recNo;
}

//=======================================================
//Local data --

#ifdef _USE_BIO
char shp_pathName[NUM_SPECIES][_MAX_PATH];
char *shp_pBaseExt[NUM_SPECIES];

char *GetShpPathName(int i,char *ext)
{
	sprintf(shp_pBaseExt[i],".%s",ext);
	return shp_pathName[i];
}

void DeleteFiles()
{
	for(int i=0;i<NUM_SPECIES;i++) {
		if(fpX[i]) {
			if(x_opened[i]) {
				fclose(fpX[i]);
				fpX[i]=NULL;
				x_opened[i]=false;
			}
			GetShpPathName(i,"shx");
			_unlink(shp_pathName[i]);
		}

		if(fpS[i]) {
			if(s_opened[i]) {
				fclose(fpS[i]);
				fpS[i]=NULL;
				s_opened[i]=false;
			}
			GetShpPathName(i,"shp");
			_unlink(shp_pathName[i]);
		}

		if(db_created[i]) {
			if(db_opened[i]) dbf_CloseDel(db[i]);
			else {
			  GetShpPathName(i,"dbf");
			  _unlink(shp_pathName[i]);
			}
			db_created[i]=db_opened[i]=false;
		}
	}
}
static int errexit(int i,int e,int typ)
{
	char *ext=NULL;

	switch(typ) {
		case ERR_SHP : 
		case ERR_SHX : ext="shp"; break;
		case ERR_DBF : ext="dbf"; break;
		case ERR_DBT : ext="dbt"; break;
	}

	DeleteFiles();

	if(ext) GetShpPathName(i,ext);
	return e;
}
static void InitMainHdr(int i,int typ)
{
	memset(&main_hdr[i],0,sizeof(SHP_MAIN_HDR));
	main_hdr[i].filecode=SWAPLONG(SHP_FILECODE);
	main_hdr[i].version=1000;
	main_hdr[i].shapetype=typ;
}
static void UpdateHdrBox(int i,SHP_BOX *box)
{
	if(box->xmin<main_hdr[i].xy_box.xmin) main_hdr[i].xy_box.xmin=box->xmin;
	if(box->ymin<main_hdr[i].xy_box.ymin) main_hdr[i].xy_box.ymin=box->ymin;
	if(box->xmax>main_hdr[i].xy_box.xmax) main_hdr[i].xy_box.xmax=box->xmax;
	if(box->ymax>main_hdr[i].xy_box.ymax) main_hdr[i].xy_box.ymax=box->ymax;
}

#else
char shp_pathName[_MAX_PATH];
char *shp_pBaseExt;

char *GetShpPathName(char *ext)
{
	sprintf(shp_pBaseExt,".%s",ext);
	return shp_pathName;
}

void DeleteFiles()
{
	if(fpX) {
		if(x_opened) {
			fclose(fpX);
			fpX=NULL;
			x_opened=false;
		}
		GetShpPathName("shx");
		_unlink(shp_pathName);
	}

	if(fpS) {
		if(s_opened) {
			fclose(fpS);
			fpS=NULL;
			s_opened=false;
		}
		GetShpPathName("shp");
		_unlink(shp_pathName);
	}

	if(db_created) {
		if(db_opened) dbf_CloseDel(db);
		else {
		  GetShpPathName("dbf");
		  _unlink(shp_pathName);
		}
		db_created=db_opened=false;
	}

	if(dbt.m_hFile!=CFile::hFileNull) {
		dbt.Abort();
		dbt.m_hFile=CFile::hFileNull;
		_unlink(GetShpPathName("dbt"));
	}
}

static int errexit(int e,int typ)
{
	char *ext=NULL;

	switch(typ) {
		case ERR_SHP : 
		case ERR_SHX : ext="shp"; break;
		case ERR_DBF : ext="dbf"; break;
		case ERR_DBT : ext="dbt"; break;
	}

	DeleteFiles();

	if(ext) GetShpPathName(ext);
	return e;
}
static void UpdateHdrBox(SHP_BOX *box)
{
	if(box->xmin<main_hdr.xy_box.xmin) main_hdr.xy_box.xmin=box->xmin;
	if(box->ymin<main_hdr.xy_box.ymin) main_hdr.xy_box.ymin=box->ymin;
	if(box->xmax>main_hdr.xy_box.xmax) main_hdr.xy_box.xmax=box->xmax;
	if(box->ymax>main_hdr.xy_box.ymax) main_hdr.xy_box.ymax=box->ymax;
}

static void InitMainHdr(int typ)
{
	memset(&main_hdr,0,sizeof(SHP_MAIN_HDR));
	main_hdr.filecode=SWAPLONG(SHP_FILECODE);
	main_hdr.version=1000;
	main_hdr.shapetype=typ;
}
#endif

static void UpdateBoxPt(SHP_BOX *box,double x,double y)
{
	if(x<box->xmin) box->xmin=x;
	if(y<box->ymin) box->ymin=y;
	if(x>box->xmax) box->xmax=x;
	if(y>box->ymax) box->ymax=y;
}

static void InitBox(SHP_BOX *box)
{
	box->xmin=box->ymin=DBL_MAX;
	box->xmax=box->ymax=-DBL_MAX;
}

#ifdef _USE_BIO
void WritePrjFile(int i,bool bNad83,int utm_zone)
{
	try {
		CFile file(GetShpPathName(i,"prj"),CFile::modeCreate|CFile::modeWrite);
		CString s;
		if(!utm_zone) {
			s.LoadString(bNad83?IDS_PRJ_WGS84:IDS_PRJ_NAD27);
		}
		else {
			s.Format((bNad83?IDS_PRJ_WGS84UTM:IDS_PRJ_NAD27UTM),utm_zone,
				(utm_zone==14)?99:((utm_zone<14)?105:93));
		}
		file.Write((LPCSTR)s,s.GetLength());
		file.Close();
	}
	catch(...) {
		AfxMessageBox("Note: A PRJ file could not be created.");
	}
}


int CloseFiles(void)
{
	int err_typ=ERR_DBF;
	double width;
	int i=0;

	for(;i<NUM_SPECIES;i++) {
		db_opened[i]=FALSE;
		if(dbf_Close(db[i])) goto _errWrite;

		//Expand extents box in main hdr by 2%--
		width=(main_hdr[i].xy_box.xmax-main_hdr[i].xy_box.xmin)/100.0;
		main_hdr[i].xy_box.xmax+=width;
		main_hdr[i].xy_box.xmin-=width;
		width=(main_hdr[i].xy_box.ymax-main_hdr[i].xy_box.ymin)/100.0;
		main_hdr[i].xy_box.ymax+=width;
		main_hdr[i].xy_box.ymin-=width;

		err_typ=ERR_SHX;
		numrecs[i]=50+(numrecs[i]*4);
		main_hdr[i].filelength=SWAPLONG(numrecs[i]);
		if(fseek(fpX[i],0,SEEK_SET) || fwrite(&main_hdr[i],sizeof(SHP_MAIN_HDR),1,fpX[i])!=1)
		  goto _errWrite;
		x_opened[i]=FALSE;
		if(fclose(fpX[i])) goto _errWrite;

		err_typ=ERR_SHP;
		main_hdr[i].filelength=SWAPLONG(offset[i]);
		if(fseek(fpS[i],0,SEEK_SET) || fwrite(&main_hdr[i],sizeof(SHP_MAIN_HDR),1,fpS[i])!=1)
		  goto _errWrite;
		s_opened[i]=FALSE;
		if(fclose(fpS[i])) goto _errWrite;

		fpS[i]=fpX[i]=NULL;
		db_created[i]=false;
		WritePrjFile(i,true,14);
	}

	return 0;

_errWrite:
	return errexit(i,SHP_ERR_WRITE,err_typ);
}
#else

void WritePrjFile(bool bNad83,int utm_zone)
{
	try {
		CFile file(GetShpPathName("prj"),CFile::modeCreate|CFile::modeWrite);
		CString s;
		if(!utm_zone) {
			s.LoadString(bNad83?IDS_PRJ_WGS84:IDS_PRJ_NAD27);
		}
		else {
			s.Format((bNad83?IDS_PRJ_WGS84UTM:IDS_PRJ_NAD27UTM),utm_zone,
				(utm_zone==14)?99:((utm_zone<14)?105:93));
		}
		file.Write((LPCSTR)s,s.GetLength());
		file.Close();
	}
	catch(...) {
		AfxMessageBox("Note: A PRJ file could not be created.");
	}
}

int CloseFiles(void)
{
	int err_typ;
	double width;

	err_typ=ERR_DBF;
	db_opened=FALSE;
	if(dbf_Close(db)) goto _errWrite;
	//This also frees the DBF cache
	if(dbt.m_hFile!=CFile::hFileNull) {
		err_typ=ERR_DBF;
		if(!dbt_WriteHdr()) goto _errWrite;
		dbt.Close();
	}

	//Expand extents box in main hdr by 2%--
	width=(main_hdr.xy_box.xmax-main_hdr.xy_box.xmin)/100.0;
	main_hdr.xy_box.xmax+=width;
	main_hdr.xy_box.xmin-=width;
	width=(main_hdr.xy_box.ymax-main_hdr.xy_box.ymin)/100.0;
	main_hdr.xy_box.ymax+=width;
	main_hdr.xy_box.ymin-=width;

	err_typ=ERR_SHX;
	numrecs=50+(numrecs*4);
	main_hdr.filelength=SWAPLONG(numrecs);
    if(fseek(fpX,0,SEEK_SET) || fwrite(&main_hdr,sizeof(SHP_MAIN_HDR),1,fpX)!=1)
	  goto _errWrite;
	x_opened=FALSE;
	if(fclose(fpX)) goto _errWrite;

	err_typ=ERR_SHP;
	main_hdr.filelength=SWAPLONG(offset);
    if(fseek(fpS,0,SEEK_SET) || fwrite(&main_hdr,sizeof(SHP_MAIN_HDR),1,fpS)!=1)
	  goto _errWrite;
	s_opened=FALSE;
	if(fclose(fpS)) goto _errWrite;

	fpS=fpX=NULL;
	db_created=false;
#if defined(_USE_GUA)
	WritePrjFile(true,0);
#elif defined(_USE_LLOYD)
	WritePrjFile(true,13);
#else
	WritePrjFile(true,14);
#endif

	return 0;

_errWrite:
	return errexit(SHP_ERR_WRITE,err_typ);
}
#endif
//================================================================================
//Routines for record transfer --

static void copy_str(char *dst,const char *src)
{
	memcpy(dst,src,strlen(src));
}

static void copy_int(char *dst,int n,int len)
{
	char argv_buf[32];
	if(sprintf(argv_buf,"%*d",len,n)==len) memcpy(dst,argv_buf,len);
}

static void copy_flt(char *dst,double f,int len,int dec)
{
	char argv_buf[32];
	if(sprintf(argv_buf,"%*.*f",len,dec,f)==len) memcpy(dst,argv_buf,len);
}

//=====================================================================================================
#ifdef _USE_BIO
int CreateFiles(CString *lpPathname)
{
	int i=0;
	int err_typ=0;
	for(;i<NUM_SPECIES;i++) {
		shp_pBaseExt[i]=trx_Stpext(strcpy(shp_pathName[i],lpPathname[i]));
		pt_rec.reclen=shx_rec.reclen=SWAPLONG(sizeof(SHP_POINT_XY)/2);
		pt_rec.typ=SHP_TYPE_POINT;

		fpS[i]=fpX[i]=NULL;
		db_created[i]=db_opened[i]=s_opened[i]=x_opened[i]=false;
		numrecs[i]=0;
		offset[i]=50;
		InitMainHdr(i,SHP_TYPE_POINT);

		GetShpPathName(i,"shp"); err_typ=ERR_SHP;
		if(!(fpS[i]=fopen(shp_pathName[i],"wb"))) goto _errCreate;
		s_opened[i]=true;
		if(fwrite(&main_hdr[i],sizeof(SHP_MAIN_HDR),1,fpS[i])!=1) goto _errWrite;

		GetShpPathName(i,"shx"); err_typ=ERR_SHX;
		if(!(fpX[i]=fopen(shp_pathName[i],"wb"))) goto _errCreate;
		x_opened[i]=true;
		if(fwrite(&main_hdr[i],sizeof(SHP_MAIN_HDR),1,fpX[i])!=1) goto _errWrite;

		GetShpPathName(i,"dbf"); err_typ=ERR_DBF;
		if(dbf_Create(&db[i],shp_pathName[i],DBF_ReadWrite,N_NUMFLDS,n_fld)) goto _errCreate;
		db_created[i]=db_opened[i]=true;
		if(!i && dbf_AllocCache(db[i],SHP_DBF_BUFRECS*dbf_SizRec(db[i]),SHP_DBF_NUMBUFS,TRUE))
			goto _errWrite;

		InitBox(&main_hdr[i].xy_box);
	}

	return 0;

_errCreate:
	return errexit(SHP_ERR_CREATE,i,err_typ); 

_errWrite:
	return errexit(SHP_ERR_WRITE,i,err_typ);
}
#else
int CreateFiles(LPCSTR lpPathname)
{
    shp_pBaseExt=trx_Stpext(strcpy(shp_pathName,lpPathname));
	pt_rec.reclen=shx_rec.reclen=SWAPLONG(sizeof(SHP_POINT_XY)/2);
	pt_rec.typ=SHP_TYPE_POINT;

	//return InitFiles(n_fld,N_NUMFLDS);
	int err_typ;

	fpS=fpX=NULL;
	db_created=db_opened=s_opened=x_opened=false;
	numrecs=0;
	offset=50;
	InitMainHdr(SHP_TYPE_POINT);

    GetShpPathName("shp"); err_typ=ERR_SHP;
	if(!(fpS=fopen(shp_pathName,"wb"))) goto _errCreate;
	s_opened=true;
	if(fwrite(&main_hdr,sizeof(SHP_MAIN_HDR),1,fpS)!=1) goto _errWrite;

    GetShpPathName("shx"); err_typ=ERR_SHX;
	if(!(fpX=fopen(shp_pathName,"wb"))) goto _errCreate;
	x_opened=true;
	if(fwrite(&main_hdr,sizeof(SHP_MAIN_HDR),1,fpX)!=1) goto _errWrite;

    GetShpPathName("dbf"); err_typ=ERR_DBF;
    if(dbf_Create(&db,shp_pathName,DBF_ReadWrite,N_NUMFLDS,n_fld)) goto _errCreate;
	db_created=db_opened=true;
	if(dbf_AllocCache(db,SHP_DBF_BUFRECS*dbf_SizRec(db),SHP_DBF_NUMBUFS,TRUE))
		goto _errWrite;

 	if(dbf_FileID(db)==DBF_FILEID_MEMO) {
		err_typ=ERR_DBT;
		dbt_nextRec=1;
		if(!dbt.Open(GetShpPathName("dbt"),CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive)
				|| !dbt_WriteHdr())
			goto _errCreate;
	}

    InitBox(&main_hdr.xy_box);

	return 0;

_errCreate:
	return errexit(SHP_ERR_CREATE,err_typ); 

_errWrite:
	return errexit(SHP_ERR_WRITE,err_typ);
}
#endif

#ifdef _USE_BIO
int WriteShpRec(int i,SHP_DATA *pData)
{
	numrecs[i]++;
	pt_rec.recno=SWAPLONG(numrecs[i]);
	shx_rec.offset=SWAPLONG(offset[i]);
    offset[i]+=sizeof(SHP_PT_REC)/2;

	memset(&dbf_shp_rec,' ',sizeof(DBF_SHP_REC));

	copy_str(dbf_shp_rec.name,pData->name);
	copy_str(dbf_shp_rec.date,pData->date);

	UpdateBoxPt(&main_hdr[i].xy_box,pt_rec.x=pData->fEasting,pt_rec.y=pData->fNorthing);

	if(fwrite(&pt_rec,sizeof(SHP_PT_REC),1,fpS[i])!=1) return errexit(i,SHP_ERR_WRITE,ERR_SHP);
	if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX[i])!=1) return errexit(i,SHP_ERR_WRITE,ERR_SHX);
	if(dbf_AppendRec(db[i],&dbf_shp_rec)) return errexit(i,SHP_ERR_WRITE,ERR_DBF);
	return 0;
}
#else
int WriteShpRec(SHP_DATA *pData)
{
	numrecs++;
	pt_rec.recno=SWAPLONG(numrecs);
	shx_rec.offset=SWAPLONG(offset);
    offset+=sizeof(SHP_PT_REC)/2;

	memset(&dbf_shp_rec,' ',sizeof(DBF_SHP_REC));

#ifdef _USE_LLOYD
	copy_str(dbf_shp_rec.Name,pData->Name);
	copy_str(dbf_shp_rec.Alt_Names,pData->Alt_Names);
	//copy_str(dbf_shp_rec.Municipio,pData->Municipio);
	copy_str(dbf_shp_rec.State,pData->State);
	if(pData->nElev) copy_int(dbf_shp_rec.Elev,pData->nElev,SHP_LEN_ELEV);
	if(pData->nLength) copy_int(dbf_shp_rec.Length,pData->nLength,SHP_LEN_LENGTH);
	if(pData->nDepth) copy_int(dbf_shp_rec.Depth,pData->nDepth,SHP_LEN_DEPTH);
	copy_str(dbf_shp_rec.Topo,pData->Topo);
	copy_str(dbf_shp_rec.Location,pData->Location);
	copy_str(dbf_shp_rec.Comments,pData->Comments);
	//copy_str(dbf_shp_rec.Site_ID,pData->Site_ID);
#endif

#ifdef _USE_GUA
	copy_str(dbf_shp_rec.ID,pData->ID);
	copy_str(dbf_shp_rec.Name,pData->Name);
	copy_str(dbf_shp_rec.Alt_Names,pData->Alt_Names);
	copy_str(dbf_shp_rec.Type,pData->Type);
	copy_str(dbf_shp_rec.Exp_Status,pData->Exp_Status);
	copy_str(dbf_shp_rec.Accuracy,pData->Accuracy);
	copy_str(dbf_shp_rec.Elev,pData->Elev);
	copy_str(dbf_shp_rec.Quad,pData->Quad);
	copy_str(dbf_shp_rec.Location,pData->Location);
	copy_str(dbf_shp_rec.Department,pData->Department);
	copy_str(dbf_shp_rec.Desc,pData->Desc);
	copy_str(dbf_shp_rec.Map_Year,pData->Map_Year);
	copy_str(dbf_shp_rec.Map_Status,pData->Map_Status);
	copy_str(dbf_shp_rec.Length,pData->Length);
	copy_str(dbf_shp_rec.Depth,pData->Depth);
	copy_str(dbf_shp_rec.Map_Links,pData->Map_Links);
	copy_str(dbf_shp_rec.Discovery,pData->Discovery);
	copy_str(dbf_shp_rec.Mapping,pData->Mapping);
	copy_str(dbf_shp_rec.Project,pData->Project);
	copy_str(dbf_shp_rec.Owner,pData->Owner);
	copy_str(dbf_shp_rec.Gear,pData->Gear);
	copy_str(dbf_shp_rec.History,pData->History);
	copy_str(dbf_shp_rec.Year_Found,pData->Year_Found);
	copy_str(dbf_shp_rec.References,pData->References);
#endif

#ifdef _USE_RUSSELL
	copy_str(dbf_shp_rec.Name,pData->Name);
	copy_str(dbf_shp_rec.Alt_Names,pData->Alt_Names);
	copy_str(dbf_shp_rec.Type,pData->Type);
	copy_str(dbf_shp_rec.TssID,pData->TssID);
	copy_str(dbf_shp_rec.Area,pData->Area);
	copy_str(dbf_shp_rec.Entrance,pData->Entrance);
	copy_str(dbf_shp_rec.Owner,pData->Owner);
	copy_str(dbf_shp_rec.Synopsis,pData->Synopsis);
	copy_str(dbf_shp_rec.Report,pData->Report);
	copy_str(dbf_shp_rec.Maps,pData->Maps);
	copy_str(dbf_shp_rec.Photos,pData->Photos);
	copy_str(dbf_shp_rec.TexBio,pData->TexBio);
	copy_str(dbf_shp_rec.Quad,pData->Quad);

	if(pData->nElev) copy_int(dbf_shp_rec.Elev,pData->nElev,SHP_LEN_ELEV);
	if(pData->nLength) copy_int(dbf_shp_rec.Length,pData->nLength,SHP_LEN_LENGTH);
	if(pData->nDepth) copy_int(dbf_shp_rec.Depth,pData->nDepth,SHP_LEN_DEPTH);
	if(pData->nVolume) copy_int(dbf_shp_rec.Volume,pData->nVolume,SHP_LEN_DEPTH);
#endif

#ifdef _USE_COA
	copy_int(dbf_shp_rec.kde_no,pData->kde_no,SHP_LEN_KDE_NO);
	copy_str(dbf_shp_rec.faunal_srv,pData->faunal_srv);
	copy_str(dbf_shp_rec.txssid,pData->txssid);
	copy_str(dbf_shp_rec.name,pData->name);
	copy_str(dbf_shp_rec.type,pData->type);
	copy_str(dbf_shp_rec.aquifer,pData->aquifer);
	copy_str(dbf_shp_rec.member,pData->member);
	if(pData->catch_area)
		copy_flt(dbf_shp_rec.catch_area,pData->catch_area,SHP_LEN_CATCH_AREA,2);
	if(pData->state83_x)
		copy_flt(dbf_shp_rec.state83_x,pData->state83_x,SHP_LEN_STATE83_X,2);
	if(pData->state83_y)
		copy_flt(dbf_shp_rec.state83_y,pData->state83_y,SHP_LEN_STATE83_Y,2);
	copy_str(dbf_shp_rec.feat_srv,pData->feat_srv);
	copy_str(dbf_shp_rec.catch_srv,pData->catch_srv);
	copy_str(dbf_shp_rec.kdb_status,pData->kdb_status);
	copy_str(dbf_shp_rec.orig_62,pData->orig_62);
#endif

#ifdef _USE_WHE
	if(pData->epe)
		copy_int(dbf_shp_rec.epe,pData->epe,SHP_LEN_EPE);
	copy_str(dbf_shp_rec.name,pData->name);
	copy_str(dbf_shp_rec.type,pData->type);
	copy_str(dbf_shp_rec.exc_recom,pData->exc_recom);
	copy_str(dbf_shp_rec.eurycea,pData->eurycea);
	copy_str(dbf_shp_rec.geo_eval,pData->geo_eval);
	copy_str(dbf_shp_rec.comment,pData->comment);
#endif

#ifdef _USE_GUA
	UpdateBoxPt(&main_hdr.xy_box,pt_rec.x=pData->fLon,pt_rec.y=pData->fLat);
#else
	UpdateBoxPt(&main_hdr.xy_box,pt_rec.x=pData->fEasting,pt_rec.y=pData->fNorthing);
#endif

	if(fwrite(&pt_rec,sizeof(SHP_PT_REC),1,fpS)!=1) return errexit(SHP_ERR_WRITE,ERR_SHP);
	if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX)!=1) return errexit(SHP_ERR_WRITE,ERR_SHX);
	if(dbf_AppendRec(db,&dbf_shp_rec)) return errexit(SHP_ERR_WRITE,ERR_DBF);
	return 0;
}
#endif