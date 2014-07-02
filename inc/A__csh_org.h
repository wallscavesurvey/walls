/* A__CSH.H -- Low-level Cache Declarations for TRX_FILE.LIB*/

#ifndef __A__CSH_H
#define __A__CSH_H

#ifndef __TRX_CSH_H
  #include <trx_csh.h>
#endif

#ifndef __DOS_IO_H
  #include <dos_io.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*Buffer status flags --*/
enum {CSH_Free,CSH_InUse=1,CSH_Updated=2,CSH_New=4};

typedef LPBYTE CSH_BUFP;
typedef struct sCSH_FILE CSH_FILE;
typedef struct sCSH_CACHE CSH_CACHE;
typedef struct sCSH_HDR CSH_HDR;
typedef CSH_FILE *CSF_NO;
typedef CSH_CACHE *CSH_NO;

typedef CSH_HDR *CSH_HDRP;

#define apfcn_hdrp CSH_HDRP PASCAL

struct sCSH_HDR { /*64 bytes*/
    CSH_BUFP    Buffer;            /*32-bit buffer address*/
    CSH_NO      Cache;             /*CSH_CACHE handle that manages buffer*/
    CSF_NO      File;              /*CSH_FILE handle which owns buffer*/
    DWORD       Status;            /*Status flags*/
    DWORD       NumLocks;          /*Locked in cache if >0*/
    DWORD       Recno;             /*Corresponds to a physical file offset*/
    CSH_HDRP    PrevMru;           /*Links all buffers within CSH_CACHE:*/
    CSH_HDRP    NextMru;           /*Mru->NextMru is the LRU buffer*/
    CSH_HDRP    PrevHash;          /*Other headers owned by File/Records*/
    CSH_HDRP    NextHash;          /*mapping to same hash table entry*/
    CSH_HDRP    HashIndex;         /*Header of record's hash table entry*/
                                   /*if PrevHash==0*/
    CSH_HDRP    HashTable;         /*Independent hash table entry comprising*/
                                   /*a separate array*/
    DWORD       Reserved[4];       /*Fill to 64-byte length*/
};

struct sCSH_CACHE {        /*Size: 24+64*/
    DWORD     Bufsiz;      /*Buffer size in bytes*/
    DWORD     Bufcnt;      /*Initial number of buffers allocated*/
    CSH_HDRP  Mru;         /*Header addr of MRU buffer*/
    CSF_NO    FstFile;     /*Base of client file chain*/
    DWORD     Maxcnt;	   /*Maximum number of buffers to allocate*/
    DWORD     HitCount;    /*For determining cache hit rate only*/
    CSH_HDR   HdrOff[1];   /*First element of header array*/
};

struct sCSH_FILE {          /*Size: 36*/
    CSH_NO Csh;             /*Cache manager for this file*/
    CSF_NO NextFile;        /*Link to next file using cache manager*/
    UINT Recsiz;            /*Record size in bytes.*/
    UINT HashBase;          /*Index in Csh->HdrOff[] of 1st HashTable entry*/
    DWORD Offset;           /*File offset of record 0 (also header size)*/
    int Handle;             /*Handle returned by dos_OpenFile(), etc.*/
    int Errno;              /*Error code of last file-related system error
                              (or detection of corruption)*/
    DWORD LastRecno;        /*Last file record with LastDataLen bytes*/
    UINT LastDataLen;       /*If nonzero,length of data in LastRecno*/
};

extern BOOL _csh_newRead; /*Set by csh_GetRecord(): TRUE if disk was read*/
extern int  csh_errno;    /*Cache-only error code. Defined in cshalloc.c*/

/* The following functions are in code segment _TRX_TEXT and are
   available only as NEAR calls from TRX_FILE library routines --*/

apfcn_vp    _csh_Buffer(CSH_HDRP hp);
apfcn_i     _csh_FlushHdr(CSH_HDRP hp);
apfcn_v     _csh_LockHdr(CSH_HDRP hp);
apfcn_v     _csh_MarkHdr(CSH_HDRP hp);
apfcn_v     _csh_PurgeHdr(CSH_HDRP hp);
apfcn_v     _csh_UnLockHdr(CSH_HDRP hp);
apfcn_i     _csh_XfrRecord(CSH_HDRP hp,int typ);

apfcn_i     _csf_AttachCache(CSF_NO csf,CSH_NO csh);
apfcn_i     _csf_DetachCache(CSF_NO csf);
apfcn_i     _csf_DeleteCache(CSF_NO csf,CSH_NO *pcsh);
apfcn_hdrp  _csf_GetRecord(CSF_NO csf,DWORD recno,int status);
apfcn_i     _csf_Flush(CSF_NO csf);
apfcn_v     _csf_Purge(CSF_NO csf);
#define	    _csf_Errno(csf) (csf->Errno)
#define	    _csf_LastErrstr(csf) csh_Errstr(csf->Errno)

/*Helper defines for cshxxx.c functions --*/
#define _Recno(hp) (hp->Recno)

#define _CSH ((CSH_NO)csh)
#define _LOAD_HDRSEG() (csh!=0)
#define _ADV_HP(hp) (hp+1)
#define _ADV_HP_N(hp,n) (hp+n)

#ifdef __cplusplus
}
#endif
#endif
/*End of A__CSH.H*/







