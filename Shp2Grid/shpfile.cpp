// wall-shp.cpp : CSegView::ExportSHP() implementation
//

#include "stdafx.h"
#include "shpfile.h"
#include "resource.h"
//=======================================================

#pragma pack(1)

//Specific shapefile structures --

typedef struct {
	char Flag;
	char total[5];
	char caves[5];
	char cavities[5];
	char shelters[5];
	char springs[5];
} DBF_SHP_REC;

static DBF_FLDDEF n_fld[] = {
	{"TOTAL"		,'N', 5 ,0},
	{"CAVES"	    ,'N', 5 ,0},
	{"CAVITIES"		,'N', 5 ,0},
	{"SHELTERS"		,'N', 5 ,0},
	{"SPRINGS"		,'N', 5 ,0}
};

#define N_NUMFLDS (sizeof(n_fld)/sizeof(DBF_FLDDEF))

#define SHP_DBF_BUFRECS 20
#define SHP_DBF_NUMBUFS 5

//Generic shapefile structures --

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)),SWAPWORD(LOWORD(x)))

#define SHP_FILECODE 9994
#define SHP_TYPE_POINT 1
#define SHP_TYPE_POINTZ 11
#define SHP_TYPE_POLYLINE 3
#define SHP_TYPE_POLYLINEZ 13
#define SHP_TYPE_POLYGON 5

typedef struct {
	double x, y;
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
	double xmin, ymin, xmax, ymax;
} SHP_BOX;

typedef struct {
	long typ;			//always SHP_TYPE_POLYLINE==3
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	SHP_POINT_XY points[2];
} SHP_POLY1;

typedef struct {
	long typ;			//always SHP_TYPE_POLYLINEZ==13
	SHP_BOX xy_box;
	long numparts;		//always 1
	long numpoints;		//always 2
	long parts[1];		//always 0 (0-based index of first point in points[])
	SHP_POINT_XY points[2];
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
	double zmin, zmax;	//extent of all Z
	double mmin, mmax;   //extent of all M
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

static FILE *fpS, *fpX;
static DBF_NO db;
static bool db_created, db_opened, s_opened, x_opened;
static char *pBaseExt;
static long numrecs, offset;
static SHX_REC shx_rec;
static SHP_PTZ_REC pt_rec;
static SHP_MAIN_HDR main_hdr;
static DBF_SHP_REC dbf_shp_rec;
enum e_err { ERR_SHP = 1, ERR_SHX = 2, ERR_DBF = 3 };

//=======================================================
//Local data --

static char pathname[_MAX_PATH];

static char *GetPathName(char *ext)
{
	sprintf(pBaseExt, ".%s", ext);
	return pathname;
}

void WritePrjFile()
{
	try {
		CFile file(GetPathName("prj"), CFile::modeCreate | CFile::modeWrite);
		CString s;
		s.LoadString(IDS_PRJ_TSMS);
		file.Write((LPCSTR)s, s.GetLength());
		file.Close();
	}
	catch (...) {
		AfxMessageBox("Note: A PRJ file could not be created.");
	}
}

void DeleteFiles()
{
	if (fpX) {
		if (x_opened) fclose(fpX);
		_unlink(GetPathName("shx"));
		x_opened = false;
	}

	if (fpS) {
		if (s_opened) fclose(fpS);
		_unlink(GetPathName("shp"));
		s_opened = false;
	}

	if (db_created) {
		if (db_opened) dbf_CloseDel(db);
		else {
			_unlink(GetPathName("dbf"));
		}
		db_created = db_opened = false;
	}

}

static int errexit(int e, int typ)
{
	char *ext = NULL;

	switch (typ) {
	case ERR_SHP:
	case ERR_SHX: ext = "shp"; break;
	case ERR_DBF: ext = "dbf"; break;
	}

	DeleteFiles();

	if (ext) GetPathName(ext);
	return e;
}

static void InitMainHdr(int typ)
{
	memset(&main_hdr, 0, sizeof(SHP_MAIN_HDR));
	main_hdr.filecode = SWAPLONG(SHP_FILECODE);
	main_hdr.version = 1000;
	main_hdr.shapetype = typ;
}

static void UpdateBoxPt(SHP_BOX *box, double x, double y)
{
	if (x < box->xmin) box->xmin = x;
	if (y < box->ymin) box->ymin = y;
	if (x > box->xmax) box->xmax = x;
	if (y > box->ymax) box->ymax = y;
}

static void UpdateHdrBox(SHP_BOX *box)
{
	if (box->xmin < main_hdr.xy_box.xmin) main_hdr.xy_box.xmin = box->xmin;
	if (box->ymin < main_hdr.xy_box.ymin) main_hdr.xy_box.ymin = box->ymin;
	if (box->xmax > main_hdr.xy_box.xmax) main_hdr.xy_box.xmax = box->xmax;
	if (box->ymax > main_hdr.xy_box.ymax) main_hdr.xy_box.ymax = box->ymax;
}

static void InitBox(SHP_BOX *box)
{
	box->xmin = box->ymin = DBL_MAX;
	box->xmax = box->ymax = -DBL_MAX;
}

int CloseFiles(void)
{
	int err_typ;
	double width;

	err_typ = ERR_DBF;
	db_opened = FALSE;
	if (dbf_Close(db)) goto _errWrite;
	//This also frees the DBF cache

	//Expand extents box in main hdr by 2%--
	width = (main_hdr.xy_box.xmax - main_hdr.xy_box.xmin) / 100.0;
	main_hdr.xy_box.xmax += width;
	main_hdr.xy_box.xmin -= width;
	width = (main_hdr.xy_box.ymax - main_hdr.xy_box.ymin) / 100.0;
	main_hdr.xy_box.ymax += width;
	main_hdr.xy_box.ymin -= width;

	err_typ = ERR_SHX;
	numrecs = 50 + (numrecs * 4);
	main_hdr.filelength = SWAPLONG(numrecs);
	if (fseek(fpX, 0, SEEK_SET) || fwrite(&main_hdr, sizeof(SHP_MAIN_HDR), 1, fpX) != 1)
		goto _errWrite;
	x_opened = FALSE;
	if (fclose(fpX)) goto _errWrite;

	err_typ = ERR_SHP;
	main_hdr.filelength = SWAPLONG(offset);
	if (fseek(fpS, 0, SEEK_SET) || fwrite(&main_hdr, sizeof(SHP_MAIN_HDR), 1, fpS) != 1)
		goto _errWrite;
	s_opened = FALSE;
	if (fclose(fpS)) goto _errWrite;

	fpS = fpX = NULL;
	db_created = false;
	return 0;

_errWrite:
	return errexit(SHP_ERR_WRITE, err_typ);
}

//================================================================================
//Routines for record transfer --

static void copy_str(char *dst, const char *src)
{
	memcpy(dst, src, strlen(src));
}

static void copy_int(char *dst, int n, int len)
{
	char argv_buf[32];
	if (sprintf(argv_buf, "%*d", len, n) == len) memcpy(dst, argv_buf, len);
}

static void copy_flt(char *dst, double f, int len, int dec)
{
	char argv_buf[32];
	if (sprintf(argv_buf, "%*.*f", len, dec, f) == len) memcpy(dst, argv_buf, len);
}

//=====================================================================================================
int CreateFiles(LPCSTR lpPathname)
{
	pBaseExt = trx_Stpext(strcpy(pathname, lpPathname));
	pt_rec.reclen = shx_rec.reclen = SWAPLONG(sizeof(SHP_POINT_XY) / 2);
	pt_rec.typ = SHP_TYPE_POINT;

	//return InitFiles(n_fld,N_NUMFLDS);
	int err_typ;

	fpS = fpX = NULL;
	db_created = db_opened = s_opened = x_opened = false;
	numrecs = 0;
	offset = 50;
	InitMainHdr(SHP_TYPE_POINT);

	err_typ = ERR_SHP;
	if (!(fpS = fopen(GetPathName("shp"), "wb"))) goto _errCreate;
	s_opened = true;
	if (fwrite(&main_hdr, sizeof(SHP_MAIN_HDR), 1, fpS) != 1) goto _errWrite;

	err_typ = ERR_SHX;
	if (!(fpX = fopen(GetPathName("shx"), "wb"))) goto _errCreate;
	x_opened = true;
	if (fwrite(&main_hdr, sizeof(SHP_MAIN_HDR), 1, fpX) != 1) goto _errWrite;

	err_typ = ERR_DBF;
	if (dbf_Create(&db, GetPathName("dbf"), DBF_ReadWrite, N_NUMFLDS, n_fld)) goto _errCreate;
	db_created = db_opened = true;
	if (dbf_AllocCache(db, SHP_DBF_BUFRECS*dbf_SizRec(db), SHP_DBF_NUMBUFS, TRUE))
		goto _errWrite;

	InitBox(&main_hdr.xy_box);

	return 0;

_errCreate:
	return errexit(SHP_ERR_CREATE, err_typ);

_errWrite:
	return errexit(SHP_ERR_WRITE, err_typ);
}

int WriteShpRec(TRXREC &trxrec, DWORD2 &xy)
{
	numrecs++;
	pt_rec.recno = SWAPLONG(numrecs);
	shx_rec.offset = SWAPLONG(offset);
	offset += sizeof(SHP_PT_REC) / 2;

	memset(&dbf_shp_rec, ' ', sizeof(DBF_SHP_REC));
	copy_int(dbf_shp_rec.total, trxrec.nCaves + trxrec.nCavities + trxrec.nShelters + trxrec.nSprings, 4);
	copy_int(dbf_shp_rec.caves, trxrec.nCaves, 4);
	copy_int(dbf_shp_rec.cavities, trxrec.nCavities, 4);
	copy_int(dbf_shp_rec.shelters, trxrec.nShelters, 4);
	copy_int(dbf_shp_rec.springs, trxrec.nSprings, 4);

	UpdateBoxPt(&main_hdr.xy_box, pt_rec.x = xy.x, pt_rec.y = xy.y);

	if (fwrite(&pt_rec, sizeof(SHP_PT_REC), 1, fpS) != 1) return errexit(SHP_ERR_WRITE, ERR_SHP);
	if (fwrite(&shx_rec, sizeof(SHX_REC), 1, fpX) != 1) return errexit(SHP_ERR_WRITE, ERR_SHX);
	if (dbf_AppendRec(db, &dbf_shp_rec)) return errexit(SHP_ERR_WRITE, ERR_DBF);
	return 0;
}
