#include <assert.h>
#include <vector>
#include <algorithm>

typedef std::pair<int,int> intPair_t;
typedef std::vector<int> intVec_t;
typedef intVec_t::iterator intVec_it;
typedef std::vector<std::pair<int,int> > intPairVec_t;
typedef intPairVec_t::iterator intPairVec_it;
typedef std::vector<intPairVec_it> intPairItVec_t;
typedef intPairItVec_t::iterator intPairItVec_it;

//wallnet-specific classes --

enum e_infvar {
    InfvarH   = (1<<6),
    InfvarV   = (1<<7),
	F_InfvarH = (1<<14),
	F_InfvarV = (1<<15)
};
    
typedef struct {
	long rec;
	long nxtrec;
} LINK_FRAGMENT;

typedef std::vector<LINK_FRAGMENT> fragVec_t;

typedef long *lnptr;

extern "C" {
	extern lnptr  jendp;	  /*jendp[sys_numendp] - string end points*/
	extern lnptr  jlst;       /*adjacency lists - indices to jendp[]*/
	extern lnptr  *jnod;      /*pointers into adjacency list jlst*/
	extern lnptr  jseq;       /*jseq[net_maxsysstr] - indices to jendp[]*/
	extern lnptr  jdeg;       /*jdeg[net_maxsysnod] - degree counts*/
	extern lnptr  jbord;      /*list of border nodes*/

	extern unsigned short *jtask; //Flags: InfvarH, InfvarV

	extern int    sys_recfirst,sys_closures;
	extern int    sys_numstr,sys_numnod,sys_numendp;
	extern int    sys_numinf[2];
}

static fragVec_t linkFragments;

//=============================================================================
//General purpose classes --

class IntSequence {
	int value;
public:
	IntSequence(int start) : value(start) {}
	int operator () () {
		return value++;
	}
};

class CompareFrag {
public:
	bool operator() (LINK_FRAGMENT &f1,LINK_FRAGMENT &f2) {
		return f1.rec<f2.rec;
	}
};

class BitFlagMatrix {
public:
	typedef unsigned long BitBlock;
	const enum {NumBitBlockBits=sizeof(BitBlock)*8};

	BitFlagMatrix() : m_bits(NULL), m_numRows(0) {}

	~BitFlagMatrix() {
		Free();
	}

	bool Allocate(int numRows,int numCols) {
		assert(!m_bits);
		m_numCols=numCols+1;
		m_numBitBlocks=(m_numCols+NumBitBlockBits-1)/NumBitBlockBits;
		m_bits=(BitBlock *)calloc(numRows*m_numBitBlocks,sizeof(BitBlock));
		if(m_bits!=NULL) {
			m_numRows=numRows;
			return true;
		}
		return false;
	}

	void Free() {
		free(m_bits);
		m_bits=NULL;
		m_numRows=0;
	}

	int NumFlags() const {return m_numCols-1;}
	int NumRows() const {return m_numRows;}

	int TestOdd(int r) const {
		return m_bits[r*m_numBitBlocks]&1;
	}

	void SetOdd(int r2,int c) {
		int r0=(r2>>1)*m_numBitBlocks;
		if(r2&1) m_bits[r0] |= 1;
		++c;
		m_bits[r0+c/NumBitBlockBits] |= (1<<(c%NumBitBlockBits));
	}

	int Compare(int r1,int r2)
	{
		BitBlock *pr1=&m_bits[r1*m_numBitBlocks];
		BitBlock *pr2=&m_bits[r2*m_numBitBlocks];
		//Consider the lowest word the most significant word and ignore 1st bit --
		BitBlock b1=pr1[0]&~1;
		BitBlock b2=pr2[0]&~1;
		if(b1!=b2) return (b1<b2)?-1:1;
		for(int i=1;i<m_numBitBlocks;i++) {
			if(*++pr1!=*++pr2) return (*pr1<*pr2)?-1:1;
		}
		return 0;
	}

private:
	int m_numCols;
	int m_numRows;
	int m_numBitBlocks;
	BitBlock *m_bits;
};

class CompareLinkMatrixRows {
public:
	CompareLinkMatrixRows(BitFlagMatrix &linkMatrix) : m_linkMatrix(linkMatrix) {}
	bool operator() (const int r1,const int r2) const
	{
		int i=m_linkMatrix.Compare(r1,r2);
		return i?(i<0):(r1<r2);
	}
private:
	BitFlagMatrix &m_linkMatrix;
};

//=============================================================================
static int getLinkFragments(BitFlagMatrix &linkMatrix)
{

	int sysEdges=linkMatrix.NumRows();
    if(!sysEdges) return 0;

	//Create vector of edge indices (0..sysEdges-1) sorted by link value
	//and edge indices within link value --
	intVec_t linkEdge(sysEdges);
	std::generate_n(linkEdge.begin(),sysEdges,IntSequence(0));
	std::sort(linkEdge.begin(),linkEdge.end(),CompareLinkMatrixRows(linkMatrix));

	LINK_FRAGMENT frag;
	int nLinks=0,nFrag=0;
	unsigned short infvar; 

	int eTop,eNxt=linkEdge[0];
	for(int e=0;e<sysEdges-1;) {
		eTop=eNxt;
		if(!linkMatrix.Compare(eTop,eNxt=linkEdge[++e])) {
			nLinks++;
			int f,eVarH,eVarV,eFst=eTop;
			if((infvar=jtask[eFst])) {
				if(infvar&InfvarH) eVarH=eFst;
				if(infvar&InfvarV) eVarV=eFst;
			}
			do {
				if((f=jtask[eNxt])) {
					if(f&infvar) {
						if(f&(infvar&InfvarH)) {
							--sys_numinf[0];
							jtask[eVarH]|=F_InfvarH;
							jtask[eNxt]|=F_InfvarH;
							jtask[eNxt]&=~InfvarH;
						}
						if(f&(infvar&InfvarV)) {
							--sys_numinf[1];
							jtask[eVarV]|=F_InfvarV;
							jtask[eNxt]|=F_InfvarV;
							jtask[eNxt]&=~InfvarV;
						}
					}
					infvar|=f;
					f=jtask[eNxt];
					if(f&InfvarH) eVarH=eNxt;
					if(f&InfvarV) eVarV=eNxt;
				}
			    assert(eFst<eNxt);
				frag.rec=(eFst<<1)+linkMatrix.TestOdd(eFst);
				frag.nxtrec=eNxt;
				linkFragments.push_back(frag);
				nFrag++;
				eFst=eNxt;
			}
			while(e<sysEdges-1 && !linkMatrix.Compare(eFst,eNxt=linkEdge[++e]));
			frag.rec=(eFst<<1)+linkMatrix.TestOdd(eFst);
			frag.nxtrec=eTop;
			linkFragments.push_back(frag);
			nFrag++;
		}
	}

	assert(nFrag==linkFragments.size());

	if(nFrag) {
		//Sequential access to database might improve updating performance,
		//otherwise this sort is unnecessary --
		std::sort(linkFragments.begin(),linkFragments.end(),CompareFrag());
	}
	return nFrag;	
}

static void fillLinkMatrix(BitFlagMatrix &linkMatrix,lnptr *stk)
{
	/*
    Depth-first search to find for each edge in a biconnected
	undirected graph the subset of cycles that contains it. This
	set will be represented by a row of flag bits set in
	linkMatrix. The number of flags (columns) in a matrix row
	is the number of cycles in a cycle-basis (edges-nodes+1).

	Edges with equal-valued linkMatrix rows are related so that
	any cycle that contains one of them contains them all. This
	relation is important for survey network analysis.

	The graph is defined as follows (4-byte array elements):

	jendp[], are indices to a node array, jnod[]. Each jnod[]
	element is a pointer to an adjacency list in jlst[]. An element
	of jlst[], in turn, represents the adjacent node as an index to
	jendp[], which also identifies the adjacent edge.
	jnod[N+1]-jnod[N] is the length of node N's adjacency list.

	For example, the ith node adjacent to node N is Ni=jendp[e]
	where e=*(jnod[N]+i-1). Also, N=jendp[e^1] and the
	NTS	record # for this string (edge) is sys_recfirst+(e>>1).

	Used as workspace is jdeg[], an int array with length
	equal to the number of nodes, and stk[], a ptr array
	of same minimal length passed by argument. No memory is
	allocated in this funtion.

	Other variables already computed (not all used here):
	sys_recfirst = nts record # of first string (edge)
	sys_closures = number of cycles (already computed)
	sys_numstr   = number of strings (edges) in system
	sys_numnod   = number of unique string endpoints (nodes);
	*/

	assert(sys_numnod>1);
	assert(sys_numstr>2);
	assert(sys_closures==sys_numstr-sys_numnod+1);

	//Use jdeg as counter to indicate node visitation --
	memset(jdeg,0,sys_numnod*sizeof(*jdeg));

	linkFragments.resize(0); //avoid reallocation

	//elements of stk[] are pointers to jlst
	int stkSize=0;
	int nCycles=0,nTree=0;

	for(int N=0;N<sys_numnod;N++) {
		if(jdeg[N]) continue;

		assert(!N); //one component expected in this context

		int nx,n=N;
		jdeg[n]=1;
		lnptr ap=jnod[n];

		while(1) {
			lnptr apMax=jnod[n+1];
			while(ap!=apMax) {
				assert(n==jendp[(*ap)^1]);
				nx=jendp[*ap];
				if(jdeg[nx]<jdeg[n]) {
				    if(!jdeg[nx]) {
						//new tree edge
						nTree++;
						jdeg[nx]=jdeg[n]+1;
						stk[stkSize++]=ap;
						n=nx;
						ap=jnod[n];
						apMax=jnod[n+1];
						continue;
					}
					assert(stkSize);
					if(((*ap)>>1) != ((*stk[stkSize-1])>>1)) {
						//new cycle --
						linkMatrix.SetOdd(*ap,nCycles);
						int j=stkSize-(jdeg[n]-jdeg[nx]);
						assert(j>=0 && jendp[(*stk[j])^1]==nx);
						for(;j<stkSize;j++) {
							linkMatrix.SetOdd(*stk[j],nCycles);
						}
						nCycles++;
					}
					//else this is the incoming tree edge
				}
				++ap;
			}
			if(!stkSize) break;
			ap=stk[--stkSize];
			n=jendp[(*ap)^1];
			ap++;
		}
	}

	assert(sys_numstr==nTree+nCycles);
	assert(sys_closures==nCycles);
}

extern "C" int linkFrag_clear()
{
	//Called after database has been ipdated with links
	linkFragments.clear();
	return 0;
}

extern "C" int linkFrag_get(LINK_FRAGMENT **L)
{
    //Called from net_solve in wnetslv.c, part of wallnet.dll.

	if(sys_closures<2) return 0;

	BitFlagMatrix linkMatrix; //Will contain edges*cycles 1-bit flags
	if(!linkMatrix.Allocate(sys_numstr,sys_closures)) return -1;

	//Depth-first search to initialize linkMatrix --
    //Use jseq as tree edge stack --
	fillLinkMatrix(linkMatrix,(lnptr *)jseq);

	//linkMatrix[e,1..cycles] defines the link (cycle set) containing edge[e].
	if(getLinkFragments(linkMatrix)) {
		*L = &linkFragments[0];
		return linkFragments.size();
	}
	return 0;
}
