#include <arc_unix.h>
#include <a__trx.h>
#include <string.h>

/*Globals that can be accessed by application --*/
//long 	bldkeys;	/*Number of keys so far added (never used!)*/
int		bldexit;	/*Set nonzero by getkey() upon abort request*/
int		bldidx;		/*Resulting IDX file ID when bldsave=TRUE*/
BOOL	bldsave;	/*If TRUE, new index will not be closed*/
BOOL	bldlovd;	/*If TRUE, above messages will be in dim video*/
BOOL	bldtcnt;	/*If TRUE, displays tree count when indexing*/
BOOL	blduniq;	/*If TRUE, do not add duplicate keys to index*/

static DWORD (*_arc_keyfcn)(void);
 
static int CALLBACK _arc_getkey(int typ)
{
	DWORD rec;

	if(typ) {
		if(trx_bld_message) {
			addstr(trx_bld_message);
			if(typ==BLD_MRGSTOP) {
				skipspc(7);
				eraseol();
				backspc(7);
			}
			refresh();
		}
		if(escpressed()) return TRX_ErrUsrAbort;
		return 0;
	}

	bldexit=0;
	if(!(rec=_arc_keyfcn())) return bldexit?TRX_ErrRead:-1;
	*(DWORD *)trx_bld_recbuf=rec;
	memcpy(trx_bld_keybuf,idxbuf,*idxbuf+1);
	return 0;
}

int buildidx(LPCSTR inam,DWORD (*keyfcn)(void))
{
    int e;

	if((e=_makeidx(&bldidx,FALSE,inam))) return e;
    _arc_keyfcn=keyfcn;
	if(blduniq) trx_SetUnique(bldidx,TRUE);
	if(bldlovd) lovideo();
	e=trx_BuildTree(bldidx,getortmp(),_arc_getkey);
	if(bldlovd) hivideo();
        if(!bldsave) {
	  if(trx_Close(bldidx) && !e) e=TRX_ErrClose;
	  bldidx=0;
	}
	arcerr=e;
	if(e) {
		if(e==TRX_ErrRead) {
			if(bldexit) e=bldexit;
			else bldexit=e=2;
		}
		else if(e==TRX_ErrUsrAbort) bldexit=e=1;
		else e=-e;
	}
	return e;
}
