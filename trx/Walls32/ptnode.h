#ifndef __VNOD_H
#define __VNOD_H

#include <vector>

#define NUM_HDR_NODES 7
//Max squared distance in pixels for a point to be near a vector --
#define VNODE_NEAR_LIMIT 9
#define VNODE_NEAR_LIMIT_SQRT 3
#define VNODE_BOX_RADIUS 8

class CShpLayer;

struct MAP_PTNODE { //7*4=28 bytes
	int xy[2];	 //screen location
	int maxdist;
	int rt,lf;
	int rec;
	CShpLayer *pLayer;
};

typedef MAP_PTNODE *MAP_PTNODE_PTR;
typedef std::vector<MAP_PTNODE_PTR> VEC_PTNODE;

class CPtNode {

public:
	CPtNode();
	~CPtNode();

	BOOL Init(int xM,int yM,int maxvecs);
	BOOL AddPtNode(int x,int y,CShpLayer *pLayer,int rec);
	MAP_PTNODE * GetPtNode(int x,int y);
	int GetVecPtNode(VEC_PTNODE &vec_pt,int x,int xBorder,int y,int yBorder);

	BOOL IsInitialized() {return vnod!=NULL && vstk!=NULL;}

private:
	MAP_PTNODE *vnod;
	int *vstk;
	int vnod_siz,vnod_len;
};

#endif