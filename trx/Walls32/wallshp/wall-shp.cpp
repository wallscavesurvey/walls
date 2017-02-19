// wall-shp.cpp : CSegView::ExportSHP() implementation
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys\stat.h>
#include <dbf_file.h>
#include <trx_str.h>
#include <assert.h>
#include "..\wall_shp.h"

#ifndef DLLExportC
#define DLLExportC extern "C" __declspec(dllexport)
#endif
//=======================================================
#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

//Shapefile structures --

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)),SWAPWORD(LOWORD(x)))

#define SHP_FILECODE 9994
#define SHP_TYPE_POINT 1
#define SHP_TYPE_POINTZ 11
#define SHP_TYPE_POLYLINE 3
#define SHP_TYPE_POLYLINEZ 13
#define SHP_TYPE_POLYGON 5

#define SHP_DEC_FLTXY_G 7

#define SHP_LEN_FLTXY 12
#define SHP_DEC_FLTXY  2
#define SHP_LEN_FLTZ  10
#define SHP_DEC_FLTZ   2
#define SHP_LEN_LRUD   8
#define SHP_DEC_LRUD   1
#define SHP_LEN_FLTAZIMUTH 5
#define SHP_DEC_FLTAZIMUTH 1
#define SHP_LEN_FLTINCLINE 6
#define SHP_DEC_FLTINCLINE 1

#define SHP_DBF_BUFRECS 20
#define SHP_DBF_NUMBUFS 5

#pragma pack(1)

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
	CFltPoint points[2];
} SHP_POLY1;

typedef struct {
	long typ;			//always SHP_TYPE_POLYLINEZ==13
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	CFltPoint points[2];
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

//Station types --
//-----------------------------------------------------------------------------
typedef struct {
	char flag;
	char prefix[SHP_LEN_PREFIX];
	char name[SHP_LEN_LABEL];
	char x[SHP_LEN_FLTXY];
	char y[SHP_LEN_FLTXY];
	char z[SHP_LEN_FLTZ];
	char left[SHP_LEN_LRUD];
	char right[SHP_LEN_LRUD];
	char up[SHP_LEN_LRUD];
	char down[SHP_LEN_LRUD];
	char lrud_az[SHP_LEN_LRUD];
} DBF_S_REC;

static DBF_FLDDEF s_fld_g[]={ /**/
	{"PREFIX"	,'C', SHP_LEN_PREFIX,0},
    {"NAME"     ,'C', SHP_LEN_LABEL,0},
	{"X"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY_G},
	{"Y"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY_G},
	{"Z"		,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"LEFT"		,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"RIGHT"	,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"UP"		,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"DOWN"		,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"LRUD_AZ"	,'N', SHP_LEN_LRUD,SHP_DEC_LRUD}
};

static DBF_FLDDEF s_fld[]={ /**/
	{"PREFIX"	,'C', SHP_LEN_PREFIX,0},
    {"NAME"     ,'C', SHP_LEN_LABEL,0},
	{"X"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY},
	{"Y"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY},
	{"Z"		,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"LEFT"		,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"RIGHT"	,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"UP"		,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"DOWN"		,'N', SHP_LEN_LRUD,SHP_DEC_LRUD},
	{"LRUD_AZ"	,'N', SHP_LEN_LRUD,SHP_DEC_LRUD}
};
#define S_NUMFLDS (sizeof(s_fld)/sizeof(DBF_FLDDEF))

//Note types --
//-----------------------------------------------------------------------------
typedef struct {
	char flag;
	char prefix[SHP_LEN_PREFIX];
	char name[SHP_LEN_LABEL];
	char x[SHP_LEN_FLTXY];
	char y[SHP_LEN_FLTXY];
	char z[SHP_LEN_FLTZ];
	char note[SHP_LEN_NOTE];
} DBF_N_REC;

static DBF_FLDDEF note_fld={"NOTE"		,'C', SHP_LEN_NOTE,0};
static DBF_FLDDEF flag_fld={"FLAGNAME"	,'C', SHP_LEN_FLAGNAME,0};

static DBF_FLDDEF n_fld[]={ /**/
	{"PREFIX"	,'C', SHP_LEN_PREFIX,0},
    {"NAME"     ,'C', SHP_LEN_LABEL,0},
	{"X"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY},
	{"Y"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY},
	{"Z"		,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"NOTE"		,'C', SHP_LEN_NOTE,0}
};
static DBF_FLDDEF n_fld_g[]={ /**/
	{"PREFIX"	,'C', SHP_LEN_PREFIX,0},
    {"NAME"     ,'C', SHP_LEN_LABEL,0},
	{"X"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY_G},
	{"Y"		,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY_G},
	{"Z"		,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"NOTE"		,'C', SHP_LEN_NOTE,0}
};
#define N_NUMFLDS (sizeof(n_fld)/sizeof(DBF_FLDDEF))

//Vector types --
//-----------------------------------------------------------------------------
typedef struct {
	char flag; //delete flag (0x20)
	char fr_prefix[SHP_LEN_PREFIX];
	char fr_name[SHP_LEN_LABEL];
	char to_prefix[SHP_LEN_PREFIX];
	char to_name[SHP_LEN_LABEL];
	char ctr_x[SHP_LEN_FLTXY];
	char ctr_y[SHP_LEN_FLTXY];
	char ctr_z[SHP_LEN_FLTZ];
	char length[SHP_LEN_FLTZ];
	char azimuth[SHP_LEN_FLTAZIMUTH];
	char incline[SHP_LEN_FLTINCLINE];
	char linetype[SHP_LEN_LINETYPE];
	char date[SHP_LEN_DATE];
	char srv_name[SHP_LEN_FILENAME];
	char srv_title[SHP_LEN_TITLE];
} DBF_V_REC;

static DBF_FLDDEF v_fld[]={ /**/
	{"FR_PREFIX",'C', SHP_LEN_PREFIX,0},
    {"FR_NAME"  ,'C', SHP_LEN_LABEL,0},
	{"TO_PREFIX",'C', SHP_LEN_PREFIX,0},
    {"TO_NAME"  ,'C', SHP_LEN_LABEL,0},
	{"CTR_X"	,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY},
	{"CTR_Y"	,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY},
	{"CTR_Z"	,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"LENGTH"	,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"AZIMUTH"	,'N', SHP_LEN_FLTAZIMUTH,SHP_DEC_FLTAZIMUTH},
	{"INCLINE"	,'N', SHP_LEN_FLTINCLINE,SHP_DEC_FLTINCLINE},
	{"LINETYPE" ,'C', SHP_LEN_LINETYPE,0},
	{"DATE"     ,'N', SHP_LEN_DATE,0},
    {"SRV_NAME" ,'C', SHP_LEN_FILENAME,0},
	{"SRV_TITLE",'C', SHP_LEN_TITLE,0}
};
static DBF_FLDDEF v_fld_g[]={ /**/
	{"FR_PREFIX",'C', SHP_LEN_PREFIX,0},
    {"FR_NAME"  ,'C', SHP_LEN_LABEL,0},
	{"TO_PREFIX",'C', SHP_LEN_PREFIX,0},
    {"TO_NAME"  ,'C', SHP_LEN_LABEL,0},
	{"CTR_X"	,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY_G},
	{"CTR_Y"	,'N', SHP_LEN_FLTXY,SHP_DEC_FLTXY_G},
	{"CTR_Z"	,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"LENGTH"	,'N', SHP_LEN_FLTZ,SHP_DEC_FLTZ},
	{"AZIMUTH"	,'N', SHP_LEN_FLTAZIMUTH,SHP_DEC_FLTAZIMUTH},
	{"INCLINE"	,'N', SHP_LEN_FLTINCLINE,SHP_DEC_FLTINCLINE},
	{"LINETYPE" ,'C', SHP_LEN_LINETYPE,0},
	{"DATE"     ,'N', SHP_LEN_DATE,0},
    {"SRV_NAME" ,'C', SHP_LEN_FILENAME,0},
	{"SRV_TITLE",'C', SHP_LEN_TITLE,0}
};
#define V_NUMFLDS (sizeof(v_fld)/sizeof(DBF_FLDDEF))

//Wall types --
//-----------------------------------------------------------------------------
#define SHP_LEN_SVGNAME 30
#define SHP_LEN_SQMETERS 8
#define SHP_DEC_SQMETERS 1
#define SHP_LEN_ELEVATION 8
#define SHP_DEC_ELEVATION 1

typedef struct {
	char flag;
	char svgname[SHP_LEN_SVGNAME];
	char groupname[SHP_LEN_MASKNAME];
	char fillcolor[SHP_LEN_FILLCOLOR];
	char polygons[SHP_LEN_POLYGONS];
	char sqmeters[SHP_LEN_SQMETERS];
} DBF_W_REC;

static DBF_FLDDEF w_fld[]={ /**/
	{"SVGNAME"	,'C', SHP_LEN_SVGNAME,0},
	{"GROUPNAME",'C', SHP_LEN_MASKNAME,0},
    {"FILLCOLOR",'C', SHP_LEN_FILLCOLOR,0},
	{"POLYGONS", 'N', SHP_LEN_POLYGONS,0},
	{"SQMETERS"	,'N', SHP_LEN_SQMETERS,SHP_DEC_SQMETERS}
};
#define W_NUMFLDS (sizeof(w_fld)/sizeof(DBF_FLDDEF))

typedef struct {
	char flag;
	char elevation[SHP_LEN_ELEVATION];
	//char sqmeters[SHP_LEN_SQMETERS];
} DBF_LRUD_REC;

static DBF_FLDDEF lrud_fld[]={ /**/
	{"ELEVATION",'N', SHP_LEN_ELEVATION,SHP_DEC_ELEVATION}
	//,{"SQMETERS"	,'N', SHP_LEN_SQMETERS,SHP_DEC_SQMETERS}
};
#define LRUD_NUMFLDS (sizeof(lrud_fld)/sizeof(DBF_FLDDEF))

#pragma pack()
//=======================================================
//Structures returned from Walls --

static LPCSTR pPrjRef; //for creating PRJ components

static SHP_TYP_VECTOR sV;
static SHP_TYP_STATION sS;
static SHP_TYP_NOTE sN;
static double fLenRatio=0;
//Assume  sizeof(sN) >= sizeof(sF) --
//static SHP_TYP_FLAG sF;

static SHP_TYP_POLYGON sP;
static double fArea,fAreaTotal;
static int nPolyTotal;
static BOOL b3D,bUTM;
static SHP_VECZ_REC v_rec;
static SHP_PTZ_REC pt_rec;

static FILE *fpS,*fpX;
static DBF_NO db;
static BOOL db_created,db_opened,s_opened,x_opened;
static char *pBaseExt;
static long numrecs,offset;
static SHX_REC shx_rec;
static DBF_V_REC dbf_v_rec;
static DBF_S_REC dbf_s_rec;
static DBF_N_REC dbf_n_rec;
static SHP_MAIN_HDR main_hdr;
static SHP_POINT_RAV rav;
static LPFN_EXPORT_CB pGetData;

static DBF_LRUD_REC dbf_lrud_rec;
static DBF_W_REC dbf_w_rec;
static SHP_POLY_REC *pp_rec;

enum e_err {ERR_SHP=1,ERR_SHX=2,ERR_DBF=3};

static char shp_pfx;

//=======================================================
//Local data --

static char pathname[_MAX_PATH];
static char argv_buf[_MAX_PATH];

static NPSTR errstr[]={
  "Unable to load NTW file",
  "Insufficient memory",
  "Wrong version of " EXP_SHPDLL_NAME,
  "User abort",
  "Error writing",
  "Cannot create",
 };

/*
enum {
  SHP_GET_NTWSTATS,
  SHP_ERR_LOADNTW,
  SHP_ERR_NOMEM,
  SHP_ERR_VERSION
  SHP_ERR_ABORTED,
  SHP_ERR_WRITE,
  SHP_ERR_CREATE,
  SHP_ERR_FINISHED
};
*/

DLLExportC char * PASCAL ErrMsg(int code)
{
	if(code==SHP_GET_NTWSTATS) sprintf(argv_buf,"Polygons: %u  Floor area in sq-meters: %.1lf",nPolyTotal,fAreaTotal);
	else if(code==-1) sprintf(argv_buf,"Polygons: %u",nPolyTotal);
	else {
		if(code>SHP_ERR_CREATE) code=SHP_ERR_VERSION;
		if(code<=SHP_ERR_ABORTED) strcpy(argv_buf,errstr[code-1]);
		else sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code-1],(LPSTR)pathname);
	}
	return argv_buf;
}

static char * GetFloatStr(double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	static char buf[40];
	BOOL bRound=TRUE;

	if(dec<0) {
		bRound=FALSE;
		dec=-dec;
	}
	if(_snprintf(buf,20,"%-0.*f",dec,f)==-1) {
	  *buf=0;
	  return buf;
	}
	if(bRound) {
		char *p=strchr(buf,'.');
		for(p+=dec;dec && *p=='0';dec--) *p--=0;
		if(*p=='.') {
		   *p=0;
		   if(!strcmp(buf,"-0")) strcpy(buf,"0");
		}
	}
	return buf;
}

static void GetPathName(char *ext)
{
	sprintf(pBaseExt,"%c.%s",shp_pfx,ext);
}

static int errexit(int e,int typ)
{
	char *ext=NULL;

	switch(typ) {
		case ERR_SHP : 
		case ERR_SHX : ext="shp"; break;
		case ERR_DBF : ext="dbf"; break;
	}

	if(fpX) {
		if(x_opened) fclose(fpX);
		GetPathName("shx");
		_unlink(pathname);
	}

	if(fpS) {
		if(s_opened) fclose(fpS);
		GetPathName("shp");
		_unlink(pathname);
	}

	if(db_created) {
		if(db_opened) dbf_CloseDel(db);
		else {
		  GetPathName("dbf");
		  _unlink(pathname);
		}
	}

    if(pp_rec) {free(pp_rec); pp_rec=NULL;}
	if(ext) GetPathName(ext);
	return e;
}

static void InitMainHdr(void)
{
	int typ;

	memset(&main_hdr,0,sizeof(SHP_MAIN_HDR));
	main_hdr.filecode=SWAPLONG(SHP_FILECODE);
	main_hdr.version=1000;
	switch(shp_pfx) {
		case 'v' : typ=(b3D?SHP_TYPE_POLYLINEZ:SHP_TYPE_POLYLINE); break;
		case 'w' : typ=SHP_TYPE_POLYGON; break;
		default  : typ=(b3D?SHP_TYPE_POINTZ:SHP_TYPE_POINT);
	}
	main_hdr.shapetype=typ;
}

static void UpdateBoxPt(SHP_BOX *box,double x,double y)
{
	if(x<box->xmin) box->xmin=x;
	if(y<box->ymin) box->ymin=y;
	if(x>box->xmax) box->xmax=x;
	if(y>box->ymax) box->ymax=y;
}

static void UpdateHdrBox(SHP_BOX *box)
{
	if(box->xmin<main_hdr.xy_box.xmin) main_hdr.xy_box.xmin=box->xmin;
	if(box->ymin<main_hdr.xy_box.ymin) main_hdr.xy_box.ymin=box->ymin;
	if(box->xmax>main_hdr.xy_box.xmax) main_hdr.xy_box.xmax=box->xmax;
	if(box->ymax>main_hdr.xy_box.ymax) main_hdr.xy_box.ymax=box->ymax;
}

static void InitBox(SHP_BOX *box)
{
	box->xmin=box->ymin=DBL_MAX;
	box->xmax=box->ymax=-DBL_MAX;
}

static int InitFiles(DBF_FLDDEF *pFldDef,int numflds)
{
	int err_typ;

	fpS=fpX=NULL;
	db_created=db_opened=s_opened=x_opened=FALSE;
	numrecs=0;
	offset=50;
	InitMainHdr();

    GetPathName("shp"); err_typ=ERR_SHP;
	if(!(fpS=fopen(pathname,"wb"))) goto _errCreate;
	s_opened=TRUE;
	if(fwrite(&main_hdr,sizeof(SHP_MAIN_HDR),1,fpS)!=1) goto _errWrite;

    GetPathName("shx"); err_typ=ERR_SHX;
	if(!(fpX=fopen(pathname,"wb"))) goto _errCreate;
	x_opened=TRUE;
	if(fwrite(&main_hdr,sizeof(SHP_MAIN_HDR),1,fpX)!=1) goto _errWrite;

    GetPathName("dbf"); err_typ=ERR_DBF;
    if(dbf_Create(&db,pathname,DBF_ReadWrite,numflds,pFldDef)) goto _errCreate;
	db_created=db_opened=TRUE;
	if(dbf_AllocCache(db,SHP_DBF_BUFRECS*dbf_SizRec(db),SHP_DBF_NUMBUFS,TRUE))
		goto _errWrite;

    InitBox(&main_hdr.xy_box);

	if(b3D && pFldDef!=w_fld) {
		main_hdr.zmin=DBL_MAX;
		main_hdr.zmax=-DBL_MAX;
		main_hdr.mmax=main_hdr.mmin=DBL_MIN;
	}

	return 0;

_errCreate:
	return errexit(SHP_ERR_CREATE,err_typ); 

_errWrite:
	return errexit(SHP_ERR_WRITE,err_typ);
}

static int Append_db(LPVOID pRec)
{
	if(dbf_AppendRec(db,pRec)) return errexit(SHP_ERR_WRITE,ERR_DBF);
	return 0;
}

static void WritePrjFile()
{
	char pathName[_MAX_PATH];
	strcpy(pathName,dbf_PathName(db));
	strcpy(trx_Stpext(pathName),".prj");
	if(pPrjRef) { 
		FILE *fp;
		if((fp=fopen(pathName,"wb"))) {
			fputs(pPrjRef,fp);
			fclose(fp);
		}
	}
	else _unlink(pathName);
}

static int CloseFiles(void)
{
	int err_typ;
	double width;

	//write prj component --
	WritePrjFile();

	err_typ=ERR_DBF;
	db_opened=FALSE;
	if(dbf_Close(db)) goto _errWrite;
	//This also frees the DBF cache

	//Expand extents box in main hdr by 25% --
	width=(main_hdr.xy_box.xmax-main_hdr.xy_box.xmin)/8.0;
	main_hdr.xy_box.xmax+=width;
	main_hdr.xy_box.xmin-=width;
	width=(main_hdr.xy_box.ymax-main_hdr.xy_box.ymin)/8.0;
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
	db_created=FALSE;
	return 0;

_errWrite:
	return errexit(SHP_ERR_WRITE,err_typ);
}

//================================================================================
//Routines for record transfer --

static void copy_str(char *dst,const char *src)
{
	memcpy(dst,src,strlen(src));
}

static void copy_int(char *dst,int n,int len)
{
	if(sprintf(argv_buf,"%*d",len,n)==len) memcpy(dst,argv_buf,len);
}

static void copy_flt(char *dst,double f,int len,int dec)
{
	if(sprintf(argv_buf,"%*.*f",len,dec,f)==len) memcpy(dst,argv_buf,len);
}

static double NEAR PASCAL atn2(double a,double b)
{
  if(a==0.0 && b==0.0) return 0.0;
  return atan2(a,b)*(180.0/3.1415927);
}

static void get_rav(double x,double y,double z)
{
  double d2=x*x+y*y;

  if(d2<0.001) rav.a=0.0;
  else {
	  rav.a=atn2(x,y);
	  if(rav.a<0.0) rav.a+=360.0;
      else if(rav.a>=360.0) rav.a-=360.0;
  }
  rav.v=atn2(z,sqrt(d2));
  rav.r=sqrt(d2+z*z);
}

static void GetVectorAttributes(void)
{
	memset(&dbf_v_rec,' ',sizeof(DBF_V_REC));

	copy_str(dbf_v_rec.fr_prefix,sV.station[0].prefix);
	copy_str(dbf_v_rec.fr_name,sV.station[0].label);
	copy_str(dbf_v_rec.to_prefix,sV.station[1].prefix);
	copy_str(dbf_v_rec.to_name,sV.station[1].label);

	copy_flt(dbf_v_rec.ctr_x,(sV.station[1].xyz[0]+sV.station[0].xyz[0])/2.0,SHP_LEN_FLTXY,bUTM?SHP_DEC_FLTXY:SHP_DEC_FLTXY_G);
	copy_flt(dbf_v_rec.ctr_y,(sV.station[1].xyz[1]+sV.station[0].xyz[1])/2.0,SHP_LEN_FLTXY,bUTM?SHP_DEC_FLTXY:SHP_DEC_FLTXY_G);
	copy_flt(dbf_v_rec.ctr_z,(sV.station[1].xyz[2]+sV.station[0].xyz[2])/2.0,SHP_LEN_FLTZ,SHP_DEC_FLTZ);

	get_rav(sV.station[1].xyz[0]-sV.station[0].xyz[0],sV.station[1].xyz[1]-sV.station[0].xyz[1],
		sV.station[1].xyz[2]-sV.station[0].xyz[2]);
	copy_flt(dbf_v_rec.length,rav.r,SHP_LEN_FLTZ,SHP_DEC_FLTZ);
	copy_flt(dbf_v_rec.azimuth,rav.a,SHP_LEN_FLTAZIMUTH,SHP_DEC_FLTAZIMUTH);
	copy_flt(dbf_v_rec.incline,rav.v,SHP_LEN_FLTINCLINE,SHP_DEC_FLTINCLINE);

	copy_str(dbf_v_rec.linetype,sV.linetype);
	copy_str(dbf_v_rec.date,sV.date);

	copy_str(dbf_v_rec.srv_name,sV.filename);
	copy_str(dbf_v_rec.srv_title,sV.title);
}

static void CopyStation(DBF_S_REC *prec,SHP_TYP_STATION *pS,BOOL bNote)
{
	numrecs++;
	pt_rec.recno=SWAPLONG(numrecs);
	shx_rec.offset=SWAPLONG(offset);

	if(b3D) {
		double z=pS->xyz[2];
		if(!bUTM) {
			if(!fLenRatio) {
				fLenRatio=fabs(pS->xyz[1]); //lat
				if(fLenRatio<1.0) fLenRatio=1;
				fLenRatio=1.0/(cos(U_DEGREES*fLenRatio)*111319.9);
			}
			z*=fLenRatio;
		}
		offset+=sizeof(SHP_PTZ_REC)/2;
		pt_rec.z=z;
		pt_rec.m=DBL_MIN;
		if(main_hdr.zmin>z) main_hdr.zmin=z;
		if(main_hdr.zmax<z) main_hdr.zmax=z;
	}
	else offset+=sizeof(SHP_PT_REC)/2;

	copy_str(prec->prefix,pS->prefix);
	copy_str(prec->name,pS->label);
	copy_flt(prec->x,pS->xyz[0],SHP_LEN_FLTXY,bUTM?SHP_DEC_FLTXY:SHP_DEC_FLTXY_G);
	copy_flt(prec->y,pS->xyz[1],SHP_LEN_FLTXY,bUTM?SHP_DEC_FLTXY:SHP_DEC_FLTXY_G);
	copy_flt(prec->z,pS->xyz[2],SHP_LEN_FLTZ,SHP_DEC_FLTZ);

	if(!bNote) {
		copy_flt(prec->left,pS->lrud[0],SHP_LEN_LRUD,SHP_DEC_LRUD);
		copy_flt(prec->right,pS->lrud[1],SHP_LEN_LRUD,SHP_DEC_LRUD);
		copy_flt(prec->up,pS->lrud[2],SHP_LEN_LRUD,SHP_DEC_LRUD);
		copy_flt(prec->down,pS->lrud[3],SHP_LEN_LRUD,SHP_DEC_LRUD);
		copy_flt(prec->lrud_az,pS->lrud[4],SHP_LEN_LRUD,SHP_DEC_LRUD);
	}
	UpdateBoxPt(&main_hdr.xy_box,pt_rec.x=pS->xyz[0],pt_rec.y=pS->xyz[1]);
}

static int GetNoteData(int typ)
{
	int e;

	if(e=InitFiles(bUTM?n_fld:n_fld_g,N_NUMFLDS)) return e;

	pGetData(typ,NULL);
	while(pGetData(typ,&sN)) {

		memset(&dbf_n_rec,' ',sizeof(DBF_N_REC));
		CopyStation((DBF_S_REC *)&dbf_n_rec,(SHP_TYP_STATION *)&sN,TRUE);
		copy_str(dbf_n_rec.note,sN.note);

		if(fwrite(&pt_rec,b3D?sizeof(SHP_PTZ_REC):sizeof(SHP_PT_REC),1,fpS)!=1) return errexit(SHP_ERR_WRITE,ERR_SHP);
		if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX)!=1) return errexit(SHP_ERR_WRITE,ERR_SHX);

		if(e=Append_db(&dbf_n_rec)) return e;
	}
	e=CloseFiles();
	return e;
}

//=============================================================

static int copy_groupname(char **pp,char *pNam,int spc)
{
	char *p=pNam+SHP_LEN_MASKNAME;

	while(p>pNam && p[-1]==' ') p--;
	int len=p-pNam;
	if(len>spc) len=spc;
	memcpy(*pp,pNam,len);
	(*pp)+=len;
	return spc-len;
}

static int append_groupname(char **pp,SHP_MASKNAME *pNam,int spc)
{
	if(pNam->iParent>0) {
		spc=append_groupname(pp,&sP.pName[pNam->iParent],spc);
		if(spc<2) return spc;
		**pp='|';
		(*pp)++;
		spc--;
	}
	return spc=copy_groupname(pp,pNam->name,spc);
}

static void GetPolyAttributes(void)
{
	char hclr[8];
	memset(&dbf_w_rec,' ',sizeof(dbf_w_rec));
	memcpy(dbf_w_rec.svgname,sP.pName->name,SHP_LEN_SVGNAME);
	copy_int(dbf_w_rec.polygons,sP.nPolygons,SHP_LEN_POLYGONS);
	copy_flt(dbf_w_rec.sqmeters,fArea,SHP_LEN_SQMETERS,SHP_DEC_SQMETERS);
	if(sP.clrFill==(COLORREF)-1) memcpy(dbf_w_rec.fillcolor,"NOFILL",6);
	else {
		sprintf(hclr,"%02X%02X%02X",GetRValue(sP.clrFill),GetGValue(sP.clrFill),GetBValue(sP.clrFill));
		memcpy(dbf_w_rec.fillcolor,hclr,SHP_LEN_FILLCOLOR);
	}
	char *p=dbf_w_rec.groupname;
	append_groupname(&p,&sP.pName[sP.iGroupName],SHP_LEN_MASKNAME);
}
/*
static void OffsetPolyBox(SHP_BOX *box)
{
	box->xmax=box->xmax+sP.datumE;
	box->xmin=box->xmin+sP.datumE;
	box->ymax=box->ymax+sP.datumN;
	box->ymin=box->ymin+sP.datumN;
}
*/
static void ReversePath(POINT *pPnt,long nPnt)
{
	POINT *pLst=pPnt+(nPnt-1);
	POINT temp;
    for(;nPnt>1;nPnt-=2) {
		temp=*pPnt;
		*pPnt++=*pLst;
		*pLst--=temp;
	}
}

static void	UpdateBoxPtSVG(SHP_BOX *box,double x,double y)
{
	CFltPoint pt(x+sP.datumE,y+sP.datumN);
	if(!bUTM)
		pGetData(SHP_UTM2LL,&pt.x);
	UpdateBoxPt(box,pt.x,pt.y);
}

static BOOL IsClockwise(POINT *pPnt,long nPnt)
{
	long n;
	double x,x0,y0,y,sum;
	double scale=sP.scale;

	//Compute sum = 2 * polygon area.

	x0=(double)pPnt->x*scale;
	y0=(double)pPnt->y*scale;
	sum=((double)pPnt[nPnt-1].x*y0 - x0*(double)pPnt[nPnt-1].y)*scale;

	UpdateBoxPtSVG(&pp_rec->xy_box,x0,y0);

	for(n=1;n<nPnt;n++) {
		x=(double)pPnt[n].x*scale;
		y=(double)pPnt[n].y*scale;
		sum += (x0*y-x*y0);
		x0=x; y0=y;
		UpdateBoxPtSVG(&pp_rec->xy_box,x0,y0);
	}
	fArea-=fabs(0.5*sum);
	return (sum<0.0);
}

static BOOL WritePoint(POINT *pt)
{
	CFltPoint dp(pt->x*sP.scale+sP.datumE,pt->y*sP.scale+sP.datumN);
	if(!bUTM)
		pGetData(SHP_UTM2LL,&dp.x);
	return fwrite(&dp,sizeof(dp),1,fpS)==1;
}

static BOOL WritePolyPts(POINT *pt,int n)
{
	POINT *pt0=pt;

	for(;n;n--,pt++) {
		if(!WritePoint(pt)) return FALSE;
	}
	return WritePoint(pt0);
}

static int write_wall_shp()
{
	int e;
	long reclen,numpoints,nPnt;
	NTW_POLYGON *pPly;
	POINT *pPnt;

	if(e=InitFiles(w_fld,W_NUMFLDS)) return e;

	while(!(e=pGetData(SHP_WALLS,&sP))) {
		free(pp_rec);
		if(!(pp_rec=(SHP_POLY_REC *)malloc(reclen=sizeof(SHP_POLY_REC)+(sP.nPolygons-1)*sizeof(long))))
			return errexit(SHP_ERR_NOMEM,0);
		pp_rec->typ=SHP_TYPE_POLYGON;
		numrecs++;
		pp_rec->recno=SWAPLONG(numrecs);
		pp_rec->numparts=sP.nPolygons;

		//Pass through points to calculate record header values and
		//to reverse point array if required.
		numpoints=0;
		fArea=0.0;
		pPly=sP.pPolygons;
		pPnt=sP.pPoints;
		InitBox(&pp_rec->xy_box);

		for(e=0;e<sP.nPolygons;e++) {
			pp_rec->parts[e]=numpoints;
			nPnt=(pPly->flgcnt&NTW_FLG_MASK);
			numpoints+=(nPnt+1);
			//Update ranges and check path direction --
			if(IsClockwise(pPnt,nPnt)!=(e==0)) ReversePath(pPnt,nPnt);
			if(!e) fArea=-fArea; //Outer polygon's area is made positive
			pPnt+=nPnt;
			pPly++;
		}
		e=reclen/2-sizeof(long)+numpoints*sizeof(double);
		pp_rec->reclen=shx_rec.reclen=SWAPLONG(e);
		pp_rec->numpoints=numpoints;
		shx_rec.offset=SWAPLONG(offset);
		offset+=(reclen/2+numpoints*sizeof(double));

		//OffsetPolyBox(&pp_rec->xy_box);
		UpdateHdrBox(&pp_rec->xy_box);

		if(fwrite(pp_rec,reclen,1,fpS)!=1) return errexit(SHP_ERR_WRITE,ERR_SHP);

		//Write record contents on second pass through points.
		pPly=sP.pPolygons;
		pPnt=sP.pPoints;
		for(e=0;e<sP.nPolygons;e++) {
			nPnt=(pPly->flgcnt&NTW_FLG_MASK);
			if(!WritePolyPts(pPnt,nPnt)) return errexit(SHP_ERR_WRITE,ERR_SHP);
			pPnt+=nPnt;
			pPly++;
		}
		if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX)!=1) return errexit(SHP_ERR_WRITE,ERR_SHX);
		fAreaTotal+=fArea;
		nPolyTotal++;
		GetPolyAttributes();
		if(e=Append_db(&dbf_w_rec)) return e;
	}

	if(e!=SHP_ERR_FINISHED) return errexit(e,ERR_SHP);

	free(pp_rec); pp_rec=NULL;
	return CloseFiles();
}

void GetLrudPolyAttributes(SHP_TYP_LRUDPOLY *pLP)
{
	LPSTR pElev=GetFloatStr(pLP->elev,2);
	int len=strlen(pElev);
	memset(&dbf_lrud_rec,' ',sizeof(DBF_LRUD_REC));
	if(len<=SHP_LEN_ELEVATION)
		memcpy(dbf_lrud_rec.elevation+(SHP_LEN_ELEVATION-len),pElev,len);
}

void UpdateLrudBox(SHP_BOX *box,SHP_TYP_LRUDPOLY *pLP)
{
	long n;
	double x,x0,y0,y,sum;

	//Compute sum = 2 * polygon area.

	CFltPoint *fpt=pLP->fpt;
	long nPnt=pLP->nPts;

	x0=fpt->x;
	y0=fpt->y;
	sum=fpt[nPnt-1].x*y0 - x0*fpt[nPnt-1].y;

	UpdateBoxPt(box,x0,y0);

	for(n=1;n<nPnt;n++) {
		x=fpt[n].x;
		y=fpt[n].y;
		sum += (x0*y-x*y0);
		x0=x; y0=y;
		UpdateBoxPt(box,x0,y0);
	}
	if(sum>=0.0) {
		//reverse points!
		CFltPoint *pfpt=&fpt[nPnt-1];
		CFltPoint ptSave;
		while(++fpt<pfpt) {
			ptSave=*fpt;
			*fpt=*pfpt;
			*pfpt--=ptSave;
		}
	}
	fArea=fabs(0.5*sum);
	fAreaTotal+=fArea;
	nPolyTotal++;
}

static apfcn_i OutLrudPoly(SHP_TYP_LRUDPOLY *pLP)
{
	SHP_POLY_REC p_rec;
	numrecs++;
	p_rec.recno=SWAPLONG(numrecs);
	p_rec.typ=SHP_TYPE_POLYGON;
	p_rec.numparts=1;
	p_rec.numpoints=pLP->nPts+1;
	p_rec.parts[0]=0;

	InitBox(&p_rec.xy_box);

	//Update ranges and check path direction --
	UpdateLrudBox(&p_rec.xy_box,pLP);
	UpdateHdrBox(&p_rec.xy_box); //added 8/28/08!!

	//Total record length in words (LRUD polys have no holes, thus 1 part) --
	int reclen=(sizeof(SHP_POLY_REC)+p_rec.numpoints*sizeof(CFltPoint))/2;

	p_rec.reclen=shx_rec.reclen=SWAPLONG(reclen-4); //added -4 8/29/08
	shx_rec.offset=SWAPLONG(offset);
	offset+=reclen;

	if(fwrite(&p_rec,sizeof(SHP_POLY_REC),1,fpS)!=1 ||
		fwrite(pLP->fpt,sizeof(CFltPoint)*pLP->nPts,1,fpS)!=1 ||
		fwrite(pLP->fpt,sizeof(CFltPoint),1,fpS)!=1)
	   return errexit(SHP_ERR_WRITE,ERR_SHP);

	if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX)!=1) return errexit(SHP_ERR_WRITE,ERR_SHX);

	GetLrudPolyAttributes(pLP);
	if(reclen=Append_db(&dbf_lrud_rec)) {
		return reclen;
	}
	return 0;
}

static int write_lrud_shp()
{
	int e;
	if(e=InitFiles(lrud_fld,LRUD_NUMFLDS)) return e;
	if((e=pGetData(SHP_WALLS,(LPVOID)OutLrudPoly))) return errexit(e,ERR_SHP);
	return CloseFiles();
}

//=====================================================================================================
//Main entry point --
DLLExportC int PASCAL Export(LPCSTR lpPathname,LPCSTR lpBasename,UINT flags,LPFN_EXPORT_CB wall_GetData)
{
	int e;
	
	pGetData=wall_GetData;
	pPrjRef=NULL;
	if(wall_GetData(SHP_VERSION,&pPrjRef)!=EXP_SHP_VERSION) return SHP_ERR_VERSION;

    pBaseExt=strcpy(pathname,lpPathname)+strlen(lpPathname);
	*pBaseExt++='\\';
	_strlwr(strcpy(pBaseExt,lpBasename));
    pBaseExt+=strlen(lpBasename);
	*pBaseExt++='_';

	b3D=(flags&SHP_3D)!=0;
	bUTM=(flags&SHP_UTM)!=0;
	fLenRatio=bUTM?1.0:0.0;
	pp_rec=NULL;
	if(flags&SHP_WALLS) {
		if((e=pGetData(SHP_WALLS,NULL))>0) return errexit(e,0);
		shp_pfx='w';
		fAreaTotal=0.0;
		nPolyTotal=0;
		e=e?write_lrud_shp():write_wall_shp();
		if(e) return e;
	}

	if(flags&SHP_VECTORS) {
		shp_pfx='v';
		if(e=InitFiles(bUTM?v_fld:v_fld_g,V_NUMFLDS)) return e;
		if(b3D) {
			v_rec.reclen=shx_rec.reclen=SWAPLONG(sizeof(SHP_POLYZ1)/2);
			v_rec.typ=SHP_TYPE_POLYLINEZ;
			v_rec.m[0]=v_rec.m[1]=v_rec.m[2]=v_rec.m[3]=DBL_MIN;
		}
		else {
			v_rec.reclen=shx_rec.reclen=SWAPLONG(sizeof(SHP_POLY1)/2);
			v_rec.typ=SHP_TYPE_POLYLINE;
		}
		v_rec.numparts=1;
		v_rec.numpoints=2;
		v_rec.parts[0]=0;
		sV.titles=sV.attributes=NULL; //Used only in WALLSHPX.DLL
		pGetData(SHP_VECTORS,NULL);
		while(pGetData(SHP_VECTORS,&sV)) {
			numrecs++;
			v_rec.recno=SWAPLONG(numrecs);
			shx_rec.offset=SWAPLONG(offset);

			if(b3D) {
				if(!bUTM) {
					if(!fLenRatio) {
						fLenRatio=fabs(sV.station[0].xyz[1]); //lat
						if(fLenRatio<1.0) fLenRatio=1;
						fLenRatio=1.0/(cos(U_DEGREES*fLenRatio)*111319.9);
					}
				}
				offset+=sizeof(SHP_VECZ_REC)/2;
				v_rec.z[0]=sV.station[0].xyz[2]*fLenRatio;
				v_rec.zmin=v_rec.zmax=v_rec.z[1]=sV.station[1].xyz[2]*fLenRatio;
				if(v_rec.zmin>v_rec.z[0]) v_rec.zmin=v_rec.z[0];
				if(v_rec.zmax<v_rec.z[0]) v_rec.zmax=v_rec.z[0];
				if(main_hdr.zmin>v_rec.zmin) main_hdr.zmin=v_rec.zmin;
				if(main_hdr.zmax<v_rec.zmax) main_hdr.zmax=v_rec.zmax;
			}
			else offset+=sizeof(SHP_VEC_REC)/2;

			InitBox(&v_rec.xy_box);
			UpdateBoxPt(&v_rec.xy_box,v_rec.point0[0]=sV.station[0].xyz[0],
				v_rec.point0[1]=sV.station[0].xyz[1]);
			UpdateBoxPt(&v_rec.xy_box,v_rec.point1[0]=sV.station[1].xyz[0],
				v_rec.point1[1]=sV.station[1].xyz[1]);
			UpdateHdrBox(&v_rec.xy_box);

			if(fwrite(&v_rec,b3D?sizeof(SHP_VECZ_REC):sizeof(SHP_VEC_REC),1,fpS)!=1) return errexit(SHP_ERR_WRITE,ERR_SHP);
			if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX)!=1) return errexit(SHP_ERR_WRITE,ERR_SHX);

			GetVectorAttributes();
			if(e=Append_db(&dbf_v_rec)) return e;
		}
		if(e=CloseFiles()) return e;
	}
 	
	//Remaining shapes will be points --
	if(b3D) {
		pt_rec.reclen=shx_rec.reclen=SWAPLONG(sizeof(SHP_POINT_XYZ)/2);
		pt_rec.typ=SHP_TYPE_POINTZ;
	}
	else {
		pt_rec.reclen=shx_rec.reclen=SWAPLONG(sizeof(SHP_POINT_XY)/2);
		pt_rec.typ=SHP_TYPE_POINT;
	}

	if(flags&SHP_STATIONS) {
		shp_pfx='s';
		if(e=InitFiles(bUTM?s_fld:s_fld_g,S_NUMFLDS)) return e;

		pGetData(SHP_STATIONS,NULL);
		while(pGetData(SHP_STATIONS,&sS)) {
			memset(&dbf_s_rec,' ',sizeof(DBF_S_REC));
			CopyStation(&dbf_s_rec,&sS,FALSE);
			if(fwrite(&pt_rec,b3D?sizeof(SHP_PTZ_REC):sizeof(SHP_PT_REC),1,fpS)!=1) return errexit(SHP_ERR_WRITE,ERR_SHP);
			if(fwrite(&shx_rec,sizeof(SHX_REC),1,fpX)!=1) return errexit(SHP_ERR_WRITE,ERR_SHX);

			if(e=Append_db(&dbf_s_rec)) return e;
		}
		if(e=CloseFiles()) return e;
	}

 	if(flags&SHP_FLAGS) {
		shp_pfx='f';
		n_fld[N_NUMFLDS-1]=flag_fld;
		if(e=GetNoteData(SHP_FLAGS)) return e;
	}

 	if(flags&SHP_NOTES) {
		shp_pfx='n';
		n_fld[N_NUMFLDS-1]=note_fld;
		if(e=GetNoteData(SHP_NOTES)) return e;
	}

	return 0;
}
