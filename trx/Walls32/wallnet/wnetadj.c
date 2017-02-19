#include <windows.h>
#include <string.h>
#include <limits.h>
#include <math.h> 
#include <float.h>
#define MAIN
#include "wnetlib.h"
#ifdef _USE_DFSTACK
#include <malloc.h> 
#endif
#include <trx_str.h>
#include <dos_io.h>

static double dbl_limits[2*Dim]={DBL_MAX,DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX};

/*==================================================================*/
/* Routines to update NTV database with corrected vectors           */
/*==================================================================*/

static apfcn_i update_vectors(void)
{
  int i,idir,idnxt;
  double varH=net_dbstrp->hv[0];
  double varV=net_dbstrp->hv[1];

  //Obtain the correction vector --
  for(i=0;i<Dim;i++) P[i]=net_dbstrp->e_xyz[i]-net_dbstrp->xyz[i];
  GOTO_VEC(net_dbstrp->vec_id);
  idnxt=next_id(net_dbvecp->id[net_dbvecp->id[0]==net_dbstrp->id[0]]);
  
  while(TRUE) {
    idnxt=next_id(idnxt);
    idir=(idnxt==net_dbvecp->id[1])?1:-1;
    if(net_dbvecp->hv[0]<=0.0)
      memcpy(net_dbvecp->e_xyz,net_dbvecp->xyz,2*sizeof(double));
    else {
      /*varH has to be nonzero --*/
      Z=idir * net_dbvecp->hv[0] / varH;
      for(i=0;i<2;i++) net_dbvecp->e_xyz[i]=(net_dbvecp->xyz[i]+Z*P[i]);
    }
    if(net_dbvecp->hv[1]<=0.0) net_dbvecp->e_xyz[2]=net_dbvecp->xyz[2];
    else {
      /*varV has to be nonzero --*/
      Z=idir * net_dbvecp->hv[1] / varV;
      net_dbvecp->e_xyz[2]=(net_dbvecp->xyz[2]+Z*P[2]);
    }
    //net_dbvecp->sys_id=net_dbstrp->sys_id;
    dbf_Mark(net_dbvec);
    if(net_dbvecp->str_nxt_vc==0) break;
    GOTO_VEC(net_dbvecp->str_nxt_vc);
  }
  return 0;
}

static apfcn_i update_flt_vectors(BOOL bh,BOOL bv)
{
	int i,idir,idnxt;
	double varH=0.0;
	double varV=0.0;
	int iFltV=0;
	int iFltH=0;
	int iCnt=0;
	
	//At least one component type is partially floated.  
	//First, obtain total variances of floated vectors --
	i=net_dbstrp->vec_id;
	while(i) {
		GOTO_VEC(i);
		iCnt++;
		if(bh && (net_dbvecp->flag&NET_STR_FLTVECTH)) {
		  varH+=net_dbvecp->hv[0];
		  iFltH++;
		}
		if(bv && (net_dbvecp->flag&NET_STR_FLTVECTV)) {
		  varV+=net_dbvecp->hv[1];
		  iFltV++;
		}
		i=net_dbvecp->str_nxt_vc;
	}

	//ASSERT(iFltH || iFltV);
	
	if(!iFltH) varH=net_dbstrp->hv[0];
	if(!iFltV) varV=net_dbstrp->hv[1];
	  
	for(i=0;i<Dim;i++) P[i]=net_dbstrp->e_xyz[i]-net_dbstrp->xyz[i];
	
	GOTO_VEC(net_dbstrp->vec_id);
	idnxt=next_id(net_dbvecp->id[net_dbvecp->id[0]==net_dbstrp->id[0]]);
	
	while(TRUE) {
		idnxt=next_id(idnxt);
		idir=(idnxt==net_dbvecp->id[1])?1:-1;
		if(iFltH && !(net_dbvecp->flag&NET_STR_FLTVECTH))
		  //Other vectors are floated so no need to adjust this one --
		  memcpy(net_dbvecp->e_xyz,net_dbvecp->xyz,2*sizeof(double));
		else {
			//Either there are NO floated vectors or at least this one is floated
			if(varH<=0.0) Z=1.0/(iFltH?iFltH:iCnt);
			else Z=net_dbvecp->hv[0]/varH;
			for(i=0;i<2;i++) net_dbvecp->e_xyz[i]=(net_dbvecp->xyz[i]+idir*Z*P[i]);
		}
		if(iFltV && !(net_dbvecp->flag&NET_STR_FLTVECTV))
		  //Other vectors are floated so no need to adjust this one --
		  net_dbvecp->e_xyz[2]=net_dbvecp->xyz[2];
		else {
		    //Either there are NO floated vectors or at least this one is floated
			if(varV<=0.0) Z=1.0/(iFltV?iFltV:iCnt);
			else Z=net_dbvecp->hv[1]/varV;
			net_dbvecp->e_xyz[2]=(net_dbvecp->xyz[2]+idir*Z*P[2]);
		}
		dbf_Mark(net_dbvec);
		if(net_dbvecp->str_nxt_vc==0) break;
		GOTO_VEC(net_dbvecp->str_nxt_vc);
	}
	return 0;
}

static apfcn_i update_ntv(void)
{
  int s;
  int f;
  BOOL bv,bh;

  for(s=1;s<=net_numstr;s++) {
    GOTO_STR(s);
    f=net_dbstr_wflag;
    //bh=(f&NET_STR_FLTMASKH)==NET_STR_FLTPARTH;
	//bv=(f&NET_STR_FLTMASKV)==NET_STR_FLTPARTV;
    bh=(f&NET_STR_FLTMSKH)!=0 && (f&NET_STR_FLTALLH)==0;
    bv=(f&NET_STR_FLTMSKV)!=0 && (f&NET_STR_FLTALLV)==0;
    if(bh || bv) f=update_flt_vectors(bh,bv);
    else f=update_vectors();
    if(f) return f;
  }
  return 0;
}

typedef struct {
	int   sys_id;   /*0 if sys_nc=1*/
	int   str_id;
	int   vec_cnt;
	/*int   nxt_seg;*/   
	double sys_uve;
	double str_uve;
} uvetyp;

typedef uvetyp FAR *uveptr;
static uveptr puve;
uveptr puve_new;

int FAR PASCAL comp_uve_fcn(int i)
{
  int id=puve_new->sys_id;
  
  uveptr puve_old=puve+i; 
  if(id!=puve_old->sys_id) {
    if(!id) return 1;
    if(!puve_old->sys_id) return -1;
    id=0;
  }
  if(!id) return puve_new->sys_uve>puve_old->sys_uve ? -1:1;
  return puve_new->str_uve>puve_old->str_uve ? -1:1;
}

/*#define XSMALL 0.000000001*/
/*DBL_EPSILON == 2.2204460492503131e-016 (smallest such that 1.0+DBL_EPSILON != 1.0)*/
/*FLT_EPSILON == 1.192092896e-07F        (smallest such that 1.0+FLT_EPSILON != 1.0 */

#define XSMALL 1.0e-012

static double get_str_uve(void)
{
  int i;
  dlnptr fv=net_dbstrp->e_xyz;
  double inc_ss[3],ss,v;
  
  for(i=0;i<3;i++) {
     /*Caution: Same as e_xyz[i]-xyz[i]. See struct strtyp in netlib.h --*/ 
	 ss=fv[i]-fv[i-5];
	 inc_ss[i]=ss*ss;
  }
  ss=0.0;
  if(!(net_dbstr_wflag&NET_STR_FLOATH) && (v=net_dbstrp->hv[0])>0.0) {
	    v-=net_dbstrp->e_hv[0];
	    if(v>XSMALL) ss=(inc_ss[0]+inc_ss[1])/v;
  }
  
  if(!(net_dbstr_wflag&NET_STR_FLOATV) && (v=net_dbstrp->hv[1])>0.0) {
	    v-=net_dbstrp->e_hv[1];
	    if(v>XSMALL) ss+=inc_ss[2]/v;
  }
  return ss; /*Se*/
}

static apfcn_i sort_strings(void)
{
  int nc,uvecnt,srec,nrec,num_sys;
  /* int num_isol;*/
  int last_id,str_id;
  static int net_uvecnt;
  lnptr pidx;

  if(open_dbstr()) return net_errno;
  
  /*The network component database is already open, For each component, we
    want to establish a sequence of linked pointers (str_next) within the string
    database.
    The order established will be: 1) strings of multi-loop systems sorted by 
    decreasing system UVE and, within each system, by decreasing F-statistic,
    followed finally by 2) isolated loops sorted by decreasing UVE. */

  CHK_MEM(puve,net_numstr*sizeof(uvetyp));
  
  if((pidx=(lnptr)dos_AllocNear(net_numstr*sizeof(int)))==0) {
    dos_FreeFar(puve);
    CHK_ABORT(NET_ErrNearHeap);
  }
  
  //if(FIRST_NET_RTN()) goto _err_exit;
  
  uvecnt=0;
  
  for(nrec=1;nrec<=net_numnets;nrec++) {
	  if(GOTO_NET_RTN(nrec)) goto _err_exit;
	  
	  if(!(srec=net_dbnetp->str_id1)) continue; /*If network has no strings*/
	  
	  /*New component: Initialize for binary insertion into the next segment of pidx[] --*/
	  net_uvecnt=0;
	  trx_Bininit(pidx+uvecnt,&net_uvecnt,(TRXFCN_IF)comp_uve_fcn);
	  puve_new=puve+uvecnt;

	  for(;srec<=net_dbnetp->str_id2;srec++,puve_new++) {
	    if(GOTO_STR_RTN(srec)) goto _err_exit;
	    nc=3*net_dbstrp->sys_nc-2*net_dbstrp->sys_ni[0]-net_dbstrp->sys_ni[1];
	    if(nc) puve_new->sys_uve=(net_dbstrp->sys_ss[0]+net_dbstrp->sys_ss[1])/nc;
	    else puve_new->sys_uve=0.0;
	    puve_new->vec_cnt=net_dbstrp->vec_cnt;
	    if(net_dbstrp->sys_nc==1) puve_new->sys_id=0;
	    else {
	      puve_new->str_uve=(nc?get_str_uve():0.0);
	      puve_new->sys_id=net_dbstrp->sys_id;
	      /*puve_new->nxt_seg=0;*/
	    }
	    trx_Binsert(srec-1,TRX_DUP_IGNORE);
	  }
	  
	  num_sys=0;
	  /*num_isol=0;*/
	  last_id=str_id=-1;
	  net_uvecnt+=uvecnt;
	  
	  for(;uvecnt<net_uvecnt;uvecnt++) {
	    puve_new=puve+pidx[uvecnt];
	    str_id++;
	    if(last_id!=puve_new->sys_id) {
	      /*New System --*/
	      last_id=puve_new->sys_id;
	      if(last_id) {
	        num_sys++; /*new system identifier*/
	      }
	      /*else num_isol++*/
	    }
	    
	    if(last_id) puve_new->sys_id=num_sys-1;
	    else puve_new->sys_id=-1;
	    puve_new->str_id=str_id;
	  }
	  net_dbnetp->num_sys=num_sys;
	  /*net_dbnetp->num_isol=num_isol;*/
	  dbf_Mark(net_dbnet);      
  }	  
  
  netopen=FALSE;
  if(CHK(dbf_Close(net_dbnet),4)) goto _err_exit;
  
  /*Sanity check --*/
  if((int)dbf_NumRecs(net_dbstr)!=uvecnt) {
    CHK(NET_ErrSysConnect,0);
    goto _err_exit;
  }
  
  for(srec=1;srec<=uvecnt;srec++) {
	if(GOTO_STR_RTN(srec)) goto _err_exit;
	puve_new=puve+(srec-1);
	net_dbstrp->sys_id=puve_new->sys_id;
	/*net_dbstrp->nxt_seg=puve_new->nxt_seg;*/
	net_dbstrp->str_id=puve_new->str_id;
	net_dbstrp->vec_cnt=puve_new->vec_cnt;
	dbf_Mark(net_dbstr);      
  }
  
  stropen=FALSE;
  CHK(dbf_Close(net_dbstr),2);
  
_err_exit:
  dos_FreeFar(puve);
  dos_FreeNear(pidx);
  return net_errno;
}

static apfcn_i update_ntp_CB(void)
{
	dbf_Mark(net_dbnet);

	memcpy(net_pData->name,net_dbnetp->name,NET_SIZNAME);
	net_pData->length=net_dbnetp->length;
	net_pData->vectors=net_dbnetp->vectors;
	NET_CB(NET_PostNetwork);
	return 0;
}

static double length3(dlnptr px)
{
	return sqrt(px[0]*px[0]+px[1]*px[1]+px[2]*px[2]);
}

static apfcn_i update_ntp()
{
  int i;
  UINT ep,j;
  UINT rec,lastrec;
  lnptr pendp,endp,*p;
  
  /*Note: To avoid roundoff, we are using doubles to accumulate
    absolute coordinates.*/
  
  int net_vecttl,net_status,net_str_id1,net_str_id2;
  double net_lenttl;
  
  #if _STACKNEAR
    int endpts;
    DIM_DPOINT xyz;
    double xyzlim[2*Dim];
    double _huge *pxyz;
  #else
    static int endpts; //must be NEAR for trx_Bininit 
    static DIM_DPOINT xyz;
    static double xyzlim[2*Dim];
    static double _huge *pxyz;
  #endif
  
  BOOL bMoveTo=TRUE;

  if(create_dbnet()) return net_errno;
  
  net_status=0;

  if(open_dbplt()) return net_errno;

  CHK_MEMH(pxyz,(DWORD)numpltendp*sizeof(DIM_DPOINT));
  
  if((pendp=(lnptr)dos_AllocNear(2*numpltendp*sizeof(DWORD)))==0) {
    dos_FreeHuge(pxyz);
    CHK_ABORT(NET_ErrNearHeap);
  }

  /*Initialize for binary insertion into pendp[] --*/
  endp=pendp+numpltendp;
  endpts=0;
  trx_Bininit(pendp,&endpts,(TRXFCN_IF)comp_node_fcn);

  lastrec=(UINT)dbf_NumRecs(net_dbplt);
  
  for(rec=1;rec<=lastrec;rec++) {
    if(GOTO_PLT_RTN(rec)) break;
    
    if((ep=net_dbpltp->vec_id)!=0) {
      //This is a DrawTo --
      if(GOTO_VEC_RTN((ep+1)>>1)) break;
      
      //If the last operation was a MoveTo, set the station id and name of
      //the *previous* plot reccord --*/

      if(bMoveTo) {
        if(GOTO_PLT_RTN(rec-1)) break;
        memcpy(net_dbpltp->name,net_dbvecp->nam[ep&1],NET_SIZNAME);
        //Retieve name's original id --
        if(net_dbpltp->end_id=net_dbvecp->id[ep&1]) {
        	net_dbpltp->end_pfx=net_dbvecp->pfx[ep&1];
			/*Maintain coordinate range for all points but the zero datum --*/ 
			for(i=0;i<Dim;i++) {
				if(xyz[i]<xyzlim[i]) xyzlim[i]=xyz[i];
				if(xyz[i]>xyzlim[i+Dim]) xyzlim[i+Dim]=xyz[i];
			}
        }
        else net_dbnetp->flag=NET_FIXED_FLAG;
        dbf_Mark(net_dbplt);
        if(GOTO_PLT_RTN(rec)) break;
        bMoveTo=FALSE;
      }
      
      net_dbpltp->seg_id=net_dbvecp->seg_id;

	  //Added  10/23/2003 --
	  memcpy(net_dbpltp->date,net_dbvecp->date,3);
      
      i=net_dbvecp->str_id;
      if(!i) memcpy(net_dbvecp->e_xyz,net_dbvecp->xyz,sizeof(DIM_POINT));
      else {
        net_dbpltp->str_id=i;
        //Update string range for current component --
        if(!net_str_id1 || net_str_id1>i) net_str_id1=i;
        if(net_str_id2<i) net_str_id2=i;
      }
      
      /*could change this to adjusted length?*/
      net_lenttl+=net_dbvecp->id[0]?length3(net_dbvecp->xyz):0.0;
      net_vecttl++;
      if(net_status==1) {
        //This is the component's first vector. Store the datum name --
      	net_status++;
        memcpy(net_dbnetp->name,net_dbvecp->nam[ep&1],NET_SIZNAME);
        //Store the NTV rec # so that WALLS can retrieve the file and line number --
        net_dbnetp->vec_id=((ep+1)>>1);
      }  
        
      if(ep&1)
        for(i=0;i<Dim;i++) xyz[i]-=net_dbvecp->e_xyz[i];
      else
        for(i=0;i<Dim;i++) xyz[i]+=net_dbvecp->e_xyz[i];
    
      /*For possible later use, could point NTV record to "drawto" record in NTP file --
      net_dbvecp->plt_id=rec;
      dbf_Mark(net_dbvec);
      */
    }
    else bMoveTo=TRUE;

    if(net_dbpltp->end_id) {
      //This is an endpoint we may need to save for later reference as opposed to
      //an intermediate string point or a termination of a dead-end string.
      //Either insert a new endpoint or confirm this is a closure --*/

      comp_node=net_dbpltp->end_id;
      p=(lnptr *)trx_Binsert((int)(endp+endpts),TRX_DUP_NONE);

      if(!trx_binMatch) {
        //A new endpoint was found (endpts was incremented) --
        
        if(bMoveTo) {
          //This is a MoveTo establishing a new endpoint *and* a new connected component.
          //The current NTN record (if any) is completed and a new record is appended.
          
          if(net_status) {
            /*finish previous record*/
            for(i=0;i<2*Dim;i++) net_dbnetp->xyzmin[i]=xyzlim[i];
            net_dbnetp->vectors=net_vecttl;
            net_dbnetp->length=net_lenttl;
            net_dbnetp->str_id1=net_str_id1;
            net_dbnetp->str_id2=net_str_id2;
            net_dbnetp->plt_id2=rec-1;
            net_dbnetp->endpts=endpts-1;
            //Send new component's statistics to WALLS --
            if(update_ntp_CB()) break;
          }
          
          FCHKN(dbf_AppendZero(net_dbnet));
          net_dbnetp->flag=NET_COMPONENT_FLAG;
          net_dbnetp->plt_id1=rec;
          net_status=1;
          net_str_id1=net_str_id2=net_vecttl=0;
          net_lenttl=0.0;
          memcpy(xyzlim,dbl_limits,2*Dim*sizeof(double));
          memset(xyz,0,sizeof(DIM_DPOINT));
          endpts=1;
          pendp[0]=(int)endp;
        }
        
        for(i=0,j=Dim*(endpts-1);i<Dim;) pxyz[j++]=xyz[i++];
        endp[endpts-1]=comp_node;
        net_dbpltp->end_pt=endpts-1;
      }
      else {
        /*An old endpoint is revisited --*/
        j=Dim*(net_dbpltp->end_pt=*p-endp);
        for(i=0;i<Dim;) xyz[i++]=pxyz[j++];
        /*No need to label station --*/
        net_dbpltp->flag=NET_REVISIT_FLAG;
        /*ep=0;*/
      }
    }
    else {
		//This DrawTo either terminates a dead-end string or establishes
		//an intermediate string point --
		if(bMoveTo) {
			//Should be impossible --
			break;
		}
		net_dbpltp->end_pt=0;
		net_dbpltp->flag=NET_RESOLVE_FLAG;
	}	
    
	for(i=0;i<Dim;i++) net_dbpltp->xyz[i]=xyz[i];
    
    if(!bMoveTo) {
      /*Set station names and flags --*/
      memcpy(net_dbpltp->name,net_dbvecp->nam[ep=1-(ep&1)],NET_SIZNAME);
      //Retieve name's original id --
      if(net_dbpltp->end_id=net_dbvecp->id[ep]) {
        net_dbpltp->end_pfx=net_dbvecp->pfx[ep];
		/*Maintain coordinate range for all points but the zero datum --*/ 
		for(i=0;i<Dim;i++) {
			if(xyz[i]<xyzlim[i]) xyzlim[i]=xyz[i];
			if(xyz[i]>xyzlim[i+Dim]) xyzlim[i+Dim]=xyz[i];
		}
      }
      else net_dbnetp->flag=NET_FIXED_FLAG;
    }
    //else we will obtain this info from the next DrawTo record
    
    dbf_Mark(net_dbplt);
  }
  
  dos_FreeNear(pendp);
  dos_FreeHuge((HPVOID)pxyz);
  
  if(rec<=lastrec) {
    if(!net_errno) {
      net_errno=NET_ErrWork;
      free_resources();
    }
    return net_errno;
  }
  
  /*Sanity check --*/
  if(net_status!=2 || dbf_NumRecs(net_dbnet)!=(DWORD)net_numnets) CHK_ABORT(NET_ErrSysConnect);
  
  /*finish last record of component database (NTN) --*/
  for(i=0;i<2*Dim;i++) net_dbnetp->xyzmin[i]=xyzlim[i];
  net_dbnetp->vectors=net_vecttl;
  net_dbnetp->length=net_lenttl;
  net_dbnetp->str_id1=net_str_id1;
  net_dbnetp->str_id2=net_str_id2;
  net_dbnetp->plt_id2=lastrec;
  net_dbnetp->endpts=endpts;
  
  if(update_ntp_CB()) return net_errno;
  pltopen=FALSE;
  FCHKP(dbf_Close(net_dbplt));
  
  return 0;
}

int PASCAL net_Adjust(char FAR *ntvpath,NET_DATA FAR *pData,TRXAPI_CB pCB)
{
  zero_resources();

  if(pData->version!=NET_VERSION) CHK_ABORT(NET_ErrVersion);

  net_pCB=pCB;
  net_pData=pData;

  if(open_dbvec(ntvpath)) return net_errno;

  if(net_analyze()) return net_errno;

  /*Adjust each system in turn --*/
  if(net_solve()) return net_errno;

  /*net_nc = Total number of closures minus floating traverses*/
  if(net_nc!=net_closures) CHK_ABORT(NET_ErrNcCompare);

  if(update_ntv()) return net_errno;
  
  stropen=FALSE;
  FCHKS(dbf_Close(net_dbstr));

  /*Opens NTN and NTP files, closes NTP file --*/
  /*Makes NET_PostNetwork callbacks*/
  if(update_ntp()) return net_errno;

  vecopen=FALSE;
  FCHKV(dbf_Close(net_dbvec));
  
  #ifdef _POSTUPDATENTV_CB
  NET_CB(NET_PostUpdateNTV);
  #endif

  if(net_numstr) {
    /*Reopens NTS file, closes NTN and NTS files --*/
    if(sort_strings()) return net_errno;
    #ifdef _POSTUPDATENTS_CB
    NET_CB(NET_PostUpdateNTS);
    #endif
  }
  
  free_resources();
  return 0;
}
