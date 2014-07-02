
#define RGB_MAXLEVELS 16

struct RGB_LVL {
	enum {BLKSHIFT=8,BLKWIDTH=256,BLKSIZE=(256*256),BLKBYTES=(256*256*3)};
	UINT nLevels;
	UINT nRec[RGB_MAXLEVELS+1];  /*first record number (0 based) of level*/
	UINT nWidth[RGB_MAXLEVELS];  /*lvl_width[0]=nWidth*/
	UINT nHeight[RGB_MAXLEVELS];
	UINT nCols[RGB_MAXLEVELS];
	UINT nRows[RGB_MAXLEVELS];

	void Init(UINT width,UINT height);
	void GetRecDims(UINT lvl,UINT rec,UINT &dimX,UINT &dimY) const
	{
		rec-=nRec[lvl];
		dimX=BLKWIDTH*((rec%nCols[lvl])+1)-nWidth[lvl];
		dimY=BLKWIDTH*(rec/nCols[lvl]+1)-nHeight[lvl];
	}

	UINT ShrinkToFitLvl(UINT width,UINT height);
};

class CRGBFileMap {
public:
	CRGBFileMap() : m_ptr(NULL), m_hFM(NULL) {}
	~CRGBFileMap() {Close();}

	enum {BLKSHIFT=8,BLKWIDTH=256,BLKSIZE=(256*256),BLKBYTES=(256*256*3)};
	bool Create(UINT width,UINT height);
	void Close();

	UINT NumRecs() {return m_lvl.nRec[m_nLevels];}
	UINT NumBytes() {return m_nSize;}
	UINT NumLevels() {return m_nLevels;}
	UINT ShrinkToFitLvl(UINT width,UINT height) {return m_lvl.ShrinkToFitLvl(width,height);}

	UINT SizeBitmap(const CRect &rectDst, UINT &lvl, CRect &rectData); 
	void ReadBitmap(LPBYTE pDst,UINT lvl,CRect rectData);
	void WriteRowBlocks(LPBYTE pSrc,UINT lvl,UINT recRow);

	RGB_LVL m_lvl;

private:
	UINT m_nWidth,m_nHeight;
	UINT m_nLevels;
	UINT m_nSize;
	LPBYTE m_ptr;
	HANDLE m_hFM;
};
