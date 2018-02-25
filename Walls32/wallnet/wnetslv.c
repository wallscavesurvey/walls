/* WNETSLV.C -- Sort and solve each independent loop system */

#include <string.h>

#include "wnetlib.h"
#include <trx_str.h>
#include <dos_io.h>

/* Column offsets for count displays --*/
enum e_cshow {c_sys=15,c_loop=27,c_str=41,c_upd=56};

/*Testing option to be used in making future improvements to the
  solution method. Set net_bwcnt_limit to the minimum nodes a system
  must have for its bandwith freqencies to be computed and saved in
  net_bwcnt[] when net_Adjust is called with flag NET_BwCount. If a
  system is checked, then bandwith b will have resulted from
  net_bwcnt[b-1] sorts based on separate starting nodes. Also, an
  optimal sort will be used for the solution.*/

#ifdef BW_CNT
  int net_bwcnt_limit=10;  /*Minimum nodes in system to be checked*/
  int net_bwcnt[MAX_BW];   /*Array for BW frequency counts*/
  int net_bwcnt_len;       /*Length of array actually used*/
#endif

static UINT   reclast,recnext;
static int    sysnext,sysid;
static int    bl,bw,nl,nw,highsc,nseq,nb_deg;
#ifdef BW_CNT
static bool   bw_detail;
#endif

#define Maxlev   3000
#define Inc      1000
#define Maxsc    3002

/*====================================================================*/
/* Routines for reading of NTS database --                            */
/*====================================================================*/
int FAR PASCAL comp_node_fcn(lnptr p)
{
  return(comp_node-*p);
}

static apfcn_v add_end(int nd)
{
  lnptr p;

  /* If id[nd] is a new name, add it to jendp[] and insert a pointer
     to it into jnod[]. Otherwise, just insert a pointer to the
     matched name into jendp[]. */

  comp_node=net_dbstrp->id[nd];
  p=trx_Binsert((int)(jendp+sys_numendp),TRX_DUP_NONE); /*Insert no duplicates*/
  jendp[sys_numendp++]=trx_binMatch?*p:comp_node;
}

static apfcn_i read_strings(void)
{
  UINT i;

  sys_numinf[0]=sys_numinf[1]=0;
  sys_closures=sys_numendp=sys_numnod=sys_numstr=0;

  /*Initialize for binary insertion into jnod[] --*/
  trx_Bininit((lnptr)jnod,&sys_numnod,(TRXFCN_IF)comp_node_fcn);

  /* Initially, fill jendp[] with vector endpoints - either the names
     (integers) themselves (in case of first appearance), or near
     pointers to the matched name already inserted in jendp[]. Also build
     jnod[] as a sorted list of pointers to the unique names that were
     inserted in jendp[]. */

  sysid=sysnext;
  sys_recfirst=recnext;
  for(i=recnext;i<=reclast;i++) {
    GOTO_STR(i);
    if(sysid==-1) sysid=net_dbstrp->sys_id;
    else if(sysid!=net_dbstrp->sys_id) {
      sysnext=net_dbstrp->sys_id;
      break;
    }
    if(sys_numstr>=net_maxstrsolve) {
      CHK_ABORT(NET_ErrMaxStrings);
    }
    add_end(0);
    add_end(1);
    jtask[sys_numstr] =
      (WORD)(((net_dbstrp->flag&NET_STR_FLOATH) && ++sys_numinf[0])?InfvarH:0)
      +
      (WORD)(((net_dbstrp->flag&NET_STR_FLOATV) && ++sys_numinf[1])?InfvarV:0);
    sys_numstr++;
	//Clear link and bridge-related flags --
	if(sys_updating) {
		net_dbstr_wflag &= ~((NET_STR_BRIDGEH+NET_STR_CHNFLTH+NET_STR_CHNFLTXH)<<net_i);
		dbf_Mark(net_dbstr);
	}
  }
  recnext=i;
  
  sys_closures=sys_numstr-sys_numnod+1;
  if(!sys_numnod || sys_closures<=0) return 0;

  /*Now replace the endpoint id's in jendp[] with the corresponding
    indices to jnod[]. The original endpoint id's are discarded;
    however, the jendp[] sequence will maintain its corrspondence with
    the original data. jnod[i] is set to i for convenience of this
    operation. */

  for(i=0;i<(UINT)sys_numnod;i++) {
    *jnod[i]=(int)(jnod+i);
    jnod[i]=(lnptr)i;
  }
  for(i=0;i<(UINT)sys_numendp;i++) jendp[i]=*(lnptr)jendp[i];

  /*jdeg[i] is set to the number of connecting vectors, which
    will be the length of this node's adjacency list in jlst[].*/
  memset(jdeg,0,sys_numnod*sizeof(*jdeg));
  for(i=0;i<(UINT)sys_numendp;i++) jdeg[jendp[i]]++;

  /*jnod[] will store pointers to the adjacency lists.*/
  jnod[0]=jlst;
  for(i=0;i<(UINT)sys_numnod;i++) jnod[i+1]=jnod[i]+jdeg[i];

  /*finally, fill the adjacency list --*/
  memset(jdeg,0,sys_numnod*sizeof(*jdeg));
  for(i=0;i<(UINT)sys_numendp;i++) *(jnod[jendp[i]]+jdeg[jendp[i]]++)=i^1;

  /*Get number of link fragments or -1 if memory error.
    Also copies InfVar flags to F_Infvar, removing Infvar from
	all but one fragment of each link --*/
  if((sys_numlinkfrag=linkFrag_get(&sys_linkfrag))<0) CHK_ABORT(NET_ErrNearHeap);

  return 0;
}

static apfcn_v init_flags(void)
{
  /*Reconstruct the adjacency list*/
  int e;
  memset(jdeg,0,sys_numnod*sizeof(DWORD));
  for(e=0;e<sys_numendp;e++) *(jnod[jendp[e]]+jdeg[jendp[e]]++)=e^1;
  for(e=0;e<sys_numstr;e++) jtask[e] &= (F_InfvarH+F_InfvarV+InfvarH+InfvarV);
}

static apfcn_i score(int n)
{
  lnptr ap;
  int sc;

  sc = (nb_deg==1) * Inc;
  if(sc+2*jdeg[n]>highsc) {
    ap=jnod[n]+jdeg[n];
    while(--ap>=jnod[n]) {
      if(0!=(Nod_inet&jtask[jendp[*ap]])) {
        sc++;
        if(jdeg[jendp[*ap]]==1) sc++;
      }
      else sc--;
    }
  }
  return(sc);
}

static apfcn_v delete_int(lnptr ap,int tgt,int newlen)
{
    /*tgt=trx_ScanWrd(ap,tgt,newlen+1);*/
	__asm {
		mov		edi,ap
		mov		eax,tgt
		mov		ecx,newlen
		inc		ecx
		mov		edx,ecx
		repne	scasd
		jnz		_nofind
		sub		edx,ecx
_nofind:
		mov		tgt,edx
	}

    if(tgt<=newlen) memcpy(ap+tgt-1,ap+tgt,(newlen-tgt+1)*sizeof(DWORD));
}

static apfcn_i update(int e)
{
  int new;
  int n=jendp[e];

  new=(jtask[n]&Nod_inet)?0:Addnode;
  if(--jdeg[n]>0) {
    delete_int(jnod[n],e^1,jdeg[n]);
    if(new) {
      nl++;
      jbord[nw++]=n;
      jtask[n] |= Nod_inet;
    }
  }
  else {
/*  if(new) new|=Fixed; else */
    if(!new && --nw) delete_int(jbord,n,nw);
    new |= (Move1|Move2);
  }
  return(new);
}

static apfcn_i genseq(int n)
{
  lnptr ap;
  int b,nb,highe;
  int new_flag;

  /*Restore jlst[], jdeg[], and initialize jtask[] --*/
  init_flags();

  *jbord=n;
  jtask[n] |= Nod_inet;
  nseq=0;
  bl=nl=bw=nw=1;

  while(nw) { /*Add one string*/
    highsc = -Maxsc;
    new_flag=0;
_pass2:
    /*Scan border node edges for highest score. On the first pass,
      check for closures. Make a second pass if none are found.*/
    for(b=0;b<nw;b++) {
      nb=jbord[b];
      ap=jnod[nb]+(nb_deg=jdeg[nb]);
      while(--ap>=jnod[nb]) {
        n=jendp[*ap];
        if(new_flag) {
          if(jtask[(*ap)>>1]&Infvar) continue;
          n=score(n);
        }
        else {
          if(0==(jtask[n]&Nod_inet)) continue;
          n=Maxlev+(nb_deg==1)+(jdeg[n]==1);
        }
        if(n>highsc) {
          highsc=n;
          highe=*ap;
          if(n>=Maxsc) goto _high_found;
        }
      }
    }

    if(highsc==-Maxsc) {
      if(new_flag^=1) goto _pass2;
      /*An attachment string is flagged infinite*/
      CHK_ABORT(NET_ErrDisconnect);
    }

_high_found:
    n = update(highe^1) & Move1;
    n |= update(highe) & (Addnode|Move2);
    jtask[highe>>1] |= n;
    if(bw<=nw) {
      bw=nw;
      if(!(n & (Addnode|Move1|Move2))) bw++;
#ifdef BW_CNT
      if(!bw_detail)
#endif
      if(bw>=sys_bandw) return 0;
    }
    if(bl<nl) bl=nl;
    jseq[nseq++] = highe;
  }
  /*Impossible error!*/
  if(nseq<sys_numstr) CHK_ABORT(NET_ErrSysConnect);
  return 0;
}

static apfcn_v scanedges(void)
{
  int s,t;

  for(s=0;s<nseq;s++) {
    jtask[s] |= Nod_inet;  /*Used now by Invert()*/
    t=jtask[jseq[s]>>1];
    /*If one node is resolved, make it the from station*/
    if((t&(Addnode|Move1|Move2))==Move2) {
      jseq[s] ^= 1;
      jtask[jseq[s]>>1] ^= Move1|Move2;
    }
  }
}

static apfcn_i sort_net(void)
{
  /*This routine creates jseq[], a sorted list of strings (jendp[]
    indices) that would give a near-optimal bandwidth for the Q
    matrix.*/

  int n;

  #ifdef BW_CNT
  bw_detail=(net_bwcnt_flag && sys_numnod>=net_bwcnt_limit);
  if(bw_detail) memset(net_bwcnt,0,sizeof(net_bwcnt));
  net_bwcnt_len=0; /*nonzero on callback only for net_maxsysid*/
  #endif

  /*Checking all potential starting nodes for the best bandwidth is
    probably counter-productive. A better approach, perhaps, is to check
    just a small proportionate sample. We can use the BW_CNT compilation
    option to test this theory. For now, if we are not testing, we just
    accept the result for the first node in the array. Limited testing
    of this sorting algorithm with unusually large loop systems have
    resulted in small bandwidths and sufficiently fast solution times.*/

  sys_bandw=MAX_BW+1; /*sys_bandw will not exceed sys_numnod+1*/
  
  Infvar=(InfvarH<<net_i); /*Global variable to check float status*/
  
  for(n=0;TRUE;n++) {
    if(genseq(n)) return net_errno;
#ifdef BW_CNT
    if(bw_detail && bw<=MAX_BW) {
      net_bwcnt[bw-1]++;
      if(bw>net_bwcnt_len) net_bwcnt_len=bw;
    }
#endif
    if(bw<=sys_bandw) {
      sys_root=n;
      sys_bandw=bw;
      sys_bandl=bl;
      if(bw<=MAX_BW)
#ifdef BW_CNT
      if(!bw_detail)
#endif
      break;
      /*if # Q elements=(bw-1)*(bl+Dim-1) small enough, break*/
    }
    if(n==sys_numnod-1) break;
  }

  if(sys_bandw>MAX_BW) CHK_ABORT(NET_ErrBwLimit);

  sys_bandl+=(1-net_i);
  
  /*Regenerate sequence if necessary --*/
  if(n!=sys_root && genseq(sys_root)) return net_errno;

  /*Flag vectors for sequential Q-matrix modification in sys_invert() --*/
  scanedges();
  return 0;
}

static apfcn_i alloc_netsolve(int numstr,int numnod)
{
	jendp=(lnptr)dos_AllocNear(sizeof(lnptr)*(
		2*numstr     /*jendp - set in read_strings()*/
		+numstr      /*jseq  - set in genseq()*/
		+numnod+1    /*jnod  - set in readstrings()*/
		+numnod+1    /*jdeg  - set in readstrings() - colnod[] in sys_invert()*/
		+numnod      /*jbord - set by genseq() - nodcol[] in in sys_invert()*/
		+2*numstr)+  /*jlst  - set in init_alist() - row[] in sys_invert()*/
		numstr*      /*jtask - flags: InfvarH/V set in readstrings(), others in genseq()*/
		sizeof(WORD));

	if(!jendp) CHK_ABORT(NET_ErrNearHeap);

	jseq=jendp+2*numstr;
	jnod=(lnptr *)(jseq+numstr);
	jdeg=(lnptr)(jnod+(numnod+1));
	jbord=jdeg+(numnod+1);
	jlst=jbord+numnod;
	jtask=(WORD *)(jlst+2*numstr);

	#ifdef _DEBUG
	nodcol=jbord;
	colnod=jdeg;
	row=jlst;
	#endif

    return 0;
}

static void free_netsolve(void)
{
  if(sys_numlinkfrag) sys_numlinkfrag=linkFrag_clear();
  dos_FreeNear(jendp);
}

apfcn_i net_solve(void)
{
   /*Adjusts each independent system in turn. Called from net_Adjust()*/

   sys_updating=FALSE;
   net_nc=0;

   /*Allocate DGROUP memory sufficient to adjust the largest system --*/
   if(alloc_netsolve(net_maxstrsolve,net_maxsysnod)) return net_errno;

   reclast=(DWORD)dbf_NumRecs(net_dbstr);
   recnext=1;
   sysnext=-1;

   while(recnext<=reclast) {

     /*Read strings of next system, creating a list of vector endpoints
       jendp[], which are indices to a node array, jnod[]. Each jnod[]
       element is a pointer to an adjacency list in jlst[]. An element
       of jlst[], in turn, represents the adjacent node as an index to
       jendp[], which also identifies the connecting vector.
       jdeg[n]=jnod[n+1]-jnod[n] is the length of the adjacency list
       for jnod[n].

	   Also, jtask[0..sys_numstr-1] is initialized with InfvarH/V flags.*/

     if(read_strings()) break;

     /*Now, for example, the ith node adjacent to node N is Ni=jendp[e]
       where e=*(jnod[N]+i-1). Note also that N=jendp[e^1] and the NTS
       record number corresponding to this string is sys_recfirst+(e>>1).*/
       
     if(sys_numnod>0) {
		net_pData->sys_id=sysid;
		if(NET_CB_RTN(NET_PostSortSys)) break;
		
		/*Q matrix will grow to size sys_bandw*sys_bandl*sizeof(Qtyp)*/
		net_i=0;
		if(sort_net() || sys_invert()) break;
		if(sys_nc!=sys_closures || sys_ni!=sys_numinf[0]) {
		 CHK_TEST(NET_ErrConstrain);
		 break;
		}
		
		net_i=1;
		if(sort_net() || sys_invert()) break;
		if(sys_nc!=sys_closures || sys_ni!=sys_numinf[1]) {
		 CHK_TEST(NET_ErrConstrain);
		 break;
		}

		net_nc+=sys_closures;
		
		//if(NET_CB_RTN(NET_PostSolveSys)) break;
     }
   }
   free_netsolve();
   return net_errno;
}

apfcn_i position_dbnet(UINT recNTS)
{
	FCHKN(dbf_First(net_dbnet));
	do {
	  if((UINT)net_dbnetp->str_id1<=recNTS && (UINT)net_dbnetp->str_id2>=recNTS) {
		return 0;
	  }
	}
	while(!dbf_Next(net_dbnet) && net_dbnetp->flag!=NET_SURVEY_FLAG);
	return net_errno=NET_ErrWork;
}

apfcn_i sys_solve(UINT recNTS)
{
    /*Called from net_Update() only when a single component (Hz or Vt) of a single
	  system is to be processed. Global net_i = 0(Hz) or 1(Vt)*/
	 
	/*First position net_dbnetp to the component containing the
	  specified string --*/
	  
	recnext=sys_numlinkfrag=0;
	sys_updating=TRUE;

	CHK_TEST(position_dbnet(recNTS));
	  
	recnext=net_dbnetp->str_id1; /*first string of component*/
	reclast=net_dbnetp->str_id2; /*last string of component*/
	
	GOTO_STR(recNTS);
	sysnext=net_dbstrp->sys_id;
	   
	//recnext at this point is the first NTS record of the affected component.
	//Advance recnext to the first string in the affected loop system --
	if(sysnext!=-1) {
		//A multi-string system --
		sys_closures=net_dbstrp->sys_nc; //Total closures including floats
		for(;recnext<=reclast;recnext++) {
			GOTO_STR(recnext);
			if(net_dbstrp->sys_id==sysnext) break;
		}
		if(recnext>reclast) CHK_ABORT(NET_ErrWork);
		//recnext is the first string of the affected loop system.
		for(recNTS=recnext+1;recNTS<=reclast;recNTS++) {
			GOTO_STR(recNTS);
			if(net_dbstrp->sys_id!=sysnext) break;
		}
		reclast=recNTS-1;
		//recnext is the last string of the affected loop system.
		//Save string and endpoint counts needed to compute memory requirements --
		net_maxstrsolve=recNTS-recnext;
		net_maxsysnod=net_maxstrsolve-sys_closures+1;
	}
	else {
	  recnext=reclast=recNTS;
	  net_maxstrsolve=net_maxsysnod=2; //play it safe
	}
	
	/*Allocate DGROUP memory sufficient to solve this system --*/
	if(alloc_netsolve(net_maxstrsolve,net_maxsysnod)) return net_errno;
	
	//recnext is already set to record number of first string.
	//read_strings() also checks against net_maxstrsolve.
	
	sysnext=-1;  //read_strings() requires this
	
	if(!read_strings() && sys_numnod>0) {
		if(!sort_net()) sys_invert();
	}
	
	free_netsolve();
	return net_errno;
}
