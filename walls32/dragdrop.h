#if !defined(DRAGDROP_H)
#define DRAGDROP_H

// registered messages - application defined
extern UINT NEAR WM_CANDROP;
extern UINT NEAR WM_DROP;

class CDragQuery : public CObject
{
	DECLARE_DYNAMIC(CDragQuery)
public:
	// drop point of client window
	CPoint m_ClientPt;
	
	// server window
	CWnd*  m_pServerWnd;
	
	// drag type - defined by server
	WORD   m_DragType;

	// TRUE = copy, FALSE = move	
	BOOL   m_bDragCopy;

	CDragQuery(CWnd* pServerWnd,const CPoint& ClientPt,BOOL bDragCopy,WORD DragType);
};

class CDragStruct : public CDragQuery
{
	DECLARE_DYNAMIC(CDragStruct)
public:
	// object being dropped
	DWORD m_DragItem;
	
	CDragStruct(CWnd* pServerWnd,
	           const CPoint& ClientPt,
	           BOOL bDragCopy,
	           WORD DragType,
	           DWORD DragItem);
};

#endif //DRAGDROP_H
