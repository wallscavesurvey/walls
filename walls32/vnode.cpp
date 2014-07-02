#include "stdafx.h"           
#include "vnode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CVNode::CVNode()
{
	vnod=NULL;
	vstk=NULL;
}

CVNode::~CVNode()
{
	free(vstk);
	free(vnod);
}

BOOL CVNode::Init(int xM,int yM,int maxvecs)
{
	//xM,yM = plot coordinates of frame's center --
	//numvecs = maximum number of vectors to be located

	if(vnod) return TRUE;

	vnod_siz=maxvecs+NUM_HDR_NODES;
	if(!(vnod=(MAP_VECNODE *)calloc(vnod_siz,sizeof(MAP_VECNODE))) ||
		!(vstk=(int *)malloc(vnod_siz*sizeof(int)))) {
		free(vnod);
		vnod=NULL;
		return FALSE;
	}

	xM>>=1;	//work with midpoint
	yM>>=1;

	//Initialize 7 tree header nodes --

	MAP_VECNODE *pn=vnod;
	pn->B_xy[0]=xM;
	pn->B_xy[1]=yM;
	pn->lf=1; pn->rt=2;
	pn++; //1
	pn->B_xy[0]=xM/2;
	pn->B_xy[1]=yM;
	pn->lf=3; pn->rt=4;
	pn++; //2
	pn->B_xy[0]=xM+xM/2;
	pn->B_xy[1]=yM;
	pn->lf=5; pn->rt=6;
	pn++; //3
	pn->B_xy[0]=xM/2;
	pn->B_xy[1]=yM/2;
	pn->lf=pn->rt=-1;
	pn++; //4
	pn->B_xy[0]=xM/2;
	pn->B_xy[1]=yM+yM/2;
	pn->lf=pn->rt=-1;
	pn++; //5
	pn->B_xy[0]=xM+xM/2;
	pn->B_xy[1]=yM/2;
	pn->lf=pn->rt=-1;
	pn++; //6
	pn->B_xy[0]=xM+xM/2;
	pn->B_xy[1]=yM+yM/2;
	pn->lf=pn->rt=-1;

	vnod_len=NUM_HDR_NODES;
	for(int n=0;n<NUM_HDR_NODES;n++) {
		vnod[n].M_xy[0]=vnod[n].B_xy[0];
		vnod[n].M_xy[1]=vnod[n].B_xy[1];
	}
	return TRUE;
}

static int dist_to_vec(int px,int py,int xb,int yb,int xr,int yr)
{
	//return squared dist of point (px,py) to vector.
	//Vector is defined as (xb,yb)+t*(xr,yr), 0<=t<=1
	int l,t;

	px-=xb;
	py-=yb;
	t=xr*px+yr*py;
	if(t>0) {
		l=xr*xr+yr*yr;
		if(t>=l) {
			px-=xr;
			py-=yr;
		}
		else {
			double dy=(double)t/l;
			double dx=(double)px-dy*xr;
			dy=(double)py-dy*yr;
			return (int)(dx*dx+dy*dy);
		}
	}
	return px*px+py*py;
}

static int dist_to_midpoint(int x,int y,MAP_VECNODE *pn)
{
	int dx=x-(pn->B_xy[0]+pn->M_xy[0])/2;
	int dy=y-(pn->B_xy[1]+pn->M_xy[1])/2;
	return dx*dx+dy*dy;
}

static int max_dist_to_vec(int px,int py,int xf,int yf,int xt,int yt)
{
	//return max squared dist of point (px,py) to vector.
	int dx,dy,dmax;

	dx=xf-px;
	dy=yf-py;
	dmax=dx*dx+dy*dy;
	dx=xt-px;
	dy=yt-py;
	dy=dx*dx+dy*dy;
	return (dy>dmax)?dy:dmax; 
}

BOOL CVNode::AddVnode(int xf,int yf,int xt,int yt,int pltrec)
{
	MAP_VECNODE *pn;
	int slevel;
	int ichild,idx;
	int divider,midp[2];

	midp[0]=(xf+xt)/2;
	midp[1]=(yf+yt)/2;

	vstk[0]=slevel=0;
	pn=&vnod[0];

	while(TRUE) {
		idx=(slevel&1);
		divider=(pn->B_xy[idx]+pn->M_xy[idx])/2;
		if(midp[idx]<divider) ichild=pn->lf;
		else ichild=pn->rt;
		if(ichild==-1) {
			if(vnod_len>=vnod_siz) return FALSE;
			if(midp[idx]<divider) pn->lf=vnod_len;
			else pn->rt=vnod_len;
			pn=&vnod[vnod_len++];
			break;
		}
		pn=&vnod[ichild];
		vstk[++slevel]=ichild;
	}
	
	pn->B_xy[0]=xf; pn->B_xy[1]=yf; 
	pn->M_xy[0]=xt; pn->M_xy[1]=yt;

	//Set maxdist to the squared distance from the vector's midpoint to an endpoint --
	pn->maxdist=((xt-xf)*(xt-xf)+(yt-yf)*(yt-yf))/4+1;

	pn->lf=pn->rt=-1;
	pn->pltrec=pltrec;

	//Now update maxdist in ancestor nodes, the computed value
	//being the maximum squared distance between this vector (any point within)
	//and the midpoint of an ancestor's vector.
	for(;slevel>=0;slevel--) {
		pn=&vnod[vstk[slevel]];
		divider=max_dist_to_vec((pn->B_xy[0]+pn->M_xy[0])/2,
			(pn->B_xy[1]+pn->M_xy[1])/2,xf,yf,xt,yt);
		if(divider>pn->maxdist) pn->maxdist=divider;
	}
	return TRUE;
}

MAP_VECNODE * CVNode::GetVecNode(int x,int y)
{
	//return vnod[] addr for vector closest to (x,y) --
	int dmid,ilast,slevel;
	MAP_VECNODE *pn;

	ASSERT(vstk && vnod);

	vstk[0]=slevel=ilast=0;

	while(slevel>=0) {
		pn=&vnod[vstk[slevel]];
		//Was the last node visited a child of this one?
		if(pn->lf==ilast || pn->rt==ilast) {
			if(pn->rt==ilast || pn->rt==-1) {
				//finished with this node --
				ilast=vstk[slevel--];
				continue;
			}
			//Finally, examine the right child --
			pn=&vnod[vstk[++slevel]=pn->rt];
		}
		//otherwise it was this node's parent, so examine this node --
		ilast=vstk[slevel];

		//Now examine pn's vector where pn=&vnod[ilast=vstk[slevel]] --

		dmid=dist_to_midpoint(x,y,pn);

		if(pn->maxdist>=dmid || //helps significantly!

			sqrt((double)dmid)-sqrt((double)pn->maxdist)<=(double)VNODE_NEAR_LIMIT_SQRT) {

			//Search this node and its children --
			if(ilast>=NUM_HDR_NODES) {
				dmid=dist_to_vec(x,y,pn->B_xy[0],pn->B_xy[1],pn->M_xy[0]-pn->B_xy[0],pn->M_xy[1]-pn->B_xy[1]);
				if(dmid<=VNODE_NEAR_LIMIT) {
					return pn;
				}
			}
			if((vstk[++slevel]=pn->lf)>0 || (vstk[slevel]=pn->rt)>0) continue;
			slevel--;
		}
		//We are finished with this node --
		slevel--;
	}
	return NULL; //distance to nearest vector --
}


