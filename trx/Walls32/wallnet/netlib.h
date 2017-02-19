/*NETLIB.H -- Interface for network adjustment library, NETLIB.LIB */

#ifndef __NETLIB_H
#define __NETLIB_H

#ifndef __DBF_TYPE_H
#include <dbf_file.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

#define NET_VEC_EXT "NTV"
#define NET_STR_EXT "NTS"
#define NET_PLT_EXT "NTP"
#define NET_NET_EXT "NTN"
#define NET_SEG_EXT "NTA"
#define NET_NAM_EXT "N$N"
#define NET_TMP_EXT "N$$"
#define NET_LST_EXT "LST"
#define NET_3D_EXT  "wrl"
#define NET_2D_EXT  "svgz"
#define NET_WMF_EXT "WMF"
#define NET_EMF_EXT "EMF"
#define NET_NTW_EXT "NTW"
#define NET_NTAC_EXT "NTAC"

#define NET_VERSION 4
#define NTA_NUMTREES 4
#define NTA_SEGTREE 0
#define NTA_PREFIXTREE 1
#define NTA_NOTETREE 2
#define NTA_LRUDTREE 3
#define NTA_CHAR_NOTEPREFIX 0xFF
#define NTA_CHAR_FLAGPREFIX 0xFE
 
#define NET_COMPONENT_FLAG '.'
#define NET_FIXED_FLAG '0'
#define NET_SURVEY_FLAG ' '
#define NET_REVISIT_FLAG '+'
#define NET_RESOLVE_FLAG '-'

#define NET_SIZNAME 8
#define NET_SIZFILE 8

#define NET_SIZ_ERRBUF	   80 

#define NET_ErrNearHeap   -10
#define NET_ErrFarHeap    -11
#define NET_ErrSystem     -12
#define NET_ErrMaxSystems -13
#define NET_ErrAlist      -14
#define NET_ErrMaxVecs    -15
#define NET_ErrMaxNodes   -16
#define NET_ErrMaxStrings -17
#define NET_ErrBwLimit    -18
#define NET_ErrDisconnect -19
#define NET_ErrSysConnect -20
#define NET_ErrNcCompare  -21
#define NET_ErrCallBack   -22
#define NET_ErrSysMem     -23
#define NET_ErrEmpty	  -24
#define NET_ErrMaxNets    -25
#define NET_ErrVersion	  -26
#define NET_ErrWork		  -27
#define NET_ErrMaxStrVecs -28
#define NET_ErrConstrain  -29

/*Callback function argument values --*/
enum e_netPost {
  NET_PostReadNTV,
  NET_PostConnect,
  NET_PostCreateNTS,
  NET_PostSortSys,
  NET_PostSolveSys,
  NET_PostSolveNet,
  NET_PostUpdateNTV,
  NET_PostUpdateNTS,
  NET_PostNetwork};

/*The BwCount option is enabled in a special NETLIB.LIB compilation --*/
enum eNET {NET_Adjust=1,NET_NewNames=2,NET_PltFile=4,NET_PntFile=8,NET_LstFile=256,
           NET_LstDetail=512,NET_BwCount=1024};

/*MAX_VECS is 16K to guarantee room for endpoint and workspace
  arrays within 64K segments during the initial structure analysis.
  This restriction is easily relaxed in a 32-bit program version.*/

/*Constants for WALLNETH.DLL --*/
#ifdef _WIN32
#define MAX_VECS ((UINT)UINT_MAX/8)
#define MAX_NODES (MAX_VECS-1)
#define MAX_STRINGVECS (16*1024)
#else
#define MAX_VECS ((UINT)32767)
#define MAX_NODES (MAX_VECS-1)
#define MAX_STRINGVECS (16*1024)
#endif

#define NET_NTS_VERSION 1116

enum {
      NET_STR_NORMAL=0,
      NET_STR_FLTVECTH=1,	 /*Some vectors are floated in data*/
	  NET_STR_FLTALLH=2,	 /*If str floated dynamically, float all vectors*/
	  NET_STR_FLTTRAVH=3,    /*All vectors are floated in data*/
      NET_STR_FLTVECTV=4,
	  NET_STR_FLTALLV=8,
	  NET_STR_FLTTRAVV=12,

	  /*The following flag bits have eqivalents in pSTR and pSTR->flag1 --*/

      NET_STR_FLOATH=(1<<4),   /*Dynamic float flags - init in wallnet, toggled by Walls*/
	  NET_STR_FLOATV=(1<<5),

	  NET_STR_REVERSE=(1<<6),  /*Reverse attachment (set and used by Walls)*/
	  NET_STR_CHNDIR=(1<<8),   /*Direction of traverse in chain (not used by Walls)*/

	  NET_STR_BRIDGEH=(1<<9), /*The string is currently a statistical bridge*/
	  NET_STR_BRIDGEV=(1<<10),

	  NET_STR_CHNFLTH=(1<<11), /*This chain member is floated */
	  NET_STR_CHNFLTV=(1<<12),

	  NET_STR_CHNFLTXH=(1<<13),/*A different chain member is floated */
	  NET_STR_CHNFLTXV=(1<<14)  
	 };

#define NET_STR_FLTMSKH (NET_STR_FLOATH+NET_STR_CHNFLTH)
#define NET_STR_FLTMSKV (NET_STR_FLOATV+NET_STR_CHNFLTV)
#define NET_STR_FLTMSK (NET_STR_FLTMSKH+NET_STR_FLTMSKV)
#define NET_STR_BRGFLTH (NET_STR_BRIDGEH+NET_STR_CHNFLTXH)
#define NET_STR_BRGFLTV (NET_STR_BRIDGEV+NET_STR_CHNFLTXV)

//*** (10/23/04) Add flag to first field of NTS file (flag1) to mark last string of a system --
#define NET_STR_SYSEND 128
#define NET_STR_LINKDIR 1
	 
#define NET_STR_FLTMASKH (NET_STR_FLOATH+NET_STR_FLTTRAVH)
#define NET_STR_FLTMASKV (NET_STR_FLOATV+NET_STR_FLTTRAVV)
#define NET_STR_FLTPARTH (NET_STR_FLOATH+NET_STR_FLTVECTH)
#define NET_STR_FLTPARTV (NET_STR_FLOATV+NET_STR_FLTVECTV)

//#define NTV_FIXOFFSET 16

typedef struct {
				 BYTE day;
				 BYTE month;
				 BYTE year;		/*year - 1800*/
} NET_DATE;

typedef struct { /*15*8 = 120 bytes*/
                 char  flag;        /* Vector flag - see above            */
				 BYTE  date[3];
 				 WORD  pfx[2];	    /* Name prefixes (NTA tree #2 1-based or 0) */
                 char  nam[2][NET_SIZNAME];  /* From/To station names    */
                 double xyz[3];     /* Raw East-North-Up (meters)*/
                 double hv[2];      /* Variance of Hz and Vt components (sq meters)*/
                 long id[2];		/* From/To station numbers            */
                 long str_id;		/* NTS rec # if in string (or 0)      */
                 long str_nxt_vc;	/* NTV rec # of next string vec (or 0)*/
                 double e_xyz[3];   /* Adjusted East-Nort-Up*/ 
				 DWORD  lineno;     /* Line number in file (1-based)*/
				 WORD  seg_id;      /* 0-based index to NTA*/
				 WORD  fileno;      /* 0-based file number*/
				 BYTE  filename[NET_SIZFILE]; /*Survey file base name*/
			   } vectyp;
typedef vectyp near *vecptr;

typedef struct { /* 12*8 + 7*4 + 3*2 + 2 = 132 bytes */
				 char  flag;     /* Flag */
				 char  flag1;    /* Additional flags - 1: last string of system*/
                 short sys_id;   /* System id (starts with 1) (-1 when net specific) */
                 long  str_id;   /* Sorted sequence no. in component (base 0)*/
                 long id[2];     /* From/To station indices */
                 double xyz[3];   /* Raw East-North-Up (meters)*/
                 double hv[2];    /* Variance of Hz and Vt components (sq meters)*/
                 double e_xyz[3]; /* Final xyz*/
                 double e_hv[2];  /* Network variance*/
                 double sys_ss[2];/* System's sum-of-squares */
                 long vec_cnt;  /* Number of vectors in string or 0 if segment */
                 WORD sys_ni[2];/* Number of floating traverses in system*/
                 long sys_nc;	 /* Total number of loops in system*/
                 long vec_id;   /* NTV rec # of first string vector   */
				 long lnk_nxt;  /* NTS rec # of next link fragment or 0 (last pts to first)*/
               } strtyp;
               
typedef strtyp near *strptr;

typedef struct { /* 14 * 4 = 56*/ 
				 char   flag;     /* Flag '.' if this is a revisit to an established point*/
				 BYTE   date[3];
				 WORD  end_pfx;  /* Name prefix (same as in vectyp)*/
                 WORD  seg_id;   /* Vector segment index if vec_id!=0*/
                 long  end_id;   /* Name id (same as in vectyp) */
                 long  str_id;   /* NTS rec # of string (0 if none)*/
                 DWORD vec_id;   /* MoveTo if 0, else NTV rec = (1+vec_id)/2, TO station if vec_id even*/
                 long	end_pt;	  /* index in virtual array of network points*/ 
                 double  xyz[3];   /* Coordinates: East, North, Up*/
				 char	name[NET_SIZNAME];
               } plttyp;
typedef plttyp near *pltptr;

typedef struct { /* 24*4 = 96 bytes */
				 char   flag;     /* Flag */
				 char	flag1;	  /*For alignment only*/
                 WORD	num_sys;  /* # of multiloop systems*/
				 DWORD	vectors;
				 long	vec_id;   /* NTV rec # of reference station*/
                 long   str_id1;  /* NTS rec # no of first string*/
                 long   str_id2;  /* NTS rec # no of last string*/
                 DWORD  plt_id1; /* NTP rec # of first point*/
                 DWORD  plt_id2; /* NTP rec # of last point*/
                 long	endpts;	  /* number of revisited points in component*/
				 double length;
				 double xyzmin[3];
				 double xyzmax[3];
				 char   name[NET_SIZNAME];  /*Reference station*/
               } nettyp;
typedef nettyp *netptr;

/*Used by WALLS: CPrjDoc::GetVecData() and CTraView::OnDrawItem() --*/
typedef struct {
	double sp[3]; 			/*Di,Az,Vt -- original vector*/
	double e_sp[3];         /*Di,Az,Vt -- best correction*/
	vectyp *pvec;           /*Ptr to corresponding vectyp*/
	long sp_index;         /*Index of measurement to be highlighted, or -1*/
} vecdata;
  
/*Structure for communicating netween Walls and WALLNET.LIB-*/

typedef struct {
  DWORD   vectors;
  double  length;
  char   name[NET_SIZNAME];
} NET_WORK;

typedef NET_WORK FAR *pNET_WORK;

typedef struct {
  WORD version;
  WORD flag;
  int sys_id;
  int net_id;
  DWORD   vectors;
  double  length;
  char   name[NET_SIZNAME];
  //DBF File handles used by net_Update() --
  DBF_NO ntv_no;
  DBF_NO nts_no;
  DBF_NO ntp_no;
  DBF_NO ntn_no;
} NET_DATA;

typedef NET_DATA FAR *pNET_DATA;

/*======================================================================*/
/*Globals calculated with each call to net_Adjust() --*/

extern int net_errno,net_errtyp,net_errline;

/*Entry points --*/

int TRXAPI net_Adjust(char FAR *ntvpath,NET_DATA FAR *pData,
                      TRXAPI_CB pCB);
                      
int TRXAPI net_Update(NET_DATA FAR *pData);

LPSTR TRXAPI net_ErrMsg(LPSTR buffer,int code);

#pragma pack()

#ifdef __cplusplus
}
#endif
#endif
