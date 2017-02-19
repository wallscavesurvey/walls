#ifndef __SEGHIER_H
#define __SEGHIER_H

/////////////////////////////////////////////////////////////////////////////
// CSegList window

//save status of label/marker checkboxes on map page
#undef _SAV_LBL

#ifndef HIERLB_H
 #include "hierlb.h"
#endif

#define GET_SGNODE(i) ((CSegListNode *)GetItemData(i))
#define GET_SGVALIDNODE(i) (i>=0?(CSegListNode *)GetItemData(i):NULL)
#define SGNODE(i) ((CSegListNode *)(i))

//For use in pNamTyp[] in graphics.cpp only --
#define M_FIXED M_PRINT

enum {M_MARKERS=1,M_NAMES=2,M_NOTES=(1<<2),M_FLAGS=(1<<3),M_FLAGGED=(1<<4),M_PRINT=(1<<5),
      M_VISIBLE=(1<<6),M_COMPONENT=(1<<7),M_METAFILE=(1<<8),M_LRUDS=(1<<9),
	  M_MAPFRAME=(1<<10),M_PREFIXES=(1<<11),M_NOVECTORS=(1<<12),M_HEIGHTS=(1<<13)};

#define M_TOGGLEFLAGS (M_NAMES+M_HEIGHTS+M_PREFIXES+M_MARKERS+M_NOTES+M_FLAGS+M_NOVECTORS+M_LRUDS)

#define M_NOTESHIFT 12 /*(15-3)*/
#define M_FLAGNAMEMASK 0xC000
#define M_ISFLAGNAME(s) (((s)&M_FLAGNAMEMASK)==M_FLAGNAMEMASK)
#define M_ISNOTE(s) ((((s)>>M_NOTESHIFT)&(M_NOTES+M_FLAGS))==M_NOTES)
#define M_ISFLAG(s) ((((s)>>M_NOTESHIFT)&(M_NOTES+M_FLAGS))==M_FLAGS)
#define M_SEGMASK 0x3FFF
#define M_FLAGIDMASK 0xFFFF

#define M_HIDDENMASK   0x2000  // 0010 0000 0000 0000
#define M_HIDDENSHIFT  13
#define M_SHADEMASK    0x1800  // 0001 1000 0000 0000
#define M_SHADESHIFT   11
#define M_SHAPEMASK    0x0700  // 0000 0111 0000 0000
#define M_SHAPESHIFT   8
#define M_SIZEMASK     0x00FF  // 0000 0000 1111 1111
#define M_STYLEINIT    0xCFFF  // 1100 1111 1111 1111
#define M_SYMBOLSHADE(s) (((s)&M_SHADEMASK)>>M_SHADESHIFT)
#define M_SETSHADE(s,typ) {s=((s&~M_SHADEMASK)|((typ&3)<<M_SHADESHIFT));} 
#define M_SYMBOLSHAPE(s) (((s)&M_SHAPEMASK)>>M_SHAPESHIFT)
#define M_SYMBOLSIZE(s) ((s)&M_SIZEMASK)
#define M_ISHIDDEN(s) (((s)&M_HIDDENMASK)>>M_HIDDENSHIFT)
#define M_SETHIDDEN(s,b)  {s=(WORD)((s&~M_HIDDENMASK)|((b)<<M_HIDDENSHIFT));}
#define M_SETSHAPE(s,typ) {s=(WORD)((s&~M_SHAPEMASK)|((typ&7)<<M_SHAPESHIFT));}   
#define M_SETSIZE(s,siz)  {s=(WORD)((s&~M_SIZEMASK)|((siz)&M_SIZEMASK));} 

#define M_COLORRGB(clr) ((clr)&0xffffff)
#define M_SETCOLOR(clr,rgb)   (clr=((clr&0xff000000)|(rgb)))
#define M_CHARIDX(clr) ((clr)>>24)
#define M_SETCHARIDX(clr,idx) (clr=((clr&0xffffff)|(((BYTE)(idx))<<24))) 

#define M_ALLFLAGS (M_MARKERS+M_NAMES+M_NOTES+M_FLAGS)
#define M_NOTEFLAGS (M_NOTES+M_FLAGS)

#pragma pack(1)
typedef	struct {
	  DWORD id;
	  DWORD color;
	  WORD seg_id;
} NTA_NOTETREE_REC;

typedef	struct {
	  WORD id;
	  WORD iseq;
	  DWORD color;
	  WORD seg_id;
} NTA_FLAGTREE_REC;

typedef struct {
	  WORD iseq; //priority
	  DWORD color;
	  WORD seg_id; //marker size and shape
} NTA_FLAGSTYLE;

typedef struct {
	float dim[4];
	float az;
	byte flags;
} NTA_LRUDTREE_REC;
#pragma pack()

#define LST_INCLUDE 0x8000

//General map options --

struct MAPFORMAT {
	double fFrameWidthInches; //Width of frame (determines scale)
	double fFrameHeightInches; //Height of frame (printer only)
	int iFrameThick; //width (pixels) of frame outline
	int iMarkerSize;  //default size in pts of station marker
	int iMarkerThick; //width of default station marker pen
	int iFlagSize;  //size in pts of default flag marker
	int iFlagThick; //width of default flag marker pen
	int iVectorThin; //width of thin vector pen
	int iVectorThick; //width of thick vector pen
    int iOffLabelX;
    int iOffLabelY;
    int iTickLength;  //length of tickmark
	int iTickThick;   //width of tickmark pen (legend outline?)
	int iLabelSpace;  //gap
	int iLabelInc;
	BOOL bLblFlags; //1 if heights, 2 if prefix1, 4 if prefix2, 8 if if prefix3
	BOOL bUseColors;
	int iFlagStyle;
	BOOL bFlagSolid;  //A tristate button (0,1,2)
	//The following items are not user input data --
	BOOL bChanged;    //Set TRUE when modified by user
	BOOL bPrinter;
	PRJFONT *pfLabelFont;
	PRJFONT *pfFrameFont;
	PRJFONT *pfNoteFont;
};            

enum e_flagsymbols {FST_SQUARES,FST_CIRCLES,FST_TRIANGLES,
					FST_PLUSES,FST_TRUETYPE,M_NUMSHAPES};
enum e_flagshades {FST_CLEAR,FST_SOLID,FST_OPAQUE,FST_TRANSPARENT,M_NUMSHADES};

//styletyp flags --
enum e_style {SEG_NOMARK=1,SEG_NONAME=2,SEG_NONOTE=4,SEG_NOFLAG=8,
			  SEG_FLOATING=16,SEG_USEOWN=32,SEG_CHANGED=64,SEG_HIDENOTES=128};
#define SEG_STAMASK 15
#define SEG_NOTEMASK 12
#define SEG_LABELMASK 3
#define SEG_STYLE_BLACK (CSegListNode::m_StyleBlack)

//Note LRUD_ENABLE is not used (HDR_OUTLINES is now attached and can be floated)
enum e_lrud {LRUD_ENABLE=1,LRUD_SOLID=2,LRUD_TICK=4,LRUD_BOTH=6,LRUD_SVG_USEIFAVAIL=8,LRUD_SVG_AVAIL=16};

#define SEG_NUM_PENSTYLES 7
#define SEG_NO_LINEIDX 6
#define SEG_MSK_LINEIDX 7
//Use LS 2 bits of high nibble for GradIdx() --
#define SEG_SHF_GRADIDX 4
#define SEG_MSK_GRADIDX 0x30
#define SEG_MSK_COLORGRAD 0x30FFFFFF
#define SEG_MSK_LRUDENLARGE 0x0F

#define LRUD_STYLEMASK (2+4+8+16)
#define LRUD_LRUDMASK (2+4)
//#define LRUD_MARKERMASK (4)
//#define LRUD_SOLIDMASK (2)
#define LRUD_SVGMASK (8+16)

//NOTE: The first two flags match CPlotView::M_MARKERS,M_NAMES.
//The default style is all flags zeroed (solid line, markers, names).

struct FAR styletyp {
  WORD seg_id;
  BYTE r;
  BYTE g;
  BYTE b;
  BYTE lineidx;
  WORD flags;
  WORD FAR * FlagPtr() {return &flags;}
  int  LineIdx() {return (lineidx&SEG_MSK_LINEIDX);}
  int  GradIdx() {return (lineidx>>SEG_SHF_GRADIDX);}
  BOOL IsNoLines() {return lineidx==SEG_NO_LINEIDX;} //Check usage when GradIdx()>0
  BOOL IsUsingOwn() {return (flags&SEG_USEOWN)!=0;} 
  BOOL IsNoMark() {return (flags&SEG_NOMARK)!=0;}
  BOOL IsHideNotes() {return (flags&SEG_HIDENOTES)!=0;}
  BOOL IsNoName() {return (flags&SEG_NONAME)!=0;}
  BOOL IsNoFlag() {return (flags&SEG_NOFLAG)!=0;}
  BOOL IsNoNote() {return (flags&SEG_NONOTE)!=0;}
  WORD StationFlags() {return ~(flags|~SEG_STAMASK);}
  WORD LabelFlags() {return ~(flags|~SEG_LABELMASK);}
  WORD NoteFlags() {return ~(flags|~SEG_NOTEMASK);}
  BOOL IsFloating() {return (flags&SEG_FLOATING)!=0;}
  BOOL IsChanged() {return (flags&SEG_CHANGED)!=0;}

  COLORREF LineColorGrad() {return (*(COLORREF *)&r)&SEG_MSK_COLORGRAD;}
  COLORREF LineColor() {return M_COLORRGB(*(COLORREF *)&r);}
  COLORREF MarkerColor() {return LineColor();}
  COLORREF NoteColor() {return LineColor();}
  COLORREF LabelColor() {return LineColor();}

  void SetLineColor(COLORREF rgb) {r=GetRValue(rgb);g=GetGValue(rgb);b=GetBValue(rgb);}
  void SetLineColorGrad(COLORREF rgb)
  {
	  BYTE i=lineidx&~SEG_MSK_GRADIDX;
	  *(COLORREF *)&r=(rgb&SEG_MSK_COLORGRAD);
	  lineidx|=i;
  }
	  
  void SetNoteColor(COLORREF rgb) {SetLineColor(rgb);}

  void SetMarkerColor(COLORREF clr) {*(COLORREF *)&r=clr;}
  COLORREF NoteColorIdx() {return *(COLORREF *)&r;}  
  void SetNoteColorIdx(COLORREF clr) {*(COLORREF *)&r=clr;}

  WORD LrudStyle() {return (flags&LRUD_STYLEMASK);}
  int LrudStyleIdx() {return (flags&LRUD_LRUDMASK)?((flags&LRUD_LRUDMASK)/2-1):0;} //0,1, or 2
  void SetLrudStyleIdx(int idx) {flags=((flags&~LRUD_LRUDMASK)|((idx+1)<<1));}
  int LrudEnlarge() {return lineidx&SEG_MSK_LRUDENLARGE;}
  void SetLrudEnlarge(UINT i) {lineidx=((lineidx&~SEG_MSK_LRUDENLARGE)|i);}
  BOOL IsSVGAvail() {return (flags&LRUD_SVG_AVAIL)!=0;}
  void SetSVGAvail(BOOL bAvail) {flags=bAvail?(flags|LRUD_SVG_AVAIL):(flags&~LRUD_SVG_AVAIL);} 
  BOOL IsSVGUseIfAvail() {return (flags&LRUD_SVG_USEIFAVAIL)!=0;}
  void SetSVGUseIfAvail(BOOL bUse) {flags=bUse?(flags|LRUD_SVG_USEIFAVAIL):(flags&~LRUD_SVG_USEIFAVAIL);}

  //Relevant only for m_StyleHdr[HDR_MARKERS] --
  WORD MarkerSize() {return M_SYMBOLSIZE(seg_id);}
  WORD MarkerShade() {return M_SYMBOLSHADE(seg_id);}
  WORD MarkerShape() {return M_SYMBOLSHAPE(seg_id);}
  WORD MarkerStyle() {return seg_id;}
  void SetMarkerSize(WORD w) {M_SETSIZE(seg_id,w);}
  void SetMarkerShade(WORD w) {M_SETSHADE(seg_id,w);}
  void SetMarkerShape(WORD w) {M_SETSHAPE(seg_id,w);}
  void SetMarkerStyle(WORD s) {seg_id=s;}

  void SetLineIdx(int i) {lineidx=((lineidx&~SEG_MSK_LINEIDX)|i);}
  void SetUsingOwn(BOOL On) {On?(flags|=SEG_USEOWN):(flags&=~SEG_USEOWN);} 
  void SetNoMark(BOOL On) {On?(flags|=SEG_NOMARK):(flags&=~SEG_NOMARK);}
  void SetHideNotes(BOOL On) {On?(flags|=SEG_HIDENOTES):(flags&=~SEG_HIDENOTES);}
  void SetNoName(BOOL On) {On?(flags|=SEG_NONAME):(flags&=~SEG_NONAME);}
  void SetFloating(BOOL On){On?(flags|=SEG_FLOATING):(flags&=~SEG_FLOATING);}
  void SetChanged() {flags|=SEG_CHANGED;} 
  void SetUnchanged() {flags&=~SEG_CHANGED;} 
};

typedef styletyp FAR *styleptr;

struct GRIDFORMAT {
	double fEastInterval;	//East grid interval in survey units (grid origin is survey datum)
	double fNorthEastRatio; //Ratio of north interval to east interval
	double fEastOffset;     //Grid origin offset for plans --
	double fNorthOffset;
	double fGridNorth;       //Direction of grid north wrt true north

	double fVertInterval;	//Vertical grid interval
	double fHorzVertRatio;  //Ratio of horizontal interval to vertical interval
	double fVertOffset;     //Grid origin offset for profiles
	double fHorzOffset;
	
	double fTickInterval;	//Tick interval in survey units
	WORD wSubInt;			//Number of grid subintervals less 1
	WORD wUseTickmarks;     //Use tickmarks instead of grid
	//The following item is not user input data --
	UINT svgflags;			//Set TRUE when modified by user
};

#define SEG_NUM_HDRSTYLES 11
enum {HDR_BKGND,HDR_LABELS,HDR_MARKERS,HDR_GRIDLINES,
	  HDR_TRAVERSE,HDR_RAWTRAVERSE,HDR_FLOATED,
	  HDR_RAWFLOATED,HDR_FLOOR,HDR_NOTES,HDR_OUTLINES};

enum {TREEID_ROOT,TREEID_GRID,TREEID_OUTLINES,TREEID_TRAVERSE,TREEID_RAWTRAVERSE};
#define SEGNODE_OFFSET 5

enum {VF_INCHUNITS=1,VF_INSIDEFRAME=2};
enum {VF_LEGLL=0,VF_LEGTL=1,VF_LEGLR=2,VF_LEGTR=3,VF_LEGNONE=4,VF_LEGMASK=28};
#define VF_ISINCHES(b) ((b)&VF_INCHUNITS)
#define VF_LEGPOS(b) (((b)&VF_LEGMASK)>>2)
#define VF_ISINSIDE(b) (((b)&VF_INSIDEFRAME)!=0)

#define VF_VIEWNOTES (1<<8)
#define VF_VIEWFLAGS (1<<9)
#ifdef _SAV_LBL
#define VF_VIEWLABEL (1<<10)
#define VF_VIEWMARK  (1<<11)
#endif

#pragma pack(1)
struct VIEWFORMAT {
	double fPanelWidth;		//Width of panel in survey meters
	double fCenterEast;		//center of view frame in meters --
	double fCenterNorth;
	double fCenterUp;
	double fPanelView;		//View direction in degrees
	int    iComponent;
	BOOL   bProfile;
	BOOL   bActive;
	BOOL   bChanged;
	double fFrameWidth;		//Frame width in inches
	double fFrameHeight;	//Frame height in inches
	BOOL   bFlags;
	float fOverlap;
	float fPageOffsetX;
	float fPageOffsetY;
};

// Walls 2B7 Prerelease -- NTA_VERSION=0x0305
struct SEGHDR_7P {
	WORD version;
	WORD numFlagNames;
	styletyp style[10]; //10*8 = 80 bytes
	GRIDFORMAT gf; //10*8 + 2*4 = 88 bytes
	VIEWFORMAT vf[2]; //11*8=88
};

// Walls 2B7 Release -- NTA_VERSION=0x0306
struct SEGHDR {
	WORD version;
	WORD numFlagNames;
	styletyp style[SEG_NUM_HDRSTYLES]; //11*8 = 88 bytes
	GRIDFORMAT gf; //10*8 + 2*4 = 88 bytes
	VIEWFORMAT vf[2]; //11*8=88
};
#pragma pack()

/////////////////////////////////////////////////////////////////////////////
struct SEGSTATS;	//Defined in Statdlg.h

typedef struct {
	styletyp style;
	
    char *m_pTitle; //NULL if using name as title
    
    //Statistics computed in CSegView::InitSegStats() --
    double m_fTtLength,m_fHzLength,m_fVtLength,m_fHigh,m_fLow;
    UINT m_nNumVecs,m_idHigh,m_idLow;
    char name[1];
} segnod;

typedef segnod FAR *segnodptr;

class CSegListNode : public CHierListNode
{
friend class CSegList;
#if defined(_DEBUG)
	DECLARE_DYNAMIC(CSegListNode) // this really only needed for debug
#endif

public:
    segnodptr m_pSeg;		      //FAR Pointer to segnod followed by name
    CSegListNode *m_pStyleParent; //Points to ancestor whose style is inherited
    BOOL m_bVisible;
	int m_uExtent;
    
    static styletyp m_StyleBlack;
    
public:
    CSegListNode() {m_pSeg=NULL; m_uExtent=0;}
    //Virtual destructor will free m_pSeg --
    virtual ~CSegListNode();
    
    HPEN PenCreate();
    
    //Map style helper functions --
    BOOL IsUsingOwn() {return m_pSeg->style.IsUsingOwn();}
	void SetUsingOwn(BOOL bOn) {m_pSeg->style.SetUsingOwn(bOn);}
    BOOL IsVisible() {return m_bVisible;}
    BOOL IsChanged() {return m_pSeg->style.IsChanged();}
	BOOL IsLinesVisible();
    void SetChanged() {m_pSeg->style.SetChanged();}
    void SetUnchanged() {m_pSeg->style.SetUnchanged();}
    int StationFlags() {return m_pStyleParent->OwnStationFlags();}
    int LabelFlags() {return m_pStyleParent->OwnLabelFlags();}
    int NoteFlags() {return m_pStyleParent->OwnNoteFlags();}
    int OwnStationFlags() {return m_pSeg->style.StationFlags();}
    int OwnLabelFlags() {return m_pSeg->style.LabelFlags();}
    int OwnNoteFlags() {return m_pSeg->style.NoteFlags();}
    COLORREF LineColor() {return m_pStyleParent->OwnLineColor();}
    COLORREF OwnLineColor() {return m_pSeg->style.LineColor();}
    COLORREF LineColorGrad() {return m_pStyleParent->OwnLineColorGrad();}
    COLORREF OwnLineColorGrad() {return m_pSeg->style.LineColorGrad();}

    int GradIdx() {return m_pStyleParent->m_pSeg->style.GradIdx();}
	BOOL IsNoLines() {return m_pStyleParent->m_pSeg->style.IsNoLines();}

    int LineIdx() {return m_pStyleParent->OwnLineIdx();}
    int OwnLineIdx() {return m_pSeg->style.LineIdx();}
    
    BOOL IsParentOf(CSegListNode *pNode);
    BOOL NonOwnersFirstChild()
    {
    	return Level()>1 && Parent()->FirstChild()==this && !Parent()->IsUsingOwn();
    }
    
    CSegListNode * StyleParent() {return m_pStyleParent;}
    styletyp FAR * OwnStyle() {return &m_pSeg->style;}
    styletyp FAR * Style() {return m_pStyleParent->OwnStyle();}
/*
	//Not used --
	void CopyStyle(CSegListNode *pNode) {
		WORD id=m_pSeg->style.seg_id;
		m_pSeg->style=pNode->m_pStyleParent->m_pSeg->style;
		m_pSeg->style.seg_id=id;
	}
*/    
    double Length() {return m_pSeg->m_fTtLength;}
    double HzLength() {return m_pSeg->m_fHzLength;}
    double VtLength() {return m_pSeg->m_fVtLength;}
    UINT NumVecs() {return m_pSeg->m_nNumVecs;}
    
    WORD FAR * OwnFlags() {return m_pSeg->style.FlagPtr();}
    void SetStyleParents(CSegListNode *pParent);
    void SetBranchVisibility();
    void InitSegStats(SEGSTATS *st);
	void AddSegStats(SEGSTATS *st, BOOL bFlt);
    void AddStats(double &fLength,UINT &uVectors);
	bool HasVecs();
    void FlagVisible(UINT mask);

    void SetBranchTitle();

	void InitHorzExtent();
	int GetHorzExtent(int extent);


    virtual int Compare(CHierListNode *pNode); //Rtns 1: Insert after last sibling
    
    //To avoid excessive casting --
    CSegListNode *Parent() {return (CSegListNode *)m_pParent;}
    CSegListNode *FirstChild() {return (CSegListNode *)m_pFirstChild;}
    CSegListNode *NextSibling() {return (CSegListNode *)m_pNextSibling;}
    BOOL IsLeaf() {return FirstChild()==NULL;}

	LPSTR Name() {return m_pSeg->name;}
	LPSTR Title() {return (m_pSeg->m_pTitle)? m_pSeg->m_pTitle:Name();}
	char * TitlePtr() {return m_pSeg->m_pTitle;}
};    
    
class CSegList : public CHierListBox
{
    friend class CSegView;

// Construction --
public:
	CSegList();
	virtual ~CSegList() {}
    
    // Operations not in parent class --
public:
	void RefreshBranchOwners(CSegListNode *pNode);
	void InitHorzExtent();

	
    CSegListNode *GetSelectedNode()
    {
      return GET_SGVALIDNODE(GetCurSel());
    }

	CPrjListNode *GetSelectedPrjListNode();

    
    BOOL RootDetached(void); //To aid display/invalidation of root icon
	//Overridden non-virtual of CListBox --
    int SetCurSel(int sel);
	int m_uExtent;

protected:
	enum {SEG_RECTRATIO_D=2,SEG_RECTRATIO_N=5};
	int BulletWidth() {return (SEG_RECTRATIO_N * (m_pResources->BitmapHeight()+4))/SEG_RECTRATIO_D;}

    CDocument *GetDocument() {return ((CView *)GetParent())->GetDocument();}
    
    // Required override (pure virtual in CListBoxEx) --
	virtual void DrawItemEx(CListBoxExDrawStruct&);

	// Optional overrides --
	virtual void OpenNode(CHierListNode *node,int index);   //Called when double-clicked
	
	void CheckExtentChange();

	void SelectSegListNode(CSegListNode *pNode);
	void OpenParentNode(CSegListNode *pNode);

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#if defined(_DEBUG)
#define ASSERT_SGNODE(n) ASSERT(n && n->IsKindOf(RUNTIME_CLASS(CSegListNode)))

/*
 inline void ASSERT_SGNODE(CHierListNode* n) 
 { 
 	ASSERT(n);
 	ASSERT(n!=HLNODE(LB_ERR)); 
 	ASSERT(n->IsKindOf(RUNTIME_CLASS(CSegListNode))); 
 }
 */
#else
 inline void ASSERT_SGNODE(CSegListNode* n) { n=n; }
#endif

#endif // __SEGHIER_H
