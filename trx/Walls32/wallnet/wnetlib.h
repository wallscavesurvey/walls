/*  WNETLIB.H --
 	Network Adjustment Library (wallnet.dll)

	Copyright (c) 1994,1996,2001,2004 by David McKenzie, Austin Tx
 
	Source files:
	WNETLIB.H    -- This file - included by all modules but WNETLNK.CPP.
	WNETDBF.C	 -- Database structures, open/create, and misc. fcns.
	WNETDFS.C    -- df_search() - finds loop systems via depth-first search.
	WNETNET.C    -- net_analyze() - determines net geometry - calls df_search().
	WNETINV.C    -- sys_invert() - constructs Q - solves for traverses.
	WNETSLV.C    -- net_solve(), sys_solve() - sorts strings and calls sys_invert().
	WNETADJ.C    -- net_Adjust() - calls net_analyze() and net_solve().
	WNETUPD.C    -- net_Update() - calls sys_solve() to update a specific system.
	WNETERR.C    -- net_ErrMsg() - returns error messages to Walls.
	WNETLNK.CPP  -- Fcns called from net_solve() to get link fragments.
*/

#ifdef MAIN
  #define EXTERN
#else
  #define EXTERN extern
#endif

#define _STACKNEAR 0

#ifndef __DBF_FILE_H
#include <dbf_file.h>
#endif
#ifndef __NETLIB_H
#include "netlib.h"
#endif

#define _USE_DFSTACK
#define DF_STACK_INITSIZE 2000
#define DF_STACK_INCSIZE 2000

#define NTV_BUFRECS 20
#define NTV_NUMBUFS 5

#define net_dbstr_wflag (*(WORD *)&net_dbstrp->flag)

/*Declarations for fcns and vars --*/
EXTERN nptr net_pPathName,net_pPathExt;
EXTERN int net_errno,net_errtyp,net_errline;
EXTERN DBF_NO net_dbvec,net_dbstr,net_dbplt,net_dbnet;
EXTERN pltptr net_dbpltp;     /*points to coorinate data in NTP buffer*/
EXTERN netptr net_dbnetp;     /*points to coorinate data in NTN buffer*/
EXTERN strptr net_dbstrp;
EXTERN vecptr net_dbvecp;

EXTERN int net_i;
#define Dim 3

EXTERN double P[Dim];
EXTERN int net_nc;

#define FIX_EXT(ext) strcpy(net_pPathExt,ext)

#ifdef _USE_DFSTACK
typedef struct {
  int n;
  UINT p_adj;
  int minlow;
} df_frame;

EXTERN df_frame *df_stack;
EXTERN int df_ptr;
EXTERN int df_nxtnode;
EXTERN int df_lenstack;
#ifdef _DEBUG
EXTERN int df_ptr_max;
#endif
#endif

//Certain callbacks are not needed in the DLL version for WALLS --
#undef _POSTREADNTV_CB
#undef _POSTCONNECT_CB 
#undef _POSTCREATENTS_CB
#undef _POSTUPDATENTV_CB
#undef _POSTUPDATENTS_CB

/* Define BW_CNT to enable bandwidth frequency counting for largest system.
   This is not used by WALLS! */
#undef BW_CNT

#define CHK(e,typ) net_chkfcn(e,typ,__LINE__)
#define FCHK(e,typ) if(CHK(e,typ)) return net_errno
#define FCHKV(e) FCHK(e,1)
#define FCHKS(e) FCHK(e,2)
#define FCHKP(e) FCHK(e,3);
#define FCHKN(e) FCHK(e,4);
#define FCHKC(e) FCHK(e,5);
#define CHK_TEST(e) CHK(e,0)
#define CHK_ABORT(e) return CHK_TEST(e)
#define GOTO_VEC(rec) FCHKV(dbf_Go(net_dbvec,(DWORD)(rec)))
#define GOTO_STR(rec) FCHKS(dbf_Go(net_dbstr,(DWORD)(rec)))
#define GOTO_PLT(rec) FCHKP(dbf_Go(net_dbplt,(DWORD)(rec)))
#define GOTO_NET(rec) FCHKN(dbf_Go(net_dbnet,(DWORD)(rec)))
#define GOTO_VEC_RTN(rec) CHK(dbf_Go(net_dbvec,(DWORD)(rec)),1)
#define GOTO_STR_RTN(rec) CHK(dbf_Go(net_dbstr,(DWORD)(rec)),2)
#define GOTO_PLT_RTN(rec) CHK(dbf_Go(net_dbplt,(DWORD)(rec)),3)
#define GOTO_NET_RTN(rec) CHK(dbf_Go(net_dbnet,(DWORD)(rec)),4)
#define FIRST_NET_RTN() CHK(dbf_First(net_dbnet),4)
#define NEXT_NET_RTN() CHK(dbf_Next(net_dbnet),4)
#define NET_CB_RTN(n) net_chkfcn(net_pCB(n),NET_ErrCallBack,__LINE__)
#define NET_CB(n) if(NET_CB_RTN(n)) return net_errno;

//Preserve old allocation names for info purposes --
#define allocfar allochuge
#define freefar freehuge
#define CHK_MEM(p,len) if(CHK(allocfar((LPVOID *)&p,len),0)) return NET_ErrFarHeap
#define CHK_MEMH(p,len) if(CHK(allochuge((HPVOID *)&p,len),0)) return NET_ErrFarHeap

typedef double DIM_POINT[Dim];
typedef double DIM_DPOINT[Dim];

#define MAX_SYSTEMS  1000
#define MAX_BW       400

EXTERN int net_numnets,numpltendp;

EXTERN BOOL	vecopen,stropen,pltopen,netopen;

EXTERN DWORD  *sys;   /*System strings and types*/
EXTERN int sysstr[MAX_SYSTEMS+1];   /*indices to sys[] */ 
#define QCOL_ARRAY sysstr

/*Since we reuse sysstr[] for the Q-matrix column far pointers, we
  will define MAX_BW so that MAX_BW+Dim <= (MAX_SYSTEMS+1)/2, assuming
  sizeof(Qptr)==sizeof(int). Also, the excess space in sysstr[],
  if sufficient, will be used to store the actual data pointed to
  for small systems.*/

#ifdef BW_CNT
EXTERN bool net_bwcnt_flag;      /*TRUE if checking enabled*/
#endif

/*Structure and callback function passed to net_Adjust --*/
EXTERN NET_DATA FAR *net_pData;
EXTERN TRXAPI_CB net_pCB;

#ifdef _DEBUG
EXTERN int *pLen_piSYS;
#endif

/*Far arrays used for initial structure analysis (netlink and netldmd) --*/
EXTERN int *net_vendp;   /*vector endpoints read from NTV file */
EXTERN DWORD *net_valist;  /*vector adj list - indices to net_vendp[]*/
EXTERN DWORD *net_string;    /*string buffer - indices to net_vendp[]*/

EXTERN DWORD *net_node;     /*endpoints as indices to net_valist[]*/
EXTERN int *net_degree;   /*# of adjacent nodes*/
#define nodlow net_degree     /*Reuse array for depth-first search*/

/*Globals referenced by netlimd.c and netldmd.c --*/
EXTERN int    net_stindex;

/*Global referenced by netlimd.c and netlsmd.c --*/
EXTERN int    comp_node;

/*Global referenced by netldmd.c --*/
EXTERN int    net_maxstringvecs;
#ifdef _DEBUG
EXTERN int    net_maxnumstr;
#endif

/*Globals calculated with each call to net_Adjust() --*/
EXTERN int    net_maxsysid,net_maxsysnod,net_maxsysstr,net_maxstrsolve;
EXTERN int    net_numsys,net_numstr;
EXTERN int    net_closures,sys_nc,sys_ni;

/*Globals referenced by vaious modules (graph representation) --*/
EXTERN lnptr  jendp;	  /*contains edges as adjacent node indices*/
EXTERN lnptr  jlst;       /*adjacency lists - indices to jendp[]*/
EXTERN lnptr  *jnod;      /*pointers to adjacency list in jlst*/
EXTERN lnptr  jseq;       /*jseq[net_maxsysstr] - indices to jendp[]*/
EXTERN lnptr  jdeg;       /*jdeg[net_maxsysnod] - degree counts*/
EXTERN lnptr  jbord;      /*list of border nodes*/
EXTERN WORD   *jtask;     /*pointer to string action flag array*/
EXTERN int    sys_numinf[2];
EXTERN int    sys_numendp,sys_recfirst,sys_closures;
EXTERN int    sys_numstr,sys_numnod,sys_root,sys_bandw,sys_bandl;
EXTERN int    sys_numzeroz; /*Count of Z<=0.0 situations in netlimd.c*/

#ifdef _DEBUG
/*Pointers used only by netlimd.c --*/
EXTERN lnptr nodcol; /*set to jbord*/
EXTERN lnptr colnod; /*set to jdeg*/
EXTERN lnptr row;	 /*set to jlst*/
#else
#define nodcol jbord
#define colnod jdeg
#define row jlst
#endif

EXTERN double sys_ss[Dim];
EXTERN double Z;
EXTERN double ZZ,M;
EXTERN WORD   Infvar;		  /*Set to InfvarH or InfvarV as appropriate*/


/* The following jtask[] flags specify actions to take when updating
   the network with a vector. They are fixed by sortnet(). */

enum e_task {
              Nod_inet = 1,
              Move1    = 2,
              Move2    = 4,
              Delete1  = 8,
              Delete2  = 16,
              Addnode  = 32
            };

enum e_infvar {
             InfvarH   = (1<<6),
             InfvarV   = (1<<7),
			 F_InfvarH = (1<<14),
			 F_InfvarV = (1<<15)
};
    
/*wnetinv.c routines --*/
apfcn_i sys_invert(void);
apfcn_i str_get_varsum(double *psum);
apfcn_i str_apply_varsum(double varSum);

/*wnetslv.c routines --*/
int FAR PASCAL comp_node_fcn(lnptr p);
apfcn_i net_solve(void);
apfcn_i sys_solve(UINT recNTS);
apfcn_i position_dbnet(UINT recNTS);

/*wnetdfs.c routine --*/
apfcn_i df_search(int n);

/*wnetnet.c routines --*/
apfcn_i net_chkfcn(int e,int typ,int line);
apfcn_v add_system(void);
apfcn_i extend_str(UINT ep);
apfcn_i net_analyze(void);

/*wnetdbf.c routines --*/
apfcn_i open_dbvec(char *ntvpath);
apfcn_i create_dbnet(void);
apfcn_i open_dbnet(void);
apfcn_i create_dbstr(void);
apfcn_i open_dbstr(void);
apfcn_i create_dbplt(void);
apfcn_i open_dbplt(void);
apfcn_v zero_resources(void);
apfcn_i allochuge(HPVOID *p,DWORD len);
apfcn_v freehuge(HPVOID *p);
apfcn_v free_resources(void);
apfcn_i net_chkfcn(int e,int typ,int line);
apfcn_i next_id(int i);

/*wnetlnk.c structures and routines --*/
typedef struct {
	long rec;
	long nxtrec;
} LINK_FRAGMENT;

EXTERN LINK_FRAGMENT *sys_linkfrag;
EXTERN int sys_numlinkfrag;
EXTERN BOOL sys_updating;

int linkFrag_clear();
int linkFrag_get(LINK_FRAGMENT **L);

/*End of wnetlib.h*/
