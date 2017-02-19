/* int trx_BuildTree(TRX_NO trx, LPCSTR pathtmp, int (*getkey)(int))
   Index File Rebuild -- Subtree Merge Technique
*/

#include <a__trx.h>
#include <trx_str.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

/*Defaults --*/
#define BLD_KBUFS 32     /*Number of trees in temporary file*/
#define BLD_CBUFS 128    /*Node buffers in cache*/
#define BLD_PREWRITE 1	 /*Prewrite count for temporary indexes - slow if 0!*/
#define BLD_TEMPMODE TRX_ReadWrite /*TRX_ReadWrite faster than TRX_UserFlush*/

static char	trx_bld_messagebuf[64];    

/*Globals that can be accessed by application --*/

BYTE *	trx_bld_keybuf;				/*Key buffer filled by caller*/
void *	trx_bld_recbuf;				/*Record buffer filled by caller*/
DWORD 	trx_bld_keycount;			/*Number of keys indexed or merged at this stage*/
DWORD	trx_bld_mrgcount;			/*Total number of keys indexed*/
DWORD	trx_bld_keydelta=500;		/*BLD_KEYCOUNT callback interval - no messages if 0*/
DWORD	trx_bld_mrgdelta=500;		/*BLD_MRGCOUNT callback interval*/
DWORD	trx_bld_treebufs=BLD_KBUFS;	/*Number of trees in temporary indexes*/
DWORD   trx_bld_cachebufs=BLD_CBUFS;/*Number of cache buffers assigned to temp indexes*/		
char *  trx_bld_message=trx_bld_messagebuf;


#define TMP_NAM "TRXBUILD.$$$"
#define TMP_NAM_LEN 12

static TRX_NO trx0,trx1;

static UINT sizRec,sizKey,initFlags;
static DWORD trx1_tree,trx0_tree;
static BYTE *comp_key;
static int comp_idx;
static TRXFCN_IF _getkey;

/*The following are arrays of length trx_bld_treebufs ==*/
static BYTE **_tkey; /*Array of key buffer ptrs for merging trees*/
static void **_trec; /*Array of record buffer ptrs for merging trees*/
static DWORD  *_tseq;  /*buffer indices sorted by current key value*/
static DWORD _tseqlen;

static int _setmsg(int typ)
{
	int e=0;
	if(trx_bld_message==trx_bld_messagebuf) {
		switch(typ) {
			case BLD_KEYSTART:
				e=sprintf(trx_bld_messagebuf,"%7lu                  \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
					trx_bld_keycount);
				break;
			case BLD_KEYCOUNT:
				e=sprintf(trx_bld_messagebuf,"%7lu",trx_bld_keycount);
				break;
			case BLD_MRGSTART:
				e=sprintf(trx_bld_messagebuf,"%7lu  Merging: %7lu",trx_bld_keycount,trx_bld_mrgcount);
				break;
			case BLD_MRGCOUNT:
				e=sprintf(trx_bld_messagebuf,"%7lu",trx_bld_mrgcount);
				break;
			case BLD_MRGSTOP:
				strcpy(trx_bld_messagebuf,"\b\b\b\b\b\b\b\b\b\b\b");
				e=11;
		}
		strcpy(trx_bld_messagebuf+e,"\b\b\b\b\b\b\b");
	}
	return _getkey(typ);
}

static void init_trees(TRX_NO trx)
{
	DWORD i;

	for(i=0;i<trx_bld_treebufs;i++) {
		_trx_GetTrx(trx++);
		_trx_pTreeVar->SizRec=sizRec;
		_trx_pTreeVar->SizKey=(BYTE)sizKey;
		_trx_pTreeVar->InitFlags=initFlags;
    }
}

static int index_to_trx0(void)
{
	int e;
	TRX_NO trx=trx0;

	trx0_tree=0;

	if((e=_setmsg(BLD_KEYSTART))) return e;

	while(TRUE) {
		_trx_SetUsrFlag(trx,TRX_KeyDescend,FALSE);

		/*_getkey(0) returns -1 upon completion --*/
		while(!(e=_getkey(0))) {
			if((e=trx_InsertKey(trx,trx_bld_recbuf,trx_bld_keybuf))) {
			   if(e==TRX_ErrDup) continue;
			   goto _err;
			}
			trx_bld_keycount++;
			if(trx_bld_keydelta && !(trx_bld_keycount%trx_bld_keydelta) &&
			  (e=_setmsg(BLD_KEYCOUNT))) return e;
			if(_trx_pTreeVar->NumNodes>trx_bld_cachebufs) break;
		}
        trx++;
		trx0_tree++;

		if(e || trx0_tree==trx_bld_treebufs) break;

		/*flush and purge cache --*/
		if((e=_csf_Flush((CSF_NO)_trx_pFile))) break;
		_csf_Purge((CSF_NO)_trx_pFile);
	}

	if(!e) return _setmsg(BLD_KEYCOUNT);

_err:
	_setmsg(BLD_KEYCOUNT);
	return e;
}

static int CALLBACK comp_key_fcn(int i)
{
	BYTE *key=_tkey[i];
	int iret;

	if(*comp_key<*key) iret=(memcmp(comp_key+1,key+1,*comp_key)<=0)?-1:1;
	else {
		iret=memcmp(comp_key+1,key+1,*key);
		if(!iret) {
			if(*key!=*comp_key) iret=1;
			else iret=(comp_idx<i)?1:-1;
		}
		else iret=(iret<0)?-1:1;
	}
	return iret;
}

static int load_tseq_key(int i,int e)
{
	if(e) return (e==TRX_ErrEOF)?0:e;

	comp_key=_tkey[i];
	comp_idx=i;
	if((e=trx_Get(trx0+i,_trec[i],sizRec,comp_key,256))) return e;
	/*The compare routine won't return 0 --*/
	(void)trx_Binsert(i,TRX_DUP_IGNORE);
	return 0;
}

static int merge_to_trx1(void)
{
	int i,e;
	TRX_NO trx=trx1+trx1_tree;
	CSF_NO csf1;

	if((e=_setmsg(BLD_MRGSTART))) return e;
    
	_trx_SetUsrFlag(trx,TRX_KeyDescend,TRUE);
	csf1=(CSF_NO)_trx_pFile;

	_tseqlen=0;
	trx_Bininit(_tseq,&_tseqlen,comp_key_fcn);

	/*Source index trx0 contains trx0_tree non-empty trees*/
    for(i=0;i<(int)trx0_tree;i++)
		if((e=load_tseq_key(i,trx_Last(trx0+i)))) return e;

    trx_bld_mrgcount=0;

	/*Perform merge --*/
    while(_tseqlen) {
		i=_tseq[--_tseqlen];
		if(((e=trx_InsertKey(trx,_trec[i],_tkey[i])) && e!=TRX_ErrDup) ||
			(e=load_tseq_key(i,trx_Prev(trx0+i)))) goto _err;
		trx_bld_mrgcount++;
		if(trx_bld_mrgdelta && !(trx_bld_mrgcount%trx_bld_mrgdelta) &&
		   (e=_setmsg(BLD_MRGCOUNT))) return e;
	}
    trx1_tree++;

	/*flush and purge cache of trx1 --*/
	if((e=_csf_Flush(csf1))) return e;
	_csf_Purge(csf1);
	e=trx_SetEmpty(trx0);
	if(!e && !(e=_setmsg(BLD_MRGCOUNT))) e=_setmsg(BLD_MRGSTOP);
	return e;

_err:
	_setmsg(BLD_MRGCOUNT);
    return e;
}

int TRXAPI trx_BuildTree(TRX_NO trx,LPCSTR tmppath,TRXFCN_IF getkey)
{
  /*trx is the file/tree identifier of an initialized empty tree
    of an open TRX file*/

  int i,e;
  char *p;
  TRX_pTREEVAR trxt;
  TRX_pFILE trxp;
  TRX_CSH usrCache,defCache;
  UINT usrPreWrite;
  char tmpnam[MAX_PATH];

  if(!(trxp=_trx_GetTrx(trx))) return TRX_ErrArg;
  trxt=_trx_pTreeVar;
  if(trxt->Root!=0L) return trx_errno=TRX_ErrNonEmpty;
  if(_trx_pFileUsr->UsrMode&TRX_Share) return trx_errno=TRX_ErrFileShared;

  sizRec=trxt->SizRec;
  sizKey=trxt->SizKey;
  if(sizKey==0) return trx_errno=sizRec?TRX_ErrTreeType:TRX_ErrNoInit;
  initFlags=(trxt->InitFlags&~TRX_KeyDescend);

  if(tmppath && *tmppath) {
	  i=strlen(trx_Stncc(tmpnam,tmppath,MAX_PATH-1));
	  if(tmpnam[i-1]!=_PATH_SEP) tmpnam[i++]=_PATH_SEP;
	  p=tmpnam+i;
  }
  else {
	p=trx_Stpnam(strcpy(tmpnam,dos_FileName(trxp->Cf.Handle)));
	i=p-tmpnam;
  }

  if(i>=MAX_PATH-TMP_NAM_LEN) return trx_errno=TRX_ErrName;
  strcpy(p,TMP_NAM);
  
  /*Allocate arrays of record and key buffers*/
  if(!(p=(char *)malloc(trx_bld_treebufs*(sizeof(DWORD)+2*sizeof(void *)+sizRec+256))))
	  return trx_errno=TRX_ErrMem;

  _trec=(void **)p;
  p+=trx_bld_treebufs*sizeof(void *);
  _trec[0]=sizRec?(BYTE *)p:0;
  _tkey=(BYTE **)(p+=trx_bld_treebufs*sizRec);
  _tkey[0]=(BYTE *)(p+=trx_bld_treebufs*sizeof(char *));
  _tseq=(DWORD *)(p+256*trx_bld_treebufs);
  for(i=1;i<(int)trx_bld_treebufs;i++) {
	 _trec[i]=(BYTE *)_trec[i-1]+sizRec;
	 _tkey[i]=(BYTE *)_tkey[i-1]+256;
  }

  trx0=trx1=0;
  defCache=trx_defCache;
  trx_defCache=0; /*This prevents automatic cache assignment*/

  if((e=trx_Create(&trx0,tmpnam,BLD_TEMPMODE,trx_bld_treebufs,0,trxp->Fd.SizNode,
	  trxp->Fd.SizNodePtr))) goto _fin;
  /*If MinPreWrite==0, newly allocated nodes are not prewritten to disk --*/
  _trx_pFile->MinPreWrite=BLD_PREWRITE;

  if((e=trx_AllocCache(trx0,trx_bld_cachebufs,TRUE)))  goto _fin;

  init_trees(trx0);

  /* Indexing Phase */

  trx_bld_keycount=0;
  _getkey=getkey;
  trx_bld_keybuf=_tkey[0];
  trx_bld_recbuf=_trec[0];

  while(!(e=index_to_trx0())) {
	  /*Cache is filled - Merge all keys into next tree of trx1*/
	  if(!trx1) {
		  /*Create trx1 and share cache with trx0 --*/
		  tmpnam[strlen(tmpnam)-1]='1';
		  if((e=trx_CreateInd(&trx1,tmpnam,BLD_TEMPMODE,trx_FileDef(trx0)))) goto _fin;
		  _trx_pFile->MinPreWrite=BLD_PREWRITE;
		  init_trees(trx1);
		  trx1_tree=0;
	  }
	  /*Move all keys in trx0 to the next empty tree of trx1, then set trx0 empty --*/
	  if((e=merge_to_trx1())) goto _fin;

	  /*In the unlikely event trx1 fills completely (to about 32 * 32 times cache capacity)
	    switch temporary indexes and merge to the first tree of the index just emptied --*/
	  if(trx1_tree==trx_bld_treebufs) {
		 e=(int)trx1;
		 trx1=trx0;
		 trx0=(TRX_NO)e;
		 trx1_tree=0;
		 if((e=merge_to_trx1())) goto _fin;
	  }
  }
  if(e!=-1) goto _fin;

  /*Cache of trx0 is partially filled --*/
  if(trx1) {
	  if((e=merge_to_trx1())) goto _fin;
	  trx_CloseDel(trx0);
	  trx0=trx1;
	  trx0_tree=trx1_tree;
  }

  /*Perform the final merge into the destination tree and flush result.
    We will temporarily assign the locally allocated cache to the
    destination file, set minPreWrite=0, and turn any flush mode off.*/

  trx1_tree=(trx&0xFF);
  trx1=trx-trx1_tree;

  /*Save the existing cache -- we won't use it*/
  /*Attaching a new cache will flush and purge this cache --*/
  usrCache=trxp->Cf.Csh;
  if(!(e=trx_AttachCache(trx1,trx_defCache))) {
	 i=(int)(_trx_pFileUsr->UsrMode&(TRX_UserFlush+TRX_AutoFlush));
	 _trx_pFileUsr->UsrMode&=~i;
	 usrPreWrite=_trx_pFile->MinPreWrite;
	 trxp->MinPreWrite=0;
	 e=merge_to_trx1();
	 trxp->MinPreWrite=usrPreWrite;
	 if(_trx_FlushHdr(trxp) && !e) e=trx_errno;
	 if(trx_AttachCache(trx1,usrCache) && !e) e=trx_errno;
	 _trx_pFileUsr->UsrMode|=i;
  }
  trx1=0;

_fin:
  free(_trec);
  /*This should delete the default cache --*/
  if(trx1) trx_CloseDel(trx1);
  if(trx0) trx_CloseDel(trx0);
  trx_defCache=defCache;
  return trx_errno=e;
}
