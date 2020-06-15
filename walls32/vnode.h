#ifndef __VNOD_H
#define __VNOD_H

#define NUM_HDR_NODES 7
//Max squared distance in pixels for a point to be near a vector --
#define VNODE_NEAR_LIMIT 9
#define VNODE_NEAR_LIMIT_SQRT 3
#define VNODE_BOX_RADIUS 8

struct MAP_VECNODE { //8*4=32 bytes
	int B_xy[2];	 //B and M are the vector's endpoints
	int M_xy[2];
	int pltrec;
	int maxdist;
	int rt, lf;
};

class CVNode {

public:
	CVNode();
	~CVNode();

	BOOL Init(int xM, int yM, int maxvecs);
	BOOL AddVnode(int xf, int yf, int xt, int yt, int pltrec);
	MAP_VECNODE * GetVecNode(int x, int y);
	BOOL IsInitialized() { return vnod != NULL && vstk != NULL; }

private:
	MAP_VECNODE *vnod;
	int *vstk;
	int vnod_siz, vnod_len;
};


#endif