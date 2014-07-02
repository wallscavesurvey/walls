/*TRXPACK.C -- Create a packed TRX file from an unpacked TRX file*/

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <a__trx.h>
#include <trx_str.h>

#define SIZ_KEYBUF 256

static TRX_NO trxold,trx;
static TRXFCN_IF _pckmsg;
static char	trx_pck_messagebuf[64];    

/*Globals that can be accessed by application --*/

BOOL	trx_pck_nodescend;
DWORD	trx_pck_siznode;
DWORD	trx_pck_oldnodes;
DWORD	trx_pck_newnodes;
DWORD 	trx_pck_keycount;			/*Number of keys indexed at this stage*/
DWORD	trx_pck_treecount;			/*Current tree number*/
DWORD	trx_pck_keydelta=500;		/*PCK_KEYCOUNT callback interval - no messages if 0*/

char *  trx_pck_message=trx_pck_messagebuf;

static int _setmsg(int typ)
{
	if(trx_pck_message==trx_pck_messagebuf) {
		if(!_pckmsg && typ!=PCK_FILETOTALS) return 0;
		switch(typ) {
			case PCK_TREESTART:
				sprintf(trx_pck_messagebuf,"Tree%3u - Keys: %7u\b\b\b\b\b\b\b",
					trx_pck_treecount,trx_pck_keycount);
				break;

			case PCK_KEYCOUNT:
				sprintf(trx_pck_messagebuf,"%7u\b\b\b\b\b\b\b",trx_pck_keycount);
				break;

			case PCK_TREETOTALS : sprintf(trx_pck_messagebuf,
				"%7u - Old size: %u x %u  New size: %u x %u",trx_pck_keycount,
				 (trx_pck_oldnodes=trx_NumTreeNodes(trxold)),trx_pck_siznode,
				 (trx_pck_newnodes=trx_NumTreeNodes(trx)),trx_pck_siznode);
				break;

			case PCK_FILETOTALS: sprintf(trx_pck_messagebuf,
				"Old file size: %u x %u  New file size: %u x %u",
				 (trx_pck_oldnodes=trx_NumFileNodes(trxold)),trx_pck_siznode,
				 (trx_pck_newnodes=trx_NumFileNodes(trx)),trx_pck_siznode);
				break;
		}
	}

	return _pckmsg?_pckmsg(typ):0;
}

static int CopyTree(void)
{
  int e;
  UINT sizrec;
  PBYTE recptr=NULL;
  byte keybuf[SIZ_KEYBUF];

  if((e=trx_SizKey(trxold))==0) return 0;

  trx_pck_keycount=0;
  if((e=_setmsg(PCK_TREESTART))) return e;

  if((sizrec=trx_SizRec(trxold)) && (recptr=malloc(sizrec))==0) return TRX_ErrMem;

  if((e=trx_InitTree(trx,sizrec,e,trx_InitTreeFlags(trxold)))) {
	  free(recptr);
	  return e;
  }

  if(!trx_pck_nodescend) trx_SetDescend(trx,TRUE);

  e=trx_Last(trxold);

  while(!e) {
    if(!(e=trx_Get(trxold,recptr,sizrec,keybuf,SIZ_KEYBUF)) &&
		!(e=trx_InsertKey(trx,recptr,keybuf))) {
		++trx_pck_keycount;
		if(!trx_pck_keydelta || (trx_pck_keycount%trx_pck_keydelta)!=0 ||
			(e=_setmsg(PCK_KEYCOUNT))==0) e=trx_Prev(trxold);
    }
  }
  if(e==TRX_ErrEOF) e=0;
  if(!e) _setmsg(PCK_TREETOTALS);
  free(recptr);
  return e;
}

/*==============================================================*/
int TRXAPI trx_Pack(TRX_NO *ptrx,TRXFCN_IF pckmsg)
{
  /*trx is the file/tree identifier of an open TRX file wit an attached cache*/

  int i,e;
  UINT nTree;
  TRX_CSH usrCache,defCache;
  char tmpnam[MAX_PATH];
  char oldnam[MAX_PATH];

  trxold=*ptrx;

  if(!_trx_GetTrx(trxold)) return trx_errno;
  nTree=_trx_tree;
  trxold&=~0xFF;

  if(_trx_pFileUsr->UsrMode&TRX_Share) return trx_errno=TRX_ErrFileShared;
  if(_trx_pFileUsr->UsrMode&TRX_ReadOnly) return trx_errno=TRX_ErrReadOnly;
  if(!(usrCache=_trx_pFile->Cf.Csh)) return trx_errno=TRX_ErrNoCache;

  _pckmsg=pckmsg;

  strcpy(oldnam,trx_PathName(trxold));
  if(!*oldnam) return trx_errno;
  strcpy(tmpnam,oldnam);
  strcpy(trx_Stpext(tmpnam),".P$$");

  if((e=trx_CreateInd(&trx,tmpnam,TRX_ReadWrite,trx_FileDef(trxold)))) return e;
   
  memcpy(trx_ExtraPtr(trx),trx_ExtraPtr(trxold),trx_SizExtra(trx));
  trx_MarkExtra(trx);
  
  trx_SetMinPreWrite(trx,0);
  trx_pck_siznode=trx_SizNode(trx);

  trx_pck_treecount=1;
  if((e=CopyTree())==0) {
	  for(i=trx_NumTrees(trx);i>1;i--) {
		trx++;
		trxold++;
		trx_pck_treecount++;
		if((e=CopyTree())) break;
	  }
  }

  if(e) trx_CloseDel(trx);
  else _setmsg(PCK_FILETOTALS);

  if(e || (e=trx_Close(trx))) return e;


  //Before closing we must insure that the existing cache is not freed --
  defCache=trx_defCache;
  trx_defCache=0;
  trx_CloseDel(trxold);
  *ptrx=0;

  if(rename(tmpnam,oldnam)) e=TRX_ErrRename;
  else {
	  trx_defCache=usrCache;
	  e=trx_Open(&trx,oldnam,TRX_ReadWrite);
  }
  trx_defCache=defCache;
  if(!e) *ptrx=trx+nTree;
  return e;

}
