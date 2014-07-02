/* A__DBF.H -- Low-level DBF File Declarations */

#ifndef __A__DBF_H
#define __A__DBF_H

#ifndef __A__CSH_H
#include <a__csh.h>
#endif

#ifndef __DBF_FILE_H
#include <dbf_file.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*Define when library is still being tested --*/
#undef _DBF_TEST
#undef _DBF2

#ifdef _DBF2
#define DBF2_SIZHDR		521
#endif

#define DBF_FILEID_MASK 0x77
#define DBF_EOFMARK     0x1A

/*Standard 32-byte DBF file header --*/

#pragma pack(1)
typedef struct DBF_sHDR {
  /*Updateable portion (16-bytes) --*/

  BYTE  FileID;           /*03h if dBase-III, 83h or 8Bh if .DBT exists*/
  DBF_HDRDATE Date;       /*Date of last close of updated file*/
  DWORD NumRecs;          /*Total number of records in file*/
  WORD  SizHdr;           /*File offset of first record*/
  WORD  SizRec;           /*Logical record size*/
  WORD  Res1;             /*Reserved*/
  BYTE  Incomplete;       /*1 if disk version is not complete*/
  BYTE  Encrypted;        /*1 if encrypted (not used here)*/

  /*The remaining 16-byte portion is currently unused but will
    be available for application to examine and/or modify --*/

  BYTE  Res2[12];         /*Reserved for LAN use*/
  BYTE  Mdx;              /*1 if production .MDX*/
  BYTE  Res3[3];          /*Reserved*/
} DBF_HDR;

#ifdef _DBF2
typedef struct {
  BYTE  FileID;		/*Has value 2 or 82h if valid DBF2 file, 82h if incomplete*/
  WORD  wNumRecs;	/*Current # of records*/
  DBF_HDRDATE Date; /*Date of last close of updated file*/
  WORD SizRec;		/*Logical record length*/
} DBF2_HDR;
#endif

/*32-byte field descriptor stored on disk (standard for dBase-III) --*/
typedef struct {
  char F_Nam[11];           /*Field name, capitalized, 0-padded*/
  BYTE F_Typ;               /*Field type, C, D, F, L, M or N*/
  BYTE F_Res1[4];
  BYTE F_Len;               /*Field length*/
  BYTE F_Dec;               /*Decimal places*/
  BYTE F_Res2[13];
  BYTE F_Mdx;               /*1 if tag field in production .MDX*/
} DBF_FILEFLD;

#ifdef _DBF2
typedef struct {			/*16 BYTES -- */
  char F_Nam[11];                       /*Field name*/
  char F_Typ;				/*"C", "N", or "L" data type*/
  BYTE F_Len;				/*Field length*/
  WORD F_Off;				/*Offset from 1st field*/
  BYTE F_Dec;				/*Decimals if "N" type*/
} DBF2_FILEFLD;
#endif

struct sDBF_FILE;
typedef struct sDBF_FILE NEAR *DBF_pFILE;
struct sDBF_FILEUSR;
typedef struct sDBF_FILEUSR NEAR *DBF_pFILEUSR;

/*Near data structure allocated for each file user --*/

typedef struct sDBF_FILEUSR { /*32+1 bytes*/
  DBF_pFILE U_pFile;        /*Pointer to DBF_FILE structure*/
  DWORD  U_Mode;            /*File oriented user flags (share, flush, etc)*/
  DWORD  U_Index;           /*_dbf_usrMap[UsrIndex] points to this structure*/
  DBF_pFILEUSR U_pNext;     /*Points to next file user in chain*/
  DWORD  U_Recno;           /*Logical record number, or 0 when not positioned*/
  DWORD  U_Recoff;          /*File offset of current record*/
  WORD   U_RecChg;          /*TRUE iff U_Rec different from file version*/
  WORD   U_Lockon;          /*Odd if using record locking, >=2 if hdr locked*/
  UINT   U_Errno;           /*Error code of last operation by this user*/
  BYTE	 U_Rec[1];          /*Buffer for current logical record*/
} DBF_FILEUSR;

typedef struct sDBF_FILE {  /*36 + 6 + 8 + 32 = 82 bytes*/
  CSH_FILE Cf;              /*36-byte struct defined in A__CSH.H*/
  BYTE HdrChg;              /*Odd if header differs from file version, >=0 if header locked*/
  BYTE BufWritten;          /*Nonzero if a cached block was prewritten*/
  BYTE FileChg;             /*Nonzero if file changed since first open*/
  BYTE FileSizChg;          /*Nonzero if NumRecs changed since first open*/
  WORD NumFlds;             /*Number of fields (at least 1)*/
  DBF_pFLDDEF pFldDef;      /*Near ptr initialized only when required*/
  DBF_pFILEUSR pFileUsr;    /*Linked list of DBF_FILEUSR structs*/
  DBF_HDR H;                /*32-byte physical file header*/
} DBF_FILE;

#pragma pack()

#define _GETDBFP (dbf=(DBF_NO)_dbf_GetDbfP(dbf))
#define _GETDBFU (dbf=(DBF_NO)_dbf_GetDbfU(dbf))

#define _DBFP ((DBF_pFILE)dbf)
#define _DBFU ((DBF_pFILEUSR)dbf)

#define _PDBFU _dbf_pFileUsr
#define _PDBFF _dbf_pFile

/*Variables defined in dbfalloc.c --*/

/*Noncompacted array. One entry per file/user pair.
  Index forms high byte of DBF_NO --*/
extern DBF_pFILEUSR _dbf_usrMap[DBF_MAX_USERS];

/*Compacted array. One entry per physical file --*/
extern DBF_pFILE _dbf_fileMap[DBF_MAX_FILES];

/*Initialized by _dbf_GetDbf() --*/
extern DBF_pFILEUSR _dbf_pFileUsr;
extern DBF_pFILE    _dbf_pFile;

/*Initialized with return value of _dbf_GetNode() --*/
extern CSH_HDRP     _dbf_hdrp;

/*Global line indicator for DBF file consistency tests --*/
#ifdef _DBF_TEST
extern int _dbf_errLine;
#define FORMAT_ERROR() _dbf_FormatErrorLine(__LINE__)
#define _AVOID(n) if(n) return FORMAT_ERROR()
#else
#define FORMAT_ERROR() _dbf_FormatError()
#define _AVOID(n) ((void)0)
#endif

/* DBF_LIB.LIB Internal Routines --*/
apfcn_i     _dbf_AllocFile(void);
apfcn_i     _dbf_AllocUsr(DBF_NO far *pdbf,DBF_pFILE dbfp,UINT mode);
apfcn_i     _dbf_CommitUsrRec(void);
apfcn_i     _dbf_ErrReadOnly(void);
apfcn_i     _dbf_FindFile(NPSTR name);
apfcn_i     _dbf_Flush(void);
apfcn_i     _dbf_FlushHdr(void);
apfcn_i     _dbf_FormatError(void);
apfcn_i     _dbf_FormatErrorLine(int line);
apfcn_v     _dbf_FreeFile(DBF_pFILE dbfp);
apfcn_v     _dbf_FreeUsr(DBF_pFILEUSR usrp);
DBF_pFILE    PASCAL _dbf_GetDbfP(DBF_NO dbf);
DBF_pFILEUSR PASCAL _dbf_GetDbfU(DBF_NO dbf);
DBF_pFLDDEF  PASCAL _dbf_GetFldDef(UINT nFld);
apfcn_i     _dbf_LoadUsrRec(void);
apfcn_i		_dbf_LockFilRec(DBF_pFILE dbfp,int typ);
apfcn_i		_dbf_LockHdr(DBF_pFILE dbfp,int typ);
apfcn_i		_dbf_RefreshHdr(DBF_pFILE pdbf,int typlock);
apfcn_i		_dbf_RefreshNumRecs(void);
apfcn_i     _dbf_WriteHdr(DBF_pFILE dbfp);

#ifdef __cplusplus
}
#endif

#endif
/*End of A__DBF.H*/
