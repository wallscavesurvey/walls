/* A__TRX.H -- Low-level Index File Declarations */

/*CAUTION: Changes to this file must be carried over to file A_TRX.ASI!*/

#ifndef __A__TRX_H
#define __A__TRX_H

#ifndef __TRX_FILE_H
#include <trx_file.h>
#endif

#ifndef __A__CSH_H
#include <a__csh.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*Tree integrity checks (remove when library is fully tested) --*/
#undef _TRX_TEST

#ifdef _TRX_TEST
 #define _SCN_TEST
 #ifdef _SCN_TEST
   extern UINT _trx_oldSpace,_trx_newSpace;
 #endif
#endif

extern UINT _trx_btsfx;
extern UINT _trx_btinc;

#define EXE_FILE_ID 0x5A4D  /*First word of an EXE file*/
#define TRX_CLOSED_FILE_ID 0x5641
#define TRX_OPENED_FILE_ID 0x5640

#define KEYPOS_MASK 0x7FFFFFFF
#define KEYPOS_HIBIT 0x80000000

/*Three basic node types requiring different low-level access methods
  are believed sufficient for most uses (the first two are implemented):

  1) Variable-length (>=0,<256) keys with fixed link size (>=0)
  2) Fixed-length (>0) records only, no keys (and no branch nodes)
  3) Fixed-length (>0) keys with fixed link size (>=0)
*/

typedef DWORD HNode;

#pragma pack(1)

typedef struct { /*20 byte node header --*/
 short  N_SizLink;  /*Link size+key size, or -(link size) if type 1*/
 WORD   N_NumKeys;  /*Number of records (or link/keys) stored in node*/
 WORD   N_FstLink;  /*Node offset of first record - fixed if type 2*/
 BYTE   N_Flags;    /*Bits 0-4:Branch level(0 if leaf) 6-7: DupCut flags*/
 BYTE   N_Tree;     /*Tree number (0..)*/
 HNode  N_This;     /*Node number of this node (1..)*/
 HNode  N_Rlink;    /*Node number of higher valued records, or 0*/
 HNode  N_Llink;    /*Node number of lower valued records, or 0*/
 BYTE   N_Buffer[0]; /*Records are right justified in node*/
} TRX_NODE;

/*;
; Subkey format    Bytes 0..n-1 Link - can have zero length if leaf.
;                  Byte  n      Length of string's non-matching suffix.
;                  Byte  n+1    Length of string's matching prefix.
;                  Bytes n+2..  String's non-matching suffix.
*/

#define N_SIZLINK     0
#define N_NUMKEYS     2
#define N_FSTLINK     4
#define N_FLAGS       6
#define N_TREE        7
#define N_THIS        8
#define N_RLINK       12
#define N_LLINK       16
#define N_BUFFER      20

/*N_Flags bit assignments - Limit MAX_BRANCH_LEVELS in A_TRX.H to 31.
  Bits 0-4 - Branch level (0 for leaf)
  Bit  5   - Indicates parent link is to potential leaf to right.
  Bits 6,7 - Duplicate key set broken at left or right, respectively
*/
enum {TRX_N_Branch=31,
      TRX_N_DupCutRight=32,
      TRX_N_DupCutLeft=64,
      TRX_N_EmptyRight=128};

typedef TRX_NODE FAR *TRX_pNODE;

typedef struct {      /*Struct passed to trx_CreateInd() (8 bytes)*/
    WORD  NumTrees;   /*Number of subtrees supported (1..255)*/
    WORD  SizExtra;   /*Total size of subtree extra data*/
    WORD  SizNode;    /*Node size in bytes*/
    WORD  SizNodePtr; /*Link size in branch nodes (2..4)*/
} TRX_FILEDEF;

typedef struct {
    WORD Id;		  /*Used in trx_Open()*/
    TRX_FILEDEF Fd;
} TRX_FILEHDR;

typedef struct {      /*Dynamic tree data stored in file (28 bytes)*/
    DWORD NumKeys;    /*Total # keys in leaves*/
    HNode Root;       /*Root node number (1..), 0 if tree empty*/
    HNode NumNodes;   /*Total nodes in tree*/
    HNode LowLeaf;    /*Leftmost (low) leaf node*/
    HNode HighLeaf;   /*Rightmost (high) leaf node*/
    WORD  SizRec;     /*Record size in leaves - SizRec=SizKey=0 if no init*/
    BYTE  SizKey;     /*Maximum allowed key length or fixed size*/
    BYTE  InitFlags;  /*trx_InitFlags() set by trx_InitTree()*/
    WORD  ExtraOff;   /*Extra data at (nptr)pTreeUsr-ExtraOff*/
    WORD  ExtraLen;   /*For convenience in manipulating extra data (?)*/
} TRX_TREEVAR;

#define TRX_TREEVAR_SIZ 28

typedef struct {      /*User's tree status maintained after open (8 bytes)*/
  HNode NodePos;      /*Subtree current node*/
  UINT  KeyPos;       /*Subtree current key position*/
  UINT  UsrFlags;     /*trx_UsrFlags() - operational mode flags*/
} TRX_TREEUSR;

#define TRX_TREEUSR_SIZ 12

typedef TRX_FILEDEF *TRX_pFILEDEF;
typedef TRX_TREEVAR *TRX_pTREEVAR;
typedef TRX_TREEUSR *TRX_pTREEUSR;

struct sTRX_FILE;
typedef struct sTRX_FILE *TRX_pFILE;
struct sTRX_FILEUSR;
typedef struct sTRX_FILEUSR *TRX_pFILEUSR;

typedef struct sTRX_FILEUSR {
  TRX_pFILE pUsrFile;        /*Pointer to TRX_FILE structure*/
  UINT UsrMode;              /*File oriented user flags (share, flush, etc)*/
  UINT UsrIndex;             /*_trx_usrMap[UsrIndex] points to this structure*/
  BYTE *UsrKeyBuffer;		 /*Points to user's key buffer if set*/
  TRX_pFILEUSR pNextFileUsr; /*Points to next file user in chain*/
  int  Errno;                /*Error code of last operation by this user*/
  TRX_TREEUSR TreeUsr[0];    /*Array of tree-oriented user data*/
} TRX_FILEUSR;

typedef struct sTRX_FILE {
  CSH_FILE Cf;               /*36-byte struct defined in A__CSH.H*/
  UINT HdrChg;               /*Nonzero if header differs from file version*/
  UINT MinPreWrite;          /*Nodes to pre-allocate to file when extending*/
  TRX_pFILEUSR pFileUsr;     /*Pointer to linked list of TRX_FILEUSR structs*/

  /*TRX file header - Length depends on NumTrees and SizExtra --*/
  WORD FileID;               /*Has value TRX_FILEID on disk when current*/
  TRX_FILEDEF Fd;            /*Fixed part specified when created - 8 bytes*/
  WORD NumPreWrite;          /*Number of unused pre-allocated nodes*/
  HNode NumNodes;            /*Used node count (last assigned number)*/
  DWORD NumFreeNodes;        /*Number of previously freed nodes*/
  HNode FirstFreeNode;       /*Node number of first free node*/
  TRX_TREEVAR TreeVar[0];     /*Array of subtree dynamic variables*/
  /*<Subtree extra data>*/   /*Array of SizExtra bytes*/
} TRX_FILE;

// TRX_FILE Structure Offsets --
// CSH_FILE component --
#define IXO_Csh            0      /*Cache manager handle*/
#define IXO_NextFile       2
#define IXO_RecSiz         4
#define IXO_HashBase       6
#define IXO_Offset         8
#define IXO_Handle         12
#define IXO_Errno          14
#define IXO_LastRecno      16
#define IXO_LastDataLen    18

#define IXO_HdrChg         22     /*Nonzero if header differs from file version*/
#define IXO_MinPreWrite    24     /*Minimal nodes to extend file when writing*/
#define IXO_pFileUsr       26     /*Pointer to first TRX_FILEUSR structure*/

//Physical file header --
#define IXO_FileID         28     /*TRX_OPENED_FILE_ID or TRX_CLOSED_FILE_ID*/

//TRX_FILEDEF - Structure parameters fixed at creation --
#define IXO_NumTrees       30     /*Total number of subtrees supported (1..254)*/
#define IXO_SizExtra       32     /*Total size of subtree extra data*/
#define IXO_SizNode        34     /*Node size in bytes*/
#define IXO_SizNodePtr     36     /*Link size in branch nodes (2..4)*/

//Variables revised during file use --
#define IXO_NumPreWrite    38
#define IXO_NumNodes       40     /*Total number of nodes (last assigned number)*/
#define IXO_NumFreeNodes   44     /*Total number of previously freed nodes*/
#define IXO_FirstFreeNode  48     /*Node number of first free node (0 if none)*/
#define IXO_TreeVar        52     /*Start of TRX_TREEVAR structure array*/

#define _Node(hp) ((TRX_pNODE)hp->Buffer)

#define _GETTRXP (trx=(TRX_NO)_trx_GetTrx(trx))
#define _GETTRXT (_trx_GetTrx(trx)?(trx=(TRX_NO)_trx_pTreeVar):0)
#define _GETTRXU (_trx_GetTrx(trx)?(trx=(TRX_NO)_trx_pTreeUsr):0)
#define _GETTRXM (_trx_GetTrx(trx)?(trx=(TRX_NO)_trx_pFileUsr):0)

#define _TRXP ((TRX_pFILE)trx)
#define _TRXT ((TRX_pTREEVAR)trx)
#define _TRXM ((TRX_pFILEUSR)trx)
#define _TRXU ((TRX_pTREEUSR)trx)

/*Total size of TRX file physical header:
(ID:2)+(FILEDEF:8)+(14)+(TREEVAR:28*trees)+(extra) = 52 bytes minimum*/

#define TRX_HDR_SIZ(nt,ex) (24+nt*sizeof(TRX_TREEVAR)+ex)

/*Total size of TRX_FILE after expansion:
  ((22)+(6))+((2)+(8)+(14))+(28*trees)+(extra) =
  (28 + (24+28*trees+extra)) = 80 bytes minimum*/

#define TRX_FILE_SIZ(nt,ex) (sizeof(TRX_FILE)+nt*sizeof(TRX_TREEVAR)+ex)

/*Variables defined in trxalloc.c --*/

extern UINT _trx_max_users;  /*== TRX_MAX_USERS*/ 
extern UINT _trx_max_files;  /*== TRX_MAX_FILES*/ 

/*Noncompacted array. One entry per file/user pair.
  Index forms high word of TRX_NO --*/
extern TRX_pFILEUSR _trx_usrMap[TRX_MAX_USERS];
#define GET_TRX_NO(idx) (TRX_NO)(((idx+1)<<16)+((_trx_usrMap[idx]->UsrMode&TRX_Share)<<8))

/*Compacted array. One entry per physical file --*/
extern TRX_pFILE _trx_fileMap[TRX_MAX_FILES];

/*Initialized by _trx_GetTrx() --*/
extern UINT         _trx_tree;  /*Tree number: 0..NumTrees-1*/
extern TRX_pTREEVAR _trx_pTreeVar;
extern TRX_pTREEUSR _trx_pTreeUsr;
extern TRX_pFILEUSR _trx_pFileUsr;
extern TRX_pFILE    _trx_pFile;

/*Initialized with return value of _trx_GetNode() --*/
extern CSH_HDRP     _trx_hdrp;
extern BOOL _trx_noErrNodRec; /*Determines if TRX_N_This is checked*/

/*Global line indicator for TRX file consistency tests --*/
#ifdef _TRX_TEST
extern int _trx_errLine;
#define FORMAT_ERROR() _trx_FormatErrorLine(__LINE__)
#define _AVOID(n) if(n) return FORMAT_ERROR()
#else
#define FORMAT_ERROR() _trx_FormatError()
#define _AVOID(n) ((void)0)
#endif

/* A_TRX.LIB Internal Routines --*/
apfcn_i     _trx_ErrFlush(void);
apfcn_i     _trx_ErrFlushHdr(void);
apfcn_i     _trx_ErrReadOnly(void);
apfcn_i     _trx_Flush(TRX_pFILE trxp);
apfcn_i     _trx_FlushHdr(TRX_pFILE trxp);
apfcn_i     _trx_FormatError(void);
apfcn_i     _trx_FormatErrorLine(int line);
TRX_pFILE NEAR PASCAL _trx_GetTrx(TRX_NO trx);
apfcn_hdrp  _trx_GetNode(HNode nodeno);
apfcn_hdrp  _trx_GetNodePos(TRX_NO trx,int offset);
apfcn_i     _trx_IncPreWrite(UINT numnodes);
apfcn_hdrp  _trx_NewNode(int level);
apfcn_i     _trx_TransferHdr(TRX_pFILE trxp,UINT typ);
apfcn_v     _trx_FreeFile(TRX_pFILE trxp);
apfcn_i     _trx_AllocFile(TRX_FILEDEF FAR *pFd);
apfcn_i     _trx_AllocUsr(TRX_NO FAR *ptrx,TRX_pFILE trxp,UINT mode);
apfcn_v     _trx_FreeUsr(TRX_pFILEUSR usrp);
apfcn_i     _trx_FindFile(NPSTR name);
#define     _trx_ErrRange(e,low,high) ((e)<(low) || (e)>(high))
apfcn_i     _trx_SetUsrFlag(TRX_NO trx,UINT flag,BOOL status);

extern DWORD  _trx_btcnt;
extern LPVOID _trx_pFindRec;

typedef struct {
    UINT CntBound;    /*Count position of upperbound rec/key for new node*/
    UINT SizDupLead;  /*Length of duplicate chain prefixing the upperbound:
                        0 if key not duplicated, else all but last dup*/
    UINT SizDupLast;  /*Length of portion to be stored at level 0 only:
                        last dup if dups attached, else whole rec/key*/
    int SpcRemain;    /*Length of header+space remaining in old node*/
    UINT SizBoundKey; /*Length of upper bound key*/
    UINT ScnOffset;   /*Offset of next link/key in old node*/
} typ_scnData;

extern typ_scnData _trx_btscnData; /*in bt_stoid.c*/

#ifndef _NO_TRXD
typedef struct {
    int     pid;
    int     fcn;
    int     idx;
    int     iret;  
    int     rec;
    BYTE  key[256];
} io_rectyp;

/*declared in trxalloc.c, set non-zero by trxd daemon --*/
extern int *io_pidlst;
extern io_rectyp *io_prec;

typedef struct {
	long mtype;
	io_rectyp io_rec;
} io_buftyp;
#endif

void		bt_srch(void);
void		bt_link(void);

apfcn_vp    _trx_Btrecp(CSH_HDRP hp,UINT keyPos);
apfcn_p     _trx_Btkeyp(CSH_HDRP hp,UINT keyPos);
apfcn_i     _trx_Btrec(CSH_HDRP hp,UINT keyPos,PVOID recbuf,UINT sizRecBuf);
apfcn_i     _trx_Btkey(CSH_HDRP hp,UINT keyPos,PBYTE key,UINT sizKeyBuf);
apfcn_i     _trx_Btfnd(CSH_HDRP hp,PBYTE key);
apfcn_i     _trx_Btfndn(CSH_HDRP hp,UINT keyPos,PBYTE key);
apfcn_ul    _trx_Btlnk(CSH_HDRP hp,PBYTE key);
apfcn_ul    _trx_Btlnkc(CSH_HDRP hp,UINT keyPos);
apfcn_v     _trx_Btlop(CSH_HDRP org_node,PBYTE bound_key);
apfcn_i     _trx_Btscn(void);
apfcn_v     _trx_Btsetc(CSH_HDRP hp,UINT keyPos,PVOID link);
apfcn_i     _trx_Btsto(CSH_HDRP hp,PBYTE key,PVOID link);
apfcn_i     _trx_Btcut(CSH_HDRP hp,PBYTE key,UINT breakpoint);
apfcn_v     _trx_Btdel(CSH_HDRP hp,UINT keyPos);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif
/*End of A__TRX.H*/
