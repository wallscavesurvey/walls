//WALLSHP.DLL functions --

#ifndef __WALL_SHP_H
#define __WALL_SHP_H 

#ifdef __cplusplus
extern "C" {
#endif

#define EXP_SVGDLL_NAME "WALLSVG.DLL"
#define EXP_SHPDLL_NAME "WALLSHP.DLL"

#ifdef _USE_SHPX
#define EXP_SHPXDLL_NAME "WALLSHPX.DLL"
#endif

#define EXP_SHPDLL_EXPORT "_Export@16"
#define EXP_SVGDLL_EXPORT "_Export@8"
#define EXP_DLL_ERRMSG "_ErrMsg@4"
#define EXP_SHP_VERSION 7
#define EXP_SVG_VERSION 121
#define EXP_SVG_VERSION_STR "1.21"

typedef int (FAR PASCAL *LPFN_EXPORT_CB)(int typ,LPVOID pData);
typedef int (FAR PASCAL *LPFN_SHP_EXPORT)(LPCSTR,LPCSTR,UINT,LPFN_EXPORT_CB);
typedef int (FAR PASCAL *LPFN_SVG_EXPORT)(UINT,LPFN_EXPORT_CB);
typedef LPSTR (FAR PASCAL *LPFN_ERRMSG)(int code);

enum e_shptyp {SHP_VECTORS=1,SHP_STATIONS=2,SHP_NOTES=4,
			   SHP_FLAGS=8,SHP_VERSION=16,SHP_WALLSHPX=32,SHP_FLAGSTYLE=64,
			   SHP_FLAGLOCATION=65,SHP_NOTELOCATION=66,SHP_STATUSMSG=67,SHP_ORPHAN1=68,SHP_ORPHAN2=69,
			   SHP_WALLS=128,SHP_3D=256,SHP_LRUDS=512,SHP_UTM=1024,SHP_UTM2LL=2048};

enum e_svgflags {SVG_GRID=1,SVG_VECTORS=2,SVG_FLAGS=4,SVG_LABELS=8,SVG_NOTES=16,
				 SVG_WALLS=(1<<5),SVG_DETAIL=(1<<6),SVG_LEGEND=(1<<7),SVG_IMAGE=(1<<8),
				 SVG_ADJUSTABLE=(1<<12) /*4096*/,SVG_USESTYLES=(1<<13),SVG_FEET=(1<<14),SVG_SHOWPREFIX=(1<<15),
				 SVG_EXISTS=(1<<27),SVG_UPDATED=(1<<28),SVG_ADDITIONS=(1<<29),
				 SVG_EXISTADJUSTABLE=(1<<30),SVG_WRITENTW=(1<<31)};

#define SVG_MERGEDLAYERS (SVG_WALLS+SVG_DETAIL+SVG_LEGEND) //1<<5,6,7
#define SVG_MERGEDCONTENT ((SVG_MERGEDLAYERS<<16)+SVG_MERGEDLAYERS) //1<<5,6,7,21,22,23

#define SVG_CHK3OPTS (SVG_GRID+SVG_VECTORS+SVG_FLAGS+SVG_LABELS+SVG_NOTES+SVG_MERGEDLAYERS) //1<<0,1,2,3,4,5,6,7
#define SVG_DFLTOPTS (SVG_GRID+SVG_VECTORS+SVG_FLAGS+SVG_LABELS+SVG_NOTES)
#define SVG_SAVEDFLAGMASK (SVG_CHK3OPTS+(SVG_CHK3OPTS<<16)+SVG_ADJUSTABLE+SVG_USESTYLES)

//SVG error codes returned from wallsvg.dll --
enum {
  SVG_ERR_SUCCESS,
  SVG_ERR_ABORTED,
  SVG_ERR_SVGCREATE,
  SVG_ERR_WRITE,
  SVG_ERR_XMLREAD,
  SVG_ERR_XMLPARSE,
  SVG_ERR_VERSION,
  SVG_ERR_NOMEM,
  SVG_ERR_INDEXINIT,
  SVG_ERR_INDEXWRITE,
  SVG_ERR_NOVECTORS,
  SVG_ERR_ADJUSTABLE,
  SVG_ERR_MRGOPEN,
  SVG_ERR_BADIDSTR,
  SVG_ERR_VECNOTFOUND,
  SVG_ERR_VECPATH,
  SVG_ERR_VECDIRECTION,
  SVG_ERR_WALLPATH,
  SVG_ERR_REFFORMAT,
  SVG_ERR_SIZREFBUF,
  SVG_ERR_NOREF,
  SVG_ERR_NOTEXT,
  SVG_ERR_NOPATH,
  SVG_ERR_LAYERS,
  SVG_ERR_BADELEMENT,
  SVG_ERR_TSPAN,
  SVG_ERR_NOMASK,
  SVG_ERR_FLATTEN,
  SVG_ERR_MASKPATHSOPEN,
  SVG_ERR_MASKNONPATH,
  SVG_ERR_GROUPLVL,
  SVG_ERR_W2DNAME,
  SVG_ERR_LINEDATA,
  SVG_ERR_RECTDATA,
  SVG_ERR_ELLIPSE,
  SVG_ERR_POLYDATA,
  SVG_ERR_BADXFORM,
  SVG_ERR_ROTXFORM,
  SVG_ERR_GRPXFORM,
  SVG_ERR_FINALMRG,
  SVG_ERR_W2DGROUP,
  SVG_ERR_W2DDUP,
  SVG_ERR_UNKNOWN,
  SVG_ERR_ENDSCAN
};

//SVG advanced dialog flags --
#define SVG_ADV_USELABELPOS 2
#define SVG_ADV_LRUDBARS 4
#define SVG_ADV_LRUDBOXES 8
#define SVG_ADV_USELINEWIDTHS 16

#define SVG_USELABELPOS_DFLT TRUE
#define SVG_USELINEWIDTHS_DFLT FALSE
#define SVG_ADV_FLAGS_DFLT SVG_ADV_USELABELPOS

#define SVG_DECIMALS_DFLT 3
#define SVG_VECTOR_CNT_DFLT 6
#define SVG_DISTANCE_EXP_DFLT "2"
#define SVG_DISTANCE_OFF_DFLT "0.01"
#define SVG_LENGTH_EXP_DFLT "0.5"
#define SVG_MAXCOORDCHG_DFLT "0.3"
#define SVG_LENGTH_MIN_DFLT "1"

struct STRING_BUF {
	enum {MAX_CNT=5000,INC_SIZ=512};
	STRING_BUF() : pbuf(NULL),len(0),cnt(0),siz(0),bOverflow(0) {};
	~STRING_BUF() {free(pbuf);}
	void Clear() {free(pbuf); pbuf=NULL; len=cnt=siz=bOverflow=0;}
	int Add(LPCSTR str)
	{
		if(cnt<MAX_CNT) {
			UINT l=strlen(str);
			if(len+l>=siz) {
				pbuf=(char *)realloc(pbuf,siz+=INC_SIZ);
			}
			strcpy(pbuf+len,str);
			len+=(l+1);
			cnt++;
			return 1;
		}
		bOverflow=1;
		return 0;
	}
	char *pbuf;
	UINT len;
	UINT cnt;
	UINT siz;
	UINT bOverflow;
};

typedef STRING_BUF * PSTRING_BUF;
typedef PSTRING_BUF (FAR PASCAL *LPFN_SVG_GETORPHANS)();

#define NTW_FLG_OPEN (1<<31)
#define NTW_FLG_FLOOR (1<<30)
#define NTW_FLG_BACKGROUND (1<<29)
#define NTW_FLG_NOFILL (1<<28)
#define NTW_FLG_OUTER (1<<27)
#define NTW_FLG_MASK 0xFFFFFF

#pragma pack(1)
typedef struct {
	double scale;   //World meters per file unit = 1 / (mrgScale*1000)
	double datumE;  //East coordinate of file point (0,0)
	double datumN;  //North coordinate of file point (0,0)
	int nPoints;    //Total number of POINTs in polygons (pt array follows this header in file)
	int nPolygons;	//Number of polygons. polygon point count array follows pt array
	int nPolyRecs;  //Number of NTW_RECORDs (outer polygon followed by inner polygons)
	int nMaskName;	//Length of SHP_MASKNAME array
	int minE;		//Point range in file units --
	int minN;
	int maxE;
	int maxN;
} NTW_HEADER;

typedef struct {
	int flgcnt;
	COLORREF clr;
} NTW_POLYGON;

#define SHP_LEN_MASKNAME 40
typedef struct {
	int iPly;  //index to NTW_POLYGON array
	char iNam; //index to SHP_MASKNAME array
} NTW_RECORD;

typedef struct {
	int iParent; //index to parent name in this array
	char name[SHP_LEN_MASKNAME];
} SHP_MASKNAME;

#pragma pack()

enum {
  SHP_GET_NTWSTATS,
  SHP_ERR_LOADNTW,
  SHP_ERR_NOMEM,
  SHP_ERR_VERSION,
  SHP_ERR_ABORTED,
  SHP_ERR_WRITE,
  SHP_ERR_CREATE,
  SHP_ERR_FINISHED,

  DEF_ERR_FLDTYPE,
  DEF_ERR_FLDLEN,
  DEF_ERR_DECCNT,
  DEF_ERR_VALSIZ,
  DEF_ERR_VALBUFSIZ,
  DEF_ERR_COLON,
  DEF_ERR_EMPTY,
  SHP_ERR_NODEF
};

typedef struct {
	WORD vecs_used;
	WORD decimals;
	float dist_exp;
	float dist_off;
	float len_exp;
	float len_min;
	float maxCoordChg;
	UINT  uFlags;
} SVG_ADV_PARAMS;

#define SIZ_DIFF_IDSTR 80

struct SVG_VIEWFORMAT {
	double view;   /* plan orientation in degrees*/
	double width;  /* frame width in points*/
	double height; /* frame height in points*/
	double scale;  /* scale*meters=points */
	double centerEast; /* metric coordinates of frame center*/
	double centerNorth;
	double gridint;  /*grid interval in survey units (feet or meters)*/
	int gridsub;  /*number of grid subintervals*/
	float labelSize; /*points*/
	float offLabelX; /*points*/
	float offLabelY; /*points*/
	float noteSize; /*points*/
	float titleSize; /*points*/
	float frameThick; /*points - dots in Options dialog?*/
	COLORREF backColor;
	COLORREF passageColor;
	COLORREF noteColor;
	COLORREF markerColor;
	COLORREF labelColor;
	COLORREF vectorColor;
	COLORREF gridColor;
	COLORREF lrudColor;
	const char *title; /*User can change this*/
	/*Pathname for generated SVG file --*/
	const char *svgpath;
	/*Custom project-specific symbols --*/
	//const char *sympath; /*<proj dir>\<projname>-sym.svg*/
	/*Paths for merged content --*/
	const char *mrgpath;
	const char *mrgpath2;
	UINT version;	/*version number stored in existing file (*100)*/
	UINT flags;		/*wallsvg processing instructions*/
	SVG_ADV_PARAMS advParams;
	float fMaxDiff;  /*max coord chg*/
	float flength_w;
	float flength_d;
	UINT tpoints_w;
	UINT tpoints_d;
	char max_diff_idstr[SIZ_DIFF_IDSTR];
	float fMaxDiff2;  /*max coord chg*/
	float flength_w2;
	float flength_d2;
	UINT tpoints_w2;
	UINT tpoints_d2;
	char max_diff_idstr2[SIZ_DIFF_IDSTR];
};

#define SHP_LEN_TITLE 48
#define SHP_LEN_NOTE 64
#define SHP_LEN_FLAGNAME 64
#define SHP_LEN_FILENAME 8
#define SHP_LEN_LABEL 8
#define SHP_LEN_PREFIX 8
#define SHP_LEN_LINETYPE 8
#define SHP_LEN_FILLCOLOR 6
#define SHP_LEN_POLYGONS 6
#define SHP_LEN_DATE 8
#define SHP_SIZ_BUF_TITLES 512
#define SHP_SIZ_BUF_ATTRIBUTES 512

struct CFltPoint {
	CFltPoint() : x(0),y(0) {}
	CFltPoint(double x0,double y0) : x(x0), y(y0) {}
	double x;
	double y;
};

typedef struct {
	double xyz[3];
	UINT id,pfx_id;
	char prefix[SHP_LEN_PREFIX+2];
	char label[SHP_LEN_LABEL+2];
	float lrud[5];
} SHP_TYP_STATION;

#ifndef LRUD_FLG_CS
#define LRUD_FLG_CS 1
#endif

typedef struct {
	double xyFr[2];
	double xyTo[2];
	double up,dn;
	byte flags;
} SHP_TYP_LRUD;

typedef struct {
	SHP_TYP_STATION station[2]; 
	UINT seg_id;
	char date[SHP_LEN_DATE+2]; //"YYYYMMDD"
	char linetype[SHP_LEN_LINETYPE+2]; //"RRGGBBxx"
	char filename[SHP_LEN_FILENAME+2];
	char title[SHP_LEN_TITLE+2];
	char *attributes;
	char *titles;
} SHP_TYP_VECTOR;

typedef struct {
	double scale;   //World meters per file unit = 1 / (mrgScale*1000)
	double datumE;  //East coordinate of file point (0,0)
	double datumN;  //North coordinate of file point (0,0)
	COLORREF clrFill;
	int nPolygons;
	POINT *pPoints;
	NTW_POLYGON *pPolygons; //*pPolygon is first (outer) of set of nPolygons
	SHP_MASKNAME *pName;	//first element is SVG file name
	int iGroupName;			//index in pName[] of group name for *pPolygon
} SHP_TYP_POLYGON;

#define MAX_LRUDPOLY_PTS 200

typedef struct {
	int nPts;
	double elev;
	CFltPoint fpt[MAX_LRUDPOLY_PTS];
} SHP_TYP_LRUDPOLY;

typedef int (FAR PASCAL * LRUDPOLY_CB)(SHP_TYP_LRUDPOLY *pLP);

typedef struct {
	SHP_TYP_STATION station;
	char note[SHP_LEN_NOTE+2];
} SHP_TYP_NOTE;

typedef struct {
	SHP_TYP_STATION station;
	char flagname[SHP_LEN_FLAGNAME+2];
} SHP_TYP_FLAG;

typedef struct {
	double xyz[3];
	UINT pfx_id;
	char prefix[SHP_LEN_PREFIX+2];
	char label[SHP_LEN_LABEL+2];
	UINT flag_id;
} SHP_TYP_FLAGLOCATION;

typedef struct {
	double xyz[3];
	UINT pfx_id;
	char prefix[SHP_LEN_PREFIX+2];
	char label[SHP_LEN_LABEL+2];
	char note[SHP_LEN_NOTE+2];
} SHP_TYP_NOTELOCATION;

typedef struct {
	WORD id;		/*flag ID (order in SRV file, 0=first)*/
	WORD wPriority; /*0=highest*/
	WORD wSize;		/*size in points*/
	BYTE bShape;	/*0-squares,1-circles,2-triangles*/
	BYTE bShade;	/*0-clear,1-solid,2-opaque*/
	DWORD color;
	char flagname[SHP_LEN_FLAGNAME+2];
} SHP_TYP_FLAGSTYLE;

class CDialog;

#define PID_RECMASK 0x7fffffff

class CExportShp {
	UINT rec,firstrec,lastrec,lastprec,len_pid;
	int next,end_id;
	int counter;
	int *pid;
	BOOL bInitStations,bInitVectors,bInitNotes,bInitPolygons,bSVGFloors;
	
public:
	DWORD *pFlagNameRec;
	BYTE *pFlagBits;
	int shp_type;
	CDialog *pDlg; //for status messages
	SHP_TYP_STATION fr_S;
	NTW_RECORD *pPolyRec;
	POINT *pPolyPnt;
	enum e_FB {FB_HIDDEN=1,FB_FLAGGED=2};
	
    ~CExportShp();
	CExportShp();	
	BOOL Init(UINT maxids);
	int GetLrud(SHP_TYP_LRUD *pL);
	int GetStation(SHP_TYP_STATION *pS,BOOL bNeedNames=FALSE);
	BOOL VecIsVertical(void);
	BOOL GetVector(SHP_TYP_VECTOR *pV);
	int GetPolygon(SHP_TYP_POLYGON *pP);
	BOOL GetNoteOrFlag(SHP_TYP_NOTE *pN,int typ);
	int GetFlagStyle(SHP_TYP_FLAGSTYLE *pS);
	int GetFlagLocation(SHP_TYP_FLAGLOCATION *pL);
	int GetNoteLocation(SHP_TYP_NOTELOCATION *pL);
	void GetNTPRecs();
	BOOL AllocFlagBits();
	BOOL SetFlagBits();
	void GetPLTCoords(double *xyz);
	BOOL GetFlagNameRecs();
	void ShpStatusMsg(int counter);
	void SvgStatusMsg(char *msg);
	void IncPolyCnt();

private:
	void GetShpLineType(SHP_TYP_VECTOR *pV);
	void Store_fr_S(SHP_TYP_STATION *pS);
};

extern CExportShp *pExpShp; //in expshp.cpp
extern bool bUseMag;
extern char shp_pfx;

#ifdef __cplusplus
}
#endif

#endif
