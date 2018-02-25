//Wallnet.c

#include <windows.h>
#include <string.h>
#include <limits.h>
#include "wnetlib.h"
#ifdef _USE_DFSTACK
#include <malloc.h> 
#endif
#include <trx_str.h>
#include <dos_io.h>

static int     net_numnods,net_numvecs,net_isolated;
static UINT    numendp;
static int     numnc,numsysstr,numsysnod,numstr;
static BOOL	   str_fst;

enum e_typstr {Normal,Reversed,Merged};

/*====================================================================*/
/* Routines for initial scan of vector database                       */
/*====================================================================*/
static void _inline add_vendp(int n)
{
  n=net_dbvecp->id[n];
  if(n>=net_numnods) net_numnods=n+1;
  net_vendp[numendp++]=n;
}

static apfcn_i read_vecs()
{
  UINT i,maxvecs;

  if((maxvecs=(UINT)dbf_NumRecs(net_dbvec))>MAX_VECS)
    CHK_ABORT(NET_ErrMaxVecs);
  
  CHK_MEMH(net_vendp,(DWORD)maxvecs*2*sizeof(DWORD));

  net_numvecs=net_numnods=0;
  numendp=0;
   
  for(i=1;i<=maxvecs;i++) {
	GOTO_VEC(i);
	net_numvecs++;
	add_vendp(0);
	add_vendp(1);
  }
  
  if(!net_numvecs) CHK_ABORT(NET_ErrEmpty);

  return 0;
}

static apfcn_v fix_valist(void)
{
  UINT i;
  int j;
  
  //net_node and net_degree are zero-initialized --

  /*net_degree[i] is set to the number of connecting vectors, which
    will be the length of this node's adjacency list in net_valist[].*/
  for(i=0;i<numendp;i++) {
    j=net_vendp[i];
    if(j>=0) net_degree[j]++;
  }

  /*Set up adjacency list pointers. Note that the first node will have
    degree zero if it's an unused control point --*/
  net_isolated=0;
  net_node[0]=0;
  for(j=0;j<net_numnods;j++) {
	if(!(i=net_degree[j])) net_isolated++;
	net_node[j+1]=net_node[j]+i;
  }

  /*Now fill the adjacency list. Each entry is an index to net_vendp[], which
    in turn contains the corresponding adjacent node's index in net_node[].
    Note that net_vendp[i] and net_vendp[i^1] are indices of adjacent nodes --*/

  for(i=0;i<numendp;i++) {
    j=net_vendp[i];
    if(j>=0) net_valist[net_node[j]+--net_degree[j]]=i^1;
  }

  /* Restore the net_degree[] counters --*/
  for(j=0;j<net_numnods;j++) net_degree[j]=net_node[j+1]-net_node[j];
}

/*==================================================================*/
/* Routines for adding system found by df_search()                  */
/*==================================================================*/

/*The following add_system() routines support string merging to
  eliminate any unnecessary string junctions. As a result, all
  string endpoints in multi-string systems will have degree > 2.*/

static apfcn_i next_str(int fnod,int s)
{
  int i;
  UINT e;

  for(i=net_stindex;i<net_numstr;i++) {
    if(i==s) continue;
    e=net_string[i];
    if(e!=(UINT)-1 && (net_vendp[e]==fnod || net_vendp[e^1]==fnod)) goto _ex;
  }
  /*impossible error*/
  net_errno=NET_ErrSystem;
_ex:
  return i;
}

static apfcn_v add_string(int s)
{
  UINT e;
  int fnod,rev;

  /*Add string containing vector with endpoints net_vendp[e^1] and net_vendp[e]
    to the system, where e=net_string[s]. If necessary, add additional
    "merged" strings if either endpoint has degree 2.*/

  e=net_string[s];

_start:
  rev=net_degree[net_vendp[e^1]]<=2 && net_degree[net_vendp[e]]>2;
  fnod=net_vendp[e^(rev^1)];

  /*Find a starting endpoint with degree>2. If there are none,
    set degree=3 for the first endpoint. rev==1 indicates
    backward traversal.*/

  if(net_degree[fnod]<=2) {
    net_degree[fnod]=1;
    while(TRUE) {
      fnod=net_vendp[e^rev];
      if(net_degree[fnod]>2) goto _start;
      if(net_degree[fnod]==1) {
        net_degree[fnod]=3;
        goto _start;
      }
      e=net_string[s=next_str(fnod,s)];
      rev=(fnod!=net_vendp[e^1]);
    }
  }

  /*Now add one string to the system, or more than one if an
    endpoint of degree<3 is encountered.*/

  while(TRUE) {
    net_string[s]=(DWORD)-1;
    sys[numsysstr++]=(e<<2)+rev;
    fnod=net_vendp[e^(rev&Reversed)];
    if(net_degree[fnod]>2) break;
    e=net_string[s=next_str(fnod,s)];
    rev=Merged+(fnod!=net_vendp[e^1]);
  }
}

static apfcn_v inc_degree(UINT e)
{
  if(0==net_degree[net_vendp[e]]++) numsysnod++;
}

apfcn_v add_system(void)
{
  int s;
  UINT e;

  /*The (net_numstr-net_stindex) strings in array net_string[] belong to an
    independent loop system. Unfortunately, some of the string endpoints
    may be unnecessary (have degree 2 within the system) because they
    are attachment points to other parts of the network. Thus, we will
    go to the trouble of merging adjacent strings when possible as we
    add them to the sys[] array. This is accomplished in add_string()
    by setting flag bits in the corresponding array typstr[] to indicate
    whether a string is reversed and/or merged with the previous
    string.*/

  /*We no longer need nodlow[] for the system nodes (except for the
    initial attachment point, whose nodlow[] value the caller saved).
    We can now reuse this array as net_degree[]. We can also revise
    net_string[] to contain net_vendp[] indices rather than pointers to same.*/

  /*Check if remaining space is sufficient --*/
  if(net_numsys>=MAX_SYSTEMS) {
    net_errno=NET_ErrMaxSystems;
    return;
  }

  for(s=net_stindex;s<net_numstr;s++) {
    net_string[s]=e=net_valist[net_string[s]];
    net_degree[net_vendp[e]]=net_degree[net_vendp[e^1]]=0;
  }

  /*Compute net_degree[] while counting nodes for "largest system" report*/

  numsysnod=0;
  for(s=net_stindex;s<net_numstr;s++) {
    e=net_string[s];
    inc_degree(e);
    inc_degree(e^1);
  }
  sysstr[net_numsys++]=numsysstr;

  /*Now add the strings to sys[]. If add_string() finds an endpoint
    with degree 2 in a multi-string system, it will add more than one
    string, setting their Merged and Reversed flags as necessary.*/

  for(e=0,s=net_stindex;s<net_numstr;s++)
    if(net_string[s]==(DWORD)-1) numsysnod--; /*if merged, reduce node count*/
    else {
      add_string(s);
      e++; /*for computing maxsysstr below*/
    }
  numstr+=(int)e;

  /*This is required for workspace allocation and "largest" system report.*/
  if(e>(UINT)net_maxstrsolve) net_maxstrsolve=(int)e;
  if(numsysnod>net_maxsysnod) {
    net_maxsysid=net_numsys;
    net_maxsysnod=numsysnod;
    net_maxsysstr=(int)e;
  }
}

/*==================================================================*/
/* Find loop systems - extend_str(), df_search(), build_systems()   */
/*==================================================================*/
static apfcn_i ntp_addrec(int n,UINT ep)
{
  FCHKP(dbf_AppendZero(net_dbplt));
  dbf_Mark(net_dbplt);
  net_dbpltp->flag=' ';
  net_dbpltp->end_id=n+1;
  net_dbpltp->vec_id=(DWORD)(ep+1);
  return 0;
}

apfcn_i extend_str(UINT ep)
{
  int n;
  UINT a;

  /*Place starting point in NTP file as a "moveto" record --*/
    if(ntp_addrec(net_vendp[ep^1],(UINT)-1)) return -1;

  /*Find string endpoint, flagging intermediate nodes, and disable
    backward path. See df_search() below.*/

  while(TRUE) {
    n=net_vendp[ep];

    /*Place this vector endpoint in NTP file as a "drawto" record --*/
    if(ntp_addrec(-1,ep)) return -1;

    /*Find this node's corresponding valist entry --*/
    ep^=1;
    for(a=net_node[n+1];--a>=net_node[n];) if(net_valist[a]==ep) goto _chk;

    /*impossible to reach this point!*/
    net_errno=NET_ErrAlist;
    return -1;

_chk:
    if((ep=net_node[n+1]-net_node[n])!=2 || nodlow[n]) {
       /*Endpoint node found. Flag incoming route as being traversed --*/
       net_valist[a]=(DWORD)-1;
       /*A dead end string will not become a system string, nor
         do we need to save its endpoint in the NTP file --*/
       if(ep==1) {
         nodlow[n]=INT_MAX;
         numpltendp--;
       }
       else net_dbpltp->end_id=n+1;
       break;
    }
    /*flag this as a non-endpoint node and continue traversal*/
    nodlow[n]=-1;
    numpltendp--;
    if(a==net_node[n]) a++; else a--;
    ep=net_valist[a];
  }
  return n;
}

static apfcn_i build_systems(void)
{
  int n;
  UINT a;

  #ifdef _USE_DFSTACK
  int e;
  df_lenstack=DF_STACK_INITSIZE;
  if(!(df_stack=(df_frame *)malloc(df_lenstack*sizeof(df_frame))))
	 CHK_ABORT(NET_ErrNearHeap);
  #ifdef _DEBUG
  df_ptr_max=0;
  #endif
  #endif
  
  net_numstr=net_numsys=net_maxsysstr=net_maxsysnod=
  net_closures=net_maxstrsolve=numsysstr=net_numnets=numstr=0;
  memset(nodlow,0,net_numnods*sizeof(DWORD));
  
  for(n=0;n<net_numnods;n++) {
    if(net_node[n]<net_node[n+1]) {
      if(!nodlow[n]) { /*new network component*/
        nodlow[n]=1;
        
        #ifdef _USE_DFSTACK
            df_ptr=0;
	        df_nxtnode=n;
	        for(e=1;e;) {
	          e=df_search(e);
	        }
        #else
        	df_search(n);
        #endif
        
        if(net_errno) break;
        net_numnets++;
      }
    }
  }
  
  #ifdef _USE_DFSTACK
	free(df_stack);
  #endif
  
  if(net_errno) CHK_ABORT(net_errno);
    
  sysstr[net_numsys]=numsysstr;

  /*System s has strings sys[sysstr[s]..sysstr[s+1]-1]. We can now
   restore the attachments of string endpoints to intermediate nodes.*/

  for(n=0;n<net_numnods;n++) if(nodlow[n]==-1)
    for(a=net_node[n+1]-1;a>=net_node[n];a--) net_vendp[net_valist[a]^1]=n;
  return 0;
}

/*==================================================================*/
/* Routines to display results, create string database (NTS file).  */
/* and to update NTV file with string pointers.                     */
/*==================================================================*/

static apfcn_i go_vendp_vec(int rec)
{
  /*Build a pointer chain in the NTV file. Field fv_str_nxt_vc is the
    NTV record number of the next vector in the string, or zero
    if it is the last string vector.

    Therefore, if this vector is not the first one, we must update that
    field in the previous record before moving to the new database
    position. Otherwise, we store the NTV record number of the first
    string vector in the NTS file entry for this string. This will point
    to the first element of the chain, enabling us later to easily
    list all station names in a string.*/

    if(str_fst) net_dbstrp->vec_id=rec;
    else net_dbvecp->str_nxt_vc=rec;
    GOTO_VEC(rec);
    net_dbvecp->str_id=net_numstr;
    net_dbvecp->str_nxt_vc=0;  /*assume this is the last vector*/

    /*flags database buffer as "modified" --*/
    dbf_Mark(net_dbvec);
    return 0;
}

static apfcn_v add_vec_data(UINT e)
{
  dlnptr vp,sp;
  
  //Increment string vector count --
  net_dbstrp->vec_cnt++;
  
  net_dbstrp->flag |= (net_dbvecp->flag & (NET_STR_FLTTRAVH + NET_STR_FLTTRAVV));

  vp=(dlnptr)net_dbvecp->xyz;
  sp=(dlnptr)net_dbstrp->xyz;
  
  //Add vector displacement and variances to string displacement and variances --
  if(e&1) for(e=0;e<(UINT)(Dim+2);e++) *(sp++)+=*vp++;
  else  {
    for(e=0;e<(UINT)Dim;e++) *(sp++)-=*vp++;
    (*sp++)+=*vp++;
    *sp+=*vp;
  }
}

static apfcn_v add_nam_data(int s_to,UINT e)
{
  net_dbstrp->id[s_to]=net_dbvecp->id[e&1];
}

static apfcn_i out_string(int sysid,UINT e)
{
  int nvecs,n,rev;
  UINT ip;

  rev=sys[e]&3;

  if((rev&Merged)==FALSE) {
    FCHKS(dbf_AppendZero(net_dbstr));
    net_dbstrp->flag=NET_STR_NORMAL;
    dbf_Mark(net_dbstr);
    net_dbstrp->sys_id=sysid;
    str_fst=TRUE;
    net_numstr++;
  }
  rev&=Reversed;
  e=sys[e]>>2;

  /*string begins with vector net_vendp[e^1] to net_vendp[e] */
  nvecs=0;
  while(TRUE) {
    net_string[nvecs++]=e^rev;
    if(nodlow[net_vendp[e]]!=-1) break;
    ip=net_node[net_vendp[e]];
    if(net_vendp[net_valist[ip]]==net_vendp[e^1]) ip++;
    e=net_valist[ip];
  }
  if(rev) {
    n=nvecs;
    rev=-1;
  }
  else {
    n=-1;
    rev=1;
  }

  while(nvecs--) {
    e=net_string[n+=rev];
    /*e corresponds to the "TO" node in the string direction sense.
      Hence, e^1 corresponds to the "FROM" node. In the vector
      database, e is the "TO" station in record 1+(e>>1) only if it
      is odd (e&1==1), otherwise it is the "FROM" station.*/

    if(go_vendp_vec(1+(e>>1))) return net_errno;
    if(str_fst) {
      add_nam_data(0,e^1); /*store first endpoint name*/
      str_fst=0;
    }
    add_vec_data(e);
    /*in case this is the last vector --*/
    add_nam_data(1,e);     /*store last endpoint name*/
  }
  /*Set dynamic float flags --*/
  
  n=net_dbstrp->flag;
  if(n&NET_STR_FLTVECTH) net_dbstrp->flag|=NET_STR_FLOATH;
  if(n&NET_STR_FLTVECTV) net_dbstrp->flag|=NET_STR_FLOATV;
  return 0;
}

static apfcn_i out_systems(void)
{
  int s,n;

  net_numstr=0; /*Recalculate due to merging*/
  for(n=1;n<=net_numsys;n++) {
    for(s=sysstr[n-1];s<sysstr[n];s++) if(out_string(n,(UINT)s)) return net_errno; //*** chk rtn (10/23/04)
	//net_dbstrp->flag1|=NET_STR_SYSEND; //Only used for GetLinks test program.
  }
  return 0;
}

apfcn_i net_analyze(void)
{
  /*First, read vector database sequentially, creating a list of vector
    endpoints net_vendp[], which are indices to a node array, net_node[].*/

  if(read_vecs()) return net_errno;

  /*Globals now calculated --
    net_numnods  - total number of stations (max id[] plus 1)
    net_numvecs  - total number of vectors */

  #ifdef _POSTREADNTV_CB
  //Not used by WALLS --
  pData->nVectors=net_numvecs;
  pData->nNames=net_numnods;
  /*pData->nClosures=net_numvecs-net_numnods+1; Not valid for multiple components*/
  NET_CB(NET_PostReadNTV);
  #endif

  /*Set up far segments for workspace -- */
  if(net_numnods>MAX_NODES) CHK_ABORT(NET_ErrMaxNodes);

  /*Allocate space for net_node[] and net_degree[] --*/
  CHK_MEM(net_node,sizeof(DWORD)*(net_numnods+1));
  CHK_MEM(net_degree,sizeof(int)*net_numnods);

  /*Allocate space for complete adjacency list (2 indices per vector) --*/
  CHK_MEMH(net_valist,(DWORD)net_numvecs*2*sizeof(int));

  /*Create the adjacency list, net_valist[]. Each net_node[] element is an index
    pointing to a sublist in net_valist[]. An element of net_valist[], in turn,
    represents an adjacent node as an index to net_vendp[], which also
    identifies the connecting vector. net_degree[n]>=1 is the length of the
    adjacency list for net_node[n].*/

  fix_valist();
 
  /*At this point we could trim dead-end strings, which would remove from
    consideration nodes of degree one and shorten the adjacency lists of
    other nodes. net_maxstringvecs would be reduced to the number of remaining
    vectors. However, we'll not do this here since we want the depth-first search
    in build_systems() to establish a plotting sequence for the NTP file.*/

  net_maxstringvecs=net_numvecs<MAX_STRINGVECS?net_numvecs:MAX_STRINGVECS;
  #ifdef _DEBUG
    net_maxnumstr=0;
  #endif
  
  /*Allocate sys[] to store system string vector IDs and types. It
    could conceivably include all vectors in the network --*/

  CHK_MEM(sys,net_maxstringvecs*sizeof(*sys));
  CHK_MEM(net_string,net_maxstringvecs*sizeof(int));

  /*Create <pathname>.NTP which, when it is finally updated,
    will contain a coordinate sequence to facilitate fast plotting. The
    initial file write is done in build_systems() below. The depth-
    first search establishes the sequence. After vectors in the .NTV
    file are adjusted, function update_ntp() fills in the final
    coordinates and stores the min/max ranges in <pathname>.NTN*/

  if(create_dbplt()) return net_errno;

  /*Find independent systems: sysstr[], sys[] arrays --*/

  numpltendp=net_numnods;
  if(build_systems()) return net_errno;
  
  //if(3*numpltendp>=16*1024) CHK_ABORT(NET_ErrNearHeap);
  
  net_numnods-=net_isolated;

  /*Close the NTP file for now. It does not need to be open
    concurrently with the NTS file. It will be reopened in
    update_ntp()*/

  pltopen=FALSE;
  FCHKP(dbf_Close(net_dbplt));

  /*Globals now calculated --
    net_numnets     - number of connected components
    net_numstr      - total number of strings
    net_numsys      - total number of independent systems
    net_maxsysid    - index of system with largest node count
    net_maxsysstr   - number of strings in largest system
    net_maxsysnod   - number of nodes in largest system
    net_maxstrsolve - maximum number of strings in any system
    net_closures    - closures found in df_search
 */

  #ifdef _POSTCONNECT_CB
  /*Again, WALLS has no use for this. The status message would be quickly overwritten --*/ 
  pData->nNetworks=net_numnets;
  pData->nStrings=numstr;
  pData->nSystems=net_numsys;
  pData->nClosures=net_numvecs-net_numnods+net_numnets;
  pData->s_id=net_maxsysid;
  pData->s_nStrings=net_maxsysstr;
  pData->s_nNodes=net_maxsysnod;
  NET_CB(NET_PostConnect);
  #endif

  #ifdef _DEBUG
  /*To test our sanity, check closure count recomputed during df_search --*/
  if(net_closures!=net_numvecs-net_numnods+net_numnets) CHK_ABORT(NET_ErrNcCompare);
  #endif
  
  /*Now create <pathname>.NTS containing system string data --*/

  if(create_dbstr()) return net_errno;

  if(out_systems()) return net_errno;

  freefar(&net_string);
  freefar(&sys);
  freehuge(&net_valist);
  freefar(&net_degree);
  freefar(&net_node);
  freehuge(&net_vendp);

  /*net_numstr = total strings after merging recalculated by out_systems()*/
  #ifdef _POSTCREATENTS_CB
  pData->nStrings=net_numstr;
  NET_CB(NET_PostCreateNTS);
  #endif

  return 0;
}
