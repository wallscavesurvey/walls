#include "stdafx.h"           
#include "ptnode.h"
#include <malloc.h>
#include <math.h> 

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CPtNode::CPtNode()
{
	vnod=NULL;
	vstk=NULL;
}

CPtNode::~CPtNode()
{
	free(vstk);
	free(vnod);
}

BOOL CPtNode::Init(int xM,int yM,int maxvecs)
{
	//xM,yM = width,height of frame --
	//maxvecs = maximum number of vectors to be located

	if(!vnod || vnod_siz<maxvecs+NUM_HDR_NODES) {
		free(vnod);
		vnod_siz=maxvecs+NUM_HDR_NODES;
		if(!(vnod=(MAP_PTNODE *)malloc(vnod_siz*sizeof(MAP_PTNODE))) ||
			!(vstk=(int *)malloc(vnod_siz*sizeof(int)))) {
			free(vnod);
			vnod=NULL;
			return FALSE;
		}
	}

	xM>>=1;	//work with midpoint
	yM>>=1;

	//Initialize 7 tree header nodes --

	MAP_PTNODE *pn=vnod;
	pn->xy[0]=xM;
	pn->xy[1]=yM;
	pn->lf=1; pn->rt=2;
	pn++; //1
	pn->xy[0]=xM/2;
	pn->xy[1]=yM;
	pn->lf=3; pn->rt=4;
	pn++; //2
	pn->xy[0]=xM+xM/2;
	pn->xy[1]=yM;
	pn->lf=5; pn->rt=6;
	pn++; //3
	pn->xy[0]=xM/2;
	pn->xy[1]=yM/2;
	pn->lf=pn->rt=-1;
	pn++; //4
	pn->xy[0]=xM/2;
	pn->xy[1]=yM+yM/2;
	pn->lf=pn->rt=-1;
	pn++; //5
	pn->xy[0]=xM+xM/2;
	pn->xy[1]=yM/2;
	pn->lf=pn->rt=-1;
	pn++; //6
	pn->xy[0]=xM+xM/2;
	pn->xy[1]=yM+yM/2;
	pn->lf=pn->rt=-1;

	vnod_len=NUM_HDR_NODES;
	return TRUE;
}

static int __inline sq_dist_to_node(int x,int y,MAP_PTNODE *pn)
{
	x-=pn->xy[0];
	y-=pn->xy[1];
	return x*x+y*y;
}

BOOL CPtNode::AddPtNode(int x,int y,CShpLayer *pLayer,int rec)
{
	MAP_PTNODE *pn;
	int slevel;
	int ichild,idx;
	int divider,midp[2];

	midp[0]=x;
	midp[1]=y;

	vstk[0]=slevel=0;
	pn=&vnod[0];

	while(TRUE) {
		idx=(slevel&1);
		divider=pn->xy[idx];
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
	
	pn->xy[0]=midp[0]; 
	pn->xy[1]=midp[1]; 

	//Set maxdist to the squared distance from the vector's midpoint to an endpoint --
	pn->maxdist=1;

	pn->lf=pn->rt=-1;
	pn->rec=rec;
	pn->pLayer=pLayer;

	//Now update maxdist in ancestor nodes, the computed value
	//being the maximum squared distance between this vector (any point within)
	//and the midpoint of an ancestor's vector.
	for(;slevel>=0;slevel--) {
		pn=&vnod[vstk[slevel]];
		int d=sq_dist_to_node(x,y,pn);
		if(d>pn->maxdist) pn->maxdist=d;
	}
	return TRUE;
}

MAP_PTNODE * CPtNode::GetPtNode(int x,int y)
{
	//return vnod[] addr for point sufficiently close to (x,y) --
	int dmid,ilast,slevel;
	MAP_PTNODE *pn;

	if(!vstk || !vnod) return FALSE;

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

		dmid=sq_dist_to_node(x,y,pn);

		if(pn->maxdist>=dmid || //helps significantly!

			sqrt((double)dmid)-sqrt((double)pn->maxdist)<=(double)VNODE_NEAR_LIMIT_SQRT) {

			//Search this node and its children --
			if(ilast>=NUM_HDR_NODES && dmid<=VNODE_NEAR_LIMIT)
				return pn;

			if((vstk[++slevel]=pn->lf)>0 || (vstk[slevel]=pn->rt)>0) continue;
			slevel--;
		}
		//We are finished with this node --
		slevel--;
	}
	return NULL; //distance to nearest vector --
}


int CPtNode::GetVecPtNode(VEC_PTNODE &vec_pt,int x,int xBorder,int y,int yBorder)
{
	//return unsorted vector of points sufficiently close to (x,y) --
	int dmid,ilast,slevel;
	MAP_PTNODE *pn;

	vec_pt.clear();
	if(!vstk || !vnod) return 0;

	int near_limit_sqr=xBorder*xBorder+yBorder*yBorder;
	double near_limit_sqrt=sqrt((double)near_limit_sqr);

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

		dmid=sq_dist_to_node(x,y,pn);

		if(pn->maxdist>=dmid || //helps significantly!

			sqrt((double)dmid)-sqrt((double)pn->maxdist)<=near_limit_sqrt) {

			//Search this node and its children --
			if(ilast>=NUM_HDR_NODES && dmid<=near_limit_sqr) {
				if(abs(x-pn->xy[0])<=xBorder && abs(y-pn->xy[1])<=yBorder) {
					vec_pt.push_back(pn);

				}
			}

			if((vstk[++slevel]=pn->lf)>0 || (vstk[slevel]=pn->rt)>0) continue;
			slevel--;
		}
		//We are finished with this node --
		slevel--;
	}

	return vec_pt.size();
}

