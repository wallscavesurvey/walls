// wallsvg.cpp : Called from CSegView::ExportSVG() in Walls v.2B7

#include <windows.h>
#include <vector>
#include <stdio.h>
#include <stdio.h>
//#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
//expat.h includes stdlib.h
#include <expat.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <sys\stat.h>
#include <time.h>
#include <ctype.h>
#include <trx_str.h>
#include <trxfile.h>
#include <zlib.h>
#include "..\wall_shp.h"

#define _USE_XMLID
#undef XMLIMPORT

static char buf_titles[SHP_SIZ_BUF_TITLES],buf_attributes[SHP_SIZ_BUF_ATTRIBUTES];

#ifndef DLLExportC
#define DLLExportC extern "C" __declspec(dllexport)
#endif

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#ifdef XMLIMPORT
#error XMLIMPORT defined
#endif
//STR(XMLIMPORT)

//=======================================================

static int errdep=0;

//Structures returned from Walls --
static SHP_TYP_VECTOR sV;
static SHP_TYP_FLAGSTYLE sFS;
//Assume  sizeof(sN) >= sizeof(sF) --
static SHP_TYP_FLAG sF;
static SVG_VIEWFORMAT *pSVG;

static double viewA,cosA,sinA,xMid,yMid,grid_xMin,grid_yMin,grid_xMax,grid_yMax,width,height,scale;
static double max_diff,max_diff_allowed;
static double max_diffx,max_diffy;
static char max_diff_idstr[SIZ_DIFF_IDSTR];

static char width_str[20],height_str[20],transformMatrix[48];
static char pathxml[MAX_PATH];
static LPSTR pnamxml;

static FILE *fpout;
static LPCSTR mrgpath;
static gzFile fpin,fpmrg,gzout;
static UINT flags, err_groupID;
static BOOL bInsideGradient;
static bool bInsidePattern;
static bool bSymbolScan,bNoOutput,bElementsSeen,bLegendSeen;
static bool bFirstCommentSeen,bWriting,bInUse;
static bool bMerge2,bMerging2,bGzout;
static BOOL bNTW;
static int bInTextElement,bInTextPath;
static double midE,midN;

#ifdef _DEBUG
static int bugcount;
#endif

static CTRXFile trx;
#define SVG_TRX_VECTORS 0
#define SVG_TRX_LABELS 1
#define MAX_SHAPEPTS 15

//===== Merged content variables =============================

//#define MAX_VEC_SKIPPED 20
#define SVG_MAX_SIZNAME 63

#pragma pack(1)
typedef struct {
	double fr_xy[2];
	double to_xy[2];
	WORD wVecColor;
} SVG_TYP_TRXVECTOR;

typedef struct {
	double fr_xy[2];
	double to_xy[2];
} SVG_TYP_TRXVECTOR_NTW;

typedef struct {
	float sx;
	float sy;
	float nx;
	float ny;
	BYTE bPresent;
} SVG_TYP_TRXLABEL;

typedef struct {
	double x;
	double y;
	char prefix[SVG_MAX_SIZNAME+1];
	char name[SVG_MAX_SIZNAME+1];
} SVG_ENDPOINT;

typedef struct {
	double x;
	double y;
} SVG_PATHPOINT;

typedef struct {
	float width;
	float height;
} SVG_VIEWBOX;

typedef struct {
	float xmin;
	float ymin;
	float xmax;
	float ymax;
	short pathstyle_idx;
	BYTE bInRange;
	BYTE bMorph;
} SVG_PATHRANGE;

typedef struct { //12*8=96 bytes (coords can be floats!) --
	double B_xy[2]; //SVG version: fr_ = B_, to_ = B_ + M_
	double M_xy[2];
	double Bn_xy[2]; //Shift parameters
	double Mn_xy[2];
	double maxdist;
	int rt,lf;
} SVG_VECNODE;
#pragma pack()

#define NUM_HDR_NODES 7
#define NUM_HDR_LEVELS 3

#define LEN_VECLST 10
#define MAX_STYLES 500	//increased from 150 for bMerging2

/*	Notes:
	Walls and Detail layers --
	In the shp layers, all path points will be warped (independently
	positioned), rotated, and scaled, while text and symbols will be
	rotated and scaled, not warped. In the sym layers, paths will be
	rotated and scaled,	while text and symbols will be scaled only.
	The final world coordinates of all elements in Walls and Details
	will depend on vector shifts. All grouped objects in the sym
	layers will be repositioned as groups. Objects in the shp
	layers will be repositioned individually as elements, except
	that path data points will be indepenently repositioned.
	
	Legend layer --
	All elements will be scaled only and will retain their world
	coodrinates.
*/

const char *st_stroke="stroke:";
const char *st_stroke_width="stroke-width:";
const char *st_stroke_dasharray="stroke-dasharray:";
const char *st_fill="fill:";
const char *st_font_size="font-size:";
const char *st_letter_spacing="letter-spacing:";

enum ew2d_flg {FLG_WALLS_SHP=1,FLG_WALLS_SYM=2,FLG_DETAIL_SHP=4,FLG_DETAIL_SYM=8,
			FLG_WALLS=32,FLG_DETAIL=64,FLG_LEGEND=128,FLG_IMAGE=256, //same as SVG_ versions
			FLG_VECTORS=(1<<11),FLG_LABELS=(1<<12),FLG_BACKGROUND=(1<<13),FLG_FRAME=(1<<14),
			FLG_GRID=(1<<15),FLG_NOTES=(1<<16),FLG_FLAGS=(1<<17),FLG_MARKERS=(1<<18),
			FLG_MASK=(1<<19),FLG_LRUDS=(1<<20),FLG_SURVEY=(1<<21),FLG_SYMBOLS=(1<<22)};

#define NUM_W2DNAMES 20
//w2d Layers examined in the merge file --
static char *w2d_name[NUM_W2DNAMES]={
	"Ref",
	"Detail",
	"Detail_shp",
	"Detail_sym",
	"Walls",
	"Walls_shp",
	"Walls_sym",
	"Legend",
	"Vectors",
	"Labels",
	"Background",
	"Frame",
	"Grid",
	"Notes",
	"Flags",
	"Markers",
	"Mask",
	"Lruds",
	"Survey",
	"Symbols"
};
static int w2d_name_len[NUM_W2DNAMES]={
	3, //"Ref",
	6, //"Detail",
	10, //"Detail_shp",
	10, //"Detail_sym",
	5, //"Walls",
	9, //"Walls_shp",
	9, //"Walls_sym",
	6, //"Legend",
	7, //"Vectors",
	6, //"Labels",
	10, //"Background",
	5, //"Frame",
	4, //"Grid",
	5, //"Notes",
	5, //"Flags",
	7, //"Markers",
	4, //"Mask",
	5, //"Lruds",
	6, //"Survey",
	7  //"Symbols"
};


enum ew2d_idx {ID_REF,ID_DETAIL,ID_DETAIL_SHP,ID_DETAIL_SYM,
			ID_WALLS,ID_WALLS_SHP,ID_WALLS_SYM,ID_LEGEND,ID_VECTORS,
			ID_LABELS,ID_BACKGROUND,ID_FRAME,ID_GRID,ID_NOTES,ID_FLAGS,
			ID_MARKERS,ID_MASK,ID_LRUDS,ID_SURVEY,ID_SYMBOLS};

static int w2d_flg[NUM_W2DNAMES]={0,FLG_DETAIL,FLG_DETAIL_SHP,FLG_DETAIL_SYM,
			FLG_WALLS,FLG_WALLS_SHP,FLG_WALLS_SYM,FLG_LEGEND,FLG_VECTORS,
			FLG_LABELS,FLG_BACKGROUND,FLG_FRAME,FLG_GRID,FLG_NOTES,FLG_FLAGS,
			FLG_MARKERS,FLG_MASK,FLG_LRUDS,FLG_SURVEY,FLG_SYMBOLS};

static BYTE w2d_name_seen[NUM_W2DNAMES];

#define FLG_MERGEDLAYERS (FLG_WALLS+FLG_DETAIL+FLG_LEGEND+FLG_MASK)
#define FLG_SHP_MSK (FLG_WALLS_SHP+FLG_DETAIL_SHP+FLG_MASK)
#define FLG_SYM_MSK (FLG_WALLS_SYM+FLG_DETAIL_SYM)
#define FLG_DETAIL_MSK (FLG_DETAIL_SHP+FLG_DETAIL_SYM)
#define FLG_WALLS_MSK (FLG_WALLS_SHP+FLG_WALLS_SYM)
#define FLG_ADJUSTED (FLG_SHP_MSK+FLG_SYM_MSK)
#define FLG_WALLS_DETAIL_MSK (FLG_DETAIL_MSK+FLG_WALLS_MSK)
#define FLG_STYLE_MSK (FLG_LABELS+FLG_BACKGROUND+FLG_FRAME+FLG_GRID+FLG_NOTES+FLG_FLAGS+FLG_MARKERS+FLG_LRUDS)

#define SIZ_REFBUF 384
static char refbuf[SIZ_REFBUF];
#define SIZ_GZBUF 1024
static char gzbuf[SIZ_GZBUF];

static NTW_HEADER ntw_hdr;
static int ntw_count,ntw_size,nMaskOpenPaths,nNTW_OpenPathLine;
static POINT *pNTWp;
static byte *pNTWf;
static NTW_POLYGON *pnPly;
static bool bAllocErr; //,bReverseY;

#define MAX_MSKLVL 10
#define MASKNAME_INC 20
static int msklvl,nMaskName,maxMaskName;
static byte bOuterSeen[MAX_MSKLVL];
static int iMaskName[MAX_MSKLVL];
static SHP_MASKNAME *pMaskName;

static NTW_RECORD *pNtwRec;
static int maxNtwRec,nNtwRec;
#define MAXNTWREC_INC 40

#define MAX_PATHCNT_INC 1000
#define MAX_SYMCNT_INC 200
static SVG_PATHRANGE *pathrange,*symrange;
static double symTransX,symTransY;
static UINT reflen,siz_vecrec;
static bool bAdjust,bPathSkipped,bTextParagraph,bSkipSym;
static SVG_VECNODE *vnod;
static int *vstk;
static int vnod_siz,vnod_len,max_slevel;
static int veccnt,skipcnt,iLevel,iLevelSym,iLevelID,numGridThick;

#ifdef _USE_XMLID
static int xmlidcnt,xmlidsubcnt,xmlidlvl;
static char xmlidbuf[12]="W2D";
#endif

static double fLabelSize,fNoteSize,fTitleSize,fMarkerThick,fVectorThick,fGridThick[2];
static COLORREF cBackClr,cPassageClr,cMarkerClr,cGridClr,cLrudClr;
static double fFrameThick;
static int pathstyle_used[MAX_STYLES+1];
static char *pathstyle[MAX_STYLES+1],*textstyle[MAX_STYLES+1],*legendstyle[MAX_STYLES+1],*groupstyle[MAX_STYLES+1];
static char *symname[MAX_STYLES+1];
static bool isSymFilter[MAX_STYLES+1];
static float flagscale[MAX_STYLES+1];
static SVG_VIEWBOX flagViewBox[MAX_STYLES+1];
static SVG_PATHPOINT legendPt,*pLegendPt;
//static int skiplin[MAX_VEC_SKIPPED];
static double max_dsquared,max_dsq_x,max_dsq_y;
static double mrgViewA,mrgCosA,mrgSinA,xMrgMid,yMrgMid,xMrgMin,yMrgMin,xMrgMax,yMrgMax;
static double mrgMidE,mrgMidN,mrgScale,scaleRatio;
static UINT node_visits;
static UINT tpoints,tpoints_w,tpoints_d,pathcnt,max_pathcnt,symcnt,max_symcnt,tptotal,tppcnt;
static double flength_w,flength_d;
static int max_iv[LEN_VECLST];  //index of nearest vector to last point
static int veclst_i[LEN_VECLST];
static double veclst_d[LEN_VECLST];
static double dist_exp,dist_off,len_exp;
static double xpath_min,xpath_max,ypath_min,ypath_max;
static double min_veclen_sq;
static int vecs_used,decimals;
static SVG_TYP_TRXLABEL labelRec;
static LPCSTR pNextPolyPt;
#define SIZ_LASTTEXTSTYLE 128
static char lastTextStyle[SIZ_LASTTEXTSTYLE];
static int lastTextStyleIdx;

#pragma pack(1)
typedef struct {
	double a;
	double b;
	double c;
	double d;
	double e;
	double f;
} SVG_CTM;
#pragma pack()
	
static SVG_CTM utm_to_xy,utm_to_mrgxy,mrgxy_to_xy,xy_to_mrgxy,pathCTM,textCTM,groupCTM,symTransCTM;
static bool bPathCTM; //,bGroupCTM;

//static clock_t tm_scan,tm_walls;

//=================================

typedef struct {
	WORD wFlag;		/*flag count for this style*/
	WORD wPriority; /*0=highest*/
	WORD wSize;		/*size in points*/
	BYTE bShape;	/*0-square,1-circle,2-triangle,3-plus*/
	BYTE bShade;	/*0-clear,1-solid,2-opaque*/
	DWORD color;
	char *pFlagname;
} SVG_TYP_FLAGSTYLE;

typedef struct {
	double xy[2];
	UINT flag_id;
} SVG_TYP_FLAGLOCATION;

struct URLNAM {
	URLNAM(LPCSTR p,int len,int id)
	{
		nam=(LPSTR)malloc(len+1);
		memcpy(nam,p,len);
		nam[len]=0;
		idx=id;
	}
	LPSTR nam;
	int idx; //path index;
};

typedef std::vector<URLNAM> VEC_URLNAM;
typedef VEC_URLNAM::iterator it_urlnam;
static VEC_URLNAM vUrlNam;

static SVG_TYP_FLAGSTYLE *pFS;
static SVG_TYP_FLAGLOCATION *pFL;

static UINT numFlagStyles;
static UINT numFlagLocations,numNoteLocations;
static char *pLabelStyle,*pNoteStyle;
static bool bUseStyles,bUseLabelPos,bMarkerWritten,bMaskPathSeen,bUseLineWidths;
#pragma pack(1)
struct CHAR6
{
	char char6[8];
};
#pragma pack()

//===================================================
//bMerge2 related --

#define SVG_TMP_EXT ".xml"

#define SYM_PREFIX_LEN 13
const LPCSTR sym_prefix="\t<symbol id=\"";
const LPCSTR sym_end="\t</symbol>\r\n";
const LPCSTR w2d_prefix="\t<g id=\"w2d_";
const LPCSTR w2d_prefixsub="\t\t<g id=\"w2d_";
const LPCSTR w2d_Backround="\t<g id=\"w2d_Background\">\r\n";
const LPCSTR w2d_Ref="\t<g id=\"w2d_Ref\">\r\n";
const LPCSTR w2d_Mask="\t<g id=\"w2d_Mask\">\r\n";
const LPCSTR w2d_Detail="\t<g id=\"w2d_Detail\">\r\n";
const LPCSTR w2d_Detail_shp="\t\t<g id=\"w2d_Detail_shp\">\r\n";
const LPCSTR w2d_Detail_sym="\t\t<g id=\"w2d_Detail_sym\">\r\n";
const LPCSTR w2d_Walls="\t<g id=\"w2d_Walls\">\r\n";
const LPCSTR w2d_Walls_shp="\t\t<g id=\"w2d_Walls_shp\">\r\n";
const LPCSTR w2d_Walls_sym="\t\t<g id=\"w2d_Walls_sym\">\r\n";
const LPCSTR w2d_Survey="\t<g id=\"w2d_Survey\">\r\n";
const LPCSTR w2d_Legend="\t<g id=\"w2d_Legend\"";
const LPCSTR w2d_Frame="\t<g id=\"w2d_Frame\">\r\n";
const LPCSTR w2d_endgrp="\t</g>\r\n";
const LPCSTR w2d_endsubgrp="\t\t</g>\r\n";

typedef struct {
	char name[8];
	LPSTR style;
} SVG_ENTITY;
typedef struct {
	char name[8];
	int idx;
} SVG_ENTITY_IDX;
static SVG_ENTITY svg_entity[2*MAX_STYLES];
static SVG_ENTITY_IDX svg_entity_idx[MAX_STYLES];
static int svg_entitycnt,svg_entityidxcnt,nLidx,nPidx,nTidx;
//static int svg_entitycnt_div;

#define INC_IDNAME 2048
typedef std::vector<int> VEC_INT;
typedef VEC_INT::iterator it_int;
static LPSTR idNameBuf; 
static int idNameLen,idNameSiz;
static VEC_INT vIdNameOff;
#define PNAME(i) ((LPCSTR)&idNameBuf[i])
struct NAME_MAP
{
	NAME_MAP(int oOff,int nOff) : oldoff(oOff),newoff(nOff),vplineoff(0),vplinecnt(0) {};
	int oldoff; //symbol name in $2.svg that that matches one in $1.svg (offset in idNameBuf)
	int newoff; //replacement name to use in $2.svg if symbol definitions don't match
	int vplineoff;
	int vplinecnt;
};
typedef std::vector<NAME_MAP> VEC_NAME_MAP;
typedef VEC_NAME_MAP::iterator it_name_map;
static VEC_NAME_MAP vNameMap;

typedef std::vector<LPSTR> VEC_LPSTR;
typedef VEC_LPSTR::iterator it_vec_lpstr;

static VEC_LPSTR vpNamOld,vpNamNew;

//end of merge2 related --
//==========================================================

struct CLR_DATA {
	CLR_DATA(COLORREF c,DWORD d) : cr(c), cnt(d) {};
	COLORREF cr;
	int cnt;
};

typedef std::vector<CLR_DATA> VEC_COLOR;
typedef VEC_COLOR::reverse_iterator IT_RVC;
static VEC_COLOR vecColor;

//typedef std::vector<CHAR6> VEC_COLOR;
//static VEC_COLOR vecColor0;
//static VEC_COLOR *pvecColor;
static int numVecColors;
static int iGroupFlg,iParentFlg,iGroupID;
static LPFN_EXPORT_CB pGetData;

static char argv_buf[2*_MAX_PATH];

int outBackground();
int outFrame();
int outGrid();
int outRef();
int outMergedContent();
int outVectors();
int outLruds();
int outMarkers();
int outFlags();
int outLabels();
int outNotes();

#define NUM_GROUPS 12

typedef int (*GRP_FCN_TYP)();

static GRP_FCN_TYP grp_fcn[NUM_GROUPS+1]={
	NULL,
	&outBackground,
	&outRef,
	NULL,
	&outVectors,
	&outLruds,
	&outMarkers,
	&outFlags,
	&outLabels,
	&outNotes,
	&outGrid,
	&outFrame,
	NULL
};

static char *grp_id[NUM_GROUPS]={
	"svg",
	"w2d_Background",
	"w2d_Ref",
	"w2d_Survey",
	"w2d_Vectors",
	"w2d_Lruds",
	"w2d_Markers",
	"w2d_Flags",
	"w2d_Labels",
	"w2d_Notes",
	"w2d_Grid",
	"w2d_Frame"
};

#define MAX_DEPTH 10
static int grp_typ[MAX_DEPTH];

enum e_gtyp {W2D_SVG,W2D_BACKGROUND,W2D_REF,W2D_SURVEY,W2D_VECTORS,W2D_LRUDS,
	W2D_MARKERS,W2D_FLAGS,W2D_LABELS,W2D_NOTES,W2D_GRID,W2D_FRAME};

#define W2D_IGNORE -3

/*
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
*/
static NPSTR errstr[]={
  "operation successful",
  "User abort",
  "Cannot create",
  "Error writing",
  "Error reading file",
  "Error parsing",
  "Incompatible version of wallsvg.dll",
  "Not enough memory or resources",
  "Error creating temporary index",
  "Error writing temporary index",
  "No survey vectors in view",
  "File not adjustable (no vector ID's or style attributes)",
  "Can't open SVG with content to merge",
  "Wrong format for vector path ID",
  "Vector %s in merged SVG not part of exported data",
  "Wrong format for vector definition",
  "Vector direction difference > 90 deg",
  "Wrong format for path attribute",
  "Unrecognized w2d_Ref format",
  "String in w2d_Ref group too long",
  "No w2d_Ref group prior to w2d_Vectors",
  "Text element expected in group w2d_Ref",
  "Path or line element expected",
  "Bad arrangement of predefined layers",
  "Unsupported element, <%s>, in predefined layer",
  "Unsupported <text>..<tspan> arrangement",
  "No w2d_Mask layer in source SVG",
  "Could not flatten curve",
  "Mask layer has %u open path(s) with fill assigned (line %d)",
  "Only path elements expected in mask layer",
  "Group depth limit exceeded",
  "Unrecognized w2d layer name (%s)",
  "Line endpoint coordinates expected",
  "Rect attributes expected",
  "Circle or ellipse attributes expected",
  "Bad format for polygon data",
  "Unsupported transform attribute",
  "Skew transforms not supported",
  "Nested group transform not supported",
  "File creation error during final merge",
  "Unexpected w2d_%s layer encountered",
  "Duplicate wd2_%s layer encountered",
  "Unknown error"
};

static LPCSTR strStyleIgnored="enable-background:new    ;";
static LPCSTR strFilter="filter:url(#";

enum e_fmt_mkr {FMT_MKR_SQUARE,FMT_MKR_CIRCLE,FMT_MKR_TRIANGLE,FMT_MKR_PLUS,FMT_MKR_MARKER};
#define NUM_PATHTYPES 7
enum e_pathtype {P_PATH=1,P_RECT,P_LINE,P_POLYGON,P_POLYLINE,P_ELLIPSE,P_CIRCLE};
static LPCSTR pathType[NUM_PATHTYPES+1]={NULL,"path","rect","line","polygon","polyline","ellipse","circle"};
static int iPathType;

#define NUM_SYMTYPES 6
static int iSymType;
enum e_symtype {S_TEXT=1,S_TSPAN,S_TEXTPATH,S_USE,S_IMAGE,S_PATTERN};
static LPCSTR symType[NUM_SYMTYPES+1]={NULL,
	"text","tspan","textPath","use","image","pattern"
};

//SVG elements for which bNoClose can be set TRUE -- terminated with "/>"
#define NUM_FILTERTYPES 15
static LPCSTR filterType[NUM_FILTERTYPES]={
	"Blend","ColorMatrix","Composite","DistantLight",
	"Flood","FuncB","FuncG","FuncR","GaussianBlur","Image",
	"MergeNode","Offset","PointLight","SpotLight","Tile"
};
//enum e_filtertype {S_BLEND=NUM_SYMTYPES+1,...

//static char *pEvenOddRule="fill-rule:evenodd;clip-rule:evenodd;";

static char *pSymbolStr[5]={
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<rect fill=\"%s\" stroke=\"%s\" stroke-width=\"0.12\" fill-opacity=\"%s\""
		" x=\"0.06\" y=\"0.06\" width=\"1.88\" height=\"1.88\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<circle fill=\"%s\" stroke=\"%s\""
		" stroke-width=\"0.12\" fill-opacity=\"%s\" r=\"0.94\" cx=\"1\" cy=\"1\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol id=\"%s\" viewBox=\"0 0 2 2\">\r\n\t\t"
		"<path fill=\"%s\" stroke=\"%s\" stroke-width=\"0.10\" fill-opacity=\"%s\""
		" d=\"M0.238,0.48H1.762L1,1.8z\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol id=\"%s\">\r\n\t\t"
		"<path stroke=\"%s%s\" stroke-width=\"0.12\" d=\"M0,1H2M1,0V2\"/>\r\n"
	"\t</symbol>\r\n",
	"\t<symbol  id=\"_m\">\r\n\t\t"
		"<!--marker-->\r\n\t\t"
		"<path style=\"fill:%s;\" d=\"M0,0h2v2H0z\"/>\r\n"
	"\t</symbol>\r\n"
};

#define MAX_LINESIZE 4096 //incl /n/r/0
#define MAX_NAMESIZE 256
static int fgetline(char *buf,FILE *fp)
{
	//Return line length, or 0 if already at EOF, or -1 if buffer length
	//exceeded. Length (and buf) includes terminating \n. Null is appended.
	int c;
	int len = 0;
	do{
		if((c=fgetc(fp))==EOF) {
			if(!len) return 0;
			c='\n';
		}
		if(++len==MAX_LINESIZE) {
			buf--;
			c='\n';
			len=-1;
		}
		*buf++=(char)c;
	}
	while(c != '\n');
	*buf= 0;
	return len;
}

//=========================================================================
//Expat parser related data and functions --
//=========================================================================
#define MRGBUFFSIZE 16384
#define SIZ_LINEBREAK 128
#define SIZ_PATH_LINEBREAK 130
#define SIZ_SVG_LINEBREAK 60
#define SIZ_FILTERNAME 80

static char MrgBuff[MRGBUFFSIZE];

typedef struct {
	int bEntity;
	int errcode;
	int errline;
} TYP_UDATA;

static TYP_UDATA udata;

static bool bNoClose;
static int Depth,nCol;
static XML_Parser parser,mrgparser;
static enum XML_Error parser_ErrorCode;
static int parser_ErrorLineNumber;
static char *pTemplate;
static UINT uTemplateLen;

#define OUTC(c) gz_putc(c)
#define OUTS(s) gz_puts(s)
#define UD(t) ((TYP_UDATA *)userData)->t

#define PI 3.141592653589793
#define PI2 (PI/2.0)
#define U_DEGREES (PI/180.0)

static char *trim_refbuf()
{
	while(reflen && isspace((BYTE)refbuf[reflen-1])) reflen--;
	refbuf[reflen]=0;
	return refbuf;
}

static void gz_write(LPCSTR s,int len)
{
	if(gzout) gzwrite(gzout,(const voidp)s,len);
	else fwrite(s,len,1,fpout);
}

static void gz_putc(int c)
{
	if(gzout) gzputc(gzout,c);
	else putc(c,fpout);
}

static void gz_puts(LPCSTR s)
{
	if(gzout) gzputs(gzout,s);
	else fputs(s,fpout);
}

static int gz_printf(char *format,...)
{
	int cnt;
    va_list marker;
    va_start(marker,format);
    cnt=_vsnprintf(gzbuf,SIZ_GZBUF,format,marker);
    va_end(marker);
	if(cnt<0) {
		assert(0); //array overflow!
		return 0;
	}
	if(gzout) gzputs(gzout,gzbuf);
	else fputs(gzbuf,fpout);
	return cnt;
}

//Geometric transformations --

static void mult(SVG_CTM *ctm,double *xy)
{
	double x=xy[0];
	xy[0]=ctm->a*x + ctm->c*xy[1] + ctm->e;
	xy[1]=ctm->b*x + ctm->d*xy[1] + ctm->f;
}

static void mult(SVG_CTM *ctm,double *x1,double *x2)
{
	double x=x1[0];
	x2[0]=ctm->a*x + ctm->c*x1[1] + ctm->e;
	x2[1]=ctm->b*x + ctm->d*x1[1] + ctm->f;
}

static void mult(SVG_CTM *ctmlf,SVG_CTM *ctmrt)
{
	double x=ctmrt->a;
	ctmrt->a=ctmlf->a*x + ctmlf->c*ctmrt->b;
	ctmrt->b=ctmlf->b*x + ctmlf->d*ctmrt->b;
	x=ctmrt->c;
	ctmrt->c=ctmlf->a*x + ctmlf->c*ctmrt->d;
	ctmrt->d=ctmlf->b*x + ctmlf->d*ctmrt->d;
	x=ctmrt->e;
	ctmrt->e = ctmlf->e + ctmlf->a*x + ctmlf->c*ctmrt->f;
	ctmrt->f = ctmlf->f + ctmlf->b*x + ctmlf->d*ctmrt->f;
}

static void ctm_invert(SVG_CTM *ctm,SVG_CTM *ctmi)
{
	double det=1.0/(ctm->a*ctm->d-ctm->b*ctm->c);
	ctmi->a=ctm->d*det;
	ctmi->b=-ctm->b*det;
	ctmi->c=-ctm->c*det;
	ctmi->d=ctm->a*det;
	ctmi->e=(ctm->c*ctm->f-ctm->d*ctm->e)*det;
	ctmi->f=(ctm->b*ctm->e-ctm->a*ctm->f)*det;
}

static void getRotateCTM(SVG_CTM *ctm,SVG_PATHPOINT *pp,double angle)
{
	ctm->a =  ctm->d = cos(angle);
	ctm->c = -(ctm->b = sin(angle));
	if(pp) {
		ctm->e = pp->x - ctm->a*pp->x - ctm->c*pp->y;
		ctm->f = pp->y - ctm->b*pp->x - ctm->d*pp->y;
	}
	else ctm->e=ctm->f=0.0;
}

static void init_utm_to_xy()
{
	//Matrix to convert world coordinates to exported page coordinates --
	double sA=scale*sinA;
	double cA=scale*cosA;

	utm_to_xy.a =  cA;
	utm_to_xy.b = -sA;
	utm_to_xy.c = -sA;
	utm_to_xy.d = -cA;
	utm_to_xy.e = xMid + sA*midN - cA*midE;
	utm_to_xy.f = yMid + sA*midE + cA*midN;
}

static void init_utm_to_mrgxy()
{
	//Matrix to convert world coordinates to merge SVG's page coordinates --
	double sA=mrgScale*mrgSinA;
	double cA=mrgScale*mrgCosA;

	utm_to_mrgxy.a =  cA;
	utm_to_mrgxy.b = -sA;
	utm_to_mrgxy.c = -sA;
	utm_to_mrgxy.d = -cA;
	utm_to_mrgxy.e = xMrgMid + sA*mrgMidN - cA*mrgMidE;
	utm_to_mrgxy.f = yMrgMid + sA*mrgMidE + cA*mrgMidN;
}

static void init_mrgxy_to_xy()
{
	//Matrix to convert from merge SVG's page coordinates to exported page coordinates --
	//mrgxy_to_xy = utm_to_xy * inverse(utm_to_mrgxy)

	ctm_invert(&utm_to_mrgxy,&mrgxy_to_xy);
	mult(&utm_to_xy,&mrgxy_to_xy);
	ctm_invert(&mrgxy_to_xy,&xy_to_mrgxy);
}

static int write_polygons()
{
	//Convert PolyBezier array to polygons with BYTE flag array replaced
	//with array of integer polygon lengths.
	int i=0;

	if(!pNTWp || !pNTWf) return SVG_ERR_NOMEM;

	HDC hdc=::GetDC(NULL);
	if(!hdc || !::BeginPath(hdc) || !::PolyDraw(hdc,pNTWp,pNTWf,ntw_count) ||
		!::EndPath(hdc) || !::FlattenPath(hdc)) {
		if(hdc) ::ReleaseDC(NULL,hdc);
		return SVG_ERR_FLATTEN;
	}

	i=::GetPath(hdc,NULL,NULL,0);
	if(i>ntw_count) {
		if(!(pNTWp=(POINT *)realloc(pNTWp,i*sizeof(POINT))) ||
		   !(pNTWf=(byte *)realloc(pNTWf,i))) i=0;
	}
	if((i && i!=::GetPath(hdc,pNTWp,pNTWf,i)) || !::ReleaseDC(NULL,hdc)) i=0;
	if(!i) {
		return SVG_ERR_FLATTEN;
	}

	ntw_hdr.nPoints=ntw_count=i;
	ntw_hdr.nPolygons=max_pathcnt; //shouldn't have changed
	ntw_hdr.minE=ntw_hdr.minN=ntw_hdr.maxE=ntw_hdr.maxN=0;
	
	//Now let's update the array pnPly[] with point counts and calculate range.
	//The pnPly array already has the flags (interior/exterior, closed/nonclosed)
	//set in the MSB of this integer --
	NTW_POLYGON *pPly=pnPly;
	BYTE *pFlg=pNTWf;
	POINT *pPnt=pNTWp;
	UINT nPly=1,np=0;

	for(i=0;i<ntw_count;i++) {
		if(pPnt->x<ntw_hdr.minE) ntw_hdr.minE=pPnt->x;
		if(pPnt->y<ntw_hdr.minN) ntw_hdr.minN=pPnt->y;
		if(pPnt->x>ntw_hdr.maxE) ntw_hdr.maxE=pPnt->x;
		if(pPnt->y>ntw_hdr.maxN) ntw_hdr.maxN=pPnt->y;
		if(((*pFlg)&PT_MOVETO)==PT_MOVETO) {
			if(i) {
#ifdef _DEBUG
				if(((pFlg[-1]&PT_CLOSEFIGURE)!=0)!=((pPly->flgcnt&NTW_FLG_OPEN)==0)) 
					return SVG_ERR_FLATTEN;
#endif
				if((*pFlg)&PT_CLOSEFIGURE) {
					#ifdef _DEBUG
						return SVG_ERR_FLATTEN;
					#endif
					np=0; //never happens with PolyDraw98
				}
				if(nPly<max_pathcnt) (pPly++)->flgcnt|=(np&NTW_FLG_MASK);
				nPly++;
			}
			np=0;
		}
		np++;
		pFlg++;
		pPnt++;
	}
	pPly->flgcnt|=(np&NTW_FLG_MASK);

	if(nPly!=max_pathcnt) {
		return SVG_ERR_UNKNOWN;
	}

	ntw_hdr.nPolyRecs=nNtwRec;
	ntw_hdr.nMaskName=nMaskName;

	//Write NTW file --
	if(!(fpout=fopen(pSVG->svgpath,"wb"))) {
	  return SVG_ERR_SVGCREATE;
	}

	if(fwrite(&ntw_hdr,sizeof(ntw_hdr),1,fpout)!=1 ||
		fwrite(pNTWp,sizeof(POINT)*ntw_count,1,fpout)!=1 ||
		fwrite(pnPly,sizeof(NTW_POLYGON)*nPly,1,fpout)!=1 ||
		fwrite(pNtwRec,sizeof(NTW_RECORD)*nNtwRec,1,fpout)!=1) {
	  return SVG_ERR_SVGCREATE;
	}
	
	if(fwrite(pMaskName,sizeof(SHP_MASKNAME)*nMaskName,1,fpout)!=1) {
	  return SVG_ERR_SVGCREATE;
	}

	return 0;
}

static void get_ntw_point(POINT *p,SVG_PATHPOINT *mp)
{
	//Convert from merge SVG's page coordinates to point in NTW file --
	double x=(mp->x-xMrgMid);
	double y=(mp->y-yMrgMid);
	//double E=x*mrgCosA-y*mrgSinA;  //(E/mrgScale)+mrgMidE is UTM coordinate
	//double N=-x*mrgSinA-y*mrgCosA;
	p->x=(long)((x*mrgCosA-y*mrgSinA)*1000);
	p->y=(long)((-x*mrgSinA-y*mrgCosA)*1000);
}

static void out_ntw_point(POINT *pt,byte flg)
{
	#define NTW_COUNT_INC 5000
	if(bAllocErr) return;
	if(ntw_count==ntw_size) {
		ntw_size+=NTW_COUNT_INC;
		if(!(pNTWp=(POINT *)realloc(pNTWp,ntw_size*sizeof(POINT))) ||
			!(pNTWf=(byte *)realloc(pNTWf,ntw_size))) {
			bAllocErr=true;
			return;
		}
	}
	pNTWp[ntw_count]=*pt;
	pNTWf[ntw_count++]=flg;
}

static void out_ntw_points(SVG_PATHPOINT *pp,char typ)
{
	static POINT lastPt,lastCtl;
	static byte lastFlg;
    
	if(typ!='m') {
		if(!pp) {
			if(typ=='z') lastFlg|=PT_CLOSEFIGURE;
			else {
				pnPly[pathcnt].flgcnt|=NTW_FLG_OPEN;
				if(!(pnPly[pathcnt].flgcnt&NTW_FLG_NOFILL)) {
					if(!nMaskOpenPaths) nNTW_OpenPathLine=XML_GetCurrentLineNumber(parser);
					nMaskOpenPaths++;
				}
			}
		}
		out_ntw_point(&lastPt,lastFlg);
	}

	if(!pp) return;

	switch(typ) {
		case 'm' :
		case 'l' :	get_ntw_point(&lastPt,pp);
					lastFlg=(typ=='m')?PT_MOVETO:PT_LINETO;
					break;

		case 'c' :	get_ntw_point(&lastCtl,pp++);
					out_ntw_point(&lastCtl,PT_BEZIERTO);
					get_ntw_point(&lastCtl,pp++);
					out_ntw_point(&lastCtl,PT_BEZIERTO);
					get_ntw_point(&lastPt,pp);
					lastFlg=PT_BEZIERTO;
					break;

		case 's' :  if(lastFlg==PT_BEZIERTO) {
						lastPt.x+=(lastPt.x-lastCtl.x);
						lastPt.y+=(lastPt.y-lastCtl.y);
					}
					tpoints++;
					out_ntw_point(&lastPt,PT_BEZIERTO);
					get_ntw_point(&lastCtl,pp++);
					out_ntw_point(&lastCtl,PT_BEZIERTO);
					get_ntw_point(&lastPt,pp);
					lastFlg=PT_BEZIERTO;
					break;
	}

}

static int GetPathType(LPCSTR name)
{
	int i=1;
	for(;i<=NUM_PATHTYPES;i++) {
		if(!strcmp(name,pathType[i])) break;
	}
	return iPathType=(i<=NUM_PATHTYPES)?i:0;
}

static int GetSymType(LPCSTR name)
{
	int i;
	if(!name || !*name) return 0;
	for(i=1;i<=NUM_SYMTYPES;i++) {
		if(!strcmp(name,symType[i])) return iSymType=i;
	}
    if(*name++=='f' && *name++=='e') {
		for(i=0;i<NUM_FILTERTYPES;i++) {
			if(!strcmp(name,filterType[i])) return iSymType=i+(NUM_SYMTYPES+1);
		}
	}
	return iSymType=0;
}

static bool isInside(double *xy)
{
	return xy[0]>0 && xy[1]>0 && xy[0]<width && xy[1]<height;
}

static void out_xmlstr(LPCSTR p)
{
	char buf[256];
	char *p0=buf;
	while(*p) {
		switch(*p) {
			case '\"'	: strcpy(p0,"&quot;"); p0+=6; break;
			case '\''	: strcpy(p0,"&apos;"); p0+=6; break;
			case '>'	: strcpy(p0,"&gt;"); p0+=4; break;
			case '<'	: strcpy(p0,"&lt;"); p0+=4; break;
			case '&'	: strcpy(p0,"&amp;"); p0+=5; break;
			default		: *p0++=*p;
		}
		p++;
	}
	*p0=0;
	OUTS(buf);
}

static char *flagname_str(char *p)
{
	static char _namebuf[256];
	char *p0=_namebuf;
	*p0++='_';
	while(*p) {
	  if(*p==' ') {
		 p++;
		 *p0++='_';
	  }
	  else if(*p>='A' && *p<='Z' || *p>='a' && *p<='z' || *p>='0' && *p<='9' ||
		*p=='.' || *p=='-') *p0++=*p++;
	  else p0+=sprintf(p0,"_%02x",(UINT)*(unsigned char *)p++);
	}
	*p0=0;
	return _namebuf;
}

static char * fltstr(char *fbuf,double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	bool bRound=true;

	if(dec<0) {
		bRound=false;
		dec=-dec;
	}
	if(_snprintf(fbuf,20,"%-0.*f",dec,f)==-1) {
	  *fbuf=0;
	  return fbuf;
	}
	if(bRound) {
		char *p=strchr(fbuf,'.');
		for(p+=dec;dec && *p=='0';dec--) *p--=0;
		if(*p=='.') {
		   *p=0;
		   if(!strcmp(fbuf,"-0")) strcpy(fbuf,"0");
		}
	}
	return fbuf;
}

static char * fltstr(double f,int dec)
{
	static char fbuf[40];
	return fltstr(fbuf,f,dec);
}

static char * fltstr(double f)
{
	return fltstr(f,decimals);
}

static char * fltstr(char *fbuf,double f)
{
	return fltstr(fbuf,f,decimals);
}

//===========================================================
//Output functions --

static int skip_depth(void)
{
	int i;
	for(i=0;i<Depth;i++) OUTC('\t');
	return Depth;
}

static void gz_outPath(const char *pStart,bool bImage)
{
	//Output string pStart, inserting a line break and Depth tabs when a string
	//of two or space characters is encountered (in which the string of spaces
	//is not output). Unless bImage==true a single space is allowed and will not
	//cause a line break.
	const char *p0;
	int col;
	
	while(p0=strchr(pStart,' ')) {
		for(col=1;*++p0==' ';col++); //col is length of space string
		if(!bImage && col==1) col=0;  //Allow a single space and don't break line
		gz_write(pStart,p0-pStart-col);
		if(col && *p0) {
		  OUTS("\r\n");
		  skip_depth();
		}
		pStart=p0;
	}
	if(*pStart) OUTS(pStart);
}

static void out_stream(char *p)
{
	//Update nCol while outputting all data --
	/*
	OUTS(p);
	while(*p) {
		if(*p++=='\n') nCol=0;
		else nCol++;
	}
	*/
	for(;*p;p++) {
		if(*p=='\r') {
			assert(0);
			continue;
		}
		if(*p=='\n') {
			OUTS("\r\n");
			nCol=0;
		}
		else {
			OUTC(*p);
			nCol++;
		}
	}
}

static void out_wraptext(LPCSTR p)
{
	for(;*p;p++) {
		if(*p==' ' && nCol>SIZ_LINEBREAK) {
			  OUTS("\r\n");
			  nCol=skip_depth();
		}
		else {
			OUTC(*p);
			nCol++;
		}
	}
}

static int trimctrl(XML_Char *s)
{
	// Replace whitespace spans with single spaces,
	// but eliminate all leading whitespace.
	// Note: isgraph(c)==1 if c printable and not space.

	XML_Char *p=s;
	int len=0;
	int bCtl=1;
	while(*s) {
		if(isgraph(*s)) bCtl=0;
		else {
		  if(bCtl) {
		    s++;
		    continue;
		  }
		  bCtl=1;
		  *s=' '; //shouldn't be required, since expat has done this
		}
	    *p++=*s++;
	    len++;
	}
	*p=0;
	return len;
}

static char *fmtCTM(SVG_CTM &ctm)
{
	double *pd=&ctm.a;
	char fbuf[6][20];

	for(int len=0;len<6;len++) fltstr(fbuf[len],*pd++);

	sprintf(argv_buf,"transform=\"matrix(%s %s %s %s %s %s)\"",
		fbuf[0],fbuf[1],fbuf[2],fbuf[3],fbuf[4],fbuf[5]);

	return argv_buf;
}

static void outStartGroup(int id,BOOL bHide)
{
	skip_depth();
	gz_printf("<g id=\"w2d_%s\"",w2d_name[id]);
	if(bHide) OUTS(" style=\"display:none\"");
	if(id==ID_LEGEND && bPathCTM) {
		gz_printf(" %s",fmtCTM(pathCTM));
	}
	OUTS(">\r\n");
	Depth++;
}

static LPCSTR getAttrVal(LPCSTR *attr,LPCSTR name)
{
	if(name) {
		for(;*attr;attr+=2) {
			if(!strcmp(*attr,name)) {
				name=attr[1];
				while(isspace((BYTE)*name)) ++name;
				return name;
			}
		}
	}
	return NULL;
}

static LPCSTR getStyleAttr(LPCSTR *attr)
{
	return getAttrVal(attr,"style");
}

static void initPathCTM()
{
	SVG_CTM ctm={1.0,0.0,0.0,1.0,0.0,0.0};
	pathCTM=ctm;
	bPathCTM=true;
}

static int getCTM(LPCSTR *attr,SVG_CTM *ctm)
{
	LPCSTR p=bInsidePattern?"patternTransform":(bInsideGradient?"gradientTransform":"transform");

	if(p=getAttrVal(attr,p)) {
		if(strlen(p)<7 || memcmp(p,"matrix(",7) || (6!=sscanf(p+7,"%lf %lf %lf %lf %lf %lf",
			&ctm->a,&ctm->b,&ctm->c,&ctm->d,&ctm->e,&ctm->f))) return -1;
		return 1;
	}
	return 0;
}

static int getPathCTM(LPCSTR *attr)
{
	int i=getCTM(attr,&pathCTM);
	bPathCTM=(i==1);
	return i;
}

static SVG_PATHPOINT *getPathCTM_Pt(LPCSTR *attr,SVG_PATHPOINT *pt)
{
	if(getPathCTM(attr)<1) return NULL;
	if(!pt) pt=(SVG_PATHPOINT *)&pathCTM.e;
	else *pt=*(SVG_PATHPOINT *)&pathCTM.e;
	return pt;
}

static void applyPathCTM(SVG_PATHPOINT *pp,int len)
{
	double t;
	for(;len;pp++,len--) {
		pp->x=pathCTM.a*(t=pp->x) + pathCTM.c*pp->y + pathCTM.e;
		pp->y=pathCTM.b*t + pathCTM.d*pp->y + pathCTM.f;
	}
}

static BOOL fixRotateCTM(SVG_PATHPOINT *pp)
{
	//Called only for P_RECT and P_ELLIPSE shapes, just prior to transform
	//attribute output --
	SVG_PATHPOINT pt;
	double rotA;

	//Compute angle rotated due to any change of coordinates --
	if(iGroupFlg&FLG_SYM_MSK) rotA=0.0;
	else {
		//In Walls, positive views correspond to ccw (negative) rotation angles in SVG --
		rotA=mrgViewA-viewA;
	}

	if(bPathCTM) {
		//Only rotation transforms are supported for path objects --
		if(pathCTM.a!=pathCTM.d || pathCTM.b!=-pathCTM.c) return FALSE;
		rotA+=atan2(pathCTM.b,pathCTM.a);
	}

	if(iPathType==P_RECT) {
		//The four points in pp[] have already been transformed to output page coordinates,
		//with both merge file and output file rotations applied depending on sym layer
		//membership. This rotation was computed above as rotA. We now need to insure the
		//rectangle's coordinates are again "level" and compute a new rotation transform
		//that will orient it correctly.

		if(rotA!=0.0) {
			//Any rotation will be about the rectangle's center point --
			pt.x=0.5*(pp[2].x+pp[0].x);
			pt.y=0.5*(pp[2].y+pp[0].y);
			getRotateCTM(&pathCTM,&pt,-rotA);
			applyPathCTM(pp,4);
		}
	}
	else pt=*pp; //P_ELLIPSE

	if(rotA!=0.0) {
		getRotateCTM(&pathCTM,&pt,rotA);
		bPathCTM=true;
	}
	else bPathCTM=false;

	return TRUE;
}

static double dist_to_vec(double px,double py, double *B, double *M)
{
	//Vector is defined as B+t*M, 0<=t<=1
	//Minimum squared length is assumed >= min_veclen_sq
	double l,t;

	px-=B[0];
	py-=B[1];
	t=M[0]*px+M[1]*py;
	if(t>0) {
		l=M[0]*M[0]+M[1]*M[1];
		if(t>=l) {
			px-=M[0];
			py-=M[1];
		}
		else {
			t/=l;
			px-=t*M[0];
			py-=t*M[1];
		}
	}
	return px*px+py*py;
}

static double max_dist_to_vec(double px,double py, double *B, double *M)
{
	//Return maximum quared distance from (px,py) to a vector.
	//Vector is defined as B+t*M, 0<=t<=1
	//Minimum squared length is assumed >= min_veclen_sq
	double dx,dy,dmax;

	dx=B[0]-px; dy=B[1]-py;
	dmax=dx*dx+dy*dy;
	dx+=M[0];
	dy+=M[1];
	dy=dx*dx+dy*dy;
	return (dy>dmax)?dy:dmax;
}

static double dist_to_midpoint(double x,double y,SVG_VECNODE *pn)
{
	double dx=x-pn->B_xy[0]-0.5*pn->M_xy[0];
	double dy=y-pn->B_xy[1]-0.5*pn->M_xy[1];
	return dx*dx+dy*dy;
}

static double insert_veclst(int id,double dist)
{
	//Largest distance is at bottom of list --
	int i,j;
	for(i=0;i<vecs_used;i++) {
		if(!(j=veclst_i[i])) break;
		if(j==id) goto _dret;
	}
	for(i=0;i<vecs_used;i++) if(dist<=veclst_d[i]) break;
	if(i<vecs_used) {
		for(j=vecs_used-1;j>i;j--) {
			veclst_i[j]=veclst_i[j-1];
			veclst_d[j]=veclst_d[j-1];
		}
		veclst_i[i]=id;
		veclst_d[j]=dist;
	}
_dret:
	return veclst_d[vecs_used-1];
}

static double adjust_veclst(double x,double y)
{
	int i,j,isav,imax=vecs_used;
	double dsav;

	for(i=0;i<vecs_used;i++) {
		if(!(j=veclst_i[i])) {
			imax=i;
			break;
		}
		veclst_d[i]=dist_to_vec(x,y,vnod[j].B_xy,vnod[j].M_xy);
	}
	for(i=0;i<imax;i++) {
		for(j=i+1;j<imax;j++) {
			if(veclst_d[j]<veclst_d[i]) {
				dsav=veclst_d[i];
				veclst_d[i]=veclst_d[j];
				veclst_d[j]=dsav;
				isav=veclst_i[i];
				veclst_i[i]=veclst_i[j];
				veclst_i[j]=isav;
			}
		}
	}
	return imax?veclst_d[imax-1]:DBL_MAX;
}

static double min_vecdist(double x,double y)
{
	//return squared distance to nearest vector in vnod[] tree --
	double dmin,dmid;
	int ilast;
	int slevel;
	int nvisits;
	SVG_VECNODE *pn;

	//Get maximum distance to set of vectors nearest last point --
	dmin=adjust_veclst(x,y);

	vstk[0]=slevel=ilast=nvisits=0;

	while(slevel>=0) {
		pn=&vnod[vstk[slevel]];
		//Was the last node visited a child of this one?
		if(pn->lf==ilast || pn->rt==ilast) {
			if(pn->rt==ilast || pn->rt==-1) {
				//finished with this node --
				ilast=vstk[slevel--];
				continue;
			}
			//Finally, examine the right child --
			pn=&vnod[vstk[++slevel]=pn->rt];
		}
		//otherwise it was this node's parent, so examine this node --
		ilast=vstk[slevel];

		//Now examine pn's vector where pn=&vnod[ilast=vstk[slevel]] --
		dmid=dist_to_midpoint(x,y,pn);
		if(dmid<=pn->maxdist || //helps significantly!
			sqrt(dmid)-sqrt(pn->maxdist)<sqrt(dmin)) {
			//Search this node and its children --
			if(ilast>=NUM_HDR_NODES) {
				nvisits++;
				dmid=dist_to_vec(x,y,pn->B_xy,pn->M_xy);
				if(dmid<dmin) dmin=insert_veclst(ilast,dmid);
			}
			if((vstk[++slevel]=pn->lf)>0 || (vstk[slevel]=pn->rt)>0) continue;
			slevel--;
		}
		//We are finished with this node --
		slevel--;
	}
	node_visits+=nvisits;
	return veclst_d[0]; //distance to nearest vector --
	//return dmin; //distance to 3rd nearest vector --
}

static void adjust_point(double *P)
{
	int i;
	double lsq[LEN_VECLST],lsqr[LEN_VECLST],wt[LEN_VECLST],wtsum;
	double S[2],*B,*M,*Bn,*Mn;
	double t,v;

	wtsum=0.0;
	for(i=0;i<vecs_used;i++) {
		M=vnod[veclst_i[i]].M_xy;
		lsqr[i]=sqrt(lsq[i]=M[0]*M[0]+M[1]*M[1]);
		wtsum+=(wt[i]=pow(lsqr[i],len_exp)/pow(dist_off+sqrt(veclst_d[i]),dist_exp));
	}

	S[0]=S[1]=0.0;
	for(i=0;i<vecs_used;i++) {
		SVG_VECNODE *pn=&vnod[veclst_i[i]];
		M=pn->M_xy;
		B=pn->B_xy;
		Bn=pn->Bn_xy;
		Mn=pn->Mn_xy;

		//relative distance along vector --
		t=(M[0]*(P[0]-B[0])+M[1]*(P[1]-B[1]))/lsq[i];
		//distance from vector (left side positive) --
		v=(M[1]*(P[0]-B[0])-M[0]*(P[1]-B[1]))/lsqr[i];

		v/=sqrt(Mn[0]*Mn[0]+Mn[1]*Mn[1]);
		S[0]+=wt[i]*(Bn[0] + t*Mn[0] + Mn[1]*v - P[0]);
		S[1]+=wt[i]*(Bn[1] + t*Mn[1] - Mn[0]*v - P[1]);
	}
	P[0]+=S[0]/wtsum;
	P[1]+=S[1]/wtsum;
}

static void process_points(SVG_PATHPOINT *pp,int npts)
{
	//If npts>0, pp[1..npts] are npts data points to be transformed.
	//On input, the coordinates are with respect to the merge file's frame.
	//On output, the coordinates are with respect to the output (xy) frame.
	//If npts=0, pp[0] is a single data point to be transformed.

	if(!bWriting) {
		udata.errcode=SVG_ERR_UNKNOWN;
	}
	else {
		if(bNTW && !bAdjust) return;

		SVG_CTM *pCTM;
		bool bAdj;

		if(!iLevelSym && (iGroupFlg&FLG_SYM_MSK)!=0) {
			pCTM=&symTransCTM;
		    bAdj=false;
		}
		else {
			pCTM=&mrgxy_to_xy;
			bAdj=bAdjust;
		}

		for(int i=(npts?1:0);i<=npts;i++) {
			if(bAdj) {
				double d;
				d=min_vecdist(pp[i].x,pp[i].y);
				if(d>max_dsquared) {
					max_dsquared=d;
					max_dsq_x=pp[i].x;
					max_dsq_y=pp[i].y;
					for(int j=0;j<vecs_used;j++) max_iv[j]=veclst_i[j];
				}
				//The nearest vectors to pp[i] are now contained in veclst_i[] --
				adjust_point(&pp[i].x);
			}
			if(!bNTW) mult(pCTM,&pp[i].x);
		}
	}
}

static BOOL getSymPathCTM(LPCSTR *attr,bool bText)
{
	SVG_PATHPOINT pt;

	if(!getPathCTM_Pt(attr,&pt)) return FALSE;
	if(iLevelSym || (iGroupFlg&FLG_LEGEND)) return TRUE;

	//transform point --
	UINT flgsav=(iGroupFlg&FLG_SYM_MSK);
	iGroupFlg&=~FLG_SYM_MSK;
	process_points(&pt,0);
	iGroupFlg|=flgsav;

	if(!bInsidePattern &&!isInside(&pt.x)) return FALSE; //shouldn't happen if symbol

	if(!(iGroupFlg&FLG_SYM_MSK) && viewA!=mrgViewA) {
		SVG_CTM ctm;
		getRotateCTM(&ctm,NULL,mrgViewA-viewA);
		mult(&ctm,&pathCTM);
	}
	if(!bText) {
		pathCTM.a*=scaleRatio;
		pathCTM.b*=scaleRatio;
		pathCTM.c*=scaleRatio;
		pathCTM.d*=scaleRatio;
	}
	pathCTM.e=pt.x;
	pathCTM.f=pt.y;
	return TRUE;
}

static int id_in_pathstyle(LPCSTR idstr)
{
	if(!idstr) return 0;
	if(vUrlNam.size()) {
		for(it_urlnam it=vUrlNam.begin();it!=vUrlNam.end();it++) {
			if(!strcmp(it->nam,idstr)) {
				return (pathrange[it->idx].bInRange)?(it->idx+1):0;
			}
		}
	}
/*
	for(int i=0;i<MAX_STYLES;i++) {
		if(!pathstyle[i]) break;
		if(pathstyle_used[i] && strstr(pathstyle[i],idstr)) return pathstyle_used[i];
	}
*/
	return 0;
}

static void getSymTransCTM(SVG_PATHRANGE *pr)
{
	//Obtain symTransCTM matrix for converting a symbol object to
	//output page coordinates. First, the symbol is translated and
	//scaled to occupy the new position of the bounding box's center.
	//Then, if the symbol is not in a SYM layer, it's rotated by mrgViewA-viewA.

	UINT flgsav=(iGroupFlg&FLG_SYM_MSK);

	SVG_PATHPOINT p,p0;
	//Bounding box is in new page coordinates --
	p.x=(pr->xmin+pr->xmax)/2.0;
	p.y=(pr->ymin+pr->ymax)/2.0;
	//Convert back to merge file coordinates --
	mult(&xy_to_mrgxy,&p.x,&p0.x);

	if(bAdjust) {
		//Assumes iLevelSym==0
		iGroupFlg&=~FLG_SYM_MSK;
		p=p0;
		process_points(&p,0);
		iGroupFlg|=flgsav;
	}

	//p is now the bounding box's new xy center while p0 is the mrgxy center.
	symTransCTM.a=symTransCTM.d=scaleRatio;
	symTransCTM.b=symTransCTM.c=0.0;
	symTransCTM.e=p.x-scaleRatio*p0.x;
	symTransCTM.f=p.y-scaleRatio*p0.y;
	if(!flgsav && viewA!=mrgViewA) {
		SVG_CTM ctm;
		getRotateCTM(&ctm,&p,mrgViewA-viewA);
		mult(&ctm,&symTransCTM);
	}
}

static void outOpenElement(LPCSTR el)
{
	if(bNoClose) {
		OUTS(">\r\n");
	    bNoClose=false;
	}
	skip_depth();
	OUTC('<');
	OUTS(el);
}

static void outCloseElement(LPCSTR el)
{
	Depth--;
	if(bNoClose) {
		OUTS("/>\r\n");
		bNoClose=false;
	}
	else {
		skip_depth();
		gz_printf("</%s>\r\n",el);
	}
}

static void outGradientStart(LPCSTR name,LPCSTR *attr)
{
	SVG_PATHPOINT pt;
	char attrbuf[4];
	int pathid;

	if(!(pathid=id_in_pathstyle(getAttrVal(attr,"id")))) return;
	pathid--;
	//needed to compute pathrange center

	outOpenElement(name);

	getPathCTM(attr);
	if(iGroupFlg&FLG_SYM_MSK) getSymTransCTM(&pathrange[pathid]);

	for(;attr[0];attr+=2) {
		if(!strcmp(attr[0],"x1") || !strcmp(attr[0],"x2") || !strcmp(attr[0],"cx")) {
			pt.x=atof(attr[1]);
			continue;
		}
		if(!strcmp(attr[0],"y1") || !strcmp(attr[0],"y2") || !strcmp(attr[0],"cy")) {
			strcpy(attrbuf,attr[0]);
			pt.y=atof(attr[1]);
			//transform and output point --
			if(bPathCTM) {
				mult(&pathCTM,&pt.x);
				//For now, assume matrix prefix is "1 0 0 -1". Consistent with Illustrator 10?
				//AI 10+ must have fixed this -- it's now "1 0 0 1" !!!
				//pt.x+=ptTrans.x;
				//if(bReverseY) pt.y=-pt.y;
				//pt.y+=ptTrans.y;
			}
			if(pathrange[pathid].bMorph)
				process_points(&pt,0);

			strcpy(attrbuf,attr[0]);
			if(*attrbuf=='y') *attrbuf='x';
			else attrbuf[1]='x';
			gz_printf(" %s=\"%s\"",attrbuf,fltstr(pt.x));
			gz_printf(" %s=\"%s\"",attr[0],fltstr(pt.y));
			continue;
		}
		if(!strcmp(attr[0],"r")) {
			gz_printf(" r=\"%s\"",fltstr(atof(attr[1])*scaleRatio));
			continue;
		}
		if(!strcmp(attr[0],"fx") || !strcmp(attr[0],"fy") ||
			!strcmp(attr[0],"gradientTransform")) continue;

		gz_printf(" %s=\"%s\"",attr[0],attr[1]);
	}
	OUTS(">\r\n");
	bInsideGradient++;
	Depth++;
}

static int find_style(char **style,LPCSTR st)
{
	if(!st) return 0;
	for(int i=0;i<MAX_STYLES;i++) {
		if(!style[i]) break;
		if(!strcmp(st,style[i])) return i+1;
	}
	return 0;
}

static int outStyle(char typ,LPCSTR st)
{
	int i=find_style(((typ=='l')?legendstyle:textstyle),st);
	return i?gz_printf("&_%c%d;\"",typ,i):gz_printf("%s\"",st);
}

static BOOL IsImageData(LPCSTR *attr)
{
	return !strcmp(*attr++,"xlink:href") &&
		*attr && strlen(*attr)>80 && !memcmp(*attr,"data:",5);
}

static void outStartSymGroup(LPCSTR *attr)
{
	outOpenElement("g");
	for(;*attr;attr+=2) {
		if(!strcmp(*attr,"transform")) continue;
		if(!strcmp(*attr,"style")) {
			OUTS(" style=\"");
			outStyle('l',attr[1]);
		}
		else if(attr[1]) gz_printf(" %s=\"%s\"",*attr,attr[1]);
	}
	gz_printf(" %s",fmtCTM(pathCTM));
	OUTS(">\r\n");
	Depth++;
}

static void outStartElement(LPCSTR el,LPCSTR *attr,bool bUsePathCTM)
{

	outOpenElement(el);
	GetSymType(el);

	bInUse=(iSymType==S_USE || iSymType==S_IMAGE);

	for(;*attr;attr+=2) {
		//if(!iLevelSym && !bSymbolScan && bInUse && !strcmp(*attr,"transform")) {
		if(bUsePathCTM && !strcmp(*attr,"transform")) {
			gz_printf(" %s",fmtCTM(pathCTM));
		}
		else {
			if(!strcmp(attr[0],"d") || iSymType==S_IMAGE && IsImageData(attr)) {
				if(**attr=='x') {
					OUTS("\r\n");
					skip_depth();
				}
				else OUTC(' ');
				gz_printf("%s=\"",attr[0]);
				gz_outPath(attr[1],iSymType==S_IMAGE);
				OUTC('\"');
			}
			else {
				gz_printf(" %s=\"",*attr);
				if((bInTextPath || iLevelSym || bSymbolScan || iSymType==S_IMAGE || !strcmp(el,"g")) &&	!strcmp(*attr,"style")) {
						outStyle((bInTextPath?'t':'l'),attr[1]);
				}
				else {
					gz_outPath(attr[1],FALSE);
					OUTC('\"');
					//gz_printf("%s\"",attr[1]);
				}
			}
		}
	}
	if(iSymType==S_TSPAN && bInTextPath) return;
	if(iSymType>=S_USE || bInsideGradient==2 || GetPathType(el)) bNoClose=true;
	else OUTS(">\r\n");
	Depth++;
}

static LPCSTR  get_symname(LPCSTR *attr)
{
	LPCSTR pnam=getAttrVal(attr,"xlink:href");
	if(pnam && pnam[0]=='#') pnam++;
	return pnam;
}

static LPCSTR getStyleValPtr(LPCSTR p,LPCSTR typ)
{
	if(p && (p=strstr(p,typ))) p+=strlen(typ);
	return p;
}

static bool isFilterPresent(LPCSTR *attr)
{
	LPCSTR p=getStyleAttr(attr);
	return getStyleValPtr(p,strFilter)!=NULL;
}

static BOOL getFilterName(LPCSTR pStyle,char *fName)
{
	if(pStyle) {
		LPCSTR p;
		pStyle=getStyleValPtr(pStyle,strFilter);
		if(pStyle && (p=strchr(pStyle,')'))) {
			int len=p-pStyle;
			if(len>=SIZ_FILTERNAME) len=SIZ_FILTERNAME-1;
			memcpy(fName,pStyle,len);
			fName[len]=0;
			return TRUE;
		}
	}
	*fName=0;
	return FALSE;
}

static int save_style(char **style,LPCSTR st)
{
	int i;
	char fName[SIZ_FILTERNAME];

	if(!st) return MAX_STYLES;
	for(i=0;i<MAX_STYLES;i++) {
		if(!style[i]) {
			if(!(style[i]=(char *)malloc(strlen(st)+1))) {
				i=MAX_STYLES;
			}
			else {
				strcpy(style[i],st);
				style[i+1]=NULL;
				if(style==legendstyle) {
					if(getFilterName(st,fName)) {
					  isSymFilter[save_style(symname,fName)]=true;
					}
				}
				else if(style==symname) {
					isSymFilter[i]=false;
				}
			}
			break;
		}
		else if(!strcmp(st,style[i])) break;
	}

	return i;
}

static int save_symname(LPCSTR *attr)
{
	LPCSTR pnam=get_symname(attr);
	return pnam?(save_style(symname,pnam)+1):(MAX_STYLES+1);
}

static void free_styles(char **style)
{
	for(int i=0;i<MAX_STYLES;i++) {
		if(!style[i]) break;
		free(style[i]);
	}
	style[0]=NULL;
}

static void transfer_groupstyles()
{
	for(LPSTR *style=groupstyle;*style;style++) {
		save_style(legendstyle,*style);
	}
}

static int outLegendPath(LPCSTR el,LPCSTR *attr)
{
	nCol=skip_depth();
	nCol+=gz_printf("<%s",el);
	for(;attr[0];attr+=2) {
		nCol+=gz_printf(" %s=\"",attr[0]);
		if(!strcmp(attr[0],"d") || !strcmp(attr[0],"points")) {
			gz_outPath(attr[1],false);
			OUTC('\"');
		}
		else if(!strcmp(attr[0],"style")) {
			nCol+=outStyle('l',attr[1]);
		}
		else nCol+=gz_printf("%s\"",attr[1]);
	}
	OUTS("/>\r\n");
	return 0;
}

static void outSavedTextElement()
{
	skip_depth();
	gz_printf("<text %s",fmtCTM(textCTM));
	if(lastTextStyleIdx>=0) {
		OUTS(" style=\"");
		if(lastTextStyleIdx>0) gz_printf("&_%c%d;",
			(iLevelSym || bSymbolScan || (iGroupFlg&FLG_LEGEND))?'l':'t',lastTextStyleIdx);
		else OUTS(lastTextStyle);
		OUTC('\"');
	}
	gz_printf(">%s</text>\r\n",trim_refbuf());
}

static BOOL IsRefbufVisible()
{
	for(char *p=refbuf;*p;p++) if(!isspace((BYTE)*p)) return TRUE;
	//reflen=0;
	//*refbuf=0;
	return FALSE;
}

static BOOL IsAngleVertical(double angle)
{
	return (fabs(fabs(angle)-90.0)<=0.0011);
}

static void saveTextElement(LPCSTR *attr,bool bTspan)
{
	LPCSTR p,pStyle;
	int styleIdx;
	static bool bLastTspan;
	static double lastAngle,lastY,lastX;

	if((pStyle=getStyleAttr(attr)) && strcmp(pStyle,strStyleIgnored)) {
		styleIdx=find_style((iLevelSym || bSymbolScan || (iGroupFlg&FLG_LEGEND))?legendstyle:textstyle,pStyle);
	}
	else {
		styleIdx=-1;
		pStyle="";
	}

	if(bTspan) {
		double angle=0.0,x=0.0,y=0.0;
		if((p=getAttrVal(attr,"rotate"))) angle=atof(p);
		if((p=getAttrVal(attr,"x"))) x=atof(p);
		if((p=getAttrVal(attr,"y"))) y=atof(p);

		if(bLastTspan) {
		  //A previous tspan component was encountered.
		  //Check if this is a continuation (AI11 bug) --
		  if(angle==lastAngle && styleIdx==lastTextStyleIdx &&
			    (styleIdx>=0 || !strcmp(pStyle,lastTextStyle)) &&
				((fabs(lastY-y)<=0.0011 && !IsAngleVertical(angle)) ||
				(fabs(lastX-x)<=0.0011 && IsAngleVertical(angle)))) {
			  return; //ignore tspan break -- continue adding to refbuf
		  }
		  if(IsRefbufVisible()) {
			  //previous tspans have accumulated non-blank text with different attributes --
			  if(!iLevelSym && !bTextParagraph) {
				  skip_depth();
				  OUTS("<g>\r\n");
				  Depth++;
				  bTextParagraph=true;
			  }
			  outSavedTextElement(); //outputs trimmed refbuf (cleared when this fcn exits)
		  }
		  //else treat as initial tspan (?)
		}

		lastAngle=angle;
		lastX=x;
		lastY=y;

		if(!iLevelSym && !bSymbolScan && !(iGroupFlg&FLG_LEGEND)) {
			x*=scaleRatio;
			y*=scaleRatio;
		}

		SVG_CTM ctm={1.0,0.0,0.0,1.0,x,y};

		if(angle!=0.0) {
			angle*=U_DEGREES;
			ctm.a=ctm.d=cos(angle);
			ctm.c=-(ctm.b=sin(angle));
		}
		mult(&pathCTM,&ctm);
		textCTM=ctm;
	}
	else {
		if(!bPathCTM) initPathCTM();
		textCTM=pathCTM;
		bTextParagraph=false;
		bInTextElement=1;
	}

	lastTextStyleIdx=styleIdx;
	trx_Stncc(lastTextStyle,pStyle,SIZ_LASTTEXTSTYLE);

	bLastTspan=bTspan;
	*refbuf=0;
	reflen=0;
}

static void outGroup(int id)
{
	skip_depth();
	gz_printf("<g id=\"w2d_%s\"/>\r\n",w2d_name[id]);
}

static void out_entity_line(LPCSTR name,char *value)
{
	if(nCol) OUTS("\r\n");
	skip_depth();
	gz_printf("<!ENTITY %s \"%s\">\r\n",name,value);
	nCol=0;
}

static int outWallsSubgroup(int svg_typ)
{
	int e;
	skip_depth();
	gz_printf("<g id=\"%s\"%s>\r\n",grp_id[svg_typ],
		(svg_typ==W2D_LRUDS)?" style=\"&D_s;\"":"");
	Depth++;
	e=grp_fcn[svg_typ]();
	Depth--;
	skip_depth();
	OUTS("</g>\r\n");
	return e;
}

static int outWallsGroup(int svg_typ,int svg_flg)
{
  int e=0;
  BOOL bNone,bHide;

  if(svg_typ==W2D_FRAME) {
	  bNone=fFrameThick<=0.0; 
	  bHide=FALSE;
  }
  else {
	  bNone=(svg_flg&flags)==0 ||
		(svg_typ==W2D_FLAGS && !numFlagLocations) ||
		(svg_typ==W2D_NOTES && !numNoteLocations);

	  bHide=bNone || ((flags>>16)&svg_flg)!=0;
  }
  
  if(bNone && !(flags&SVG_ADJUSTABLE)) return 0;

  Depth=1;
  gz_printf("\t<g id=\"%s\"%s",grp_id[svg_typ],bHide?" style=\"display:none\"":"");
  if(bNone) OUTS("/>\r\n");
  else {
	  OUTS(">\r\n");
	  Depth++;
	  if(svg_typ==W2D_SURVEY) {
		  e=outWallsSubgroup(W2D_VECTORS);
		  if(!e && (pSVG->advParams.uFlags&SVG_ADV_LRUDBARS)) e=outWallsSubgroup(W2D_LRUDS);
		  if(!e) e=outWallsSubgroup(W2D_MARKERS);
	  }
	  else e=grp_fcn[svg_typ]();
	  OUTS("\t</g>\r\n");
	  Depth--;
  }
  return e;
}

static char *colorstr(char *fbuf,COLORREF clr)
{
	if(clr==(COLORREF)-1) strcpy(fbuf,"none   ");
	else sprintf(fbuf,"#%06X",trx_RevDW(clr)>>8);
	return fbuf;
}

static COLORREF colorval(LPCSTR cbuf)
{
	if(cbuf) {
		DWORD dw;
		sscanf(cbuf,"%6x",&dw);
		return (COLORREF)trx_RevDW(dw<<8);
	}
	return (COLORREF)-1;
}

static void store_style_attr(char *pos,char *str)
{
	char *p=strchr(pos,';');
	if(!p) {
		p=pos+strlen(pos);
		*p=';'; p[1]=0;
	}
	int len=strlen(str);
	if(p-pos!=len) memmove(pos+len,p,strlen(p)+1);
	memcpy(pos,str,len);
}

static LPCSTR getColorPtr(LPCSTR p,LPCSTR typ)
{
	if((p=getStyleValPtr(p,typ)) && *p=='#' && p[7]==';') return ++p;
	return NULL;
}

static COLORREF getColorAttr(LPCSTR p,LPCSTR typ)
{
	return (p=getColorPtr(p,typ))?colorval(p):(COLORREF)-1;
}

static void store_color(LPCSTR typ,COLORREF clr)
{
	char clrbuf[10];
	char *p=(char *)getColorPtr(MrgBuff,typ);
	if(p) memcpy(p-1,colorstr(clrbuf,clr),7);
}

static void out_color_entity(LPCSTR name,LPCSTR typ, COLORREF clr)
{
	store_color(typ,clr);
	out_entity_line(name,MrgBuff);
}

static void out_grid_entity(LPCSTR entityName)
{
	strcat(MrgBuff,"opacity:0.5;");
	out_color_entity(entityName,st_stroke,cGridClr);
}

static void out_frame_entity()
{
	if(fFrameThick<=0.0) return;
	char *p=(char *)getStyleValPtr(MrgBuff,st_stroke_width);
	if(p) store_style_attr(p,fltstr(fFrameThick));
	out_entity_line("F_s",MrgBuff);
}

static void out_font_entity(LPCSTR name,COLORREF clr,double size)
{
	char *p=strstr(MrgBuff,st_font_size);
	if(p) store_style_attr((char *)p+10,fltstr(size));
	if(clr!=-1) store_color(st_fill,clr);
	out_entity_line(name,MrgBuff);
}

static void outFlagScaleEntities()
{
	int iSym;
	double d;
	for(UINT s=0;s<numFlagStyles;s++)
		if(pFS[s].wFlag>0) {
			//instances exist in frame --
			if(!bUseStyles || !(iSym=find_style(symname,pFS[s].pFlagname)) || !(d=flagscale[iSym])) {
				d=pFS[s].wSize/2.0;
			}
			out_entity_line(pFS[s].pFlagname,fltstr(d*scaleRatio));
		}
}

static void ExtractStyleArg(char *dst,char *src,const char *typ,BOOL bForced)
{
	char *p=strstr(src,typ);
	if(p) {
		char *srcbrk=p;
		int len=strlen(typ);
		char *srcnxt=strchr(p+=len,';');
		if(!srcnxt) srcnxt=p+strlen(p);
		else srcnxt++;
		strcpy(dst,typ);
		dst+=len;
		while(TRUE) {
			strcpy(dst,fltstr(atof(p)*scaleRatio));
			dst+=strlen(dst);
			if(!(p=strchr(p,' ')) || p>=srcnxt) break;
			p++;
			if(!isdigit((BYTE)*p)) break;
			*dst++=' ';
		}
		*dst++=';'; *dst=0;
		strcpy(srcbrk,srcnxt);
	}
	else if(bForced && strstr(src,"stroke:#")) {
		//If there is a stroke, then replace assumed stroke-width:1 with scaled value --
		sprintf(dst,"%s%s;",typ,fltstr(scaleRatio));
	}
	else *dst=0;
}

static void outStyles(char **style,int pfx,BOOL bScale)
{
	char *st;
	char buf[16];
	char stroke_width[32];
	char font_size[32];
	char stroke_dasharray[128];
	char letter_spacing[32];
	char stylebuf[256];

	int *pIdx=NULL;

	if(bMerge2) {
		switch(pfx) {
			case 'l' : pIdx=&nLidx;
				break;
			case 't' : pIdx=&nTidx;
				break;
			case 'p' : pIdx=&nPidx;
				break;
		}
		if(pIdx && !bMerging2) {
			*pIdx=1; //next entity number for this type
		}
	}

	for(int s=0;s<MAX_STYLES;s++) {
		if(!(st=style[s])) {
			break;
		}
		if(pfx=='p' && !pathstyle_used[s]) continue;
		if(bScale) {
			st=strcpy(stylebuf,st);
			if(pfx=='t') {
				*font_size=*letter_spacing=0;
				ExtractStyleArg(font_size,st,st_font_size,FALSE);
				ExtractStyleArg(letter_spacing,st,st_letter_spacing,FALSE);
				if(*font_size) strcat(st,font_size);
				if(*letter_spacing) strcat(st,letter_spacing);
			}
			else {
				*stroke_dasharray=*stroke_width=0;
				ExtractStyleArg(stroke_width,st,st_stroke_width,TRUE);
				ExtractStyleArg(stroke_dasharray,st,st_stroke_dasharray,FALSE);
				if(*stroke_width) strcat(st,stroke_width);
				if(*stroke_dasharray) strcat(st,stroke_dasharray);
			}
		}
		sprintf(buf,"_%c%d",pfx,s+1);

		if(bMerge2 && pIdx) {
			SVG_ENTITY *pe;
			if(!bMerging2) {
				//save entity for final file --
				pe=&svg_entity[svg_entitycnt++];
				strcpy(pe->name,buf);
				pe->style=strdup(st);
				*pIdx=s+2; //potential starting index for this type in $2.svg
			}
			else {
				int i=0;
				for(;i<svg_entitycnt;i++) {
					pe=&svg_entity[i];
					if(pfx==pe->name[1] && !strcmp(st,pe->style)) {
						//style present, but may need to use a different name
						break;
					}
				}
				if(i==svg_entitycnt) {
					//new style encountered in in $2.svg
					//if(!svg_entitycnt_div) svg_entitycnt_div=i;
					pe=&svg_entity[svg_entitycnt++];
					pe->style=strdup(st);
					sprintf(pe->name,"_%c%d",pfx,*pIdx);
					(*pIdx)++;
				}
				svg_entity_idx[svg_entityidxcnt].idx=i;
				strcpy(svg_entity_idx[svg_entityidxcnt++].name,buf);
#ifdef _DEBUG
				out_entity_line(buf,st); //won't need this in $2.svg unless displayed
#endif
				continue; // speeds final merge but $2.svg invalid
			}
		}
		out_entity_line(buf,st);
	}
}

static void free_entities()
{
	for(int i=0;i<svg_entitycnt;i++) {
		free(svg_entity[i].style);
	}
	svg_entitycnt=0;
	if(idNameBuf) {
		VEC_INT().swap(vIdNameOff);
		VEC_NAME_MAP().swap(vNameMap);
		free(idNameBuf);
		idNameBuf=NULL;
	}
}

static int outFlagSymbols()
{
	char fill[10];
	char stroke[10];
	char opacity[4];

	for(UINT s=0;s<numFlagStyles;s++) 
		if(pFS[s].wFlag>0 && pFS[s].bShape<4 && !find_style(symname,pFS[s].pFlagname)) {
			SVG_TYP_FLAGSTYLE *pfs=&pFS[s];
			strcpy(opacity,"1");
			colorstr(stroke,pfs->color);
			if(pfs->bShape==3) {
				//plus sign
				*fill=0;
			}
			else if(pfs->bShade==0) {
				//clear
				strcpy(fill,"none");
			}
			else if(pfs->bShade==1) {
				//solid
				colorstr(fill,pfs->color);
				strcpy(stroke,"black");
			}
			else if(pfs->bShade==2) {
				//opaque
				colorstr(fill,cPassageClr);
			}
			else {
				//transparent
				colorstr(fill,pfs->color);
				strcpy(stroke,"black");
				strcpy(opacity,"0.5");
			}
			gz_printf(pSymbolStr[pfs->bShape],pfs->pFlagname,fill,stroke,opacity);
			nCol=0;
		}
	return 0;
}

static int get_idstr(char *idstr,char *prefix,char *label,BOOL bForcePrefix)
{
	char *p0=idstr;
	char *p=prefix;
	int iLabel=0;
	*p0++='_';
	while(TRUE) {
	  if(!*p) {
		  if(iLabel) break;
		  iLabel++;
		  p=label;
		  if(p0-idstr>1) *p0++='.';
		  continue;
	  }
	  if(*p>='A' && *p<='Z' || *p>='a' && *p<='z' || *p>='0' && *p<='9') *p0++=*p;
	  else p0+=sprintf(p0,"-%02x",(UINT)*(unsigned char *)p);
	  p++;
	}
	*p0=0;

	iLabel=p0-idstr;
	if(!bForcePrefix && !isdigit((BYTE)idstr[1]) && idstr[1]!='-') {
		memmove(idstr,idstr+1,iLabel--);
	}
	return iLabel;
}

static char *get_vecID(char *idstr,SHP_TYP_STATION *pFr,SHP_TYP_STATION *pTo)
{
	int i=get_idstr(idstr,pFr->prefix,pFr->label,FALSE);
	get_idstr(idstr+i,pTo->prefix,pTo->label,TRUE);
	return idstr;
}

static void outSymInstance(SVG_VIEWBOX *pvb,LPCSTR pName,double x,double y,BOOL bScaled)
{
	skip_depth();
	gz_printf("<use xlink:href=\"#%s\"",pName);

	gz_printf(" width=\"%s\"",fltstr(pvb->width));
	gz_printf(" height=\"%s\"",fltstr(pvb->height));
	gz_printf(" x=\"%s\"",fltstr(-pvb->width/2.0));
	gz_printf(" y=\"%s\" transform=\"matrix(",fltstr(-pvb->height/2.0));

	if(bScaled) gz_printf("&%s; 0 0 -&%s; ",pName,pName);
	else OUTS("1 0 0 -1 ");
	OUTS(fltstr(x));
	OUTC(' ');
	OUTS(fltstr(y));
	OUTS(")\"/>");
	OUTS("\r\n");
}

static int outFlags()
{
	if(!(flags&SVG_FLAGS)) return 0;

	UINT n=numFlagStyles;
	SVG_TYP_FLAGLOCATION *pfl;
	SVG_TYP_FLAGSTYLE *pfs;
	SVG_VIEWBOX vb={2.0f,2.0f};
	//SVG_VIEWBOX *pvb;
	//UINT iSym;

	while(n-->0) {
		for(UINT l=0;l<numFlagLocations;l++) {
			pfl=&pFL[l];
			pfs=&pFS[pfl->flag_id];
			if(pfs->wPriority!=n) continue;

			//if(!fpmrg || !bUseStyles || !(iSym=find_style(symname,pfs->pFlagname))) pvb=&vb;
			//else pvb=&flagViewBox[iSym];

			outSymInstance(&vb,pfs->pFlagname,pfl->xy[0],pfl->xy[1],TRUE); //bScaled=TRUE
		}
	}
	return 0;
}

static BOOL getLabelRec(LPCSTR pID)
{
	char key[32];

	trx_Stncp(key,pID,32);
	if(key[*key]=='_') key[0]--;
	trx.UseTree(SVG_TRX_LABELS);
	BOOL bRet=!trx.Find(key) && !trx.GetRec(&labelRec,sizeof(SVG_TYP_TRXLABEL));
	trx.UseTree(SVG_TRX_VECTORS);
	return bRet;
}

static int outNotes()
{
	if(!pGetData(SHP_NOTELOCATION,NULL)) return 0;

	SHP_TYP_NOTELOCATION sN;
	double xy[2];
	double n_x=fNoteSize*0.15;
	double n_y=fNoteSize*-0.15;

	while(pGetData(SHP_NOTELOCATION,&sN)) {
		mult(&utm_to_xy,sN.xyz,xy);
		if(!isInside(xy)) continue;

		bool bOverride=false;

		if(bUseLabelPos || (flags&SVG_ADJUSTABLE)) {
			get_idstr(argv_buf,sN.prefix,sN.label,FALSE);
			if(bUseLabelPos && getLabelRec(argv_buf)) {
				//labelRec has the label's offset in final page units.
				//We'll override the offset if this station has note in merge file --
				if(labelRec.bPresent&2)	bOverride=true;
			}
		}

		if(bOverride) {
			xy[0]+=labelRec.nx; xy[1]+=labelRec.ny;
		}
		else {
			xy[0]+=n_x; xy[1]+=n_y;
		}

		skip_depth();
		if(flags&SVG_ADJUSTABLE) gz_printf("<g id=\"%s_\">",argv_buf);
		OUTS("<text style=\"&N_s;\" x=\"");
		OUTS(fltstr(xy[0]));
		OUTS("\" y=\"");
		OUTS(fltstr(xy[1]));
		OUTS("\">");
		out_xmlstr(sN.note);
		OUTS("</text>");
		if(flags&SVG_ADJUSTABLE) OUTS("</g>");
		OUTS("\r\n");
	}
	return 0;
}

static int outBackground()
{
	skip_depth();
	OUTS("<path style=\"&B_s;\" d=\"M0,0H");
	OUTS(fltstr(width));
	OUTC('V');
	OUTS(fltstr(height));
	OUTS("H0z\"/>\r\n");
	return 0;
}

//Grid fcns ----------------------------------
enum e_outcode {O_LEFT=1,O_RIGHT=2,O_BOTTOM=4,O_TOP=8};

UINT OutCode(double x,double y)
{
	UINT code=0;
	if(x<grid_xMin) code=O_LEFT;
	else if(x>grid_xMax) code=O_RIGHT;
	if(y<grid_yMin) code |= O_TOP;
	else if(y>grid_yMax) code |= O_BOTTOM;
	return code;
}

static apfcn_b ClipEndpoint(double x0,double y0,double *px1,double *py1,UINT code)
{
  //If necessary, adjusts *px1,*py1 to point on frame without changing slope.
  //Returns TRUE if successful (line intersects frame), otherwise FALSE.
  //Assumes code==OutCode(*px1,*py1) and that OutCode(x0,y0)&code==0;

  double slope;

  if(code&(O_RIGHT|O_LEFT)) {
	slope=(*py1-y0)/(*px1-x0);
    *px1=(code&O_RIGHT)?grid_xMax:grid_xMin;
    *py1=y0+slope*(*px1-x0);
	if(*py1<grid_yMin) code=O_TOP;
	else if(*py1>grid_yMax) code=O_BOTTOM;
	else code=0;
  }

  if(code&(O_TOP|O_BOTTOM)) {
	slope=(*px1-x0)/(*py1-y0);
    *py1=(code&O_BOTTOM)?grid_yMax:grid_yMin;
    *px1=x0+slope*(*py1-y0);
	if(*px1<grid_xMin) code=O_LEFT;
	else if(*px1>grid_xMax) code=O_RIGHT;
	else code=0;
  }

  return code==0;
}

static apfcn_b ClipEndpoints(double &x0, double &y0, double &x1, double &y1)
{
	UINT out0=OutCode(x0,y0);
	UINT out1=OutCode(x1,y1);
	if((out0&out1) ||
	   (out1 && !ClipEndpoint(x0,y0,&x1,&y1,out1)) ||
	   (out0 && !ClipEndpoint(x1,y1,&x0,&y0,out0))) return FALSE;
	return TRUE;
}

static apfcn_v GetRange(double &minE,double &maxE,double &minN,double &maxN,double x,double y)
{
	double Tx=cosA*(x-xMid)-sinA*(y-yMid);
	double Ty=-cosA*(y-yMid)-sinA*(x-xMid);
	double d;
	
	d=Tx/scale+midE;
	if(d<minE) minE=d;
	if(d>maxE) maxE=d;
	
	d=Ty/scale+midN;
	if(d<minN) minN=d;
	if(d>maxN) maxN=d;
}

static apfcn_d GridX(double E,double N)
{
	return scale*((E-midE)*cosA-(N-midN)*sinA)+xMid;
}

static apfcn_d GridY(double E,double N)
{
	return -scale*((E-midE)*sinA+(N-midN)*cosA)+yMid;
}

static void outPt(double x,double y)
{
	OUTS(fltstr(x));
	OUTC(' ');
	OUTS(fltstr(y));
}

static void outGridLn(double *xy0,double *xy1,char c)
{
	char x0s[40],y0s[40],x1s[40],y1s[40];

	if(!ClipEndpoints(xy0[0],xy0[1],xy1[0],xy1[1])) return;

	skip_depth();
	gz_printf("<path style=\"&G_%c;\" d=\"M",c);
	fltstr(x0s,xy0[0]);
	fltstr(y0s,xy0[1]);
	fltstr(x1s,xy1[0]-xy0[0]);
	fltstr(y1s,xy1[1]-xy0[1]);
	OUTS(x0s);
	OUTC(' ');
	OUTS(y0s);
	if(!strcmp(x1s,"0")) {
		OUTC('v');
		OUTS(y1s);
	}
	else if(!strcmp(y1s,"0")) {
		OUTC('h');
		OUTS(x1s);
	}
	else {
		OUTC('l');
		OUTS(x1s);
		OUTC(' ');
		OUTS(y1s);
	}
	OUTS("\"/>\r\n");
}

static int outGrid()
{
	if(!(flags&SVG_GRID)) return 0;

	UINT numSubs=pSVG->gridsub;
	double fIntval=pSVG->gridint/numSubs;

	if(fIntval*scale<width/300) return 0;

	double minE=DBL_MAX,maxE=-DBL_MAX,minN=DBL_MAX,maxN=-DBL_MAX;
	double f,xy0[2],xy1[2],xyz0[2],xyz1[2];

	grid_xMin=grid_yMin=fFrameThick;
	grid_xMax=width-fFrameThick;
	grid_yMax=height-fFrameThick;

	GetRange(minE,maxE,minN,maxN,grid_xMin,grid_yMin);
	GetRange(minE,maxE,minN,maxN,grid_xMin,grid_yMax);
	GetRange(minE,maxE,minN,maxN,grid_xMax,grid_yMin);
	GetRange(minE,maxE,minN,maxN,grid_xMax,grid_yMax);
	
	UINT iSub=0;
	
	//Draw North-South gridlines --
	f=minE-fmod(minE,pSVG->gridint);
	if(f>minE) f-=pSVG->gridint;
	while(f<maxE) {
		xyz0[0]=f; xyz0[1]=maxN;
		xyz1[0]=f; xyz1[1]=minN;
		mult(&utm_to_xy,xyz0,xy0);
		mult(&utm_to_xy,xyz1,xy1);
		outGridLn(xy0,xy1,'1');
		if(numSubs>1 && !(iSub%numSubs)) outGridLn(xy0,xy1,'0'); //Major grid line
		f+=fIntval;
		iSub++;
	}

	//Draw West-East gridlines --
	iSub=0;
	f=minN-fmod(minN,pSVG->gridint);
	if(f>minN) f-=pSVG->gridint;
	while(f<maxN) {
		xyz0[0]=minE; xyz0[1]=f;
		xyz1[0]=maxE; xyz1[1]=f;
		mult(&utm_to_xy,xyz0,xy0);
		mult(&utm_to_xy,xyz1,xy1);
		outGridLn(xy0,xy1,'1');
		if(numSubs>1 && !(iSub%numSubs)) outGridLn(xy0,xy1,'0');
		f+=fIntval;
		iSub++;
	}
	return 0;
}
//--------------------------------------------
static void outSymInstances()
{
	int i;
	double h,lasth,margin=fFrameThick+0.5*fTitleSize;

	for(i=0;i<MAX_STYLES;i++) {
		if(!symname[i] || !isSymFilter[i]) break;

	}
	if(!symname[i]) return;

	skip_depth();
	gz_printf("<g id=\"w2d_Symbols\" style=\"display:none\">\r\n");
	Depth++;
	lasth=margin;
	for(i=0;i<MAX_STYLES;i++) {
		if(!symname[i]) break;
		if(isSymFilter[i]) continue;
		h=flagViewBox[i+1].height;
		outSymInstance(&flagViewBox[i+1],symname[i],
			margin+0.5*flagViewBox[i+1].width,lasth+0.5*h,FALSE);
		lasth+=h;
	}
	Depth--;
	skip_depth();
	OUTS("</g>\r\n");
}

static int outRef()
{
	skip_depth();
	OUTS("<text style=\"&T_s;\" x=\"");
	OUTS(fltstr(fFrameThick+0.25*fTitleSize));
	OUTS("\" y=\"");
	OUTS(fltstr(height-fFrameThick-0.25*fTitleSize));
	OUTS("\">");
	out_xmlstr(pSVG->title);
	OUTS(" Plan: ");
	OUTS(fltstr(pSVG->view,6));
	OUTS(" Scale: 1:");
	OUTS(fltstr((72*12)/(scale*0.3048),6));

	double f=(flags&SVG_FEET)?(1.0/0.3048):1.0;

	OUTS(" Frame: ");
	OUTS(fltstr(f*width/scale,3));
	OUTC('x');
	OUTS(fltstr(f*height/scale,3));
	OUTS((flags&SVG_FEET)?"f":"m");
	OUTS(" Center: ");
	OUTS(fltstr(f*midE,3));
	OUTC(' ');
	OUTS(fltstr(f*midN,3));
	OUTS("</text>\r\n");
	return 0;
}

static char *get_idkey(char *buf,SHP_TYP_STATION *pFr,SHP_TYP_STATION *pTo)
{
	char *p=buf+1;

	p+=strlen(strcpy(p,pFr->prefix)); *p++=0;
	p+=strlen(strcpy(p,pFr->label)); *p++=0;
	p+=strlen(strcpy(p,pTo->prefix)); *p++=0;
	p+=strlen(strcpy(p,pTo->label));
	*buf=(char)(p-buf-1);
	return buf;
}

static int find_vecColor(COLORREF c)
{
	for(IT_RVC itr=vecColor.rbegin();itr!=vecColor.rend();itr++) {
		if(itr->cr==c) {
			#ifdef _DEBUG
				int jj=(itr.base()-vecColor.begin());
				assert(jj>0 && jj<=numVecColors);
			#endif
			return (itr.base()-vecColor.begin()); //index+1
		}
	}
	return 0;
}

static int add_vecColor(LPCSTR p)
{
	COLORREF c=colorval(p);
	assert(vecColor.size()==numVecColors);
	int i=find_vecColor(c);
	if(!i) {
		vecColor.push_back(CLR_DATA(c,1));
		return numVecColors++;
	}
	vecColor[--i].cnt++;
	return i;
}

static void get_station_ptrs(SHP_TYP_VECTOR *psV,SHP_TYP_STATION **pF,SHP_TYP_STATION **pT)
{
	SHP_TYP_STATION *pFr,*pTo;
	int i;

	pFr=&psV->station[0];
	pTo=&psV->station[1];
	if((i=strcmp(pFr->prefix,pTo->prefix))==0) i=strcmp(pFr->label,pTo->label);
	if(i>0) {
		*pF=pTo;
		*pT=pFr;
	}
	else {
		*pF=pFr;
		*pT=pTo;
	}
}

static bool isStationInFrame(SHP_TYP_STATION *pS)
{
	double xy[2];
	mult(&utm_to_xy,pS->xyz,xy);
	return isInside(xy);
}

static int createIndex(void)
{
	int i;
	SHP_TYP_VECTOR sV;
	SHP_TYP_STATION *pFr,*pTo;
	SVG_TYP_TRXVECTOR rec;
	char idstr[64];

	if(!pGetData(SHP_VECTORS,NULL)) {
		return SVG_ERR_NOVECTORS;
	}
	siz_vecrec=bNTW?sizeof(SVG_TYP_TRXVECTOR_NTW):sizeof(SVG_TYP_TRXVECTOR);

	strcpy(argv_buf,pSVG->svgpath);
	*trx_Stpext(argv_buf)=0;
	if(trx.Create(argv_buf,siz_vecrec,
		0,bUseLabelPos+1,0,1024) || trx.AllocCache(32))
		return SVG_ERR_INDEXINIT;
	trx.SetUnique(TRUE); trx.SetExact(TRUE);

	if(bUseLabelPos) {
		trx.UseTree(SVG_TRX_LABELS);
		if(trx.InitTree(sizeof(SVG_TYP_TRXLABEL),TRX_MAX_KEYLEN,
			CTRXFile::KeyUnique|CTRXFile::KeyExact)) return SVG_ERR_INDEXINIT;
		trx.UseTree(SVG_TRX_VECTORS);
	}

	//Index current version of survey vectors provided by Walls,
	//eliminating any duplicates.
	//Index will not depend on frame dimensions --

	sV.titles=sV.attributes=NULL; //Used only in WALLSHPX.DLL

	vecColor.clear();
	numVecColors=0;

	while(pGetData(SHP_VECTORS,&sV)) {
		get_station_ptrs(&sV,&pFr,&pTo);
		if(flags&SVG_MERGEDCONTENT) {
			memcpy(rec.fr_xy,pFr->xyz,sizeof(rec.fr_xy));
			memcpy(rec.to_xy,pTo->xyz,sizeof(rec.to_xy));
		}
		if(!bNTW) {
			if(isStationInFrame(pFr) || isStationInFrame(pTo)) { 
				rec.wVecColor=add_vecColor(sV.linetype); //e>=0
			}
			else rec.wVecColor=(WORD)-1;
		}
		if(i=trx.InsertKey(&rec,get_idkey(idstr,pFr,pTo))) {
			if(i!=CTRXFile::ErrDup)	return SVG_ERR_INDEXWRITE;
			if(!bNTW && rec.wVecColor!=(WORD)-1) {
				vecColor[rec.wVecColor].cnt--; //avoid unused vec color entity
				assert(vecColor[rec.wVecColor].cnt>=0);
			}
		}
	}

	if(trx.NumRecs()) return 0;

	return SVG_ERR_NOVECTORS;
}

static int outVectors()
{
	SHP_TYP_VECTOR sV;
	SHP_TYP_STATION *pFr,*pTo;
	SVG_TYP_TRXVECTOR rec;
	double xyFr[2],xyTo[2];
	char idstr[64];
	int iVecColor;

    if(!(flags&SVG_VECTORS)) return 0;

	sV.titles=sV.attributes=NULL; //Used only in WALLSHPX.DLL
	if(!pGetData(SHP_VECTORS,NULL)) return 0;

	while(pGetData(SHP_VECTORS,&sV)) {
		get_station_ptrs(&sV,&pFr,&pTo);

		if(trx.Find(get_idkey(idstr,pFr,pTo))) {
			return SVG_ERR_UNKNOWN; //index failure shouldn't happen!
		}
		if(trx.GetRec(&rec,sizeof(SVG_TYP_TRXVECTOR))) {
			return SVG_ERR_UNKNOWN; //index failure shouldn't happen!
		}
		if(rec.wVecColor==WORD(-1)) {
			continue;
		}

		assert((int)vecColor[rec.wVecColor].cnt>0);

		iVecColor=rec.wVecColor; //Color of merged version if present
		rec.wVecColor=(WORD)-1;
		trx.PutRec(&rec);

		mult(&utm_to_xy,pFr->xyz,xyFr);
		mult(&utm_to_xy,pTo->xyz,xyTo);

		if(!isInside(xyFr) && !isInside(xyTo)) {
			assert(0);
			continue;
		}

		skip_depth();
		OUTS("<path ");
		if(flags&SVG_ADJUSTABLE) {
			OUTS("id=\"");
			OUTS(get_vecID(idstr,pFr,pTo));
			OUTS("\" ");
		}

		OUTS("style=\"");

		//Check for color override --
		//if(!bUseStyles) iVecColor=find_vecColor(sV.linetype);
		if(iVecColor && numVecColors>1) gz_printf("&V_%d;",iVecColor);
		else OUTS("&V_s;");

		OUTS("\" d=\"M");
		OUTS(fltstr(xyFr[0]));
		fltstr(idstr,xyFr[1]);
		if(*idstr!='-') OUTC(',');
		OUTS(idstr);
		OUTC('l');
		OUTS(fltstr(xyTo[0]-xyFr[0]));
		fltstr(idstr,xyTo[1]-xyFr[1]);
		if(*idstr!='-') OUTC(',');
		OUTS(idstr);
		OUTS("\"/>\r\n");
	}
	return 0;
}

static int outMarkers()
{
	SHP_TYP_STATION sS;
	double xy[2];

	if(!(flags&SVG_VECTORS) || !pGetData(SHP_STATIONS,NULL)) return 0;
	while(pGetData(SHP_STATIONS,&sS)) {
		mult(&utm_to_xy,sS.xyz,xy);
		if(!isInside(xy)) continue;
		skip_depth();
		OUTS("<use xlink:href=\"#_m\" width=\"2\" height=\"2\" x=\"-1\" y=\"-1\" transform=\"matrix(&_M; 0 0 -&_M; ");
		OUTS(fltstr(xy[0]));
		OUTC(' ');
		OUTS(fltstr(xy[1]));
		OUTS(")\"/>\r\n");
	}
	return 0;
}

static void outLrudBox(char *fbuf,SHP_TYP_LRUD &sL)
{
	#define LRUD_OFFSET 1.0 //offset in meters of left edge of box from bar's right endpoint
	int i;
	double a,h,p[8];
	double l=0.0;

	for(i=0;i<2;i++) {
		p[i]=sL.xyTo[i]-sL.xyFr[i];
		l+=p[i]*p[i];
	}
	l=sqrt(l); //length of bar in meters
	a=atan2(p[0],p[1]); //UTM direction of bar
    //Define box's center of rotation near bar's right end (TO position) --
	p[0]=sL.xyTo[0] + LRUD_OFFSET * sin(a);
	p[1]=sL.xyTo[1] + LRUD_OFFSET * cos(a);

    //Define TL, TR, BR corner points by simply traversing to them.
	//To avoid sin/cos we could have moved orthogonally and used a rotation matrix --
	a-=PI2;
	h=sL.up;
	p[0] += h * sin(a);
	p[1] += h * cos(a);
	a+=PI2;
	p[2]=p[0] + l * sin(a);
	p[3]=p[1] + l * cos(a);
	a+=PI2;
	h+=sL.dn;
	p[4]=p[2] + h * sin(a);
	p[5]=p[3] + h * cos(a);
	a+=PI2;
	p[6]=p[4] + l * sin(a);
	p[7]=p[5] + l * cos(a);

	//Convert to page coordinates --
	for(i=0;i<8;i+=2) {
		mult(&utm_to_xy,p+i);
		if(!isInside(p+i)) return;
	}
	for(i=6;i;i-=2) {
		p[i+1]-=p[i-1];
		p[i]-=p[i-2];
	}

	skip_depth();
	char cmd='M';
	OUTS("<path d=\"");
	for(i=0;i<8;i+=2) {
		OUTC(cmd);
		OUTS(fltstr(p[i]));
		fltstr(fbuf,p[i+1]);
		if(*fbuf!='-') OUTC(',');
		OUTS(fbuf);
		cmd='l';
	}
	OUTS("z\"/>\r\n");
}

static int outLruds()
{
	SHP_TYP_LRUD sL;
	double xyFr[2],xyTo[2];
	char fbuf[40];

	if(!pGetData(SHP_LRUDS,NULL)) return 0;

	while(pGetData(SHP_LRUDS,&sL)) {

		mult(&utm_to_xy,sL.xyFr,xyFr);
		mult(&utm_to_xy,sL.xyTo,xyTo);

		if(!isInside(xyFr) && !isInside(xyTo)) continue;

		skip_depth();
		OUTS("<path d=\"M"); //All lruds have same style

		OUTS(fltstr(xyFr[0]));
		fltstr(fbuf,xyFr[1]);
		if(*fbuf!='-') OUTC(',');
		OUTS(fbuf);
		OUTC('l');
		OUTS(fltstr(xyTo[0]-xyFr[0]));
		fltstr(fbuf,xyTo[1]-xyFr[1]);
		if(*fbuf!='-') OUTC(',');
		OUTS(fbuf);
		OUTS("\"/>\r\n");

		if((sL.flags&LRUD_FLG_CS) && (pSVG->advParams.uFlags&SVG_ADV_LRUDBOXES)) {
			outLrudBox(fbuf,sL);
		}
	}
	return 0;
}

static int outLabels()
{
	if(!pGetData(SHP_STATIONS,NULL)) return 0;

	SHP_TYP_STATION sS;
	double xy[2];
	double l_x=scaleRatio*(pSVG->offLabelX-1);
	double l_y=scaleRatio*(pSVG->offLabelY-1)+fLabelSize;

	while(pGetData(SHP_STATIONS,&sS)) {
		mult(&utm_to_xy,sS.xyz,xy);
		if(!isInside(xy)) continue;

		bool bOverride=false;

		if(bUseLabelPos || (flags&SVG_ADJUSTABLE)) {
			get_idstr(argv_buf,sS.prefix,sS.label,FALSE);
			if(bUseLabelPos && getLabelRec(argv_buf)) {
				//labelRec has the label's offset in final page units.
				//We'll override the offsets or omit label if not in merge file --
				if((labelRec.bPresent&1)==0) continue;
				bOverride=true;
			}
		}

		if(bOverride) {
			xy[0]+=labelRec.sx; xy[1]+=labelRec.sy;
		}
		else {
			xy[0]+=l_x; xy[1]+=l_y;
		}

		skip_depth();
		if(flags&SVG_ADJUSTABLE) gz_printf("<g id=\"%s\">",argv_buf);
		OUTS("<text style=\"&L_s;\" x=\"");
		OUTS(fltstr(xy[0]));
		OUTS("\" y=\"");
		OUTS(fltstr(xy[1]));
		OUTS("\">");
		if((flags&SVG_SHOWPREFIX) && sS.prefix[0]) {
			out_xmlstr(sS.prefix);
			OUTC(':');
		}
		out_xmlstr(sS.label);
		OUTS("</text>");
		if(flags&SVG_ADJUSTABLE) OUTS("</g>");
		OUTS("\r\n");
	}
	return 0;
}

static void GetLineStyle(double t)
{
	if(t<0.01) t=0.01;
	sprintf(MrgBuff,"stroke:#000000;fill:none;stroke-width:%.2lf;",t);
}

//==================================================================
//Handlers for expat --

static void out_veccolor_entity(char *pColor,int i)
{
	char name[8];
	strcpy(name,"V_s");
	if(i) sprintf(name+2,"%d",i);
	char buf[10];
	colorstr(buf,vecColor[i].cr);
	memcpy(pColor,buf+1,6);
	out_entity_line(name,MrgBuff);
}

static void out_entity (void *userData,
                          const XML_Char *entityName,
                          int is_parameter_entity,
                          const XML_Char *value,
                          int value_length,
                          const XML_Char *base,
                          const XML_Char *systemId,
                          const XML_Char *publicId,
                          const XML_Char *notationName)
{
	if(!value) return;
	memcpy(MrgBuff,value,value_length);
	MrgBuff[value_length]=0;

	if(!strcmp(entityName,"_M")) {
		if(flags&SVG_VECTORS) {
			out_entity_line(entityName,fltstr(fMarkerThick));
		}
		outFlagScaleEntities();
		if(fpmrg) {
			outStyles(pathstyle,'p',!bUseLineWidths);
			outStyles(textstyle,'t',TRUE);
			outStyles(legendstyle,'l',FALSE);
		}
		return;
	}

	if(!strcmp(entityName,"G_0")) {
		//Major grid lines --
		if((flags&SVG_GRID) && pSVG->gridsub>1) {
			//Obtain grid line thicknesses based on vector thicknesses --
			GetLineStyle((numGridThick==2)?(fGridThick[1]*scaleRatio):(3*fVectorThick));
			out_grid_entity(entityName);
		}
		return;
	}

	if(!strcmp(entityName,"G_1")) {
		//Minor grid lines --
		if(flags&SVG_GRID) {
			//Obtain grid line thicknesses based on vector thicknesses --
			GetLineStyle((numGridThick>0)?(fGridThick[0]*scaleRatio):fVectorThick);
			out_grid_entity(entityName);
		}
		return;
	}

	if(!strcmp(entityName,"F_s")) {
		out_frame_entity();
		return;
	}
	if(!strcmp(entityName,"B_s")) {
		if(((cBackClr&0xFFFFFF)==0xFFFFFF) &&
			(!bMaskPathSeen || ((cPassageClr&0xFFFFFF)==0xFFFFFF))) {
			strcat(MrgBuff,"opacity:0;");
			out_entity_line(entityName,MrgBuff);
		}
		else out_color_entity(entityName,st_fill,cBackClr);
		return;
	}

	if(bMaskPathSeen && !strcmp(entityName,"P_s")) {
		//Only to be applied to first path of each mask layer group. If both
		//the background and passage color is white, make the opacity zero --
		if(((cBackClr&0xFFFFFF)==0xFFFFFF) && ((cPassageClr&0xFFFFFF)==0xFFFFFF)) {
			strcat(MrgBuff,"fill-opacity:0;");
			out_entity_line(entityName,MrgBuff);
		}
		else out_color_entity(entityName,st_fill,cPassageClr);
		return;
	}

	if(!strcmp(entityName,"V_s")) {
		if(flags&SVG_VECTORS) {
			//Obtain line thickness based on label font size --
			GetLineStyle(fVectorThick);
			//Output vector line styles --
			for(int i=0;i<numVecColors;i++) {
				if(vecColor[i].cnt>0)
				  out_veccolor_entity((char *)MrgBuff+8,i);
			}
		}
		return;
	}

	if(!strcmp(entityName,"D_s")) {
		if((flags&SVG_VECTORS) && (pSVG->advParams.uFlags&SVG_ADV_LRUDBARS)) {
			//Obtain line thickness based on label font size --
			GetLineStyle(fVectorThick);
			out_color_entity(entityName,st_stroke,cLrudClr);
		}
		return;
	}

	if(!strcmp(entityName,"T_s")) {
		//Black frame title --
		out_font_entity(entityName,0,fTitleSize);
		return;
	}
	if(!strcmp(entityName,"N_s")) {
		if(numNoteLocations) {
			if(pNoteStyle) strcpy(MrgBuff,pNoteStyle);
			out_font_entity(entityName,pNoteStyle?-1:pSVG->noteColor,fNoteSize);
		}
		return;
	}
	if(!strcmp(entityName,"L_s")) {
		if(pLabelStyle) strcpy(MrgBuff,pLabelStyle);
		out_font_entity(entityName,pLabelStyle?-1:pSVG->labelColor,fLabelSize);
		return;
	}
	out_entity_line(entityName,MrgBuff);
}

static void out_startDoctype(void *userData,
                               const XML_Char *doctypeName,
                               const XML_Char *sysid,
                               const XML_Char *pubid,
                               int has_internal_subset)
{
	gz_printf("<!DOCTYPE %s ",doctypeName);
	if(pubid && *pubid) gz_printf("PUBLIC \"%s\" ",pubid);
	else if(sysid) OUTS("SYSTEM ");
	if(sysid && *sysid) gz_printf("\"%s\"",sysid);
	if(has_internal_subset) {
		UD(bEntity)=1;
		OUTS(" [\r\n");
	}
	nCol=0;
	Depth=1;
}

static void out_endDoctype(void *userData)
{
	if(UD(bEntity)) {
		OUTS("]>\r\n\r\n");
		UD(bEntity)=0;
	}
	else OUTS(">\r\n");
	nCol=Depth=0;
}

static void out_instruction(void *userData,const XML_Char *target,const XML_Char *data)
{
	gz_printf("<?walls updated=\"no\" merged-content=\"%s\""
		" adjustable=\"%s\" dll-version=" EXP_SVG_VERSION_STR "?>\r\n",
		(flags&SVG_MERGEDCONTENT)?"yes":"no",
		(flags&SVG_ADJUSTABLE)?"yes":"no");
}

static void out_xmldecl(void *userData,
                       const XML_Char  *version,
                       const XML_Char  *encoding,
                       int             standalone)
{
	OUTS("<?xml");
	if(version) gz_printf(" version=\"%s\"",version);
	if(encoding) gz_printf(" encoding=\"%s\"",encoding);
	gz_printf(" standalone=\"%s\"?>\r\n",standalone?"yes":"no");
}

static void out_chardata(void *userData,const XML_Char *s,int len)
{
	//Note: chardata is anything that occurs between tags. EOL appears as an
	//isolated 0xA, thereby breaking up a succession of data into blocks.
	//An end of buffer condition will also presumably cause a break in the
	//stream. Here we will output all data as given while keeping track of
	//column number.

	if(UD(errcode) || bNoOutput) return;
	if(bNoClose) {
			OUTC('>');
			nCol++;
			bNoClose=false;
	}
	memcpy(MrgBuff,s,len);
	MrgBuff[len]=0;
	out_stream(MrgBuff);
}

static void out_comment(void *userData,const XML_Char *s)
{
	if(bNoOutput) return;
	if(!bFirstCommentSeen) {
		//Ignore first comment, a caution message --
		char date[10],time[10];
		_strdate(date);
		_strtime(time);
		sprintf(MrgBuff,"Generator: Walls 2.0 B7, wallsvg.dll "
			EXP_SVG_VERSION_STR ", %s %s",date,time);
		s=MrgBuff;
		bFirstCommentSeen=true;
	}
	if(!bElementsSeen) {
		if(nCol) OUTS("\r\n");
		nCol=skip_depth();
	}
	OUTS("<!--");
	nCol+=4;
	out_stream((char *)s);
	OUTS("-->");
	if(!bElementsSeen) {
		OUTS("\r\n");
		nCol=0;
	}
	else nCol+=3;
}

static int get_grp_typ(LPCSTR pAttr)
{
  int typ;

  for(typ=1;typ<NUM_GROUPS;typ++) {
	  if(!strcmp(grp_id[typ],pAttr)) break;
  }
  return typ;
}

static void out_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;
  bElementsSeen=true;

  int typ;
  BOOL bNone=FALSE,bHide=FALSE;
  LPCSTR pAttr;

  //Now determine typ --
  if(!strcmp(el,"g")) {
	  bNoOutput=true; //No more comments
	  typ=get_grp_typ(pAttr=attr[1]);
  }
  else if(!strcmp(el,"svg")) {
	  typ=W2D_SVG;
	  Depth=0;
  }
  else typ=NUM_GROUPS;

  if(typ==W2D_BACKGROUND) {
	OUTS("\r\n");
	nCol=0;
	if(fpmrg) {
	  bSymbolScan=true;
	  if(outMergedContent()) return; //error condition
	  //else continue parsing XML template --
	  bSymbolScan=false;
	}
	if(!bMarkerWritten) {
		gz_printf(pSymbolStr[FMT_MKR_MARKER],colorstr(argv_buf,cMarkerClr));
	}
	if(!bMerging2)
		outFlagSymbols();
	OUTS("\r\n");
	nCol=0;
	bNoOutput=true;
  }

  if(!nCol && Depth) nCol=skip_depth();

  grp_typ[Depth++]=typ;

  nCol+=gz_printf("<%s", el);

  //Note: Within attribute values, each CTRL character (LF, TAB, etc.)
  //will appear as a space.

  char viewbox_str[80];
  char *pValue;

  for(int i=0;pAttr=attr[i];i+=2) {
	if(nCol>(typ?SIZ_LINEBREAK:SIZ_SVG_LINEBREAK)) {
		OUTS("\r\n");
		nCol=skip_depth();
	}
	else {
		OUTC(' ');
		nCol++;
	}
	pValue=(char *)attr[i+1];
	if(typ==W2D_SVG) {
		if(!strcmp(pAttr,"width")) sprintf(pValue=argv_buf,"%spt",width_str);
		else if(!strcmp(pAttr,"height")) sprintf(pValue=argv_buf,"%spt",height_str);
		else if(!strcmp(pAttr,"viewBox")) {
			sprintf(viewbox_str,"0 0 %s %s",width_str,height_str);
			pValue=viewbox_str;
		}
		else if(!strcmp(pAttr,"style")) {
			sprintf(viewbox_str,"overflow:visible;enable-background:new 0 0 %s %s",width_str,height_str);
			pValue=viewbox_str;
		}
		else trimctrl(pValue); 
	}
	else trimctrl(pValue);

	sprintf(MrgBuff,"%s=\"%s\"",pAttr,pValue);
	out_wraptext(MrgBuff);
  }

  bNoClose=(typ==NUM_GROUPS || typ<0);

  if(!bNoClose) {
	  OUTC('>');
	  if(typ!=W2D_SVG) {
		OUTS("\r\n");
		nCol=0;
	  }
	  else if(!(flags&SVG_ADJUSTABLE)) {
		OUTS("\r\n\r\n\t<!--NOTE: This inner svg clips output to the frame. Remove it for AI 10 compatibility!-->\r\n");
		OUTS("\t<svg width=\"");
		OUTS(fltstr(width));
		OUTS("\" height=\"");
		OUTS(fltstr(height));
		OUTS("\" overflow=\"hidden\">");
	  }

	  if(!bNone && grp_fcn[typ]) UD(errcode)=grp_fcn[typ]();
  }
}

static void out_endElement(void *userData, LPCSTR el)
{
  if(UD(errcode)) return;
  int typ=grp_typ[--Depth];

  if(bNoClose) {
	  if(XML_GetCurrentByteCount(parser)==0) OUTS("/>");
	  else OUTC('>');
	  bNoClose=false;
  }

  if(typ>=NUM_GROUPS || typ==W2D_IGNORE || typ==W2D_SVG) return;

  if(nCol==0) skip_depth();
  gz_printf("</%s>\r\n",el);
  nCol=0;

  if(typ==W2D_REF) {
	//Ignore rest of template --
	udata.errcode=SVG_ERR_ENDSCAN;
  }
}

//=========================================================================
static void walls_instruction(void *userData,const XML_Char *target,const XML_Char *data)
{
	LPCSTR p;

	if(!strcmp(target,"walls")) {
		if(!(strstr(data,"updated=\"no\""))) {
			pSVG->flags|=SVG_UPDATED;
		}
		if(!(strstr(data,"merged-content=\"no\""))) {
			pSVG->flags|=SVG_ADDITIONS;
		}
		if(strstr(data,"adjustable=\"yes\"")) {
			pSVG->flags|=SVG_EXISTADJUSTABLE;
		}
		if(p=strstr(data,"dll-version=\"")) {
			pSVG->version=(UINT)floor(0.5+100*atof(p+13));
		}
	}
	UD(errcode)=SVG_ERR_ENDSCAN;
}

static void first_comment(void *userData,const XML_Char *s)
{
	UD(errcode)=SVG_ERR_ENDSCAN;
}

//=========================================================================

static void free_merge_arrays(int e)
{
	//called at start of finish()

	if(numVecColors) {
		VEC_COLOR().swap(vecColor);
		numVecColors=0;
	}
	if(vUrlNam.size()) {
		for(it_urlnam it=vUrlNam.begin();it!=vUrlNam.end();it++)
			free(it->nam);
		VEC_URLNAM().swap(vUrlNam);
	}

	if(vstk) {free(vstk); vstk=NULL;}
	if(vnod) {free(vnod); vnod=NULL;}
	if(!bNTW) {
		if(e || !bMerge2 || bMerging2) {
			free(pLabelStyle); pLabelStyle=NULL;
			free(pNoteStyle); pNoteStyle=NULL;
		}
		free_styles(groupstyle);
		free_styles(symname);
		free_styles(pathstyle);
		free_styles(textstyle);
		free_styles(legendstyle);

		if(symrange) {free(symrange); symrange=NULL; symcnt=max_symcnt=0;}
		if(pathrange) {free(pathrange); pathrange=NULL; pathcnt=max_pathcnt=0;}
	}
	else {
		if(pnPly) {free(pnPly); pnPly=NULL;}
		if(pNTWp) {free(pNTWp); pNTWp=NULL;}
		if(pNTWf) {free(pNTWf); pNTWf=NULL;}
		if(pNtwRec) {free(pNtwRec); pNtwRec=NULL;}
		if(pMaskName) {free(pMaskName); pMaskName=NULL;}
	}
}

static int close_output(int e0)
{
	int e=0;
	if(gzout) {
		if(gzclose(gzout)) e=SVG_ERR_WRITE;
	}
	if(fpout) {
		if(fclose(fpout)) e=SVG_ERR_WRITE;
	}
	if(!e0) e0=e;
#ifndef _DEBUG
	if(e0 && (fpout || gzout))
		unlink(pathxml);
#endif
	fpout=NULL; gzout=NULL;
	return e0;
}

static int finish(int e)
{
	free_merge_arrays(e);

	if(bWriting) {
		if(close_output(e) && e<=0) e=SVG_ERR_WRITE;
	}
	if(fpin) {gzclose(fpin); fpin=NULL;};
	if(fpmrg) {gzclose(fpmrg); fpmrg=NULL;};

	if(e || !bMerge2 || bMerging2) {
		if(pFS) {free(pFS); pFS=NULL;}
		if(pFL) {free(pFL); pFL=NULL;}
		if(trx.Opened()) trx.CloseDel();
	}
	if(e && bMerge2) {
		free_entities();
	}
	return e;
}

DLLExportC char * PASCAL ErrMsg(int code)
{
  if(code<0) code=-code;
  if(code>SVG_ERR_UNKNOWN) {
	  code=SVG_ERR_UNKNOWN;
  }
  if(code>SVG_ERR_ABORTED && code<=SVG_ERR_WRITE) {
	sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code],trx_Stpnam(pSVG->svgpath));
  }
  else if(code==SVG_ERR_MASKPATHSOPEN) {
	  sprintf(argv_buf,errstr[code],nMaskOpenPaths,nNTW_OpenPathLine);
  }
  else {
	  int len=0;
	  if((flags&SVG_MERGEDCONTENT) && mrgpath && *mrgpath)
		  len=sprintf(argv_buf,"\nFile %s - ",trx_Stpnam(mrgpath));
	  if(parser_ErrorLineNumber>0) len+=sprintf(argv_buf+len,"Line %d: ",parser_ErrorLineNumber);
	  if(code==SVG_ERR_XMLPARSE) strcpy(argv_buf+len,XML_ErrorString((enum XML_Error)parser_ErrorCode));
	  else if(code==SVG_ERR_VECNOTFOUND || code==SVG_ERR_BADELEMENT) {
		  sprintf(argv_buf+len,errstr[code],refbuf);
	  }
	  else if(code==SVG_ERR_W2DGROUP|| code==SVG_ERR_W2DDUP) {
		  sprintf(argv_buf+len, errstr[code], w2d_name[err_groupID]);
	  }
	  else if(code==SVG_ERR_W2DNAME) {
		  sprintf(argv_buf+len, errstr[code], refbuf);
	  }
	  else strcpy(argv_buf+len,errstr[code]);
  }
  strcat(argv_buf,".");
  return argv_buf;
}

//================================================================================

static int parse_file(XML_Parser p,gzFile fp)
{
	int e=0;
	char *buffer;
	UINT bufsize;

	udata.bEntity=0;
	udata.errcode=0;
	udata.errline=0;
	XML_SetUserData(p,&udata);
	XML_SetParamEntityParsing(p,XML_PARAM_ENTITY_PARSING_NEVER);
	XML_SetPredefinedEntityParsing(p,XML_PREDEFINED_ENTITY_PARSING_NEVER);

	if(fp) {
		buffer=MrgBuff;
		bufsize=MRGBUFFSIZE;
	}
	else buffer=pTemplate;
	parser=p;

	for (;;) {
		int done,len;
		if(fp) {
			len=gzread(fp,buffer,bufsize);
			if(len<0) {
				e=SVG_ERR_XMLREAD;
				break;
			}
			done=len<(int)bufsize;
		}
		else {
			len=uTemplateLen;
			done=TRUE;
		}
		if(!XML_Parse(p,buffer,len,done) && !udata.errcode) {
			udata.errline=XML_GetCurrentLineNumber(p);
			udata.errcode=SVG_ERR_XMLPARSE;
			parser_ErrorCode=XML_GetErrorCode(p);
			break;
		}
		if(done || udata.errcode) break;
	}

	if(udata.errcode) {
		if(udata.errcode==SVG_ERR_ENDSCAN) {
			udata.errcode=0;
		}
		else {
			parser_ErrorLineNumber=udata.errline;
			e=udata.errcode;
		}
	}
	XML_ParserFree(p);
	return e;
}

static double init_veclst()
{
	for(int i=0;i<vecs_used;i++) {
		veclst_d[i]=DBL_MAX;
		veclst_i[i]=0;
	}
	return DBL_MAX;
}


static int outFrame()
{
	double t=fFrameThick/2.0;
	skip_depth();
	OUTS("<path style=\"&F_s;\" d=\"");
	OUTC('M');
	OUTS(fltstr(t));
	OUTC(',');
	OUTS(fltstr(t));
	OUTC('H');
	OUTS(fltstr(width-t));
	OUTC('V');
	OUTS(fltstr(height-t));
	OUTC('H');
	OUTS(fltstr(t));
	OUTC('z');
	OUTS("\"/>\r\n");
	return 0;
}

static int outWallsData()
{
	Depth=1;
	int e=outWallsGroup(W2D_SURVEY,SVG_VECTORS);
	if(!e) e=outWallsGroup(W2D_FLAGS,SVG_FLAGS);
	if(!e) e=outWallsGroup(W2D_LABELS,SVG_LABELS);
	if(!e) e=outWallsGroup(W2D_NOTES,SVG_NOTES);
	if(!e) e=outWallsGroup(W2D_GRID,SVG_GRID);
	return e;
}

static int write_svg(void)
{
    XML_Parser xmlparser;
	int e;

	assert(!bMerging2 || pTemplate);

	if(!bMerging2) {
		pTemplate=NULL;
		HMODULE h=GetModuleHandle("WALLSVG");
		HRSRC hrsrc=FindResource(h,"WALLSVG.XML",RT_HTML);
		if(hrsrc) {
			uTemplateLen=SizeofResource(h,hrsrc);
			pTemplate=(char *)LockResource(LoadResource(h,hrsrc));
		}
		if(!pTemplate) {
			//UINT e=GetLastError();
			return SVG_ERR_VERSION;
		}
	}

	strcpy(pathxml,pSVG->svgpath);
	pnamxml=trx_Stpnam(pathxml);

	if(bMerge2) {
		strcpy(pnamxml,bMerging2?"$2" SVG_TMP_EXT:"$1" SVG_TMP_EXT);
	}

	if(bGzout && !bMerge2) {
		if(!(gzout=gzopen(pathxml,"wb")))
			return SVG_ERR_SVGCREATE;
		fpout=0;
	}
	else {
		if(!(fpout=fopen(pathxml,"wb")))
			return SVG_ERR_SVGCREATE;
		gzout=0;
	}

	bWriting=true;

	if (!(xmlparser=XML_ParserCreate(NULL))) return SVG_ERR_NOMEM;

	XML_SetXmlDeclHandler(xmlparser,out_xmldecl);
	XML_SetElementHandler(xmlparser,out_startElement,out_endElement);
	XML_SetProcessingInstructionHandler(xmlparser,out_instruction);
	XML_SetCharacterDataHandler(xmlparser,out_chardata);
	XML_SetCommentHandler(xmlparser,out_comment);
	XML_SetDoctypeDeclHandler(xmlparser,out_startDoctype,out_endDoctype);
	XML_SetEntityDeclHandler(xmlparser,out_entity);
	XML_SetGeneralEntityParsing(xmlparser,XML_GENERAL_ENTITY_PARSING_NEVER);

	//Format comments in SVG portion differently --
	bMarkerWritten=false;
	bSymbolScan=bNoOutput=bElementsSeen=false;
	//Skip first comment, a CAUTION statement, in WALLSVG.XML--
	bFirstCommentSeen=bNoClose=false;
	//When "<!ENTITY _M" is seen, call out_FlagEntities() -- 
	nCol=Depth=0;
	if(fVectorThick<0.0) fVectorThick=fLabelSize*0.03;
	if(fMarkerThick<0.0) fMarkerThick=fLabelSize*0.133;
	if(fpmrg) {
		fMarkerThick*=scaleRatio;
		fVectorThick*=scaleRatio;
		fLabelSize*=scaleRatio;
		fNoteSize*=scaleRatio;
		fTitleSize*=xMid/xMrgMid;
	}
	else {
		mrgScale=scale;
		scaleRatio=1.0;
	}
	if(fpmrg && bAdjust) {
		tppcnt=0;
		node_visits=0;
		init_veclst();
	}
	e=parse_file(xmlparser,NULL);
	xmlparser=NULL;
	if(e) return e;

	if(flags&SVG_MERGEDCONTENT) {
		if(e=outMergedContent()) return e;
	}
	else {
		if(flags&SVG_ADJUSTABLE) {
			outGroup(ID_MASK);
			outStartGroup(ID_DETAIL,FALSE);
			outGroup(ID_DETAIL_SHP);
			outGroup(ID_DETAIL_SYM);
			outCloseElement("g");
			outStartGroup(ID_WALLS,FALSE);
			outGroup(ID_WALLS_SHP);
			outGroup(ID_WALLS_SYM);
			outCloseElement("g");
		}
		if(e=outWallsData()) return e;
		if(flags&SVG_ADJUSTABLE)
			outGroup(ID_LEGEND);
	}
	//if(!bMerging2)
		outWallsGroup(W2D_FRAME,0);
	if(!(flags&SVG_ADJUSTABLE)) OUTS("\t</svg> <!--NOTE: Remove for AI 10 -->\r\n");
	OUTS("</svg>\r\n");
	return 0;
}

static int scan_svgheader(void)
{
	//determine status of any pre-existing svg that would be overwritten
	if(!(fpin=gzopen(pSVG->svgpath,"rb")))
		return SVG_ERR_SVGCREATE;
	if (!(mrgparser=XML_ParserCreate(NULL)))
		return SVG_ERR_NOMEM;

	XML_SetCommentHandler(mrgparser,first_comment);
	XML_SetProcessingInstructionHandler(mrgparser,walls_instruction);

	XML_SetGeneralEntityParsing(mrgparser,XML_GENERAL_ENTITY_PARSING_NEVER);

	parse_file(mrgparser,fpin); //ignore parsing errors
	mrgparser=NULL;
	return 0;
}

static int GetFlagData()
{
	if(numFlagStyles=pGetData(SHP_FLAGSTYLE,NULL)) {
		pFS=(SVG_TYP_FLAGSTYLE *)malloc(numFlagStyles*(sizeof(SVG_TYP_FLAGSTYLE)+SHP_LEN_FLAGNAME+2));
		if(!pFS) return SVG_ERR_NOMEM;
		SHP_TYP_FLAGSTYLE sFS;
		char *pNext=(char *)&pFS[numFlagStyles];
		UINT len,id;
		for(len=0;len<numFlagStyles;len++) pFS[len].wFlag=0;
		for(len=0;len<numFlagStyles;len++) {
			if(!pGetData(SHP_FLAGSTYLE,&sFS)) break;
			id=sFS.id;
			//sanity check --
			if(id>=numFlagStyles || pFS[id].wFlag!=0) {
				return SVG_ERR_UNKNOWN;
			}
			pFS[id].wPriority=sFS.wPriority;
			pFS[id].wSize=sFS.wSize;
			pFS[id].bShape=sFS.bShape;
			pFS[id].bShade=sFS.bShade;
			pFS[id].color=sFS.color;
			pFS[id].pFlagname=strcpy(pNext,flagname_str(sFS.flagname));
			pNext+=strlen(pNext)+1;
		}
		if(!(pFS=(SVG_TYP_FLAGSTYLE *)_expand(pFS,pNext-(char *)pFS))) return SVG_ERR_NOMEM;
	}

	if(numFlagLocations=pGetData(SHP_FLAGLOCATION,NULL)) {
		pFL=(SVG_TYP_FLAGLOCATION *)malloc(numFlagLocations*sizeof(SVG_TYP_FLAGLOCATION));
		if(!pFL) return SVG_ERR_NOMEM;
		SHP_TYP_FLAGLOCATION sFL;
		UINT l=0;
		while(pGetData(SHP_FLAGLOCATION,&sFL)) {
			if(sFL.flag_id>=numFlagStyles || l>=numFlagLocations) {
				return SVG_ERR_UNKNOWN;
			}
			//if(pFS[sFL.flag_id].wHidden) continue;
			//check and convert coordinates --
			mult(&utm_to_xy,sFL.xyz,pFL[l].xy);
			if(isInside(pFL[l].xy)) {
				pFL[l].flag_id=sFL.flag_id;
				pFS[sFL.flag_id].wFlag++;
				l++;
			}
		}
		if(l) {
			if(!(pFL=(SVG_TYP_FLAGLOCATION *)_expand(pFL,l*sizeof(SVG_TYP_FLAGLOCATION))))
				return SVG_ERR_NOMEM;
			numFlagLocations=l;
		}
		else {
			//No flag instances to worry about!
			//We'll include the symbols, however.
			free(pFL); pFL=NULL;
			numFlagLocations=0;
		}
	}

	return 0;
}

static void getNumNoteLocations()
{
	SHP_TYP_NOTELOCATION sN;
	UINT count;
	double xy[2];

	count=0;
	if(pGetData(SHP_NOTELOCATION,NULL)) {
		while(pGetData(SHP_NOTELOCATION,&sN)) {
			mult(&utm_to_xy,sN.xyz,xy);
			if(isInside(xy)) count++;
		}
	}
	numNoteLocations=count;
}


static void init_constants()
{
	decimals=pSVG->advParams.decimals;
	char *p=fltstr(pSVG->width);
	p[sizeof(width_str)-1]=0;
	strcpy(width_str,p);
	width=atof(width_str);
	p=fltstr(pSVG->height);
	p[sizeof(height_str)-1]=0;
	strcpy(height_str,p);
	height=atof(height_str);

	xMid=0.5*width;
	yMid=0.5*height;
	grid_xMin=grid_yMin=0.0;
	grid_xMax=width;
	grid_yMax=height;
	viewA=pSVG->view*U_DEGREES;
	sinA=sin(viewA);
	cosA=cos(viewA);
	midE=pSVG->centerEast;
	midN=pSVG->centerNorth;
	scale=pSVG->scale; //pts per meter
	init_utm_to_xy();

	numNoteLocations=numFlagLocations=numFlagStyles=0;
	pFS=NULL;
	pFL=NULL;
	fLabelSize=pSVG->labelSize/10.0;
	fNoteSize=pSVG->noteSize/10.0;
	fTitleSize=pSVG->titleSize/10.0;
	fVectorThick=fMarkerThick=-1.0; //still to be determined
	numGridThick=0;
	fFrameThick=pSVG->frameThick;
	cPassageClr=pSVG->passageColor;
	cBackClr=pSVG->backColor;
	cGridClr=pSVG->gridColor;
	cLrudClr=pSVG->lrudColor;
	cMarkerClr=pSVG->markerColor;
	if(flags&SVG_MERGEDCONTENT) {
		bUseStyles=(flags&SVG_USESTYLES)!=0;
		bUseLabelPos=(flags&(SVG_LABELS+SVG_NOTES))!=0 && (pSVG->advParams.uFlags&SVG_ADV_USELABELPOS)!=0;
		bUseLineWidths=(pSVG->advParams.uFlags&SVG_ADV_USELINEWIDTHS)!=0;
		dist_exp=pSVG->advParams.dist_exp;
		dist_off=pSVG->advParams.dist_off;
		len_exp=pSVG->advParams.len_exp;
		vecs_used=pSVG->advParams.vecs_used;
	}
	else bUseStyles=bUseLabelPos=bUseLineWidths=FALSE;
}

//======================================================================
//Merge file functions --
static BOOL is_hex(int c)
{
	return isdigit((BYTE)c) || (c>='a' && c<='f');
}

static int get_name_from_id_component(char *rnam, LPCSTR pnam)
{
	UINT uhex;
	while(*pnam) {
		if(*pnam=='-') {
			if(!is_hex(pnam[1]) || !is_hex(pnam[2]) || 1!=sscanf(pnam+1,"%02x",&uhex)) {
				return SVG_ERR_BADIDSTR;
			}
			pnam+=3;
			*rnam++=(byte)uhex;
		}
		else *rnam++=*pnam++;
	}
	*rnam=0;
	return 0;
}

static int get_ep_name(LPCSTR pnam,SVG_ENDPOINT *ep)
{
	int e=0;
	LPSTR p=(LPSTR)strchr(pnam,'.');
	if(!p) ep->prefix[0]=0;
	else {
	   *p++=0;
	   e=get_name_from_id_component(ep->prefix,pnam);
	   pnam=p;
	}
	if(!e) e=get_name_from_id_component(ep->name,pnam);
	return e;
}

static int get_ep_names(LPCSTR ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	char idbuf[SVG_MAX_SIZNAME+1];
	char *p1,*p0=idbuf;
	int i;

	if(strlen(ids)>SVG_MAX_SIZNAME) {
		return SVG_ERR_BADIDSTR;
	}
	strcpy(p0,ids);
	if(*p0=='_') p0++;
	if(!(p1=strchr(p0,'_'))) {
		return SVG_ERR_BADIDSTR;
	}
	*p1++=0;
	if(get_ep_name(p0,ep0) || get_ep_name(p1,ep1) || (i=strcmp(ep0->prefix,ep1->prefix))>0 ||
			(i==0 && strcmp(ep0->name,ep1->name)>=0)) {
		return SVG_ERR_BADIDSTR;
	}
	return 0;
}

static int copy_prefixname(char *p,SVG_ENDPOINT *ep)
{
	if(ep->prefix[0]) return sprintf(p,"%s:%s",ep->prefix,ep->name);
	return strlen(strcpy(p,ep->name));
}

static int chk_idkey(LPCSTR ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	int e;
	char key[SVG_MAX_SIZNAME+1];
	char *p=key+1;

	if(e=get_ep_names(ids,ep0,ep1)) return e;

	p+=strlen(strcpy(p,ep0->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep0->name)); *p++=0;
	p+=strlen(strcpy(p,ep1->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep1->name));
	*key=(char)(p-key-1);
	if(trx.Find(key)) {
		//This will be a frequent problem!
		//Record names for reporting in refbuf --
		e=copy_prefixname(refbuf,ep0);
		strcpy(refbuf+e," - ");
		copy_prefixname(refbuf+e+3,ep1);
		return SVG_ERR_VECNOTFOUND;
	}

	if(bUseLabelPos) {
		//Insert endpoint station name IDs. Later, mark them "present" when
		//they are found in the w2d_Labels layer, in which case their station-
		//relative position in the merge file will also be recorded. Finally,
		//when the new w2d_Labels layer is written, omit a label if it exists
		//in this index but is not marked. If it exists and is marked, use the
		//recorded position.
		SVG_TYP_TRXLABEL lrec;
		trx.UseTree(SVG_TRX_LABELS);
		lrec.bPresent=0;
		lrec.sx=lrec.nx=(float)ep0->x;
		lrec.sy=lrec.ny=(float)ep0->y; //merge file page coordinates of endpoint
		get_idstr(key,ep0->prefix,ep0->name,FALSE);
		trx.InsertKey(&lrec,trx_Stxcp(key));
		lrec.sx=lrec.nx=(float)ep1->x;
		lrec.sy=lrec.ny=(float)ep1->y; //merge file page coordinates of endpoint
		get_idstr(key,ep1->prefix,ep1->name,FALSE);
		trx.InsertKey(&lrec,trx_Stxcp(key));
		trx.UseTree(SVG_TRX_VECTORS);
	}

	return 0;
}

static SVG_VECNODE *alloc_vnode(double *b_xy,double *m_xy)
{
	SVG_VECNODE *pn,*pnret;
	int slevel;
	int ichild,idx;
	double divider,midp[2];

	midp[0]=b_xy[0]+0.5*m_xy[0];
	midp[1]=b_xy[1]+0.5*m_xy[1];

	vstk[0]=slevel=0;
	pn=&vnod[0];

	while(TRUE) {
		idx=(slevel&1);
		divider=pn->B_xy[idx]+0.5*pn->M_xy[idx];
		if(midp[idx]<divider) ichild=pn->lf;
		else ichild=pn->rt;
		if(ichild==-1) {
			if(vnod_len>=vnod_siz) return NULL;
			if(midp[idx]<divider) pn->lf=vnod_len;
			else pn->rt=vnod_len;
			pn=&vnod[vnod_len++];
			break;
		}
		pn=&vnod[ichild];
		vstk[++slevel]=ichild;
	}
	
	//Set maxdist to the squared distance from the vector's midpoint to an endpoint --
	pn->maxdist=0.25*(m_xy[0]*m_xy[0]+m_xy[1]*m_xy[1]);
	pn->B_xy[0]=b_xy[0]; pn->B_xy[1]=b_xy[1]; 
	pn->M_xy[0]=m_xy[0]; pn->M_xy[1]=m_xy[1];
	pn->lf=pn->rt=-1;
	pnret=pn;

	if(slevel>max_slevel) max_slevel=slevel;

	//Now update maxdist in ancestor nodes, the computed value
	//being the maximum distance between this vector (any point within)
	//and the midpoint of an ancestor's vector.
	for(;slevel>=0;slevel--) {
		pn=&vnod[vstk[slevel]];
		divider=max_dist_to_vec(pn->B_xy[0]+0.5*pn->M_xy[0],
			pn->B_xy[1]+0.5*pn->M_xy[1],b_xy,m_xy);
		if(divider>pn->maxdist) pn->maxdist=divider;
	}
	return pnret;
}

static BOOL length_small(double *xy)
{
	if(xy[0]*xy[0]+xy[1]*xy[1]<min_veclen_sq) {
		//if(skipcnt<MAX_VEC_SKIPPED) skiplin[skipcnt]=XML_GetCurrentLineNumber(parser);
		skipcnt++;
		return TRUE;
	}
	return FALSE;
}

static LPCSTR get_coord(LPCSTR p0,double *d)
{
	LPCSTR p=p0;

	if(*p=='-') p++;
	else{
		p0++; //skip command letter or separator
		while(isspace((BYTE)*p0)) p0++;
		p=p0;
		if(*p=='-') p++;
	}

	while(isdigit((BYTE)*p)) p++;
	if(*p=='.') {
		p++;
		while(isdigit((BYTE)*p)) p++;
	}
	if(p==p0) return 0;

	//c=*p;
	//*p=0;
	*d=atof(p0);
	//*p=c;
	return p;
}

static LPCSTR get_path_coords(LPCSTR data,SVG_PATHPOINT *pt)
{
	if(data=get_coord(data,&pt->x)) data=get_coord(data,&pt->y);
	return data;
}

static BOOL get_named_coords(LPCSTR *attr,SVG_PATHPOINT *pt,char *xnam,char *ynam)
{
	LPCSTR p;
	
	if(!(p=getAttrVal(attr,xnam))) return FALSE;
	pt->x=atof(p);
	if(!(p=getAttrVal(attr,ynam))) return FALSE;
	pt->y=atof(p);
	return TRUE;
}

static double abs_max(double a,double b)
{
	a=fabs(a); b=fabs(b);
	return (a>b)?a:b;
}

static void trim_idsuffix(LPSTR p)
{
	LPSTR p0=p+strlen(p)-1;
	while(p0>p && p0[-1]!='_') p0--;
	if(p0>p) p0--;
	*p0=0;
}

static int addto_vnode(LPCSTR *attr)
{
	LPCSTR idstr=getAttrVal(attr,"id");
	LPCSTR style=getAttrVal(attr,"style");

	if(!idstr || !*idstr || !style) {
		skipcnt++;
		return 0; //Must be a vertical shot
	}

	//Correct for AI id renaming bug!!
	if(idstr[strlen(idstr)-1]=='_') {
		trim_idsuffix((char *)idstr);
	}

	if(strlen(idstr)<3) {
		skipcnt++;
		return 0; //Must be a vertical shot
	}

	BOOL bUseVec=(*idstr!='_' || isdigit((BYTE)idstr[1]) || idstr[1]=='-');
	if(!bUseVec) memmove((char *)idstr,idstr+1,strlen(idstr));

	SVG_ENDPOINT ep0,ep1;
	LPCSTR data;

	if(iPathType==P_PATH) {

	    data=getAttrVal(attr,"d");

		if(!data || *data!='M' || !(data=get_coord(data,&ep0.x)) || !(data=get_coord(data,&ep0.y))) {
			return SVG_ERR_VECPATH;
		}
		while(isspace((BYTE)*data)) data++;

		//data is postioned at type char for TO station's coordinates --
		if(*data==0) {
			//if(skipcnt<MAX_VEC_SKIPPED) skiplin[skipcnt]=XML_GetCurrentLineNumber(parser);
			skipcnt++;
			return 0;
		}

		if(!strchr("lLvVhH",*data)) {
			return SVG_ERR_VECPATH;
		}
		BOOL bRelative=islower(*data);

		switch(toupper(*data)) {
			case 'L' :
				if(!(data=get_coord(data,&ep1.x)) || !(data=get_coord(data,&ep1.y))) {
					return SVG_ERR_VECPATH;
				}
				if(bRelative) {
					ep1.x+=ep0.x; ep1.y+=ep0.y;
				}
				break;
			case 'V' :
				ep1.x=ep0.x;
				if(!(data=get_coord(data,&ep1.y))) {
					return SVG_ERR_VECPATH;
				}
				if(bRelative) ep1.y+=ep0.y;
				break;
			case 'H' :
				ep1.y=ep0.y;
				if(!(data=get_coord(data,&ep1.x))) {
					return SVG_ERR_VECPATH;
				}
				if(bRelative) ep1.x+=ep0.x;
				break;
		}
	}
	else if(iPathType==P_LINE) {
		if(!get_named_coords(attr,(SVG_PATHPOINT *)&ep0,"x1","y1") ||
			!get_named_coords(attr,(SVG_PATHPOINT *)&ep1,"x2","y2")) return SVG_ERR_LINEDATA;
	}
	else {
		return SVG_ERR_NOPATH;
	}

	int e;

    if(e=chk_idkey(idstr,&ep0,&ep1)) return e;

	SVG_TYP_TRXVECTOR rec;
	if(e=trx.GetRec(&rec,siz_vecrec)) {
		return SVG_ERR_UNKNOWN;
	}

	if(bUseStyles) {
		//Override vector color index if merged version is different --
		LPCSTR p=getColorPtr(style,st_stroke);
		if(p) {
			//For now, marker color is set to color of first merged vector!
			if(cMarkerClr==(COLORREF)-1) cMarkerClr=colorval(p);
			if(rec.wVecColor!=(WORD)-1) {
				vecColor[rec.wVecColor].cnt--;
				if((e=add_vecColor(p))!=(int)rec.wVecColor) {
					rec.wVecColor=(WORD)e;
					if(trx.PutRec(&rec)) return SVG_ERR_INDEXWRITE;
				}
			}
		}
		if(fVectorThick<0.0) {
			if(p=getStyleValPtr(style,st_stroke_width)) fVectorThick=atof(p);
			else fVectorThick=1.0; //Illustrator will not specify stroke widths of 1.0
		}
	}

	if(!bUseVec) {
		return 0;
	}

	double abs_diffx,abs_diffy,bxy[2],mxy[2];

	mult(&utm_to_mrgxy,rec.fr_xy,bxy);
	mult(&utm_to_mrgxy,rec.to_xy,mxy);

	//Save maximum absolute change in coordinates --
	abs_diffx=abs_max(bxy[0]-ep0.x,mxy[0]-ep1.x);
	abs_diffy=abs_max(bxy[1]-ep0.y,mxy[1]-ep1.y);

	//Parametric form --
	ep1.x-=ep0.x; ep1.y-=ep0.y;
	mxy[0]-=bxy[0];
	mxy[1]-=bxy[1];

 	//Don't use a vector with squared hz length less than min_veclen_sq --
	if(length_small(&ep1.x) || length_small(mxy)) {
		return 0;
	}

	//Check vector orientation
	double d=fabs(atan2(ep1.y,ep1.x)-atan2(mxy[1],mxy[0]));
	if(d>PI) d=2*PI-d;
	if(d>PI/2) {
		skipcnt++;
		return 0; //don't use this vector -- should be warning
		//return SVG_ERR_VECDIRECTION;
	}

	d=0.0;
	if(abs_diffx>max_diffx) {
		d=max_diffx=abs_diffx;
	}
	if(abs_diffy>max_diffy) {
		max_diffy=abs_diffy;
		if(max_diffy>d) d=abs_diffy;
	}
	if(d>max_diff) {
		max_diff=d;
		if(d>max_diff_allowed) {
			trx_Stncc(max_diff_idstr,idstr,SIZ_DIFF_IDSTR);
		}
	}

	SVG_VECNODE *pn=alloc_vnode(&ep0.x,&ep1.x);

	if(!pn) {
		return SVG_ERR_UNKNOWN;
	}

	for(e=0;e<2;e++) {
		pn->Bn_xy[e]=bxy[e];
		pn->Mn_xy[e]=mxy[e];
	}

	veccnt++;
	return 0;
}

static void update_pathrange(SVG_PATHPOINT *pp,int npts)
{
	SVG_PATHPOINT p;

	if(!npts) {
	  mult((iLevelSym?&groupCTM:&mrgxy_to_xy),&pp[0].x,&p.x);
	  xpath_min=xpath_max=p.x;
	  ypath_min=ypath_max=p.y;
	}
	else
	for(int i=1;i<=npts;i++) {
	  mult((iLevelSym?&groupCTM:&mrgxy_to_xy),&pp[i].x,&p.x);
	  if(xpath_min>p.x) xpath_min=p.x;
	  else if(xpath_max<p.x) xpath_max=p.x;
	  if(ypath_min>p.y) ypath_min=p.y;
	  else if(ypath_max<p.y) ypath_max=p.y;
	}
}

static double path_length(SVG_PATHPOINT *pp,int i)
{
	double dx=pp[i].x-pp[0].x;
	double dy=pp[i].y-pp[0].y;
	return sqrt(dx*dx+dy*dy);
}

static void out_point(double x,double y,BOOL bComma)
{
	char fbuf[40];

	fltstr(fbuf,x);
	if(bComma && *fbuf!='-') {
		OUTC(','); nCol++;
	}
	OUTS(fbuf);
	nCol+=strlen(fbuf);

	fltstr(fbuf,y);
	if(*fbuf!='-') {
		OUTC(','); nCol++;
	}
	OUTS(fbuf);
	nCol+=strlen(fbuf);
}

static BOOL check_prange(SVG_PATHRANGE *pr)
{
	if(pr->xmax+max_diffx<=0 || pr->xmin-max_diffx>=width ||
	   pr->ymax+max_diffy<=0 || pr->ymin-max_diffy>=height) return FALSE;
	return TRUE;
}

static LPCSTR getTransformMatrixPtr(LPCSTR *attr)
{
	LPCSTR p=getAttrVal(attr,"transform");
	if(p && strlen(p)>7 && !memcmp(p,"matrix(",7)) return p+7;
	return NULL;
}

static LPCSTR getGradientTransformMatrixPtr(LPCSTR *attr)
{
	LPCSTR p=getAttrVal(attr,"gradientTransform");
	if(p && strlen(p)>7 && !memcmp(p,"matrix(",7)) return p+7;
	return NULL;
}

static BOOL outLegendElement(LPCSTR *attr)
{
	if(pLegendPt) {

		if(getPathCTM(attr)<0) return FALSE;
		if(bPathCTM) mult(&pathCTM,&legendPt.x);

		SVG_PATHPOINT p;
		mult(&mrgxy_to_xy,&legendPt.x,&p.x);

		//legendPt is now the position of legend in mrgxy coordinates.
		//p is its new position in xy xoordinates.

		if((flags&SVG_ADJUSTABLE) || isInside(&p.x)) {
			//Legend must now be translated and scaled but not rotated --
			if(bPathCTM || scaleRatio!=1.0 || p.x!=legendPt.x || p.y!=legendPt.y) {
				SVG_CTM ctm;
				ctm.a=ctm.d=scaleRatio;
				ctm.b=ctm.c=0.0;
				ctm.e=p.x-scaleRatio*legendPt.x;
				ctm.f=p.y-scaleRatio*legendPt.y;
				if(bPathCTM) mult(&ctm,&pathCTM);
				else {
					pathCTM=ctm;
					bPathCTM=true;
				}
			}
			outStartGroup(iGroupID,FALSE);					
			iParentFlg=iGroupFlg;
			iLevel=2;
		}
		bPathCTM=false;
	}
	else if((flags&(SVG_ADJUSTABLE+SVG_LEGEND))==(SVG_ADJUSTABLE+SVG_LEGEND)) {
		outGroup(ID_LEGEND);
	}

	return TRUE;
}

static void updateRange(SVG_PATHRANGE *r,SVG_PATHRANGE *s)
{
	if(r->xmax<s->xmax) r->xmax=s->xmax;
	if(r->ymax<s->ymax) r->ymax=s->ymax;
	if(r->xmin>s->xmin) r->xmin=s->xmin;
	if(r->ymin>s->ymin) r->ymin=s->ymin;
}

static void updateRange(SVG_PATHRANGE *r,SVG_PATHPOINT *p)
{
	float x=(float)p->x,y=(float)p->y;

	if(r->xmax<x) r->xmax=x;
	if(r->ymax<y) r->ymax=y;
	if(r->xmin>x) r->xmin=x;
	if(r->ymin>y) r->ymin=y;
}

static BOOL updateSymRange(LPCSTR *attr)
{
	SVG_PATHPOINT pt;
	if(getPathCTM_Pt(attr,&pt)) {
		if(iLevelSym) {
			mult(&groupCTM,&pt.x);
			updateRange(&symrange[symcnt],&pt);
		}
		else mult(&mrgxy_to_xy,&pt.x);
		return isInside(&pt.x);
	}
	return FALSE;
}

static COLORREF GetFillColor(LPCSTR *attr)
{
	  LPCSTR p=getStyleAttr(attr);
	  return p?getColorAttr(p,st_fill):((COLORREF)-1);
}

static int traverse_path(LPCSTR *attr)
{
	int tp,npts;
	double tl;
	SVG_PATHPOINT bp,mp,pp[6];
	BOOL bRelative;
	LPCSTR data;
	char typ;

	data=getAttrVal(attr,"d");
	data=get_path_coords(data,&bp);

	pp[0]=bp; //unadjusted base point is saved in bp

	//Init range or adjust point --
	if(bWriting) {
		process_points(pp,0);
		if(bNTW) {
			out_ntw_points(pp,'m');
		}
		else {
			nCol+=gz_printf("d=\"M");
			out_point(pp[0].x,pp[0].y,0);
		}
	}
	else {
		if(!bNTW) update_pathrange(pp,0);
		mp=pp[0];
	}

	tp=1;
	tl=0.0;

	while(isspace((BYTE)*data)) data++;

	while(*data) {
		//data is postioned at type char for next set of control points --
		if(!strchr("cChHlLMsSvVz",*data)) {
			return -SVG_ERR_WALLPATH;
		}
		bRelative=islower(*data);

		switch(typ=tolower(*data)) {
			case 'z' :
				while(isspace((BYTE)*++data));
				if(bWriting) {
					if(!bNTW) {
						OUTC('z');
						nCol++;
					}
					else goto _write;
				}
				else {
					pp[1]=mp;
					if(!bNTW && (iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP))) tl+=path_length(pp,1);
				}
				continue;

			case 'm' :
				//Compound path --
				if(iGroupFlg&FLG_MASK) goto _write;

				//Can't get here when bNTW --

				if(!(data=get_path_coords(data,&bp))) {
					return -SVG_ERR_WALLPATH;
				}
				while(isspace((BYTE)*data)) data++;
				pp[0]=bp; //unadjusted base point is saved in bp

				if(bWriting) {
					//Adjust point --
					process_points(pp,0);
					OUTC('M'); nCol++;
					out_point(pp[0].x,pp[0].y,0);
				}
				else mp=bp;
				continue;

			case 'l' :
				if(!(data=get_path_coords(data,&pp[1]))) {
					return -SVG_ERR_WALLPATH;
				}
				npts=1;
				break;

			case 'v' :
				pp[1].x=bRelative?0:bp.x;
				if(!(data=get_coord(data,&pp[1].y))) {
					return -SVG_ERR_WALLPATH;
				}
				typ='l';
				npts=1;
				break;

			case 'h' :
				pp[1].y=bRelative?0:bp.y;
				if(!(data=get_coord(data,&pp[1].x))) {
					return -SVG_ERR_WALLPATH;
				}
				typ='l';
				npts=1;
				break;

			case 's' :
			case 'c' :
				npts=(typ=='c')?3:2;
				for(int i=1;i<=npts;i++) {
					if(!(data=get_path_coords(data,&pp[i])))
						return -SVG_ERR_WALLPATH;
				}
				break;
				
		}

		if(bRelative) {
			for(int i=1;i<=npts;i++) {
				pp[i].x+=bp.x;
				pp[i].y+=bp.y;
			}
		}
		bp=pp[npts]; //save new unadjusted base point

		tp+=npts;

		if(bWriting) process_points(pp,npts);
		else if(!bNTW) update_pathrange(pp,npts);

		if(bWriting) {
			if(bNTW) out_ntw_points(&pp[1],typ);
			else {
				if(nCol>SIZ_PATH_LINEBREAK) {
					OUTS("\r\n\t");
					nCol=skip_depth()+1;
				}
				OUTC(typ); nCol++;
				for(int i=1;i<=npts;i++) {
					out_point(pp[i].x-pp[0].x,pp[i].y-pp[0].y,i>1);
				}
			}
		}
		else if(!bNTW && (iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP)))
			tl+=path_length(pp,npts);

		while(isspace((BYTE)*data)) data++;

		if(*data) pp[0]=pp[npts];
	}

_write:
	if(bWriting) {
		if(bNTW) {
			out_ntw_points(NULL,typ); //write last record with close flag if typ=='z'
			if(bAllocErr) return -SVG_ERR_NOMEM;
		}
		else {
			OUTS("\"/>\r\n");
			nCol=0;
		}
	}
	else if(!bNTW) {
		if(iGroupFlg&FLG_WALLS_SHP) {
			flength_w+=tl;
		}
		else if(iGroupFlg&FLG_DETAIL_SHP) flength_d+=tl;
	}
	return tp;
}

static void out_point_coords(SVG_PATHPOINT *pt)
{
	nCol+=gz_printf(fltstr(pt->x));
	if(pt->y>=0.0) {
		OUTC(','); nCol++;
	}
	nCol+=gz_printf(fltstr(pt->y));
}

static int GetNextPolyPt(SVG_PATHPOINT *pp)
{
	LPCSTR p=pNextPolyPt;

	while(isspace((BYTE)*p)) p++;
	if(*p) {
		pp->x=atof(p);
		while(*++p) {
			if((*p==',' && ++p) || *p=='-') break;
		}
		if(!*p) return -SVG_ERR_POLYDATA;
		pp->y=atof(p);
		while(!isspace((BYTE)*p) && *p) p++;
		pNextPolyPt=p;
		return 1;
	}
	pNextPolyPt=p;
	return 0;
}

static int expand_ellipse(LPCSTR *attr,SVG_PATHPOINT *pp)
{
	//Expand points around center at pp for range calculation --
	LPCSTR p=getAttrVal(attr,(iPathType==P_CIRCLE)?"r":"rx");
	if(!p) return 1;
	double r=atof(p);
	pp[1].x=pp[0].x-r;
	pp[2].x=pp[0].x+r;
	pp[1].y=pp[2].y=pp[0].y;
	if(iPathType==P_ELLIPSE && (p=getAttrVal(attr,"ry"))) r=atof(p);
	pp[3].y=pp[0].y-r;
	pp[4].y=pp[0].y+r;
	pp[3].x=pp[4].x=pp[0].x;
	return 5;
}

static int get_shape_pts(LPCSTR *attr,SVG_PATHPOINT *pp)
{
	int e;

	switch(iPathType) {
		case P_LINE:
			return (get_named_coords(attr,pp,"x1","y1") &&
				get_named_coords(attr,pp+1,"x2","y2"))?2:-SVG_ERR_LINEDATA;

		case P_CIRCLE:
		case P_ELLIPSE:
			if(get_named_coords(attr,pp,"cx","cy")) {
				if(!bWriting) {
					//Expand points around center for range calculation --
					e=expand_ellipse(attr,pp);
				}
				else e=1;
				if(bPathCTM) applyPathCTM(pp,e); //should be an ellipse rotation only!
				return e;
			}
			return -SVG_ERR_ELLIPSE;

		case P_POLYLINE:
		case P_POLYGON:
			if(pNextPolyPt || (pNextPolyPt=getAttrVal(attr,"points"))) {
				int i;
				for(i=0;i<MAX_SHAPEPTS;i++) {
					if((e=GetNextPolyPt(pp+i))<=0) break;
				}
				return (e<0)?e:i;
			}

		case P_RECT:
			if(get_named_coords(attr,pp,"x","y") &&
				get_named_coords(attr,pp+1,"width","height")) {
					pp[2].x=pp[0].x+pp[1].x;
					pp[2].y=pp[0].y+pp[1].y;
					pp[3].x=pp[0].x;
					pp[3].y=pp[2].y;
					pp[1].x+=pp[0].x;
					pp[1].y=pp[0].y;
					if(bPathCTM) applyPathCTM(pp,4);
					return 4;
			}
			return -SVG_ERR_RECTDATA;
	}
	return SVG_ERR_UNKNOWN;
}

static void out_ntw_point_array(SVG_PATHPOINT *pp,int len)
{
	for(int i=0;i<len;i++) out_ntw_points(pp++,'l');
}

static void out_point_array(SVG_PATHPOINT *pp,int len)
{
	for(int i=0;i<len;i++) {
		if(nCol>SIZ_PATH_LINEBREAK) {
			  OUTS("\r\n\t");
			  nCol=skip_depth()+1;
		}
		else {
			OUTC(' ');
			nCol++;
		}
		out_point_coords(pp++);
	}
}

static int traverse_shape(LPCSTR *attr)
{
	//Process P_LINE, P_RECT, P_POLYLINE, P_POLYGON, P_CIRCLE, or P_ELLIPSE
	int nLen,tp=0;
	double tl=0.0;
	LPCSTR px,py;
	SVG_PATHPOINT bp,pp[MAX_SHAPEPTS+1];

	pNextPolyPt=NULL;

	while((nLen=get_shape_pts(attr,pp))>0) {
		if(bWriting) {
			process_points(pp-1,nLen);
			if(!tp) {
				if(bNTW) {
					//mask layer processing only --
					out_ntw_points(pp,'m');
					out_ntw_point_array(pp+1,nLen-1);
				}
				else {
					if(iPathType==P_POLYGON || iPathType==P_POLYLINE) {
						OUTS("points=\""); nCol+=8;
						out_point_coords(pp);
						out_point_array(pp+1,nLen-1);
						if(nLen<MAX_SHAPEPTS) break;
					}
					else {
						if(iPathType==P_LINE) {
							OUTS("x1=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" y1=\""); OUTS(fltstr(pp[0].y));
							OUTS("\" x2=\""); OUTS(fltstr(pp[1].x));
							OUTS("\" y2=\""); OUTS(fltstr(pp[1].y));
						}
						else if(iPathType==P_RECT) {
							//Unapply old rotation and apply new one --
							if(bPathCTM || ((iGroupFlg&FLG_SYM_MSK)==0 && viewA!=mrgViewA)) {
								if(!fixRotateCTM(pp)) return -SVG_ERR_ROTXFORM;
								if(bPathCTM) {
									gz_printf("%s ",fmtCTM(pathCTM));
								}
							}
							OUTS("x=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" y=\""); OUTS(fltstr(pp[0].y));
							OUTS("\" width=\""); OUTS(fltstr(pp[2].x-pp[0].x));
							OUTS("\" height=\""); OUTS(fltstr(pp[2].y-pp[0].y));
						}
						else if(iPathType==P_CIRCLE) {
							OUTS("cx=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" cy=\""); OUTS(fltstr(pp[0].y));
							if(!(px=getAttrVal(attr,"r"))) return -SVG_ERR_ELLIPSE;
							OUTS("\" r=\""); OUTS(fltstr(atof(px)*scaleRatio));
						}
						else { //P_ELLIPSE
							if(bPathCTM || ((iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP)) &&  sinA!=0.0)) {
								if(!fixRotateCTM(pp)) return -SVG_ERR_ROTXFORM;
								if(bPathCTM) {
									gz_printf("%s ",fmtCTM(pathCTM));
								}
							}
							OUTS("cx=\""); OUTS(fltstr(pp[0].x));
							OUTS("\" cy=\""); OUTS(fltstr(pp[0].y));
							if(!(px=getAttrVal(attr,"rx")) || !(py=getAttrVal(attr,"ry"))) {
								return -SVG_ERR_ELLIPSE;
							}
							OUTS("\" rx=\""); OUTS(fltstr(atof(px)*scaleRatio));
							OUTS("\" ry=\""); OUTS(fltstr(atof(py)*scaleRatio));
						}
						break;
					}
				}
			}
			else {
				//Writing more polygon points --
				if(bNTW) {
					out_ntw_point_array(pp,nLen);
				}
				else {
					out_point_array(pp,nLen);
				}
			}
		}
		else {
			//Not writing --
			if(!bNTW) {
				if(!tp) {
					update_pathrange(pp,0);
					if(nLen>1) update_pathrange(pp,nLen-1);
				}
				else update_pathrange(pp-1,nLen);

				if(iPathType==P_CIRCLE || iPathType==P_ELLIPSE) {
					return 1;
				}

				if(iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP)) {
					//must be P_POLYGON, P_POLYLINE, or P_LINE
					for(int i=0;i<nLen-1;i++) {
						tl+=path_length(pp+i,1);
					}
					if(tp) {
						tl+=path_length(pp,MAX_SHAPEPTS);
					}
					if(iPathType==P_POLYGON) {
						if(!tp) bp=pp[0];
						pp[MAX_SHAPEPTS]=pp[nLen-1];
					}
				}
			}
		}
		tp+=nLen;
		if(nLen<MAX_SHAPEPTS) break;
	}

	if(nLen<0) return nLen;

	//No more shape pts --
	if(bWriting) {
		if(!bNTW) OUTS("\"/>\r\n");
		else {
			out_ntw_points(0,(iPathType==P_LINE || iPathType==P_POLYLINE)?0:'z');
			if(bAllocErr) return -SVG_ERR_NOMEM;
		}
	}
	else if(!bNTW && (iGroupFlg&(FLG_DETAIL_SHP+FLG_WALLS_SHP))) {
		if(iPathType==P_POLYGON) {
			pp[0]=bp;
			tl+=path_length(pp,MAX_SHAPEPTS);
		}
		if(iGroupFlg&FLG_WALLS_SHP) {
			flength_w+=tl;
		}
		else flength_d+=tl;
	}
	return tp;
}

int SkipPath(LPCSTR *attr)
{
	LPCSTR data;
	SVG_PATHPOINT pt;

	if(getPathCTM(attr)<0 || (bPathCTM && (iPathType!=P_RECT && iPathType!=P_ELLIPSE))) {
		//Illustrator transforms only rect and ellipse elements --
		return SVG_ERR_BADXFORM;
	}

	//Set data to start of data string --
	if(iPathType==P_PATH) {
		//if((data=getStyleAttr(attr)) && !strcmp(data,"fill:none;")) return -1;
		if(!(data=getAttrVal(attr,"d")) || *data!='M' || !(data=get_path_coords(data,&pt))) {
			return SVG_ERR_WALLPATH;
		}
		while(isspace((BYTE)*data)) data++;
		if(!*data) return -1; //ignore single point path
	}
	else if(iPathType==P_CIRCLE || iPathType==P_ELLIPSE) {
		//These elements ignored in mask layer for now --
		if(bNTW) return -1;
	}

	return 0;
}

static int process_path(LPCSTR *attr)
{
	int tp;
	LPCSTR p;

	if(tp=SkipPath(attr)) {
		bPathSkipped=true;
		return (tp<0)?0:tp;
	}

	if(bNTW) goto _start;

	if(bWriting) {
		if(iLevelSym) {
			if(!bSkipSym) outStartElement(pathType[iPathType],attr,false);
			pathcnt++;
			return 0;
		}
		if(!pathrange[pathcnt].bInRange) {
			pathcnt++;
			return 0;
		}
			
		if(iGroupFlg&FLG_SYM_MSK) {
			//Compute translation vector as shift in bounding box's center --
			getSymTransCTM(&pathrange[pathcnt]);
		}
		else {
			if(iPathType==P_RECT && bAdjust) iPathType=P_POLYGON;
		}

		nCol=skip_depth();
		nCol+=gz_printf("<%s ",pathType[iPathType]);

		//Output id and style attributes (if writing) --
		if(p=getAttrVal(attr,"id")) nCol+=gz_printf("id=\"%s\" ",p);
		if(p=getStyleAttr(attr)) {
			nCol+=gz_printf("style=\"");
			if((tp=pathrange[pathcnt].pathstyle_idx)>MAX_STYLES) {
				//We must be in the mask layer at this point. Also, bUseStyles must be FALSE.
				if(tp==MAX_STYLES+1) nCol+=gz_printf("&P_s;\" ");
				else nCol+=gz_printf("&B_s;\" ");
			}
			else if(tp<MAX_STYLES) {
				nCol+=gz_printf("&_p%d;\" ",tp+1);
			}
			else nCol+=gz_printf("%s\" ",p);
		}
	}
	else {
		//save path bounding boxes and total path count --
		if(pathcnt>=max_pathcnt) {
			pathrange=(SVG_PATHRANGE *)realloc(pathrange,(max_pathcnt+=MAX_PATHCNT_INC)*sizeof(SVG_PATHRANGE));
		}
		pathrange[pathcnt].bMorph=iLevelSym?0:1;
		//save path styles for entity definitions --
		if(p=getStyleAttr(attr)) {
			if(!bUseStyles && (iGroupFlg&FLG_MASK)) {
				//The styles for paths in the mask layer will be determined
				//by the B_s and P_s entity assignments. If this is the first (outline)
				//path that has a fill, we'll save the color as cPassageClr to test
				//against additional fill colors in this layer. Thereafter, a path filled with
				//cPassageClr will be assigned style P_s and a path filled with cBackClr will be
				//assigned style B_s. Any other fill color will remain unaltered --
				COLORREF clr=getColorAttr(p,st_fill);
				if(clr!=(COLORREF)-1) {
					if(cPassageClr==(COLORREF)-1) {
						bMaskPathSeen=true;
						cPassageClr=clr;
					}
					if(cPassageClr==clr) tp=(MAX_STYLES+1);
					else if(cBackClr==clr) tp=(MAX_STYLES+2);
					else tp=save_style(pathstyle,p);
				}
				else tp=MAX_STYLES;
			}
			else {
				tp=save_style(iLevelSym?groupstyle:pathstyle,p);
				LPCSTR purl=strstr(p,":url(#");
				if(purl && (p=strchr(purl,')'))) {
					purl+=6;
					vUrlNam.push_back(URLNAM(purl,p-purl,pathcnt));
				}
			}
		}
		else tp=MAX_STYLES;
		pathrange[pathcnt].pathstyle_idx=tp;
	}

_start:

	if(iPathType==P_PATH) tp=traverse_path(attr);
	else tp=traverse_shape(attr);

	if(tp<0) return -tp;

	tpoints+=tp;

	if(!bNTW && !bWriting) {
		if(iGroupFlg&FLG_WALLS_SHP) tpoints_w+=tp;
		else if(iGroupFlg&FLG_DETAIL_SHP) tpoints_d+=tp;
		pathrange[pathcnt].xmax=(float)xpath_max;
		pathrange[pathcnt].xmin=(float)xpath_min;
		pathrange[pathcnt].ymax=(float)ypath_max;
		pathrange[pathcnt].ymin=(float)ypath_min;
		if(iLevelSym) {
			updateRange(&symrange[symcnt],&pathrange[pathcnt]);
		}
	}
	pathcnt++;

	if(bAdjust) {
		tp=(int)((100.0*tpoints)/tptotal);
		if(tp>=(int)tppcnt+5) {
			sprintf(argv_buf,"Adjusting features: %3d%%",tppcnt=(UINT)tp);
			pGetData(SHP_STATUSMSG,argv_buf);
		}
	}
	return 0;
}

static BOOL IsW2dGroup(LPCSTR name)
{
	return name && strlen(name)>4 && !memicmp(name,"w2d_",4);
}

static int GetGroupID(LPCSTR name)
{
	if(IsW2dGroup(name)) {
		name+=4;
		for(int i=0; i<NUM_W2DNAMES; i++) {
			int j=w2d_name_len[i];
			if(j<=(int)strlen(name) && !memcmp(name, w2d_name[i], j) && (!name[j] || name[j]=='_' && isdigit(name[j+1]))) {
				return i;
			}
		}
		return -2; //unrecognized "w2d_" name
	}
	return -1;
}

static void GetGroupFlg(LPCSTR name)
{
	int i=GetGroupID(name); //-1 if not wd2_ previx, -2 if bad wd2_ suffix (not possible at this stage)
	assert(i>=-1);
	if(i>=0) {
		iGroupFlg=w2d_flg[i];
		iGroupID=i;
		return;
	}
	iGroupFlg=0;
	iGroupID=-1;
}

static SVG_PATHPOINT *getOffset(LPCSTR *attr,SVG_PATHPOINT *pt)
{

	LPCSTR id[4]={"x","y","dx","dy"};
	double val[4]={DBL_MIN,DBL_MIN,DBL_MIN,DBL_MIN};
	for(;attr[0];attr+=2) {
		for(int i=0;i<4;i++) if(!strcmp(attr[0],id[i])) {
			val[i]=atof(attr[1]);
			break;
		}
	}
	if(val[0]>DBL_MIN && val[1]>DBL_MIN) {
		pt->x=val[0];
		if(val[2]>DBL_MIN) pt->x+=val[2];
		pt->y=val[1];
		if(val[3]>DBL_MIN) pt->y+=val[3];
		return pt;
	}

	return NULL;
}

static BOOL isGradient(LPCSTR el)
{
	return (!strcmp(el,"linearGradient") || !strcmp(el,"radialGradient"));
}

static void GetViewBox(SVG_VIEWBOX *vb,LPCSTR *attr)
{
	float x;
	LPCSTR p=getAttrVal(attr,"viewBox");
	if(!p || 4!=sscanf(p,"%f %f %f %f",&x,&x,&vb->width,&vb->height)) {
		vb->height=vb->width=0.0f;
	}
}

static LPSTR alloc_name(int len)
{
	if(idNameLen+len>idNameSiz) {
		if(!(idNameBuf=(LPSTR)realloc(idNameBuf,idNameSiz+=INC_IDNAME))) {
			return NULL;
		}
	}
	return &idNameBuf[idNameLen];
}

static bool add_IdName(LPCSTR pNam)
{
	int len=strlen(pNam)+1;
	LPSTR p=alloc_name(len);
	if(!p) return false;
	strcpy(p,pNam);
	vIdNameOff.push_back(idNameLen);
	idNameLen+=len;
	return true;
}

static bool map_new_name(int off)
{
	LPCSTR pNam=PNAME(off);
	int lenpNam=strlen(pNam);
	LPSTR pNew=alloc_name(lenpNam+5);
	if(!pNew) return false;
	//first check $1.svg names
	for(int i=1;i<1000;i++) {
		sprintf(pNew,"%s_%d",pNam,i);
		bool bConflict=false;
		for(it_int it=vIdNameOff.begin();it!=vIdNameOff.end();it++) {
			if(!strcmp(pNew,PNAME(*it))) {
				bConflict=true;
				break;
			}
		}
		if(!bConflict) {
			for(it_name_map it=vNameMap.begin();it!=vNameMap.end();it++) {
				//can't use an existing replacement!
				if(!strcmp(pNew,PNAME(it->newoff))) {
						bConflict=true;
					break;
				}
			}
			if(!bConflict) {
				//found valid replacement, add to vNameMap --
				vNameMap.push_back(NAME_MAP(off,idNameLen));
				idNameLen+=(strlen(pNew)+1);
				return true;
			}
		}
	}
	return false;
}

static int CheckMerge2IDs(LPCSTR pNam)
{
	if(!strcmp(pNam,"_m") || !strcmp(pNam,"Layer_1"))
		return 0;

	//all id's must be unique!
	if(!bMerging2) {
		if(!add_IdName(pNam)) {
			return SVG_ERR_NOMEM;
		}
		return 0;
	}
	//check if name is already mapped
	bool bNew=true;
	for(it_name_map it=vNameMap.begin();it!=vNameMap.end();it++) {
		if(!strcmp(pNam,PNAME(it->oldoff))) {
			bNew=false;
			break;
		}
	}
	if(bNew) {
		for(it_int it=vIdNameOff.begin();it!=vIdNameOff.end();it++) {
			if(!strcmp(pNam,PNAME(*it))) {
				if(!map_new_name(*it)) {
					return SVG_ERR_NOMEM;;
				}
				break;
			}
		}
	}
	return 0;
}

static bool __inline IsXMLID(LPCSTR pNam)
{
   return *pNam=='X' && strlen(pNam)>=5 && !memcmp(pNam,"XMLID",5);
}

static void scanw_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;

  if(Depth<-5)
	  errdep++;

  LPCSTR pNam=getAttrVal(attr,"id");

  if(bMerge2 && pNam && (strcmp(el,"g") || !IsW2dGroup(pNam))) {
	  if(UD(errcode)=CheckMerge2IDs(pNam))
		  return;
  }

  if(iLevel<2) {

	if(bSymbolScan) {
		if(!strcmp(el,"symbol")) {
			int i=strcmp("_m",pNam);
			if((!i && bUseStyles) || (i && (i=find_style(symname,pNam)))) {
				if(!i) bMarkerWritten=true;
				GetViewBox(&flagViewBox[i],attr);
				outStartElement(el,attr,false);
				iLevel=2;
			}
		}
		else if(!strcmp(el,"filter")) {
			if(find_style(symname,pNam)) {
				outStartElement(el,attr,false);
				iLevel=2;
			}
		}
		else if(!strcmp(el,"pattern")) {
				outStartElement(el,attr,false);
				iLevel=2;
		}
		else {
			if(!strcmp(el,"g") && IsW2dGroup(pNam)) {
				UD(errcode)=SVG_ERR_ENDSCAN;
			}
		}
	}
	else if(!strcmp(el,"g") && pNam) {
		GetGroupFlg(pNam);

		if(iGroupFlg&FLG_MERGEDLAYERS) {
			if(iLevel!=0 || Depth!=1) {
				close_output(0);
				UD(errcode)=SVG_ERR_LAYERS;
				goto _errexit;
			}
			if(iGroupID==ID_LEGEND) {
				if(!outLegendElement(attr)) {
					UD(errcode)=SVG_ERR_BADXFORM;
					goto _errexit;
				}
				bLegendSeen=true;
			}
			else if(iGroupID==ID_MASK) {
				outStartGroup(iGroupID,FALSE);
				iParentFlg=iGroupFlg;
				iLevel=2;
			}
			else if(((iGroupFlg&FLG_WALLS)&&(flags&SVG_WALLS)) ||
				    ((iGroupFlg&FLG_DETAIL)&&(flags&SVG_DETAIL))) {
				outStartGroup(iGroupID,((iGroupFlg<<16)&flags)!=0);
				iParentFlg=iGroupFlg;
				iLevel=1;
			}
			else if(flags&SVG_ADJUSTABLE) {
				outStartGroup(iGroupID,FALSE);
				if(iGroupFlg&FLG_WALLS) {
					outGroup(ID_WALLS_SHP);
					outGroup(ID_WALLS_SYM);
				}
				else if(iGroupFlg&FLG_DETAIL) {
					outGroup(ID_DETAIL_SHP);
					outGroup(ID_DETAIL_SYM);
				}
				outCloseElement(el);
			}
		}
		else if(iLevel==1 && (iGroupFlg&FLG_ADJUSTED)) {
			if(Depth!=2 ||
				((iGroupFlg&FLG_DETAIL_MSK)&&iParentFlg!=FLG_DETAIL) ||
				((iGroupFlg&FLG_WALLS_MSK)&&iParentFlg!=FLG_WALLS)) {
				UD(errcode)=SVG_ERR_LAYERS;
				goto _errexit;
			}
			outStartGroup(iGroupID,FALSE);
			iGroupFlg|=iParentFlg;
			iLevel++;
		}
		else if(iGroupFlg&FLG_VECTORS) {
			if(!(UD(errcode)=outWallsData()) && (bLegendSeen || !(flags&SVG_LEGEND))) UD(errcode)=SVG_ERR_ENDSCAN;
		}
	}
	iLevelSym=0;
	return;
  }

  if(bSymbolScan) {
	  if(iLevel==2 && !strcmp(el,"g") && !attr[0]) return;
	  iLevel++;
	  switch(GetSymType(el)) {
		  case S_TEXT :
			//For now, assume text was originated in Illustrator or Walls, in which
			//case we can always expect a transform.
			getPathCTM(attr);
			saveTextElement(attr,false); //sets reflen==0, bInTextElement=1
			break;

		  case S_TSPAN :
			if(bInTextElement) {
			  saveTextElement(attr,true);
			}
			iLevel--;
			break; //else bInTextElement==0

		  default: outStartElement(el,attr,false);

	  }
	  return;
  }

  if(bInsideGradient) {
	  //Ignore all but <stop> elements --
	  if(bInsideGradient==2 && !strcmp(el,"stop")) outStartElement(el,attr,false);
	  return;
  }

  //processing shapes (detail, walls, mask, or legend  --
  if(!strcmp(el,"g")) {
	  iLevel++;
	  getPathCTM(attr);

#ifdef _USE_XMLID
	  if(!iLevelSym && (iGroupFlg&FLG_ADJUSTED)) {
		
		 if(pNam && IsXMLID(pNam)) {
			assert(!strcmp(attr[0],"id")); //illustrator generated, styles come later
			if(!xmlidlvl) {
				if(iGroupFlg&(FLG_WALLS_SHP+FLG_DETAIL_SHP)) {
					xmlidsubcnt=0;
					sprintf(xmlidbuf,"W2D%u",++xmlidcnt);
					attr[1]=pNam=xmlidbuf;
					xmlidlvl++;
				}
			}
			if(pNam!=xmlidbuf) {
				//either not in a shp group or an XML id nested within a parent XML id group
				//simply remove id -- same as AI
				pNam=NULL;
				attr+=2;
			}
		 }
		 else if(xmlidlvl) {
			assert(iGroupFlg&(FLG_WALLS_SHP+FLG_DETAIL_SHP));
			//a user-named group could conceivably exist beneath an XMLID-named group,
			//in which case we leave it alone
			assert(!pNam ||!strcmp(attr[0],"id"));
			if(!pNam) {
				sprintf(xmlidbuf,"W2D%us%u",xmlidcnt,++xmlidsubcnt);
				if(attr[0]) {
					int i=0;
					while(attr[i]) i++;
					assert(!(i&1));
					memmove(&attr[0]+2*sizeof(LPCSTR *),&attr[0],(i+1)*sizeof(LPCSTR *));
				}
				else attr[2]=NULL;
				attr[0]="id";
				attr[1]=pNam=xmlidbuf;
			}
			xmlidlvl++;
		 }
	  }
#endif

	  if(!iLevelSym && (iGroupFlg&FLG_WALLS_DETAIL_MSK)) {
		if(bPathCTM || !pNam || isFilterPresent(attr)) {
			bSkipSym=!check_prange(&symrange[symcnt]);
			if(!bSkipSym) {
				getSymTransCTM(&symrange[symcnt]);
				if(bPathCTM) mult(&symTransCTM,&pathCTM); //transform to place on group element
				else pathCTM=symTransCTM;
				outStartSymGroup(attr); //increments Depth
			}
			iLevelSym=iLevel;
			//Increment symcnt when we leave the group --
			return;
		}
	  }

	  if(!iLevelSym || !bSkipSym) outStartElement(el,attr,false);
	  return;
  }

  if(GetPathType(el)) {
	  //Path definition, either a sym layer element  or a morphable shape.
	  //Compare the path's bounding box with the view frame --
	  bPathSkipped=false;
	  if(iGroupFlg&FLG_LEGEND) {
		  UD(errcode)=outLegendPath(el,attr);
		  bPathSkipped=true;
	  }
	  else {
		  UD(errcode)=process_path(attr);
	  }
	  goto _errexit;
  }

  if(iLevelSym && bSkipSym) return;

  //Fix following logic for iLevelSym>0 ---
  if(isGradient(el)) {
	  bInsideGradient=1;
	  outGradientStart(el,attr); //increments bInsideGradient again if gradient used --
  }
  else switch(GetSymType(el)) {

	  case S_TEXT :
		//For now, assume text was originated in Illustrator or Walls, in which
		//case we can always expect a transform.
		if(getSymPathCTM(attr,true)) { //Test start out of range
			saveTextElement(attr,false); //sets reflen==0, bInTextElemt=1 and then 2
		}
		//else ignore text element (assume out of frame)
		//else {
		//	outOpenElement(el);
		//	OUTS(">\r\n");
		//	Depth++;
		//}
		break;

	  case S_TSPAN :
		if(bInTextElement) {
			//This is the first tspan of a text element--
			saveTextElement(attr,true);
		}
		else if(bInTextPath) {
			outStartElement(el,attr,false);
			*refbuf=0;
			reflen=0;
		}
		break; //else bInTextElement==0
	  
	  case S_PATTERN:
		bInsidePattern=true;
		if(getSymPathCTM(attr,false)) {
		  outStartElement(el,attr,true); //Uses pathCTM and sets bInUse=true
		  bInUse=true;
		}
		bInsidePattern=false;
		break;

	  case S_USE :
		if(!find_style(symname,get_symname(attr))) {
		  break;
		}

	  case S_IMAGE :
		if(getSymPathCTM(attr,false)) {
		  outStartElement(el,attr,true); //Uses pathCTM and sets bInUse=true
		}
		break;

	  case S_TEXTPATH :
	    outStartElement(el,attr,false);
		bInTextPath=1;
	    break;

	  default :
		  trx_Stncc(refbuf,el,sizeof(refbuf));
		  UD(errcode)=SVG_ERR_BADELEMENT;
  }

_errexit:
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanw_endElement(void *userData, LPCSTR el)
{
	if(UD(errcode)) return;

	if(!strcmp(el,"g")) {

		if(iLevel>0) {
			if(bSymbolScan) {
				if(iLevel==2) return;
				outCloseElement(el); //decrements Depth
			}
			else {
				if(!iLevelSym || !bSkipSym) {
					outCloseElement(el);
				}
				if(iLevel==iLevelSym) {
				  symcnt++;
				  //Depth--; //*** Provisional!
				  iLevelSym=0;
				}
			}
			iLevel--;

#ifdef _USE_XMLID
			if(xmlidlvl && !--xmlidlvl) {
				xmlidsubcnt=0;
			}
#endif
			if(iLevel==1 && (iGroupFlg&(FLG_LEGEND+FLG_MASK))) {
				iLevel--;
				//if(iGroupFlg&FLG_LEGEND) UD(errcode)=SVG_ERR_ENDSCAN;
			}
		}
		return;
	}

	if(bInTextElement) {
		if(!strcmp(el,"text")) {
			if(IsRefbufVisible()) {
				outSavedTextElement();
				/* Removed 9/18/2010 --
				if(bTextParagraph) {
					Depth--;
					skip_depth();
					OUTS("</g>\r\n");
					bTextParagraph=false;
				}
				*/
			}
			else {
				//ignore trailing empty text
				reflen=0;
				*refbuf=0;
			}

			//Inserted 9/18/2010 --
			if(bTextParagraph) {
				Depth--;
				skip_depth();
				OUTS("</g>\r\n");
				bTextParagraph=false;
			}

			bInTextElement=0;
			if(bSymbolScan) {
				if(--iLevel==1) iLevel--;
			}
		}
		else if(strcmp(el,"tspan")) {
			UD(errcode)=SVG_ERR_TSPAN;
		}
	}
	else if(bInTextPath) {
		if(!strcmp(el,"tspan")) {
			gz_printf(">%s</tspan>\r\n",trim_refbuf());
			*refbuf=0;
			reflen=0;
		}
		else if(!strcmp(el,"textPath")) {
		    bInTextPath=0;
			outCloseElement(el);
			//iLevel--;
		}
	}
	else if(bSymbolScan) {
		if(iLevel>0) outCloseElement(el);
		if(--iLevel==1) iLevel--;
	}
	else if(bInUse) {
		outCloseElement(el);
		bInUse=false;
	}
	else if(bInsideGradient) {
		if(bNoClose) outCloseElement(el);
		else if(isGradient(el)) {
			if(bInsideGradient==2) outCloseElement(el);
			bInsideGradient=0;
		}
	}
	else {
		if(iLevelSym && !bSkipSym && !bPathSkipped) {
			outCloseElement(el);
		}
		bPathSkipped=false;
	}
}

static void scan_chardata(void *userData,const XML_Char *s,int len)
{
	if(bInTextPath || bInTextElement>0) {
		UINT utf8=0; 
		if(*s&0x80 && len<=3) {
			utf8=((byte)(*s<<len)>>len);
			while(--len) {
				assert((s[1]&0xC0)==0x80);
				utf8=(utf8<<6)+((*++s)&0x3F);
			}
			len=8;
		}
		if(reflen+len>=SIZ_REFBUF) {
		  UD(errcode)=SVG_ERR_SIZREFBUF;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
		}
		if(!utf8) memcpy(refbuf+reflen,s,len);
		else _snprintf(refbuf+reflen,len,"&#x%04x;",utf8);
		refbuf[reflen+=len]=0;
	}
}

static int outMergedContent()
{
	/*
	Called from write_svg() -
	The first time is during the parsing of the of the XML template
	in out_startElement(), witb bSymbolScan=true.
	
    write_svg() is called in Export() - once for a 1-file merge, twice for
	for a 2-file merge to produce %1.svg and %2.svg

	Here we perform the merge, doing a nested parse of the
	external SVG containing graphics elements. We'll output
	the content of the following layers in order:

    w2d_Symbols (predefined symbols in merged file -- AI11 bug workaround)
	w2d_Mask  (optional mask layer)
	w2d_Detail ("Detail" layer in Walls2D)
		w2d_Detail_shp (paths to be morphed)
		w2d_Detail_sym (paths, text, and use elements to be translated)
	w2d_Walls ("Walls" layer in Walls2D)
		w2d_Walls_shp (paths to be morphed)
		w2d_Walls_sym (paths, text, and use elements to be translated)
    outWallsData() -- ouput Walls survey layers:
		w2d_Vectors, w2d_Flags, w2d_Labels, w2d_Notes, w2d_Grid
	w2d_Legend ("Legend" layer: objects whose world coordinates are fixed)

	In all but the w2d_Legend layer, the world coordinates of objects
	will change if the positions of nearby survey vectors have changed.
	Of course, the page (not world) coordinates of all content might
	change to accommodate a new map scale or view.
	*/

	if(!bSymbolScan && !bMerge2 && (flags&SVG_ADJUSTABLE)) {
		outSymInstances();
	}

	if(gzseek(fpmrg,0,SEEK_SET)) return SVG_ERR_MRGOPEN;

	pathcnt=symcnt=0;
	iLevel=0;
	bInTextElement=bInsideGradient=0;
	tpoints=0;
	max_dsquared=0.0;
	bMaskPathSeen=bLegendSeen=bInUse=false;

	mrgparser=XML_ParserCreate(NULL);
	if(!mrgparser) return SVG_ERR_NOMEM;

	XML_SetElementHandler(mrgparser,scanw_startElement,scanw_endElement);
	XML_SetCharacterDataHandler(mrgparser,scan_chardata);
	int i=parse_file(mrgparser,fpmrg);
	mrgparser=NULL;
	return i;
}

static BOOL get_mskname(LPCSTR name)
{
	if(!name) {
		iMaskName[msklvl]=iMaskName[msklvl-1];
		return TRUE;
	}

	if(nMaskName==maxMaskName) {
		pMaskName=(SHP_MASKNAME *)realloc(pMaskName,sizeof(SHP_MASKNAME)*(maxMaskName+=MASKNAME_INC));
		if(!pMaskName) return FALSE;
	}

	char *p=pMaskName[nMaskName].name;
	LPCSTR psrc=name;
	size_t len=0;

	for(;*psrc && len<SHP_LEN_MASKNAME;len++) {
		if(msklvl && *psrc=='_' && strlen(psrc)>6 && psrc[6]=='_' && !memcmp(psrc+1,"x00",3)) {
			short b;
			if(1==sscanf(psrc+4,"%02hx",&b)) {
				*p++=(char)b;
				psrc+=7;
				continue;
			}
		}
		*p=*psrc++;
		if(!msklvl) {
			if(*p=='.') break; //truncate file extension
		}
		else if(*p=='_') *p=' ';
		p++;
	}
	if(len<SHP_LEN_MASKNAME) memset(p,' ',SHP_LEN_MASKNAME-len);
	pMaskName[nMaskName].iParent=msklvl?iMaskName[msklvl-1]:-1;
	iMaskName[msklvl]=nMaskName++;
	return TRUE;
}

static int alloc_ntw_rec(void)
{
	if(nNtwRec==maxNtwRec) {
		pNtwRec=(NTW_RECORD *)realloc(pNtwRec,(maxNtwRec+=MAXNTWREC_INC)*sizeof(NTW_RECORD));
		if(!pNtwRec) return SVG_ERR_NOMEM;
	}
	pNtwRec[nNtwRec].iNam=iMaskName[msklvl];
	pNtwRec[nNtwRec++].iPly=pathcnt;
	return 0;
}

static void scanmask_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;

  LPCSTR pNam=getAttrVal(attr,"id");

  if(iLevel<1) {
	if(!strcmp(el,"g") && pNam) {
		GetGroupFlg(pNam);
		if(iGroupID==ID_MASK) {
			iLevel=1;
			msklvl=0;
			bOuterSeen[0]=0;
			if(!get_mskname(trx_Stpnam(mrgpath))) UD(errcode)=SVG_ERR_NOMEM;
		}
		else if(iGroupFlg&(FLG_VECTORS|FLG_ADJUSTED)) {
			UD(errcode)=SVG_ERR_ENDSCAN;
		}
	}
	return;
  }

  //In case mask layer has grouping --
  if(!strcmp(el,"g")) {
	  iLevel++;
	  if(++msklvl==MAX_MSKLVL) {
		  UD(errcode)=SVG_ERR_GROUPLVL;
		  goto _error;
	  }
	  bOuterSeen[msklvl]=0;
	  if(!get_mskname(pNam)) {
		  UD(errcode)=SVG_ERR_NOMEM;
		  goto _error;
	  }
	  return;
  }

  if(GetPathType(el)) {
	  //Check fill color of this path to determine floor status and background status.
	  //The first filled path determines the floor color. Also, the first path in
	  //a group will be flagged as an outline (outer) path, in which case a new shapefile
	  //record is established consisting of an outer polygon followed by zero or more
	  //inner polygons.

	  COLORREF clr=GetFillColor(attr);
	  
	  if(clr!=(COLORREF)-1) {
		  if(cPassageClr==(COLORREF)-1) cPassageClr=clr;
		  if(cPassageClr==clr) pnPly[pathcnt].flgcnt |= NTW_FLG_FLOOR;
		  if(cBackClr==clr) pnPly[pathcnt].flgcnt |= NTW_FLG_BACKGROUND;
	  }
	  else {
		  pnPly[pathcnt].flgcnt |= NTW_FLG_NOFILL;
	  }
	  pnPly[pathcnt].clr=clr;
	  //The first path in a group is considered an outline path --
	  assert(msklvl>=0);
	  if(!bOuterSeen[msklvl]) {
		  pnPly[pathcnt].flgcnt |= NTW_FLG_OUTER;
		  bOuterSeen[msklvl]++;
		  if(UD(errcode)=alloc_ntw_rec()) goto _error;
	  }
	  UD(errcode)=process_path(attr);
  }
  else UD(errcode)=SVG_ERR_MASKNONPATH;

_error:
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanmask_endElement(void *userData, LPCSTR el)
{
	if(UD(errcode)) return;

	if(!strcmp(el,"g")) {
		if(iLevel>0) {
			iLevel--;
			assert(iLevel || !msklvl); 
			if(msklvl) {
				bOuterSeen[--msklvl]=0;
			}
			if(!(iGroupFlg&FLG_MASK) ||(!iLevel && (pathcnt!=max_pathcnt || ntw_count!=(int)tpoints))) {
				UD(errcode)=SVG_ERR_UNKNOWN;
			}
		}
	}
}

static int write_ntw(void)
{
	bWriting=true;

	if(gzseek(fpmrg,0,SEEK_SET)) return SVG_ERR_MRGOPEN;

	max_pathcnt=pathcnt;
	pathcnt=0;
	tpoints=0;
	iLevel=msklvl=0;
	max_dsquared=0.0;
	ntw_count=ntw_size=nMaskOpenPaths=nNtwRec=maxNtwRec=nMaskName=maxMaskName=0;
	bAllocErr=false;
	cPassageClr=(COLORREF)-1;

	if(!(pnPly=(NTW_POLYGON *)calloc(max_pathcnt*sizeof(NTW_POLYGON),1))) {
		return SVG_ERR_NOMEM;
	}

	if(bAdjust) {
		tppcnt=0;
		node_visits=0;
		init_veclst();
	}

	int i=SVG_ERR_NOMEM;
	mrgparser=XML_ParserCreate(NULL);

	if(mrgparser) {
		XML_SetElementHandler(mrgparser,scanmask_startElement,scanmask_endElement);
		//tm_walls=clock();
		i=parse_file(mrgparser,fpmrg);
		//tm_walls=clock()-tm_walls;
		mrgparser=NULL;
		if(!i) i=write_polygons();
		if(!i && nMaskOpenPaths>0) i=-SVG_ERR_MASKPATHSOPEN;
	}
	return i;
}

int init_vnod(double xM,double yM)
{
	vnod_siz=trx.NumRecs()+NUM_HDR_NODES;
	if(!(vnod=(SVG_VECNODE *)calloc(vnod_siz,sizeof(SVG_VECNODE))) ||
	   !(vstk=(int *)malloc(vnod_siz*sizeof(int)))) return SVG_ERR_NOMEM;

	//Initialize 7 tree header nodes --

	for(int n=0;n<NUM_HDR_NODES;n++) {
		vnod[n].M_xy[0]=vnod[n].M_xy[1]=0.0;
	}
	SVG_VECNODE *pn=vnod;
	pn->B_xy[0]=xM;
	pn->B_xy[1]=yM;
	pn->lf=1; pn->rt=2;
	pn++; //1
	pn->B_xy[0]=0.5*xM;
	pn->B_xy[1]=yM;
	pn->lf=3; pn->rt=4;
	pn++; //2
	pn->B_xy[0]=1.5*xM;
	pn->B_xy[1]=yM;
	pn->lf=5; pn->rt=6;
	pn++; //3
	pn->B_xy[0]=0.5*xM;
	pn->B_xy[1]=0.5*yM;
	pn->lf=pn->rt=-1;
	pn++; //4
	pn->B_xy[0]=0.5*xM;
	pn->B_xy[1]=1.5*yM;
	pn->lf=pn->rt=-1;
	pn++; //5
	pn->B_xy[0]=1.5*xM;
	pn->B_xy[1]=0.5*yM;
	pn->lf=pn->rt=-1;
	pn++; //6
	pn->B_xy[0]=1.5*xM;
	pn->B_xy[1]=1.5*yM;
	pn->lf=pn->rt=-1;
	vnod_len=7;
	return 0;
}

static int parse_georef()
{
	LPCSTR p;
	//Tamapatz Project  Plan: 90  Scale: 1:75  Frame: 17.15x13.33m  Center: 489311.3 2393778.12
	if(reflen && (p=strstr(refbuf," Plan: "))) {
		char cUnits;
		int numval=sscanf(p+7,"%lf Scale: 1:%lf Frame: %lfx%lf%c Center: %lf %lf",
			&mrgViewA,&mrgScale,&xMrgMid,&yMrgMid,&cUnits,&mrgMidE,&mrgMidN);
		if(numval==7 && mrgScale>0.0) {
			mrgViewA*=U_DEGREES;
			mrgSinA=sin(mrgViewA);
			mrgCosA=cos(mrgViewA);
			if(cUnits=='f') {
				xMrgMid*=0.3048;
				yMrgMid*=0.3048;
				mrgMidE*=0.3048;
				mrgMidN*=0.3048;
			}
			mrgScale=(72*12.0/0.3048)/mrgScale;  //page pts per world meter
			xMrgMid*=(0.5*mrgScale);
			yMrgMid*=(0.5*mrgScale);

			init_utm_to_mrgxy();

			if(bNTW) {
				//not necessary? 
				scale=mrgScale;

				ntw_hdr.scale=1.0/(1000*mrgScale); //world meters per file unit
				ntw_hdr.datumE=mrgMidE;	   //world East coordinate of (0,0)
				ntw_hdr.datumN=mrgMidN;	   //world North coordinate of (0,0)
			}
			else init_mrgxy_to_xy();

			scaleRatio=scale/mrgScale;

			min_veclen_sq=mrgScale*pSVG->advParams.len_min;
			min_veclen_sq*=min_veclen_sq;
			//Could conceivably have multiple vector groups, so initialize now --
			return init_vnod(xMrgMid,yMrgMid);
		}
	}
	return SVG_ERR_REFFORMAT;
}

static double getFontSize(LPCSTR p)
{
	if((p=strstr(p,st_font_size))) return atof(p+10);
	return -1.0;
}

static char *getLabelStyle(double *pSize,LPCSTR *attr)
{
	LPCSTR p=getStyleAttr(attr);
	if(p) {
	  *pSize=getFontSize(p);
	  //I think we can assume that either tspan or text has the complete style spec.
	  //If fLabelSize>0 then the remaining relevant text attributes should also be here --
	  if(bUseStyles && *pSize>0.0) return _strdup(p);
	}
	return NULL;
}

static void setLabelPosition(LPCSTR *attr)
{
	//Station label or note ID is in index at current position. Mark it "present"
	//and replace the vector endpoint page coordinates with the label's or note's
	//offset with respect to this endpoint.
	SVG_PATHPOINT pt;

	if(getPathCTM_Pt(attr,&pt) || getOffset(attr,&pt)) {
		float *px;
		if(iGroupID==ID_NOTES) {
			labelRec.bPresent|=2;
			px=&labelRec.nx;
		}
		else {
			labelRec.bPresent|=1;
			px=&labelRec.sx;
		}
		//Now store label's offsets wrt to endpoint in final map units --
		px[0]=(float)(scaleRatio*(pt.x-px[0]));
		px[1]=(float)(scaleRatio*(pt.y-px[1]));
		trx.UseTree(SVG_TRX_LABELS);
		trx.PutRec(&labelRec);
		trx.UseTree(SVG_TRX_VECTORS);
	}
}

static void scan_ntw_vecs_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;

  LPCSTR pNam;

  if(!iLevel) {
	//We are not already in a recognized group -- 
	if(!strcmp(el,"g") && (pNam=getAttrVal(attr,"id"))) {
		GetGroupFlg(pNam);
		if(iGroupID==ID_REF) {
			iLevel=-1;
		}
		else {
			if(iGroupFlg&(FLG_VECTORS|FLG_MASK|FLG_BACKGROUND)) {
				if(!(iGroupFlg&FLG_BACKGROUND) && mrgScale==0.0) {
					UD(errcode)=SVG_ERR_NOREF;
					UD(errline)=XML_GetCurrentLineNumber(parser);
				}
				else iLevel=1;
			}
		}
	}
	return;
  }

  if(iLevel<0) {
	  //processing reference --
	  if(iLevel<-1) return;
	  if(!strcmp(el,"text")) {
		  //now look for character data --
		  iLevel=-2;
		  bInTextElement=1;
		  reflen=0;
	  }
	  else UD(errcode)=SVG_ERR_NOTEXT;
  }
  else {
	  //processing vectors, or mask  --
	  if(!strcmp(el,"g")) {
		  iLevel++;
		  return;
	  }
	  if(!GetPathType(el)) return;

	  if(iGroupFlg&FLG_VECTORS) {
		//Path definition --
		UD(errcode)=addto_vnode(attr);
	  }
	  else {
		  if(iGroupFlg&FLG_MASK) UD(errcode)=process_path(attr);
		  else {
			  //Must be path in background. Assume last one is rock color --
			  cBackClr=GetFillColor(attr);
		  }
		  return;
	  }
  }

  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scan_ntw_vecs_endElement(void *userData, LPCSTR el)
{
  if(UD(errcode)) return;

  //iLevel can be -1,-2 (Ref) or >0 (Walls or Vectors)
  if(!iLevel) return;

  if(iLevel>0) {
	  if(!strcmp(el,"g")) {
		  iLevel--;
		  if(!iLevel && (iGroupFlg&FLG_VECTORS)) UD(errcode)=SVG_ERR_ENDSCAN;
	  }
  }
  else if(!strcmp(el,"text")) {
	  int e=parse_georef();
	  if(e) {
		  UD(errcode)=e;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  iLevel=0;
	  bInTextElement=0;
  }
}

static float getFlagScale(LPCSTR *attr)
{
	//Get flag scale override from transform --
	LPCSTR p;
	double scale=0.0;
	if(p=getTransformMatrixPtr(attr)) {
	  scale=atof(p);
	  if(p=getAttrVal(attr,"width")) scale*=atof(p)/2.0;
	  else scale=0.0;
	}
	return (float)scale;
}

static void scanv_startElement(void *userData, LPCSTR el, LPCSTR *attr)
{
  if(UD(errcode)) return;

  LPCSTR pNam=getAttrVal(attr,"id");
  LPCSTR pStyle=getAttrVal(attr,"style");

  int i=pNam?GetGroupID(pNam):-1;
  if(i!=-1) {
	  int e=0;
	  if(i==-2) {
		  e=SVG_ERR_W2DNAME;
		  trx_Stncc(refbuf, pNam, sizeof(refbuf));
	  }
	  else if(strcmp(el,"g")) {
		  e=SVG_ERR_W2DGROUP;
		  err_groupID=i;
	  }
	  else if(w2d_name_seen[i]) {
		  e=SVG_ERR_W2DDUP;
		  err_groupID=i;
	  }
	  if(e) {
		  UD(errcode)=e;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
	  }
	  w2d_name_seen[i]=1;
  }

  if(!iLevel) {
	//We are not already in a recognized group -- 
	if(!strcmp(el,"g") && pNam) {
		if(i>=0) {
			iGroupFlg=w2d_flg[i];
			iGroupID=i;
		}
		else {
			iGroupFlg=0;
			iGroupID=-1;
		}

		if(iGroupID==ID_REF) {
			iLevel=-1;
		}
		else {
			UINT target=FLG_VECTORS+FLG_MASK+FLG_BACKGROUND;
			if(flags&SVG_DETAIL) target|=(FLG_DETAIL_SHP+FLG_DETAIL_SYM);
			if(flags&SVG_WALLS) target|=(FLG_WALLS_SHP+FLG_WALLS_SYM);
			if(flags&SVG_LEGEND) target|=FLG_LEGEND;
			if(bUseStyles) {
				target|=(FLG_FRAME+FLG_LABELS);
				if(flags&SVG_NOTES) target|=FLG_NOTES;
				if(flags&SVG_FLAGS) target|=FLG_FLAGS;
				if(flags&SVG_GRID) target|=FLG_GRID;
				if(flags&SVG_VECTORS) {
					target|=FLG_MARKERS;
					if(pSVG->advParams.uFlags&SVG_ADV_LRUDBARS) target|=FLG_LRUDS;
				}
			}
			if(bUseLabelPos) {
				if(flags&SVG_LABELS) target|=FLG_LABELS;
				if(flags&SVG_NOTES) target|=FLG_NOTES;
			}
			if(iGroupFlg&target) {
				if(mrgScale==0.0 && (iGroupFlg&(FLG_BACKGROUND+FLG_GRID))==0) {
					UD(errcode)=SVG_ERR_NOREF;
					UD(errline)=XML_GetCurrentLineNumber(parser);
				}
				else {
					iLevelSym=0;
					iLevel=1;
					iLevelID=0;
				}
			}
		}
	}
	else if(!strcmp(el,"symbol")) {
		//Save styles for entities and (most important) look for referenced symbols --
		iLevel=-3;
	}
	else if(!strcmp(el,"pattern")) {
		//Save styles for entities and (most important) look for referenced symbols --
		iLevel=-4;
	}
	return;
  }

  if(iLevel<0) {
	  if(iLevel==-4) {
		  //Pattern definitions --
		  //For now, output style entites, referenced filters and symbols even
		  //if parent symbol isn't referenced in output SVG
		  if(!strcmp(el,"use"))
			  save_symname(attr);
		  if(pStyle) {
			  save_style(legendstyle,pStyle);
		  }
		  return;
	  }
	  if(iLevel==-3) {
		  //Symbol definitions --
		  //For now, output style entites, referenced filters and symbols even
		  //if parent symbol isn't referenced in output SVG
		  if(!strcmp(el,"use"))
			  save_symname(attr);
		  if(pStyle) {
			  save_style(legendstyle,pStyle);
		  }
		  return;
	  }
	  //processing reference --
	  if(iLevel==-2) {
		  if(pStyle && fTitleSize<=0.0 && !strcmp(el,"tspan"))
			  fTitleSize=getFontSize(pStyle);
		  return;
	  }
	  if(!strcmp(el,"text")) {
		  if(bUseStyles && pStyle && fTitleSize<=0.0)
			  fTitleSize=getFontSize(pStyle);
		  //now look for character data --
		  iLevel=-2;
		  bInTextElement=1;
		  reflen=0;
	  }
	  else UD(errcode)=SVG_ERR_NOTEXT;
  }
  else {
	  //processing vectors, shapes (detail, walls, or mask), labels, or legend  --
	  if(!strcmp(el,"g")) {
		  iLevel++;
#ifdef _DEBUG
		  assert(attr);
		  if(iLevelSym && *attr && !strcmp(*attr,"transform")) {
			  int jj=0;
		  }
#endif
		  if(getPathCTM(attr)<0) {
				UD(errcode)=SVG_ERR_BADXFORM;
				goto _eret;
		  }
		  if(bPathCTM && iLevelSym) {
#ifdef _DEBUG
			int ll=XML_GetCurrentLineNumber(parser);
			bugcount++;
#endif
			//UD(errcode)=SVG_ERR_GRPXFORM;
			//goto _eret;
		  }

#ifdef _USE_XMLID
		  if(!iLevelSym && iGroupFlg&(FLG_SHP_MSK)) {
			if(pNam && IsXMLID(pNam)) {
				//Assume start of live paint group or already within one
				xmlidlvl++;
			}
			else if(xmlidlvl) {
				xmlidlvl++;
				//assume unnamed groups within a live paint group are named groups
				//to allow adjustment!
				if(!pNam) pNam=""; //value not used but must be non-null
			}
		  }
#endif
		  if(!iLevelSym && (iGroupFlg&FLG_WALLS_DETAIL_MSK)) {

				if((bPathCTM || !pNam || isFilterPresent(attr))) {
					//unnamed group -- let's start calculating bounding box in symrange[] --
					iLevelSym=iLevel;
					if(symcnt>=max_symcnt) {
						symrange=(SVG_PATHRANGE *)realloc(symrange,(max_symcnt+=MAX_SYMCNT_INC)*sizeof(SVG_PATHRANGE));
						if(!symrange) {
							UD(errcode)=SVG_ERR_NOMEM;
							goto _eret;
						}
					}
					if(bPathCTM) {
						groupCTM=pathCTM;
						mult(&mrgxy_to_xy,&groupCTM);
					}
					else groupCTM=mrgxy_to_xy;

					symrange[symcnt].xmax=symrange[symcnt].ymax=-FLT_MAX;
					symrange[symcnt].xmin=symrange[symcnt].ymin=FLT_MAX;
					//Increment symcnt when we leave the group --
				}
		  }
		  else {
			  if(bUseLabelPos && (iGroupFlg&(FLG_LABELS+FLG_NOTES))!=0 && !iLevelID && pNam) {
					assert(!bPathCTM && pNam && !isFilterPresent(attr));
					//Station name ID should already be in index, store record in labelRec --
					//Correct for AI id renaming bug!!
					if(*pNam && pNam[strlen(pNam)-1]=='_') {
						trim_idsuffix((char *)pNam);
					}
					if(*pNam && getLabelRec(pNam)) iLevelID=iLevel;
			  }
		  }
		  if(iGroupFlg&(FLG_ADJUSTED+FLG_LEGEND)) {
			  save_style(iLevelSym?groupstyle:legendstyle,getStyleAttr(attr));
		  }
		  return;
	  }

	  if(iGroupFlg&FLG_VECTORS) {
			//Path or line definition --
		    GetPathType(el);
			int e=addto_vnode(attr);
			if(e==SVG_ERR_VECNOTFOUND) {
				if(pGetData(bMerging2?SHP_ORPHAN2:SHP_ORPHAN1,refbuf)) 
					return; //ignore vector;
			}
			UD(errcode)=e;
	  }
	  else if(iGroupFlg&FLG_ADJUSTED) {

		  if(GetPathType(el)) {
			  UD(errcode)=process_path(attr);
		  }
		  else if(!(iGroupFlg&FLG_MASK)) { //exclude mask layer
			  switch(GetSymType(el)) {
				  case S_TEXT :
					  bInTextElement=-updateSymRange(attr);

				  case S_TSPAN :
					  if(bInTextPath) {
						  if(pStyle) save_style(textstyle,pStyle); 
						  break;
					  }

				  case S_IMAGE :
					  if(bInTextElement && pStyle &&
						  (iSymType!=S_TEXT || strcmp(pStyle,strStyleIgnored)))
						   save_style(iLevelSym?groupstyle:textstyle,pStyle);
					  break;

				  case S_TEXTPATH :
					  bInTextPath=1;
					  break;

				  case S_PATTERN :
					  {
						  //Will write all pattern definitions for now
#ifdef _DEBUG
						  //bInsidePattern=true;
#endif
						  break;
					  }

				  case S_USE :
					  //Save symbol ID only if it's in range --
					  if(updateSymRange(attr)) {
						  save_symname(attr);
					  }
#ifdef _DEBUG
					  else {
						  LPCSTR pnam=get_symname(attr);
						  pnam=0;
					  }
#endif
					  //bInsidePattern=false;
					  break;
			  }
			  return;
		  }
	  }
	  else if(iGroupFlg&FLG_LEGEND) {
		  GetSymType(el);
		  if(iSymType==S_USE) {
			  //Save symbol ID (no style assigned here by Illustrator --
			  save_symname(attr);
			  return;
		  }
		  if(pStyle && (iSymType!=S_TEXT||strcmp(pStyle,strStyleIgnored)))
			  save_style(legendstyle,pStyle);
		  //Determine position of legend block from position of first text element --
		  if(!pLegendPt && iSymType==S_TEXT) {
			  pLegendPt=getPathCTM_Pt(attr,&legendPt);
		  }
	  }
	  else if(iGroupFlg&FLG_STYLE_MSK) {
		  if(iGroupFlg&(FLG_LABELS+FLG_NOTES)) {
			  //Relevant style can be either in the <text> or <tspan> element --
			  GetSymType(el);
			  if(iSymType!=S_TEXT && iSymType!=S_TSPAN) return;
			  if(bUseStyles) {
				  if(iGroupID==ID_NOTES) {
					  if(fNoteSize<=0.0 && (pNoteStyle=getLabelStyle(&fNoteSize,attr)) &&
						  !bUseLabelPos) iLevel=0;
				  }
				  else {
					  if(fLabelSize<=0.0) {
						 if((pLabelStyle=getLabelStyle(&fLabelSize,attr)) &&  !bUseLabelPos) {
							 iLevel=0;
						 }
					  }
				  }
			  }
			  if(iLevel && iSymType==S_TEXT && iLevelID) {
				  //Assume transform is on text element --
				  setLabelPosition(attr);
				  iLevelID=0;
			  }
		  }
		  else if(iGroupFlg&(FLG_FLAGS+FLG_MARKERS)) {
			  if(!strcmp(el,"use")) {
				  if(iGroupID==ID_FLAGS) {
					  int i=save_symname(attr);
					  if(i<=MAX_STYLES && !flagscale[i]) flagscale[i]=getFlagScale(attr);
				  }
				  else if(fMarkerThick<=0.0 && (el=getTransformMatrixPtr(attr)))
					  fMarkerThick=atof(el);
			  }
		  }
		  else if(pStyle) {
			  COLORREF *pc=NULL;
			  LPCSTR pTyp;
			  switch(iGroupID) {
				case ID_BACKGROUND: {
									pc=&cBackClr; pTyp=st_fill; break;
									}

				case ID_LRUDS:		{
									pc=&cLrudClr; pTyp=st_stroke; break;
									}

				case ID_GRID:		if(numGridThick<2) {
										el=getStyleValPtr(pStyle,st_stroke_width);
										fGridThick[numGridThick]=el?atof(el):1.0;
										if(numGridThick==1 && fGridThick[0]!=fGridThick[1]) {
											numGridThick++;
											if(fGridThick[0]>fGridThick[1]) {
												double d=fGridThick[0];
												fGridThick[0]=fGridThick[1];
												fGridThick[1]=d;
											}
											if(cGridClr>=0) iLevel=0;
										}
									}
									pc=&cGridClr;
									pTyp=st_stroke;
									break;

				case ID_FRAME: 		if(el=getStyleValPtr(pStyle,st_stroke_width)) {
					  					fFrameThick=(xMid/xMrgMid)*atof(el);
										if(fFrameThick<0.01) fFrameThick=0.01;
									}
									else fFrameThick=1.0;
									iLevel=0;
									break;
			  }
			  if(pc && *pc==(COLORREF)-1) {
				  if((*pc=getColorAttr(pStyle,pTyp))==(COLORREF)-1) {
					  if(iGroupID==ID_BACKGROUND) {
						  cBackClr=0xFFFFFF; //Same as "none"
						  iLevel=0;
					  }
				  }
				  else if(iGroupID!=ID_GRID) iLevel=0;
			  }
		  }
	  }
  }
_eret:
  if(UD(errcode)) UD(errline)=XML_GetCurrentLineNumber(parser);
}

static void scanv_endElement(void *userData, LPCSTR el)
{
  if(UD(errcode)) return;

  //iLevel can be -1,-2 (Ref) or >0 (Walls or Vectors)
  if(!iLevel) return;

  if(iLevel>0) {
	  if(!strcmp(el,"g")) {
		  if(iLevel==iLevelSym) {
			  if(check_prange(&symrange[symcnt])) {
				  //Transfer group styles to legend styles --
				  transfer_groupstyles();
			  }
			  free_styles(groupstyle);
			  symcnt++;
			  iLevelSym=0;
		  }
	#ifdef _USE_XMLID
		  if(xmlidlvl) xmlidlvl--;
	#endif
		  iLevel--;
		  iLevelID=0;
	  }
	  else if(!strcmp(el,"text")) bInTextElement=0;
	  else if(!strcmp(el,"textPath")) bInTextPath=0;
  }

  else if(iLevel==-3) {
	  if(!strcmp(el,"symbol")) iLevel=0;
  }
  else if(iLevel==-4) {
	  if(!strcmp(el,"pattern")) iLevel=0;
  }

  //w2d_Ref --
  else if(!strcmp(el,"text")) {
	  int e=parse_georef();
	  if(e) {
		  UD(errcode)=e;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  iLevel=0;
	  bInTextElement=0;
  }
}

static int scan_merge_file(void)
{
	//Process w2d_Ref and w2d_Vector groups in merged file.
	//Obtain array pathrange[] for paths in w2d_Walls and w2d_Detail.
	//Obtain arrays pathstyles[] and textstyles[] for paths and tspans in w2d_Walls and w2d_Detail.
	//Obtain style for first tspan element in w2d_Labels so that they can be rescaled.
	//Obtain sizes for symbols found in w2d_Flags if we're inheriting styles
	//Also look for <use> elements, saving any symbol IDs.

	int i;

	if(!(fpmrg=gzopen(mrgpath,"rb"))) return SVG_ERR_MRGOPEN;

    veccnt=skipcnt=max_slevel=0;

	if(!bMerging2) {
		fLabelSize=fNoteSize=fTitleSize=-1.0;
		//Attributes possibly obtained from merge file elements --
		cMarkerClr=cGridClr=cBackClr=cPassageClr=cLrudClr=(COLORREF)-1;
		memset(flagscale,0,sizeof(flagscale));
	}

	iLevel=0;
#ifdef _USE_XMLID
	xmlidlvl=0;
#endif
	pathcnt=max_pathcnt=0;
	tpoints=tpoints_w=tpoints_d=0;
	flength_w=flength_d=0.0;
	mrgScale=0.0;
	bAdjust=bMaskPathSeen=false;
	max_diffx=max_diffy=0.0;
	pLegendPt=NULL;

	mrgparser=XML_ParserCreate(NULL);
	if(!mrgparser) return SVG_ERR_NOMEM;

#ifdef _DEBUG
	bugcount=0;
#endif
	memset(w2d_name_seen,0,NUM_W2DNAMES); //for avoiding duplication

	XML_SetElementHandler(mrgparser,scanv_startElement,scanv_endElement);
	XML_SetCharacterDataHandler(mrgparser,scan_chardata);
	//NOTE: We will parse general entities --
	//tm_scan=clock();
	i=parse_file(mrgparser,fpmrg);
	//tm_scan=clock()-tm_scan;
	mrgparser=NULL;

#ifdef _DEBUG
	if(bugcount) {
		char *pp=pathstyle[0];
		bugcount=0;
	}
#endif

	if(i) return i;

	if(fLabelSize<=0.0) fLabelSize=pSVG->labelSize/10.0;
	if(fNoteSize<=0.0) fNoteSize=pSVG->noteSize/10.0;
	if(fTitleSize<=0.0) fTitleSize=pSVG->titleSize/10.0;
	if(cMarkerClr==(COLORREF)-1) cMarkerClr=pSVG->markerColor;
	if(cBackClr==(COLORREF)-1 || !bUseStyles) cBackClr=pSVG->backColor;
	if(cPassageClr==(COLORREF)-1 || !bUseStyles) cPassageClr=pSVG->passageColor;
	if(cGridClr==(COLORREF)-1) cGridClr=pSVG->gridColor;
	if(cLrudClr==(COLORREF)-1) cLrudClr=pSVG->lrudColor;
	if(!veccnt) {
		return SVG_ERR_ADJUSTABLE;
	}
	if(veccnt<vecs_used) vecs_used=veccnt;
	//Flag to indicate if feature adjustment is required --
	max_diff_allowed=pSVG->advParams.maxCoordChg*mrgScale;
	bAdjust=(max_diffx>max_diff_allowed || max_diffy>max_diff_allowed);
	if(!bMerging2) {
		if(bAdjust) {
			pSVG->fMaxDiff=(float)(max_diff/mrgScale);
			strcpy(pSVG->max_diff_idstr,max_diff_idstr);
		}
		pSVG->flength_w=(float)(flength_w/mrgScale);
		pSVG->flength_d=(float)(flength_d/mrgScale);
		pSVG->tpoints_w=tpoints_w;
		pSVG->tpoints_d=tpoints_d;
	}
	else {
		if(bAdjust) {
			pSVG->fMaxDiff2=(float)(max_diff/mrgScale);
			strcpy(pSVG->max_diff_idstr2,max_diff_idstr);
		}
		pSVG->flength_w2=(float)(flength_w/mrgScale);
		pSVG->flength_d2=(float)(flength_d/mrgScale);
		pSVG->tpoints_w2=tpoints_w;
		pSVG->tpoints_d2=tpoints_d;
	}

	//Now let's see which paths and path styles will need to be included --
	memset(pathstyle_used,0,sizeof(pathstyle_used));
	for(i=0;i<(int)pathcnt;i++) {
	  if(check_prange(&pathrange[i])) {
		  pathrange[i].bInRange=1;
		  if(pathrange[i].bMorph) {
			 short idx=pathrange[i].pathstyle_idx;
			//do this only for paths in SHP layer!!
			//flag pathstyle for entity output
			if(idx<=MAX_STYLES)  //***bug fixed!!
				pathstyle_used[idx]=i+1;
		  }
	  }
	  else {
		  pathrange[i].bInRange=0;
		  pathrange[i].pathstyle_idx=-1;
	  }
	}

	return 0;
}

static int scan_ntw_vectors(void)
{
	//Process w2d_Ref and w2d_Vector groups in SVG source file.

	int i;

	if(!(fpmrg=gzopen(mrgpath,"rb"))) return SVG_ERR_MRGOPEN;

    veccnt=skipcnt=max_slevel=0;

	cBackClr=(COLORREF)-1;
	iLevel=0;
	pathcnt=0;
	mrgScale=0.0;
	bAdjust=false;
	max_diffx=max_diffy=0.0;
	tpoints=0;

	mrgparser=XML_ParserCreate(NULL);
	if(!mrgparser) return SVG_ERR_NOMEM;

	XML_SetElementHandler(mrgparser,scan_ntw_vecs_startElement,scan_ntw_vecs_endElement);
	XML_SetCharacterDataHandler(mrgparser,scan_chardata);
	//NOTE: We will parse general entities --
	//tm_scan=clock();
	i=parse_file(mrgparser,fpmrg);
	mrgparser=NULL;
	if(i) return i;

	//tm_scan=clock()-tm_scan;

	if(!(max_pathcnt=pathcnt)) return SVG_ERR_NOMASK;

	if(!veccnt) {
		return SVG_ERR_NOVECTORS;
	}
	if(veccnt<vecs_used) vecs_used=veccnt;

	//Flag to indicate if feature adjustment is required --
	max_diff_allowed=pSVG->advParams.maxCoordChg*mrgScale;

	bAdjust=(max_diffx>max_diff_allowed || max_diffy>max_diff_allowed);

	return 0;
}

static void init_scan_vars()
{
	fpout=NULL;
	gzout=NULL;
	bWriting=false;
	fpin=fpmrg=NULL;
	vstk=NULL; vnod=NULL;
	assert(!pathrange && !pathcnt && !max_pathcnt);
	assert(!symrange && !symcnt && !max_symcnt);
	pathstyle[0]=textstyle[0]=legendstyle[0]=symname[0]=groupstyle[0]=NULL;
	parser_ErrorLineNumber=0;
	if(!bMerging2) {
		free(pLabelStyle); free(pNoteStyle);
		pLabelStyle=pNoteStyle=NULL;
	}
}

//======================================================================
//merge2 related --

static void out_merged_entities(char ctyp)
{
	for(int i=0;i<svg_entitycnt;i++) {
		if(svg_entity[i].name[1]==ctyp) {
			gz_printf("\t<!ENTITY %s \"%s\">\r\n",svg_entity[i].name,svg_entity[i].style);
		}
	}
}

static void replace_name(LPSTR lineoff,int lenoldname,LPCSTR newname,bool bAdd)
{
	if(bAdd && lenoldname<256) {
		assert(bMerge2);
		char oldname[256];
		memcpy(oldname,lineoff,lenoldname);
	    oldname[lenoldname]=0;
		it_vec_lpstr itnew=vpNamNew.begin();
		for(it_vec_lpstr it=vpNamOld.begin(); it!=vpNamOld.end(); it++, itnew++) {
			if(!strcmp(oldname, *it)) {
				assert(!strcmp(newname,*itnew));
				replace_name(lineoff,lenoldname, newname, false);
				return;
			}
		}
		vpNamOld.push_back(strdup(oldname));
		vpNamNew.push_back(strdup(newname));
	}
	int newlen=strlen(newname);
	memmove(lineoff+newlen,lineoff+lenoldname,strlen(lineoff+lenoldname)+1);
	memcpy(lineoff,newname,newlen);
}


static bool fix_entity_name(LPSTR pid)
{
	int oldlen;
	LPSTR pend=strstr(pid,";\"");
	char oldname[MAX_NAMESIZE];

	if(!pend || (oldlen=pend-pid)>=8)
		return false;
	memcpy(oldname,pid,oldlen);
	oldname[oldlen]=0;
	for(int i=0;i<svg_entityidxcnt;i++) {
		if(!strcmp(oldname,svg_entity_idx[i].name)) {
			//replace entity name with corresponding name in first svg --
			LPSTR newname=svg_entity[svg_entity_idx[i].idx].name;
			if(strcmp(newname,oldname)) {
				replace_name(pid,oldlen,newname,false);
			}
			break;
		}
	}
	return true;
}

static NAME_MAP *getNameMapPtr(LPSTR pid)
{
	int len;
	LPSTR pend=strchr(pid,'\"');
	char name[MAX_NAMESIZE];

	if(!pend || (len=pend-pid)>=MAX_NAMESIZE) {
		assert(0);
		return NULL;
	}
	memcpy(name,pid,len);
	name[len]=0;
	for(it_name_map it=vNameMap.begin();it!=vNameMap.end();it++) {
		if(!strcmp(name,PNAME(it->oldoff))) {
			//$1.svg symbol definition
			return &*it;
		}
	}
	return NULL;
}

static bool fix_mrg_name(LPSTR pid)
{
	int oldlen;
	char oldname[MAX_NAMESIZE];
	LPSTR pend=strchr(pid,'\"');

	if(!pend || (oldlen=pend-pid)>=MAX_NAMESIZE)
		return false;

	memcpy(oldname,pid,oldlen);
	oldname[oldlen]=0;
	for(it_name_map it=vNameMap.begin();it!=vNameMap.end();it++) {
		if(!strcmp(oldname,PNAME(it->oldoff))) {
			//replace name for symbol
			if(it->newoff>=0) {
				replace_name(pid,oldlen,PNAME(it->newoff),true);
			}
			break;
		}
	}
	return true;
}

static bool fix_symbol_references(LPSTR line)
{
	LPSTR pNext=line;
	if(!memcmp(pNext,sym_prefix,SYM_PREFIX_LEN)) {
		//symbol id already handled!
		pNext+=SYM_PREFIX_LEN;
	}

	if(pNext=strstr(pNext," id=\"")) {
		if(!fix_mrg_name(pNext+=5))
			return false;
	}

	pNext=line;
	while((pNext=strstr(pNext,"href=\"#"))) {
		if(!fix_mrg_name(pNext+=7))
			return false;
	}
	return true;
}

static bool fix_entity_references(LPSTR line)
{
	LPSTR pNext=line;
	while((pNext=strstr(pNext,"=\"&_"))) {
		pNext+=4;
		if(*pNext=='p' || *pNext=='l' || *pNext=='t') {
			if(!fix_entity_name(pNext-1))
				return false;
		}
	}
	return true;
}

static bool fix_mrg_references(LPSTR line)
{
	if(fix_entity_references(line)) {
		return fix_symbol_references(line);
	}
	return false;
}

static void close_merge2_temps(FILE *fpin1,FILE *fpin2)
{
	if(fpin1) {
		fclose(fpin1); fpin1=NULL;
		strcpy(pnamxml,"$1" SVG_TMP_EXT);
#ifndef _DEBUG
		unlink(pnamxml);
#endif
	}
	if(fpin2) {
		fclose(fpin2); fpin2=NULL;
		strcpy(pnamxml,"$2" SVG_TMP_EXT);
#ifndef _DEBUG
		unlink(pnamxml);
#endif
	}
}

static int advance_to_w2d(FILE *fpin,LPSTR line,LPCSTR pid,BOOL bWrite)
{
	//return <=0 if error or no w2d of any kind found, 1 if wrong id found (or group empty), 2 if exact match
	//assumes already read past last w2d group
	LPCSTR pfx=(pid[1]=='\t')?w2d_prefixsub:w2d_prefix;
	int pfxlen=strlen(pfx);
	int iret;

	//Read and optionally output lines until encountering w2d group id, starting at current line
	do {
		if(!memcmp(line,pfx,pfxlen)) {
			iret=1;
			if(!memcmp(line,pid,strlen(pid)-3)) {
				if(strstr(line,"/>\r\n")) {
					if((iret=fgetline(line,fpin))>0) iret=1;
				}
				else iret=2;
			}
			break;
		}
		if(bWrite) {
			if(bWrite>1)
				fix_mrg_references(line);
			gz_puts(line);
		}
	}
	while((iret=fgetline(line,fpin))>0);
	return iret;
}

static int advance_to_end(FILE *fpin,LPSTR line,LPCSTR pend,BOOL bWrite)
{
	//return <=0 if error or no grp end not found, 1 if end found
	//assumes already read past last w2d group
	int iret;

	//Read and optionally output lines until encountering w2d group id, starting at current line
	while((iret=fgetline(line,fpin))>0) {
		if(!strcmp(line,pend)) {
			return 1;
		}
		if(strstr(line,"<g id=\"w2d_")) {
			return -1;
		}
		if(bWrite) {
			if(bWrite>1) {
				fix_mrg_references(line);
			}
			gz_puts(line);
		}
	}
	return iret;
}

int merge_w2d(FILE *fpin1,LPSTR line1,FILE *fpin2,LPSTR line2,LPCSTR pid)
{
	LPCSTR pEnd=(pid[1]=='\t')?w2d_endsubgrp:w2d_endgrp;
	gz_puts(pid);
	int iFound=advance_to_w2d(fpin1,line1,pid,0);
	if(iFound==2) {
		gz_printf("%s<!-- %s -->\r\n",(pEnd==w2d_endsubgrp)?"\t\t\t":"\t\t",trx_Stpnam(pSVG->mrgpath));
		if(advance_to_end(fpin1,line1,pEnd,1)<=0) {
			assert(0);
			return 1;
		}
	}
	iFound=advance_to_w2d(fpin2,line2,pid,0);
	if(iFound==2) {
		gz_printf("%s<!-- %s -->\r\n",(pEnd==w2d_endsubgrp)?"\t\t\t":"\t\t",trx_Stpnam(pSVG->mrgpath2));
		if(advance_to_end(fpin2,line2,pEnd,2)<=0) {
			assert(0);
			return 1;
		}
	}
	gz_puts(pEnd);
	return 0;
}

bool IsSymMarkerName(LPCSTR line)
{
	return (line[SYM_PREFIX_LEN]=='_' && line[SYM_PREFIX_LEN+1]=='m' && line[SYM_PREFIX_LEN+2]=='\"');
}

void free_vpline(VEC_LPSTR &vpline)
{
	if(vpline.size()) {
		for(it_vec_lpstr it=vpline.begin();it!=vpline.end();it++)
			free(*it);
	}
}

bool get_renamed_value(LPSTR pSym)
{
	if(vpNamOld.size()) {
		LPSTR pq=strchr(pSym, '\"');
		if(pq) {
			*pq=0;
			it_vec_lpstr itnew=vpNamNew.begin();
			for(it_vec_lpstr it=vpNamOld.begin(); it!=vpNamOld.end(); it++,itnew++) {
				if(!strcmp(pSym,*it)) {
					int len=strlen(pSym);
					*pq='\"';
					replace_name(pSym,len,*itnew,false);
					return true;
				}
			}
			*pq='\"';
		}
	}
	return false;
 }

int combine_merged()
{
	int e=SVG_ERR_FINALMRG;
	FILE *fpin1=NULL,*fpin2=NULL;
	char line1[MAX_LINESIZE];
	char line2[MAX_LINESIZE];
	VEC_LPSTR vpline1,vpline2;
	bool bInGroup;


	strcpy(pnamxml,"$1" SVG_TMP_EXT);
	if(!(fpin1=fopen(pathxml,"rb")))
		goto _eret;

	strcpy(pnamxml,"$2" SVG_TMP_EXT);
	if(!(fpin2=fopen(pathxml,"rb")))
		goto _eret;

	strcpy(pathxml,pSVG->svgpath); //may be unlinked
#ifdef XML_EXT
	if(!bGzout) 
		strcpy(trx_Stpext(pathxml),XML_EXT);
#endif
	if(bGzout) {
		fpout=NULL;
		if(!(gzout=gzopen(pathxml,"wb")))
			goto _eret;
	}
	else {
		gzout=NULL;
		if(!(fpout=fopen(pathxml,"wb")))
			goto _eret;
	}
	
	assert(nPidx>1);

	int len;
	while((len=fgetline(line1,fpin1))>0) {
		if(*line1=='\t') {
			if(len>14 && !memcmp(line1,"\t<!ENTITY _p",12))
				break;
		}
		gz_puts(line1);
	}
	if(len<=0) goto _eret;

	out_merged_entities('p');
	out_merged_entities('t');
	out_merged_entities('l');

	//now skip past remaining entities in fpin1 and close section --
	while((len=fgetline(line1,fpin1))>0) {
		if(len>=4 && !memcmp(line1,"]>\r\n",4)) {
			gz_puts(line1);
			break;
		}
	}
	if(len<=0) goto _eret;

	//Read and output $1.svg symbols until w2d_Backround encountered
	NAME_MAP *mapptr=NULL;
	int lineidx=0;
	bool bSymbolSeen=false;

	while((len=fgetline(line1,fpin1))>0) {
		if(!strcmp(line1,w2d_Backround)) {
			break;
		}
		if(!memcmp(line1,sym_prefix,SYM_PREFIX_LEN)) {
			if(!bSymbolSeen) {
				gz_printf("\t<!-- %s -->\r\n", trx_Stpnam(pSVG->mrgpath));
				bSymbolSeen=true;
			}
			if(!IsSymMarkerName(line1)) {
				//Is this symbol also in $2.svg?
				assert(!mapptr);
				if((mapptr=getNameMapPtr(line1+SYM_PREFIX_LEN))) {
					mapptr->vplineoff=vpline1.size();
					mapptr->vplinecnt=0;
				}
			}
		}

		gz_puts(line1);
		if(mapptr) {
			if(!strcmp(line1,sym_end)) {
				assert(mapptr->vplinecnt);
				mapptr=NULL;
			}
			else {
				//Save all lines but sym_end of symbol definition to allow comparing with version in $2.svg --
				vpline1.push_back(strdup(line1));
				mapptr->vplinecnt++;
			}
		}
	}
	if(len<=0)
		goto _eret;

	gz_printf("\t<!-- %s -->\r\n", trx_Stpnam(pSVG->mrgpath2));

	//in $2.svg, skip to first symbol past "_m" or w2d_ref
	bInGroup=false; //in symbol section
	bool bInMatchedSymbol=false; //in symbol definition

	while((len=fgetline(line2,fpin2))>0) {
		if(!strcmp(line2,w2d_Backround)) {
			//Through with symbols, stopped here when reading $1.svg
			gz_puts(line2);
			break;
		}
		if(!memcmp(line2,sym_prefix,SYM_PREFIX_LEN)) {

			//replace symbol id if required --
			assert(!bInMatchedSymbol);
			if(!bInGroup && IsSymMarkerName(line2)) {
				assert(!bInGroup);
				continue;
			}
			bInGroup=true;

			//Check if symbol name was encountered in $1.svg
			mapptr=getNameMapPtr(line2+SYM_PREFIX_LEN);

			if(mapptr) {
				//Name matches an already defined symbol.

				//If the symbol was already referenced within a previous symbol,
				//we will have already renamed it assuming it could be different.
				//In this case there's no need for a line-by-line check for equivalence
				//before replacing the name with the renamed value!

				if(get_renamed_value(line2+SYM_PREFIX_LEN)) {
					bInMatchedSymbol=false;
				}
				else {
					//start saving lines instead of writing them
					assert(!vpline2.size());
					bInMatchedSymbol=true;
				}
			}
			else {
				bInMatchedSymbol=false;
			}
		}
		if(bInMatchedSymbol) {
			//processing lines of a symbol with a matched name --
			assert(mapptr);
			if(!strcmp(line2,sym_end)) {
				//finished saving symbol lines, so compare results --
				if(mapptr->vplinecnt==vpline2.size()) {
					it_vec_lpstr it1=vpline1.begin()+mapptr->vplineoff;
					it_vec_lpstr it2=vpline2.begin();
					for(;it2!=vpline2.end();it2++,it1++) {
						if(strcmp(*it2,*it1)) break;
					}
					if(it2!=vpline2.end()) {
						//symbol definitions are different, so output the saved lines with the replacement id name.
						//entity and symbol references should have already already replaced.
						it2=vpline2.begin();
						strcpy(line2,*it2++);
						replace_name(line2+SYM_PREFIX_LEN,strlen(PNAME(mapptr->oldoff)),PNAME(mapptr->newoff),true);
						gz_puts(line2);
						for(;it2!=vpline2.end();it2++) {
							gz_puts(*it2);
						}
						gz_puts(sym_end);
					}
					else {
						//replacements for this symbol won't be needed
						mapptr->newoff=-1;
					}
				}
				bInMatchedSymbol=false;
				free_vpline(vpline2);
				vpline2.clear();
			}
			else {
				// after fixing references, save this line for comparison when end is reached
				if(!fix_mrg_references(line2))
					goto _eret;
				vpline2.push_back(strdup(line2));
			}
		}
		else if(bInGroup) {
			//replace entity and symbol references as required
			if(!fix_mrg_references(line2))
				goto _eret;
			gz_puts(line2);
		}
	}
	//finished processing $2.svg symbols --
	if(len<=0)
		goto _eret;

	//position fpin1
	while((len=fgetline(line1,fpin1))>0) {
		gz_puts(line1);
		if(!strcmp(line1,w2d_Ref)) break;
	}
	if(len<=0)
		goto _eret;
	fgetline(line1,fpin1); //advance to first line within w2d_Ref

	//position fpin2
	while((len=fgetline(line2,fpin2))>0) {
		if(!strcmp(line2,w2d_Ref)) break;
	}
	if(len<=0)
		goto _eret;
	fgetline(line2,fpin2); //advance to first line within w2d_Ref


	//finish writing ref group, end with line1 containing <g id=w2d_mask (if it exists)
	//or the w2d group that follows
	len=advance_to_w2d(fpin1,line1,w2d_Mask,1); //return 2 if found, 1 if empty
	if(len<1)
		goto _eret;

	merge_w2d(fpin1,line1,fpin2,line2,w2d_Mask);

	gz_puts(w2d_Detail);
	merge_w2d(fpin1,line1,fpin2,line2,w2d_Detail_shp);
	merge_w2d(fpin1,line1,fpin2,line2,w2d_Detail_sym);
	gz_puts(w2d_endgrp);

	gz_puts(w2d_Walls);
	merge_w2d(fpin1,line1,fpin2,line2,w2d_Walls_shp);
	merge_w2d(fpin1,line1,fpin2,line2,w2d_Walls_sym);
	gz_puts(w2d_endgrp);

	if((len=advance_to_w2d(fpin1,line1,w2d_Survey,0))!=2)
		goto _eret;

	gz_puts(w2d_Survey);

	//Now write all other non-merged groups, until t w2d_Legend layer in either $1.svg or $2.svg or both!
	int len_l=strlen(w2d_Legend);
	int len_f=strlen(w2d_Frame);
	bool bLeg1=false,bLeg2=false;

	while((len=fgetline(line1, fpin1))>0) {
		if(!memcmp(line1, w2d_Legend, len_l)) {
			bLeg1=true;
			break;
		}
		if(!memcmp(line1, w2d_Frame, len_f)) {
			break;
		}
		gz_puts(line1);
	}

	while((len=fgetline(line2, fpin2))>0) {
		if(!memcmp(line2, w2d_Legend, len_l)) {
			bLeg2=true;
			break;
		}
		if(!memcmp(line1, w2d_Frame, len_f)) {
			break;
		}
	}
	if(bLeg1 || bLeg2) {
		LPCSTR p;
		gz_puts(w2d_Legend); gz_puts(">\r\n");
		if(bLeg1) {
			gz_printf("\t\t<!-- %s -->\r\n\t\t<g",trx_Stpnam(pSVG->mrgpath));
			p=strstr(line1,"transform");
			if(p) gz_printf(" %s",p);
			else gz_puts(">\r\n");
			while((len=fgetline(line1, fpin1))>0) {
				if(!stricmp(line1, "\t</g>\r\n")) {
					break;
				}
				gz_printf("\t%s",line1);
			}
			gz_puts("\t\t</g>\r\n");
		}

		if(bLeg2) {
			gz_printf("\t\t<!-- %s -->\r\n\t\t<g", trx_Stpnam(pSVG->mrgpath2));
			p=strstr(line2, "transform");
			if(p) gz_printf(" %s", p);
			else gz_puts(">\r\n");
			while((len=fgetline(line2, fpin2))>0) {
				if(!stricmp(line2, "\t</g>\r\n")) {
					break;
				}
				fix_mrg_references(line2);
				gz_printf("\t%s", line2);
			}
			gz_puts("\t\t</g>\r\n");
		}
		gz_puts("\t</g>\r\n");
	}

	//Now write the remaining non-merged groups $1.svg, that is w2d_Frame
	while((len=fgetline(line1,fpin1))>0) {
		gz_puts(line1);
	}
	if(len<0)
		goto _eret;

	e=0;

_eret:
	free_vpline(vpline1);
	free_vpline(vpline2);
	free_vpline(vpNamOld);
	free_vpline(vpNamNew);
	if(fpout || gzout) {
		close_output(e);
	}
	close_merge2_temps(fpin1,fpin2);
	return e;
}


//=====================================================================================================
//Main entry point --
DLLExportC int PASCAL Export(UINT uFlags,LPFN_EXPORT_CB wall_GetData)
{
	int e;

	pGetData=wall_GetData;
	flags=uFlags;
	if(wall_GetData(SHP_VERSION,&pSVG)!=EXP_SVG_VERSION)
		return SVG_ERR_VERSION;

	mrgpath=pSVG->mrgpath;
	bMerge2=(pSVG->mrgpath2!=NULL && *pSVG->mrgpath2!=0);
	bMerging2=false;
	bGzout=strlen(trx_Stpext(pSVG->svgpath))==5;

	init_scan_vars();

	bNTW=(flags&SVG_WRITENTW)!=0;

	if(bNTW) {
		pnPly=NULL;
		pNtwRec=NULL;
		pMaskName=NULL;
		pNTWp=NULL; pNTWf=NULL;
		dist_exp=pSVG->advParams.dist_exp;
		dist_off=pSVG->advParams.dist_off;
		len_exp=pSVG->advParams.len_exp;
		vecs_used=pSVG->advParams.vecs_used;
		scale=0.0; //just in case
		flags|=SVG_MERGEDCONTENT;
		bUseLabelPos=FALSE; //accessed in createIndex
		if(e=createIndex()) return finish(e);
		if(e=scan_ntw_vectors()) return finish(e);
		tptotal=tpoints; //for calculating percent progress.
		e=write_ntw();
		return finish(e);
	}

	if(flags&SVG_EXISTS) {
		//determine status of any pre-existing svg that would be overwritten --
		pSVG->flags&=~(SVG_EXISTS+SVG_UPDATED+SVG_ADDITIONS+SVG_EXISTADJUSTABLE);
		pSVG->version=0;
		e=finish(scan_svgheader());
		if(!e) pSVG->version=EXP_SVG_VERSION;
		return e;
	}
	init_constants();

	if(e=createIndex()) return finish(e);

	if(flags&SVG_MERGEDCONTENT) {
		//Scan the SVG to be merged, processing w2d_Ref, w2d_Vector, w2d_Walls, w2d_Detail,
		//and w2d_Mask groups, building searchable binary tree of vectors, vnod[], and an
		//array, pathrange[], of shape or wall segment ranges. We also determine the location
		//of the first w2d_Legend text object, save names of referenced symbols, and check
		//if a marker symbol (_m) and marker size entity (_M) is defined.
		if(e=scan_merge_file()) return finish(e);
		tptotal=tpoints; //for calculating percent progress.
	}

 	if(flags&SVG_FLAGS) {
		//Need flag info to define symbols in output SVG --
		if(e=GetFlagData()) return finish(e);
	}

	if(flags&SVG_NOTES) {
		//Determine if note entities need to be defined --
		getNumNoteLocations();
	}

	//Write the SVG file while parsing the XML template and also
	//parsing again, if necessary, the SVG to be merged --

	if(bMerge2) {
		//will be saving p, t, and l entities
		svg_entitycnt=svg_entityidxcnt=nLidx=nPidx=nTidx=0;
		//svg_entitycnt_div=0;
		idNameBuf=NULL;
		idNameLen=idNameSiz=0;
	}

#ifdef _USE_XMLID
	xmlidlvl=xmlidcnt=0;
#endif
	e=write_svg(); //outpath based on mrgpath if bMerge2

	if(!e && bMerge2) {
		finish(0);
		bMerging2=true;
		//flags&=~(SVG_FLAGS|SVG_NOTES|SVG_LABELS|SVG_VECTORS|SVG_GRID|SVG_LEGEND);
		flags&=~(SVG_FLAGS|SVG_NOTES|SVG_LABELS|SVG_VECTORS|SVG_GRID);
		init_scan_vars();
		mrgpath=pSVG->mrgpath2;
		if(e=scan_merge_file()) return finish(e);
		tptotal=tpoints; //for calculating percent progress.
#ifdef _USE_XMLID
		xmlidlvl=0;
#endif
		e=write_svg();
		if(!e) {
			pGetData(SHP_STATUSMSG, "");
			finish(0); //closes temp files
			if(!(e=combine_merged())) {
				free_entities(); 
				return 0;
			}
		}
	}

	pGetData(SHP_STATUSMSG, "");
	return finish(e);
}
