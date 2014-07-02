// TRXFILE.H -- C++ Wrapper for TRX_FILE.LIB --

#ifndef __TRXFILE_H
#define __TRXFILE_H

#ifndef __TRX_FILE_H
#include <trx_file.h>
#endif

class CTRXFile {
public:

// File oriented user flags - used with Open() and Create() --
   enum {
      ReadWrite=0,Share=1,DenyWrite=2,ReadOnly=4,
      UserFlush=8,AutoFlush=16,ForceOpen=32
   };
      
/*Tree oriented structural and user flags --
  KeyMaxSize through KeyUnique can be set by InitTree()
  as permanent structural flags. KeyUnique through KeyDescend
  can be set as temporary user flags via Set..() functions.*/

enum {KeyMaxSize=2,KeyBinary=4,
      RecMaxSize=8,RecPosition=16,KeyUnique=32,
      KeyExact=64,KeyDescend=128};
      
//Basic error codes common to DBF and TRX file types --
   enum {
      OK,
      ErrName,
      ErrFind,
      ErrArg,
      ErrMem,
      ErrRecsiz,
      ErrNoCache,
      ErrLocked,
      ErrRead,
      ErrWrite,
	  ErrCreate,
      ErrOpen,
      ErrClose,
      ErrDelete,
      ErrLimit,
      ErrShare,
      ErrFormat,
      ErrReadOnly,
      ErrSpace,
      ErrFileType,
      ErrDiskIO,
      ErrLock,
	  ErrFileShared,
	  ErrUsrAbort,
	  ErrRename
     };

//TRX file specific errors --
enum {ErrLevel=TRX_ERR_BASE,
      ErrSizNode,
      ErrNodLimit,
      ErrDup,
      ErrNonEmpty,
      ErrEOF,
      ErrNoInit,
      ErrMatch,
      ErrTreeType,
      ErrDeleted,
      ErrSizKey,
      ErrNotClosed,
      ErrTruncate
     };

// Attributes
protected:
   TRX_NO m_trxno,m_trxbase;
    
// Constructors
public:
   CTRXFile() {m_trxno=m_trxbase=0;};
   virtual ~CTRXFile() {if(m_trxbase) trx_Close(m_trxbase);}
   
// Operations 
   int Open(const char* pszFileName,UINT mode=0) {
   	 //Should chk if already open.
     int e=trx_Open(&m_trxbase,(LPSTR)pszFileName,mode);
     m_trxno=m_trxbase;
     return e;
   }

   int Create(const char* pszFileName,UINT sizRec=0,UINT mode=0,UINT numTrees=1,
	 UINT sizExtra=0,UINT sizNode=1024,UINT sizNodePtr=2)
   {
   	 //Should chk if already open.
     int e=trx_Create(&m_trxbase,(LPSTR)pszFileName,mode,numTrees,sizExtra,sizNode,sizNodePtr);
     if(!e && (e=trx_InitTree(m_trxbase,sizRec,1,KeyMaxSize))) CloseDel();
     m_trxno=m_trxbase;
     return e; 
   }
   
   int CreateInd(LPSTR pathName,UINT mode,LPVOID fileDef)
   {
   	 //Should chk if already open.
     int e=trx_CreateInd(&m_trxbase,pathName,mode,fileDef);
     m_trxno=m_trxbase;
     return e; 
   }
   
   int UseTree(UINT nTree)
   {
   	 //No error checking here --
   	 m_trxno=m_trxbase?(m_trxbase+nTree):0;
   	 return m_trxno?0:ErrOpen;
   }

   int GetTree() {return m_trxno-m_trxbase;}
   
   int InitTree(UINT sizRec,UINT sizKey=TRX_MAX_KEYLEN,UINT initFlags=0)
   {
     // sizKey is normally the maximum key length that will be accepted when indexing.
   	 // The following flag is useful when sizNode<1024 --
   	 //
   	 //	KeyMaxSize	- Allowed key length will be adjusted downward from TRX_MAX_KEYLEN
   	 //				  as necessary to fit sizNode. An adjusted length less than sizKey
   	 //				  causes return code ErrSizNode.
   
     return trx_InitTree(m_trxno,sizRec,sizKey,initFlags);
   }

   BOOL Opened() {return m_trxbase!=0;}
   
   int Close(BOOL bDelCache=TRUE) {
     int e;
     //detaches and conditionally frees --
     if(bDelCache && (e=trx_DeleteCache(m_trxbase))) trx_Close(m_trxbase);
     else e=trx_Close(m_trxbase);
     m_trxbase=m_trxno=0;
     return e;
   }
   
   void CloseDel(BOOL bDelCache=TRUE)
   {
	 if(m_trxbase) {
		 trx_Purge(m_trxbase);
		 if(bDelCache) trx_DeleteCache(m_trxbase);
		 trx_CloseDel(m_trxbase);
		 m_trxbase=m_trxno=0;
	 }
   }

   //If makedef==TRUE, the cache will be made the default for indexes.
   //This cache will be freed when the last file detaches --   
   int AllocCache(UINT bufcnt,BOOL makedef=TRUE) {
         return trx_AllocCache(m_trxbase,bufcnt,makedef);}
         
   int AttachCache(TRX_CSH csh) {return trx_AttachCache(m_trxbase,csh);}
   int DetachCache() {return trx_DetachCache(m_trxbase);}
   //Detaches and conditionally frees cache if no other files are attached --
   int DeleteCache() {return trx_DeleteCache(m_trxbase);}
   int Flush() {return trx_Flush(m_trxbase);}
   void Purge() {trx_Purge(m_trxbase);}
   static TRX_CSH DefaultCache() {return trx_defCache;}

   static void FreeDefaultCache()
   {
	   if(trx_defCache) {
		   csh_Free(trx_defCache);
		   trx_defCache=NULL;
	   }
   }

   int SetMinPreWrite(UINT minPreWrite) { return trx_SetMinPreWrite(m_trxbase,minPreWrite);}

   // Result of last Find() or FindNext() --
   // 0000 - exact match
   // xx00 - prefix match, xx=next key's suffix length
   // 00xx - xx=length of unmatching suffix
   // FFFF - error or EOF
   static DWORD FindResult() {return trx_findResult;}
   
   int Find(LPVOID pKey) {return trx_Find(m_trxno,(LPBYTE)pKey);}
   int FindNext(LPVOID pKey) {return trx_FindNext(m_trxno,(LPBYTE)pKey);}
   int First() {return trx_First(m_trxno);}
   int SetPosition(DWORD position) {return trx_SetPosition(m_trxno,position);}
   int Go(DWORD position) {return trx_SetPosition(m_trxno,position);}
   int Last() {return trx_Last(m_trxno);}
   int Next() {return trx_Next(m_trxno);}
   int Prev() {return trx_Prev(m_trxno);}
   int Skip(long len) {return trx_Skip(m_trxno,len);}

   // Data retrieving functions --
   int Get(LPVOID recBuf,UINT sizRecBuf,LPVOID keyBuf,UINT sizKeyBuf=256)
   {
     return trx_Get(m_trxno,recBuf,sizRecBuf,(LPBYTE)keyBuf,sizKeyBuf);
   }
   
   int Get(LPVOID recBuf,LPVOID keyBuf)
   {
     return trx_Get(m_trxno,recBuf,trx_SizRec(m_trxno),(LPBYTE)keyBuf,256);
   }
   
   int GetKey(LPVOID keyBuf,UINT sizKeyBuf=256) {return trx_GetKey(m_trxno,(LPBYTE)keyBuf,sizKeyBuf);}
   int GetRec(LPVOID pBuf,UINT sizRecBuf=4) {return trx_GetRec(m_trxno,pBuf,sizRecBuf);}

   DWORD GetPosition() {return trx_GetPosition(m_trxno);}
   DWORD Position() {return trx_GetPosition(m_trxno);}
   DWORD RunLength() {return trx_RunLength(m_trxno);}
   
   // Extra data manipulation --
   PVOID ExtraPtr() {return trx_ExtraPtr(m_trxbase);}
   int MarkExtra() {return trx_MarkExtra(m_trxbase);}
   
   // Record/key update functions --
   int DeleteKey() {return trx_DeleteKey(m_trxno);}
   int InsertKey(LPVOID pRec,LPVOID pKey) {return trx_InsertKey(m_trxno,pRec,(LPBYTE)pKey);}
   int InsertKey(LPVOID pKey) {return trx_InsertKey(m_trxno,NULL,(LPBYTE)pKey);}
   int ReplaceKey(LPVOID pRec,LPVOID pKey) {return trx_ReplaceKey(m_trxno,pRec,(LPBYTE)pKey);}
   int PutRec(LPVOID pRec) {return trx_PutRec(m_trxno,pRec);}

   // Adjustment of tree operational modes --
   int SetDescend(BOOL status=TRUE) {return trx_SetDescend(m_trxno,status);}
   int SetExact(BOOL status=TRUE) {return trx_SetExact(m_trxno,status);}
   int SetUnique(BOOL status=TRUE) {return trx_SetUnique(m_trxno,status);}
   int SetKeyBuffer(LPVOID pBuf) {return trx_SetKeyBuffer(m_trxno,pBuf);}
   int FillKeyBuffer(int offset) {return trx_FillKeyBuffer(m_trxno,offset);}
 
   // Return file status or description --*/
   TRX_CSH Cache() {return trx_Cache(m_trxbase);}

   //int	   Errno() {return trx_Errstr(trx_Errno(m_trxbase));}
   static  int     Errno() {return trx_errno;}
   static  LPSTR   Errstr(UINT e) {return trx_Errstr(e);}
   static  LPSTR   Errstr() {return trx_Errstr(trx_errno);}

   LPVOID  FileDef() {return trx_FileDef(m_trxbase);}
   UINT    FileMode() {return trx_FileMode(m_trxbase);}
   NPSTR   FileName() {return trx_FileName(m_trxbase);}
   DWORD   NumFreeNodes() {return trx_NumFreeNodes(m_trxbase);}
   DWORD   NumFileNodes() {return trx_NumFileNodes(m_trxbase);}
   DWORD   SizFile() {return trx_SizFile(m_trxbase);}
   UINT    NumTrees() {return trx_NumTrees(m_trxbase);}
   UINT    NumUsers() {return trx_NumUsers(m_trxbase);}
   NPSTR   PathName() {return trx_PathName(m_trxbase);}
   UINT    SizExtra() {return trx_SizExtra(m_trxbase);}
   UINT    SizNode()  {return trx_SizNode(m_trxbase);}
   UINT    SizNodePtr() {return trx_SizNodePtr(m_trxbase);}
   UINT    MinPreWrite()  {return trx_MinPreWrite(m_trxbase);}
   UINT    NumPreWrite()  {return trx_NumPreWrite(m_trxbase);}
   
   // Return tree status or description --
   DWORD	NumTreeNodes() {return trx_NumTreeNodes(m_trxno);}
   DWORD	NumKeys() {return trx_NumKeys(m_trxno);}
   DWORD    NumKeys(UINT nTree) {return m_trxbase?trx_NumKeys(m_trxbase+nTree):0;}
   DWORD	NumRecs() {return trx_NumRecs(m_trxno);}
   UINT  	SizRec() {return trx_SizRec(m_trxno);}
   UINT 	SizKey() {return trx_SizRec(m_trxno);}
   UINT 	InitTreeFlags() {return trx_InitTreeFlags(m_trxno);}
   UINT 	UserTreeFlags() {return trx_UserTreeFlags(m_trxno);}
   UINT 	UserFileFlags() {return trx_UserFileFlags(m_trxno);}
};
#endif
