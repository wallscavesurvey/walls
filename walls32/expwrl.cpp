// expwrl.cpp : CSegView::ExportWRL() implementation
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "segview.h"
#include "compile.h"
#include "wall_wrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define LEN_RGB 20

typedef	std::vector<FLTPT> VEC_FLTPT;

class CExport {
	UINT rec,lastrec;
	float fScaleV;
	int len_pid,max_id,max_rgb,rgb_count;
	int next,end_id,from_id;
	int FAR *pid;
	int FAR *pseg;
	COLORREF rgb[LEN_RGB];
	VEC_FLTPT vwpt;
	
public:
	//CSegView *pSV;
	double xyz[3];
	double xyz_minmax[6];
	double xy_offset[2];
	UINT flags; //WRL_F_xx flags in wall_wrl.h
	UINT dimGrid;
	BOOL bScaleV,bOffset;
	
    ~CExport();
	CExport(float fScaleVert,UINT uFlags,UINT uDimGrid);	
	BOOL Init(UINT maxids,UINT maxsegs);
	BOOL GetPoint();
	int GetLrudPt(WALL_TYP_LRUDPT *pL);
	int GetWallPt(UINT rec,NTA_LRUDTREE_REC &lrud);
	BOOL GetColor(COLORREF FAR *c);
	BOOL GetIndex(int FAR *i);
	void CheckRanges();
};

static CExport *pex;

CExport::CExport(float fScaleVert,UINT uFlags,UINT uDimGrid)
{
    bScaleV=(fScaleVert!=(float)1.0);
    fScaleV=fScaleVert;
	rec=(UINT)pNTN->plt_id1;
	lastrec=(UINT)pNTN->plt_id2;
	end_id=from_id=max_id=max_rgb=next=0;
	rgb_count=-1;
	flags=uFlags;
	dimGrid=uDimGrid;
	if(!(uFlags&WRL_F_COLORS)) rgb[max_rgb++]=RGB_WHITE;
	for(int i=0;i<3;i++) {
	  xyz_minmax[i]=DBL_MAX;
	  xyz_minmax[i+3]=-DBL_MAX;
	}
	bOffset=FALSE;
    pid=pseg=NULL;
}

CExport::~CExport()
{
    if(pseg) free(pseg);
	if(pid) free(pid);
}

BOOL CExport::Init(UINT maxids,UINT maxsegs)
{
	if(!(pid=(int FAR *)calloc(len_pid=maxids,sizeof(int))) ||
	   !(pseg=(int FAR *)calloc(maxsegs,sizeof(int)))) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
		return FALSE;
	}
	return TRUE;
}	

void CExport::CheckRanges()
{
	if(!bOffset) {
		bOffset=TRUE;
		memcpy(xy_offset,xyz,sizeof(xy_offset));
	}
	xyz[0]-=xy_offset[0];
	xyz[1]-=xy_offset[1];
	for(int i=0;i<3;i++) {
	  if(xyz[i]<xyz_minmax[i]) xyz_minmax[i]=xyz[i];
	  if(xyz[i]>xyz_minmax[i+3]) xyz_minmax[i+3]=xyz[i];
	}
}

int CExport::GetWallPt(UINT rec,NTA_LRUDTREE_REC &lrud)
{
	//if rec!=0 fill vwpt vector, return size
	//else return ray count only
	FLTPT wpt;
	bool bVert=(lrud.az==LRUD_BLANK);
	double az=bVert?0.0:(lrud.az*(PI/180.0));

	if(lrud.dim[0]==LRUD_BLANK_NEG) {
		if(rec) {
			wpt=*(FLTPT *)pPLT->xyz;
			if(!bVert) {
				ASSERT(lrud.dim[1]!=LRUD_BLANK);
				az=lrud.az*(PI/180.0); //radians
				wpt.xyz[0]+=lrud.dim[1]*sin(az);
				wpt.xyz[1]+=lrud.dim[1]*cos(az);
			}
			else {
				ASSERT(lrud.dim[1]==0.0);
			}
			if(lrud.dim[2]!=0) {
				ASSERT(lrud.dim[3]==0);
				wpt.xyz[2]+=lrud.dim[2];
			}
			else {
				wpt.xyz[2]-=lrud.dim[3];
			}
			vwpt.push_back(wpt);
		}
		return 1;
	}

	int count=0;
    if(!bVert) {
		if(lrud.dim[0]!=LRUD_BLANK) {
			//get left ray --
			if(rec) {
				wpt=*(FLTPT *)pPLT->xyz;
				wpt.xyz[0]+=lrud.dim[0]*sin(az-PI2);
				wpt.xyz[1]+=lrud.dim[0]*cos(az-PI2);
				vwpt.push_back(wpt);
			}
			count++;
		}
		if(lrud.dim[1]!=LRUD_BLANK) {
			//get right ray --
			if(rec) {
				wpt=*(FLTPT *)pPLT->xyz;
				wpt.xyz[0]+=lrud.dim[1]*sin(az+PI2);
				wpt.xyz[1]+=lrud.dim[1]*cos(az+PI2);
				vwpt.push_back(wpt);
			}
			count++;
		}
	}

	if(lrud.dim[2]!=LRUD_BLANK) {
		if(rec) {
			wpt=*(FLTPT *)pPLT->xyz;
			wpt.xyz[2]+=lrud.dim[2];
			vwpt.push_back(wpt);
		}
		count++;
	}

	if(lrud.dim[3]!=LRUD_BLANK) {
		if(rec) {
			wpt=*(FLTPT *)pPLT->xyz;
			wpt.xyz[2]-=lrud.dim[3];
			vwpt.push_back(wpt);
		}
		count++;
	}

	return count;
}

int CExport::GetLrudPt(WALL_TYP_LRUDPT *pL)
{
	NTA_LRUDTREE_REC lrud;

	static char key[6];
	static int iNewKey,raycount,nLruds;

    if(!pL) {
	   memset(pid,0,len_pid*sizeof(int));
	   rec=(UINT)pNTN->plt_id1;
	   *key=3;
	   next=-1;
	   iNewKey=1;
	   return 1;
	}

_loop:
	if(!iNewKey)
		goto _loop0; //next ray for this station

	if(rec>lastrec || dbNTP.Go(rec)) return 0;

	if(!memcmp(pPLT->name,"4N5 ",4) || !memcmp(pPLT->name,"4N14 ",5)) {
		int jj=0;
	}

	if(!(end_id=pPLT->end_id) || pid[end_id] || !pSV->LstInclude(pPLT->seg_id) && (pid[end_id]=1)) {
		rec++;
		goto _loop; //next record
	}
	//does this station have LRUDs?
	pid[end_id]=1; //flag station as processed
	raycount=nLruds=0;

	*(UINT *)(key+1)=end_id;
	
	//scan lruds for this station
_loop0:
	if(raycount) goto _loop1;

	int iTreeSav=ixSEG.GetTree();
	ixSEG.UseTree(NTA_LRUDTREE);
	if(iNewKey?(ixSEG.Find(key)):(ixSEG.FindNext(key))) {
		iNewKey=1;
		rec++;
		ixSEG.UseTree(iTreeSav);
		goto _loop; //next record
	}

	//station has at least one lrud ray --
	ASSERT(ixSEG.SizRec()<=sizeof(NTA_LRUDTREE_REC));
	VERIFY(!ixSEG.GetRec(&lrud,ixSEG.SizRec()));
	ixSEG.UseTree(iTreeSav);
	
	if(pL->pwpt) {
		//filling Coordinate3 array --
		vwpt.clear();  //for storing wall points for one wall pt or up to 4 lrud rays 
		if(!(raycount=GetWallPt(rec,lrud))) { //fills vwpt, returns size
			iNewKey=0;
			goto _loop0;
		}
		if(!nLruds) {
			nLruds++;
			iNewKey=0;
			if(pL->ispt) {
				memcpy(xyz,pPLT->xyz,3*sizeof(double));
				CheckRanges();
				if(bScaleV) xyz[2] *= (double)fScaleV;
				*pL->pwpt=*(FLTPT *)xyz;
				return 1;
			}
		}
		goto _loop1;
	}

	//filling coordinate indexes only --
	if(!(raycount=GetWallPt(0,lrud))) { //returns rays remaining
		iNewKey=0;
		goto _loop0;
	}
	if(!nLruds) {
		nLruds++;
		pL->ispt=++next; //sta index in Coordinate3 array
		iNewKey=0;
	}

_loop1:
	if(pL->pwpt) {
		raycount--;
		*(FLTPT *)xyz=vwpt[raycount];
		//memcpy(xyz,&vwpt[raycount],3*sizeof(double));
		CheckRanges();
		if(bScaleV) xyz[2] *= (double)fScaleV;
		*pL->pwpt=*(FLTPT *)xyz;
	}
	else {
		raycount--;
		pL->iwpt=++next;
	}
	return 1;
}

BOOL CExport::GetPoint()
{
	if(next) {
	  if(next==1) goto _next1;
	  goto _next2;
	}

_loop:
	if(rec>lastrec || dbNTP.Go(rec)) return FALSE;
				  
	//for(rec=(UINT)pNTN->plt_id1;rec<=lastrec;rec++,from_id=end_id) {
	if(!(end_id=pPLT->end_id)) goto _cont;
	if(!from_id || !pPLT->vec_id || !pSV->LstInclude(pPLT->seg_id)) goto _save; //styletyp *st=(pSV->m_pSegments+segid)->Style()
	styletyp *st=(pSV->m_pSegments+pPLT->seg_id)->Style();
	if(st->LineIdx()==CSegView::PS_NULL_IDX) {
		goto _save;
	}
	
	//A visible vector is found --
	rgb_count=0;
	
	if(!pid[from_id]) {
	    pid[from_id]=++max_id;
	    next=1;
	    return TRUE;
	}
	
_next1:
	if(!pid[end_id]) {
		//ExportPoint(&file,pPLT->xyz,xyz_minmax,max_id);
		pid[end_id]=++max_id;
		memcpy(xyz,pPLT->xyz,3*sizeof(double));
		if(bScaleV) xyz[2] *= (double)fScaleV;
		next=2;
		return TRUE;
	}

_next2:
	if(!(flags&WRL_F_COLORS)) pseg[pPLT->seg_id]=1;
	else if(!pseg[pPLT->seg_id]) {
		COLORREF c=pSV->SegColor(pPLT->seg_id);
		int i;
		for(i=0;i<max_rgb;i++) if(rgb[i]==c) break;
		if(i==max_rgb) {
		   if(i==LEN_RGB) i--;
		   else rgb[max_rgb++]=c;
		}
		pseg[pPLT->seg_id]=i+1;
	}
	
_save:  
	if(next<2) {
	  memcpy(xyz,pPLT->xyz,3*sizeof(double));
	  if(bScaleV) xyz[2] *= (double)fScaleV;
	}  
	next=0;
	
_cont:
	from_id=end_id;
	rec++;
	goto _loop;		  
}

BOOL CExport::GetColor(COLORREF FAR *pc)
{
	if(rgb_count>=max_rgb) return FALSE;
	//Initialize for next GetIndex() pass --
	next=0;
	max_id=from_id=0;
	rec=(UINT)pNTN->plt_id1;
	*pc=rgb[rgb_count++];
	return TRUE;
}

BOOL CExport::GetIndex(int FAR *pi)
{
	if(next) {
	  if(next==1) goto _next1;
	  goto _next2;
	}

_loop:
	if(rec>lastrec || dbNTP.Go(rec)) return FALSE;
	end_id=pPLT->end_id;
	if(!end_id || !from_id || !pPLT->vec_id || rgb_count!=pseg[pPLT->seg_id]) {
		from_id=end_id;
		rec++;
		goto _loop;		  
	}
	 
	//NOTE: Uncomment this as soon as GLVIEW can properly handle polylines!!
	if(from_id==max_id) goto _next2;
	
	if(max_id) {
		//Terminate polyline --
		*pi=-1;
		next=1;
		return TRUE;
	}
	
_next1:
	//write from station ID --
	*pi=pid[from_id]-1;
	next=2;
	return TRUE;
	
_next2:
	*pi=pid[end_id]-1; 
	next=0;
	from_id=max_id=end_id;
	rec++;
	return TRUE;
}

int PASCAL wall_GetData(int typ,LPVOID pData)
{
	switch(typ) {
	  case WRL_INDEX:
	  			return pex->GetIndex((int FAR *)pData);
	  
	  case WRL_POINT:
	  			if(!pex->GetPoint()) return FALSE;
				pex->CheckRanges();
	  			memcpy(pData,pex->xyz,3*sizeof(double));
	  			break;
	  			
	  case WRL_COLOR:
	  			return pex->GetColor((COLORREF FAR *)pData);
	  			
	  case WRL_BACKCOLOR:
	  			*(COLORREF FAR *)pData=(pex->flags&WRL_F_COLORS)?pSV->BackColor():RGB_BLACK;
	    		break;
		   
	  case WRL_LRUDCOLOR:
	  			*(COLORREF FAR *)pData=pSV->LrudColor();
	    		break;
		   
	  case WRL_LRUDPTCOLOR:
	  			*(COLORREF FAR *)pData=pSV->FloorColor();
	    		break;
		   
	  case WRL_RANGE:
	 			memcpy(pData,pex->xyz_minmax,6*sizeof(double));
	 			break;
	 			
	  case WRL_FLAGS:
	  			*(UINT FAR *)pData=pex->flags;
	  			break;

	  case WRL_LRUDPT:
				return pex->GetLrudPt((WALL_TYP_LRUDPT *)pData);
	 			
	  case WRL_DIMGRID:
	  			*(UINT FAR *)pData=pex->dimGrid;
	  			break;
	}
	
	return TRUE;
} 

BOOL CSegView::ExportWRL(LPCSTR pathname,LPSTR title,UINT flags,UINT dimGrid,float fScaleVert)
{
	CExport ex(fScaleVert,flags,dimGrid);
	int e;
	pex=&ex;
	
	if(!ex.Init(dbNTV.MaxID()+1,(UINT)ixSEG.NumRecs())) return FALSE;
	
	char buffer[120];
	if(ex.bScaleV &&
	_snprintf(buffer,120,"%Fs (Vert x %.1f)",title,fScaleVert)<80) title=buffer;
	
	e=WrlExport(pathname,title,wall_GetData);
 	  
	if(e>1) {  //e==1 implies user abort
	    CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPERRMSG1,WrlErrMsg(e));
	}

 	return e==0;
}
