// wall-shp.cpp : CSegView::ExportSHP() implementation
//

#include "stdafx.h"
#include "resource.h"
#include <trx_str.h>
#include <georef.h>
#include "XlsExportDlg.h"

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

#define SHP_DBF_BUFRECS 20
#define SHP_DBF_NUMBUFS 5

#pragma pack(1)

struct DPOINT {
	double x, y;
};

struct SHP_POINT_XY {
	long typ;			//always SHP_TYPE_POINT==3
	double x;
	double y;
};

struct SHP_POINT_XYZ {
	long typ;			//always SHP_TYPE_POINTZ==11
	double x;
	double y;
	double z;
	double m;
};

struct SHP_BOX {
	double xmin, ymin, xmax, ymax;
};

struct SHP_MAIN_HDR {
	long filecode;		//9994 (big endian)
	long unused[5];
	long filelength;	//file length in 16-bit words (big endian)
	long version;		//1000
	long shapetype;		//1-Point, 3-PolyLine, 5-Polygon, etc.
	SHP_BOX xy_box;		//extent of all (X,Y) points in file
	double zmin, zmax;	//extent of all Z
	double mmin, mmax;   //extent of all M
};

struct SHX_REC {
	long offset;		//word offset of corrsponding rec in SHP file (big endian)
	long reclen;		//reclen in SHP file (big endian)
};

struct SHP_PT_REC {
	long recno;			//record number in associated DBF (1-based, big endian)
	long reclen;        //sizeof(SHP_POINT_XY)/2 (big endian)
	long typ;			//always SHP_TYPE_POINT==1
	double x;
	double y;
};


#pragma pack()
//=======================================================
static FILE *fpS, *fpX;
static DBF_NO db;
static bool db_created, db_opened, s_opened, x_opened;
static long numrecs, offset;
static SHP_MAIN_HDR main_hdr;
static SHX_REC shx_rec;
static SHP_PT_REC pt_rec;

enum e_err { ERR_SHP = 1, ERR_SHX = 2, ERR_DBF = 3, ERR_DBT = 4 };

static CFile dbt;
static UINT dbt_nextRec;
static BYTE dbt_hdr[512];

static BOOL dbt_WriteHdr(void)
{
	//Must be called prior DBT close.
	//Assumes dbt_nextRec stores number of 512-byte blocks including header block.
	memset(dbt_hdr, 0, 512);
	*(UINT *)dbt_hdr = dbt_nextRec;
	dbt_hdr[16] = 0x03;
	try {
		dbt.SeekToBegin();
		dbt.Write(dbt_hdr, 512);
	}
	catch (...) {
		return FALSE;
	}
	return TRUE;
}

UINT dbt_AppendRec(LPCSTR pData)
{
	//Assumes dbt_nextRec stores current number of 512-byte blocks including header block.
	UINT len = strlen(pData) + 1;
	UINT recPos = dbt_nextRec;
	try {
		dbt.SeekToEnd(); //should be unnecessary
		while (len) {
			memset(dbt_hdr, 0, 512);
			if (len > 1) {
				UINT sizData = len - 1;
				if (sizData > 512) sizData = 512;
				memcpy(dbt_hdr, pData, sizData);
				pData += sizData;
				len -= sizData;
			}
			else {
				dbt_hdr[--len] = 0x1A;
			}
			dbt.Write(dbt_hdr, 512);
			dbt_nextRec++;
		}
	}
	catch (...) {
		return 0;
	}
	return recPos;
}

//=======================================================
//Local data --

void CXlsExportDlg::DeleteFiles()
{
	if (fpX) {
		if (x_opened) {
			fclose(fpX);
			fpX = NULL;
			x_opened = false;
		}
		_unlink(FixShpPath(".shx"));
	}

	if (fpS) {
		if (s_opened) {
			fclose(fpS);
			fpS = NULL;
			s_opened = false;
		}
		_unlink(FixShpPath(".shp"));
	}

	if (db_created) {
		if (db_opened) dbf_CloseDel(db);
		else {
			_unlink(FixShpPath(".dbf"));
		}
		db_created = db_opened = false;
	}

	if (dbt.m_hFile != CFile::hFileNull) {
		dbt.Abort();
		dbt.m_hFile = CFile::hFileNull;
		_unlink(FixShpPath(".dbt"));
	}
}

int CXlsExportDlg::errexit(int e, int typ)
{
	char *ext = NULL;

	switch (typ) {
	case ERR_SHP:
	case ERR_SHX: ext = ".shp"; break;
	case ERR_DBF: ext = ".dbf"; break;
	case ERR_DBT: ext = ".dbt"; break;
	}

	DeleteFiles();

	if (ext) FixShpPath(ext);
	return e;
}

static void UpdateHdrBox(SHP_BOX *box)
{
	if (box->xmin < main_hdr.xy_box.xmin) main_hdr.xy_box.xmin = box->xmin;
	if (box->ymin < main_hdr.xy_box.ymin) main_hdr.xy_box.ymin = box->ymin;
	if (box->xmax > main_hdr.xy_box.xmax) main_hdr.xy_box.xmax = box->xmax;
	if (box->ymax > main_hdr.xy_box.ymax) main_hdr.xy_box.ymax = box->ymax;
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

static void InitBox(SHP_BOX *box)
{
	box->xmin = box->ymin = DBL_MAX;
	box->xmax = box->ymax = -DBL_MAX;
}

void CXlsExportDlg::WritePrjFile(BOOL bNad, int utm_zone)
{
	//bNad=0 (wgs84), 1 (nad83), 2 (nad27)
	try {
		CFile file(FixShpPath(".prj"), CFile::modeCreate | CFile::modeWrite);
		CString s;
		if (!utm_zone) {
			s.LoadString(bNad ? ((bNad == 1) ? IDS_PRJ_NAD83 : IDS_PRJ_NAD27) : IDS_PRJ_WGS84);
		}
		else {
			s.Format(bNad ? ((bNad == 1) ? IDS_PRJ_NAD83UTM : IDS_PRJ_NAD27UTM) : IDS_PRJ_WGS84UTM,
				abs(utm_zone), (utm_zone < 0) ? 'S' : 'N',
				(utm_zone < 0) ? 10000000 : 0, -183 + utm_zone * 6);
		}
		file.Write((LPCSTR)s, s.GetLength());
		file.Close();
	}
	catch (...) {
		AfxMessageBox("Note: A PRJ file could not be created.");
	}
}

int CXlsExportDlg::CloseFiles(void)
{
	int err_typ;
	double width;

	err_typ = ERR_DBF;
	db_opened = FALSE;
	if (dbf_Close(db)) goto _errWrite;
	//This also frees the DBF cache
	if (dbt.m_hFile != CFile::hFileNull) {
		err_typ = ERR_DBF;
		if (!dbt_WriteHdr()) goto _errWrite;
		dbt.Close();
	}

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

	WritePrjFile((m_iDatum != geo_WGS84_datum) ? ((m_iDatum != geo_NAD27_datum) ? 1 : 2) : 0, m_bLatLon ? 0 : m_psd->iZoneDflt);

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
int CXlsExportDlg::InitShapefile()
{
	pt_rec.reclen = shx_rec.reclen = SWAPLONG(sizeof(SHP_POINT_XY) / 2);
	pt_rec.typ = SHP_TYPE_POINT;

	int err_typ;

	fpS = fpX = NULL;
	db_created = db_opened = s_opened = x_opened = false;
	numrecs = 0;
	offset = 50;
	InitMainHdr(SHP_TYPE_POINT);

	err_typ = ERR_SHP;
	if (!(fpS = fopen(FixShpPath(".shp"), "wb"))) goto _errCreate;
	s_opened = true;
	if (fwrite(&main_hdr, sizeof(SHP_MAIN_HDR), 1, fpS) != 1) goto _errWrite;

	if (!(fpX = fopen(FixShpPath(".shx"), "wb"))) goto _errCreate;
	x_opened = true;
	if (fwrite(&main_hdr, sizeof(SHP_MAIN_HDR), 1, fpX) != 1) goto _errWrite;

	if (dbf_Create(&db, FixShpPath(".dbf"), DBF_ReadWrite, m_psd->numFlds, &m_psd->v_fldDef[0]))
		goto _errCreate;
	db_created = db_opened = true;
	if (dbf_AllocCache(db, SHP_DBF_BUFRECS*dbf_SizRec(db), SHP_DBF_NUMBUFS, TRUE))
		goto _errWrite;

	if (dbf_FileID(db) == DBF_FILEID_MEMO) {
		err_typ = ERR_DBT;
		dbt_nextRec = 1;
		if (!dbt.Open(FixShpPath(".dbt"), CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive)
			|| !dbt_WriteHdr())
			goto _errCreate;
	}

	InitBox(&main_hdr.xy_box);

	return 0;

_errCreate:
	return errexit(SHP_ERR_CREATE, err_typ);

_errWrite:
	return errexit(SHP_ERR_WRITE, err_typ);
}

int CXlsExportDlg::WriteShpRec(double x, double y)
{
	numrecs++;
	pt_rec.recno = SWAPLONG(numrecs);
	shx_rec.offset = SWAPLONG(offset);
	offset += sizeof(SHP_PT_REC) / 2;

	UpdateBoxPt(&main_hdr.xy_box, pt_rec.x = x, pt_rec.y = y);

	if (fwrite(&pt_rec, sizeof(SHP_PT_REC), 1, fpS) != 1) return errexit(SHP_ERR_WRITE, ERR_SHP);
	if (fwrite(&shx_rec, sizeof(SHX_REC), 1, fpX) != 1) return errexit(SHP_ERR_WRITE, ERR_SHX);
	if (dbf_AppendBlank(db)) return errexit(SHP_ERR_WRITE, ERR_DBF);
	return 0;
}

int CXlsExportDlg::StoreDouble(DBF_FLDDEF &fd, int fld, double d)
{
	char str[32];
	int len = sprintf(str, "%*.*f", fd.F_Len, fd.F_Dec, d);
	dbf_PutFldStr(db, str, fld + 1);
	return fd.F_Len - len;
}

void CXlsExportDlg::StoreText(int fld, LPCSTR p)
{
	VERIFY(!dbf_PutFldStr(db, p, fld + 1));
}
