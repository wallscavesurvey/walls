// hierlb.cpp : implementation file
//

#include "stdafx.h"
#include "hierlb.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHierListNode 

#if defined(_DEBUG)
 IMPLEMENT_DYNAMIC(CHierListNode,CObject)
#endif

//Normally overriden by derived class to free memory associated with data members --
CHierListNode::~CHierListNode()
{
}

CHierListNode::CHierListNode()
{
    //Makes new node. Use attach() to connect it to a given parent.

	m_pParent=m_pFirstChild=m_pNextSibling=0;
	m_LastChild=0;
	m_bOpen=m_bFloating=FALSE;
#ifdef _GRAYABLE
	m_bGrayed=FALSE;
#endif
	m_Level=0;
}

int CHierListNode::Compare(CHierListNode *pNode)
{
	//Virtual function --
	return -1; //Insert as first sibling 
}

void CHierListNode::SetOpenFlags(int level)
{
  //Sets m_bOpen TRUE for the subtree begining with this node
  //up to a depth of level. The flags are zeroed at deeper levels.
  //If level<=0 all flags are zeroed.
  //NOTE: This function is used recursively.

  if(!m_pFirstChild) {
    m_bOpen=FALSE;
    return;
  }
  m_bOpen=level>0;
  
  CHierListNode *pNode=m_pFirstChild;
  while(pNode) {
    pNode->SetOpenFlags(level-1);
    pNode=pNode->m_pNextSibling;
  }
}

int CHierListNode::GetWidth(BOOL bIgnoreFlags)
{
  //Calculate line count of displayed subtree. If bIgnoreFlags==FALSE
  //assume the m_bOpen flags reflect whether or not nodes are displayed,
  //otherwise assume subtree is expanded. This node is always assumed
  //displayed, so the return value is at least one.

  int i=1;
  if(m_bOpen || bIgnoreFlags) {  
    CHierListNode *pNode=m_pFirstChild;
    while(pNode) {
      i+=pNode->GetWidth(bIgnoreFlags);
      pNode=pNode->m_pNextSibling;
    }
  }
  return i;
}

/*
CHierListNode * CHierListNode::Ancestor(int level)
{
	//Get ancestor at level>=0;
	if((level=m_Level-level)<=0) return NULL;
	
	CHierListNode *pNode=m_pParent;
	while(--level) pNode=pNode->m_pParent;
	return pNode;
}
*/

int CHierListNode::GetMaxWidth()
{
  //Calculate line count of subtree items including this item.

  int i=1;
  CHierListNode *pNode=m_pFirstChild;
  while(pNode) {
    i+=pNode->GetMaxWidth();
    pNode=pNode->m_pNextSibling;
  }
  return i;
}

int CHierListNode::GetIndex()
{
  //Get 0-based line number, or -1 if parent not open.
  
  CHierListNode *pParent=m_pParent;
  if(pParent && !pParent->m_bOpen) return -1;
  
  int i=0;
  CHierListNode *pChild=this;
  
  while(pParent) {
     ASSERT(pParent->m_bOpen);
     CHierListNode *pNode=pParent->m_pFirstChild;
     i++;
     while(pNode && pNode!=pChild) {
       ASSERT(pNode);
       i+=pNode->GetWidth(FALSE); //Do *not* ignore m_bOpen flags
       pNode=pNode->m_pNextSibling;
     }
     pChild=pParent;
     pParent=pParent->m_pParent;
  }
  return i;
}

BOOL CHierListNode::FixChildLevels()
{
  //Recalculates m_Level and m_LastChild flags of all children and
  //all siblings beneath this node.
  //This is a recursive function used by by Attach() and Detach().
  //If the level limit is exceeded, FALSE is returned to indicate failure.
  
  if(m_pParent) {
    if((m_Level=m_pParent->m_Level+1)>16) return FALSE;
    m_LastChild=m_pParent->m_LastChild;
  }
  else {
    m_Level=0;
    m_LastChild=0;
  }
  
  if(m_pNextSibling) {
    if(!m_pNextSibling->FixChildLevels()) return FALSE;
    if(m_Level && m_pNextSibling->m_bFloating) m_LastChild=m_pNextSibling->m_LastChild;
  }
  else if(m_Level) m_LastChild|=(0x01<<(m_Level-1));
  
  return m_pFirstChild?m_pFirstChild->FixChildLevels():TRUE;
}

CHierListNode * CHierListNode::PrevLastChild()
{
    //Get first "next-to-last child" sibling assuming this
    //is a non-floating last child.
    
    CHierListNode *pPrev=m_pParent?m_pParent->m_pFirstChild:this;

    for(CHierListNode *pNode=pPrev;pNode!=this;pNode=pNode->m_pNextSibling) {
      ASSERT(pNode);
      if(!pNode->m_bFloating) pPrev=pNode;
    }
    return pPrev;
}

CHierListNode * CHierListNode::LastChild()
{
    
    CHierListNode *pNode=m_pFirstChild;
    if(pNode) while(pNode->m_pNextSibling) pNode=pNode->m_pNextSibling;
    return pNode;
}

CHierListNode * CHierListNode::PrevSibling()
{
    CHierListNode *pNode=m_pParent;
    if(!pNode || (pNode=pNode->m_pFirstChild)==this) return NULL;
    while(pNode->m_pNextSibling!=this) {
      pNode=pNode->m_pNextSibling;
      ASSERT(pNode);
    }
    return pNode;
}

int CHierListNode::SetFloating(BOOL bFloating)
{
    //Hide or unhide line to parent by changing its "floating" status.
    //To hide the entire line we may also have to change the "last child"
    //status of one or more previous siblings.
    //Return total width of the affected sibling branches if this occurs,
    //otherwise return zero.
    
    CHierListNode *pPrev;
    WORD wLastChild;
    
    if(!m_pParent) return 0;
    
    m_bFloating=bFloating;
        
    if((wLastChild=(m_LastChild&(0x01<<(m_Level-1))))==0 ||
      (pPrev=PrevLastChild())==this) return 0;

    //The newly detached or reattached node is in the "last child" group.
    //The last child status of this node doesn't change. However, it
    //does change for at least one sibling --
    
    //In the case of detachment (bFloating==TRUE):
    //   The last child status of all siblings between this node and the parent,
    //   back to and including the first non-floating sibling (backward toward
    //   the parent) must change from FALSE to TRUE.
    
    //In the case of reattachment (bFloating==FALSE):
    //   The last child status of the chain of siblings just defined must
    //   change from TRUE to FALSE.
    
    //We can now update each child branch in the chain --
    int iWidth=0;
    do {
      if(bFloating) pPrev->m_LastChild |= wLastChild;
      else pPrev->m_LastChild &= ~wLastChild;
      //The sibling's children also inherit the change --
      if(pPrev->m_pFirstChild) (void)pPrev->FixChildLevels();
      iWidth+=pPrev->GetWidth();
      pPrev=pPrev->m_pNextSibling;
    }
    while(pPrev!=this);
    return iWidth;
}

int CHierListNode::Attach(CHierListNode *pPrev,BOOL bIsSibling)
{
    //Attaches node (and all its children) to a an existing tree.
    //pPrev is either the parent node, in which case the insertion
    //is done in Compare() order, or a sibling node (bIsSibling==TRUE),
    //in which case the node is attached just underneath the sibling.
    //If pPrev is NULL, the node is made a valid level 0 root
    //(m_Level and m_LastChild flags adjusted in the new tree).
    
    //Return value: 0 if successful. If the maximum level is exceeded
    //the attach fails and HN_ERR_MaxLevels is returned. HN_ERR_Compare
    //is returned if a comparison with potential siblings fails (see below).
    
    //When bIsSibling==FALSE, the position of this node among siblings
    //is determined by the Compare(<sibling node>) function, which must
    //return -1 (insert before), 1 (insert after), or 0 (do NOT insert).
    //In the last case, this function returns error code HN_ERR_Compare.
    //By default, CHierListNode::Compare() always returns -1, causing
    //assignment of the new node as the first sibling. Alternatively,
    //always returning 1 would cause the tree to reflect the order items
    //were attached. Compare() is virtual and can be overriden by the
    //derived class.
    
    if(!pPrev) {
      //We are establishing the root node --
      m_Level=0;
      m_pParent=0;
      m_pNextSibling=0; //Only one root allowed for now.
      m_LastChild=0;
      m_bFloating=TRUE;
      if(m_pFirstChild && !m_pFirstChild->FixChildLevels()) return HN_ERR_MaxLevels;
      return 0;
    }
    
    if(!IsCompatible(pPrev)) return HN_ERR_Compare;
    
    if(bIsSibling) {
      m_pParent=pPrev->m_pParent; //May be NULL
      m_pNextSibling=pPrev->m_pNextSibling;
    }
    else {
      m_pParent=pPrev;
      CHierListNode *pNext=pPrev->m_pFirstChild;
      int e;
      while(pNext && (e=Compare(pNext))>=0) {
        if(!e) return HN_ERR_Compare;
        pPrev=pNext;
        pNext=pNext->m_pNextSibling;
      }
      m_pNextSibling=pNext;
    }
    m_Level=m_pParent->m_Level+1;
    m_LastChild=m_pParent->m_LastChild;
    
    int wLastChild=(0x01<<(m_Level-1));
    
    //We are a last child if there is no next sibling or if the
    //next sibling is a floating last child --
    if(!m_pNextSibling ||
       m_pNextSibling->m_bFloating && (m_pNextSibling->m_LastChild&wLastChild))
       m_LastChild|=wLastChild;

    if(m_pFirstChild && !m_pFirstChild->FixChildLevels()) return HN_ERR_MaxLevels;
    
    //No errors are possible from here on. We can revise the pre-existing tree -- 
    if(pPrev==m_pParent) pPrev->m_pFirstChild=this;
    else {
      pPrev->m_pNextSibling=this;
      if(!m_bFloating && (m_LastChild&wLastChild)!=0) {
        //If we are attaching a non-floating last child, we must also remove
        //last child status from one or more sibling branches --
        pPrev=PrevLastChild();
        ASSERT(pPrev!=this);
        while(pPrev!=this) {
           ASSERT_NODE(pPrev);
           pPrev->m_LastChild &= ~wLastChild;
           if(pPrev->m_pFirstChild) (void)pPrev->m_pFirstChild->FixChildLevels();
           pPrev=pPrev->m_pNextSibling;
        }
      }
    }
    return 0;
}

void CHierListNode::Detach(CHierListNode *pPrev)
{
    //Detaches node from its parent and siblings. If pPrev is not NULL
    //the function does nothing unless pPrev->m_pNextSibling==this.
    //This allows detachment of a level 0 subtree, and will improve
    //efficiency when the previous sibling is known by the caller.
    //NOTE: This function does nothing if both m_pParent and pPrev are NULL.

      if(!pPrev) {
         if(!m_pParent) return;
         pPrev=m_pParent;
         CHierListNode *pNext=pPrev->m_pFirstChild;
         while(pNext && pNext!=this) {
           pPrev=pNext;
           pNext=pNext->m_pNextSibling;
         }
         ASSERT_NODE(pNext);
         if(!pNext) return;
      }
      else {
        ASSERT(pPrev->m_pNextSibling==this);
        if(pPrev->m_pNextSibling!=this) return;
      }
      
      //pPrev is either the parent or previous sibling and is not NULL --
      
      if(pPrev==m_pParent) {
        if((pPrev->m_pFirstChild=m_pNextSibling)==NULL) pPrev->m_bOpen=FALSE;
      }
      else {
        if(m_Level && !m_bFloating) {
          WORD wLastChild=m_LastChild&(0x01<<(m_Level-1));
          if(wLastChild) {
            //In detaching a non-floating last child branch, we must give one or
            //more previous siblings last child status --
            pPrev=PrevLastChild();
            while(TRUE) {
              ASSERT_NODE(pPrev);
              pPrev->m_LastChild |= wLastChild;
              if(pPrev->m_pFirstChild) (void)pPrev->m_pFirstChild->FixChildLevels();
              if(pPrev->m_pNextSibling==this) break;
              pPrev=pPrev->m_pNextSibling;
            }
          }
        }
        pPrev->m_pNextSibling=m_pNextSibling;
      }
      return;
}

void CHierListNode::DeleteChildren()
{
   //Delete all children of this node --
   CHierListNode *pChild;
   
   while(m_pFirstChild) {
     pChild=m_pFirstChild;
     m_pFirstChild=pChild->m_pNextSibling;
     pChild->DeleteChildren();
     delete pChild;
   }
   m_bOpen=FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CHierListBox

BEGIN_MESSAGE_MAP(CHierListBox,CListBoxEx)
	//{{AFX_MSG_MAP(CHierListBox)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CHAR()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CHierListBox::CHierListBox(BOOL lines, WORD offset/*=16*/, WORD rootOffset/*=0*/)
{
    m_pRoot = NULL;
	m_bLines = lines;
	m_Offset = offset;
	m_RootOffset = rootOffset;
	
	// you can enable drag and drop in your derived class
	EnableDragDrop(FALSE);
}

CHierListBox::~CHierListBox()
{
}

/////////////////////////////////////////////////////////////////////////////
// CHierListBox message handlers

int CHierListBox::HideChildren(int index)
{
   //Remove the listbox items corresponding to the children of the
   //node currently at listbox position index. This function assumes index
   //is within range and always returns index+1.
   
   //NOTE: This does NOT affect the m_bOpen flags of the subtree.

   CHierListNode *pNode=GET_HLNODE(index);
   ASSERT_NODE(pNode);
   
   int ParentLevel=pNode->m_Level;
   int count=GetCount();
   index++;
   while(index<count) {
     pNode=GET_HLNODE(index);
     ASSERT_NODE(pNode); 
     if(pNode->m_Level<=ParentLevel) break;
     DeleteString(index);
     count--;
   }
   return index;
}
     
int CHierListBox::DisplayNode(CHierListNode *pNode,int index)
{
     //Display an attached (but hidden) node in the listbox at position index.
     //Both it and its children will be opened according to their m_bOpen values.
     //The return value is the index of the last item inserted plus one
     //(if successful), or LB_ERRSPACE if InsertString() fails. In the latter
     //case, any strings added due to this invocation are removed --
     
     if(InsertString(index,(LPCSTR)pNode)<0) return LB_ERRSPACE;
     if(!pNode->m_bOpen) return index+1;
     
     int ParentIndex=index++; //Save for error recovery
     
     pNode=pNode->m_pFirstChild;
     while(pNode) {
       if((index=DisplayNode(pNode,index))<0) {
         HideChildren(ParentIndex);
         DeleteString(ParentIndex);
         return LB_ERRSPACE;
       }
       pNode=pNode->m_pNextSibling;
     }
     return index;
}

int CHierListBox::OpenToLevel(int index,int level)
{
     //Expands (or collapses) node at a listbox position to a specified relative
     //level. OpenToLevel(index,0), for example, "closes" the node at position index
     //and sets m_bOpen FALSE for all nodes in the corresponding subtree.
     
     //The return value is the index of the next displayed node (if any) following
     //the subtree just processed, or the number of listbox items if we have opened
     //or closed the last node. If the listbox capacity is exceeded, LB_ERRSPACE is
     //returned. When this occurs, the set of open nodes may change, but both the
     //listbox and data structures should remain in a consistent state. (All children
     //of a parent are either displayed or hidden.)
     
     //NOTE: The listbox is updated but SetRedraw(), Invalidate(), and any desired
     //scrolling is left up to the caller.
     
     CHierListNode *pNode=GET_HLNODE(index);
     ASSERT_NODE(pNode);
      
     //Node closure. Note that this operation assumes only that pNode
     //is displayed at position index. The m_bOpen flags of pNode and
     //its subtree may or may not reflect the actual contents of the listbox.
     //This feature is used for error recovery by the calling function.
     
     if(!level || !pNode->m_pFirstChild) {
       pNode->SetOpenFlags(0); //Sets m_bOpen FALSE for pNode and its subtree.
       return HideChildren(index); //This does NOT affect the m_bOpen flags
     }
     
     //Open pNode to a relative depth of level. For now, lets simply close
     //the node and use DisplayNode() as required for each child node-- 
     
     OpenToLevel(index,0);
     pNode->SetOpenFlags(level);
     pNode=pNode->m_pFirstChild;
     int ParentIndex=index++;
     while(pNode) {
       if((index=DisplayNode(pNode,index))<0) {
         //Assumming we overflowed the listbox --
         OpenToLevel(ParentIndex,0);
         return LB_ERRSPACE;
       }
       pNode=pNode->m_pNextSibling;
     }
     return index;
}     

int CHierListBox::InsertNode(CHierListNode* pNode,int PrevIndex,BOOL bPrevIsSibling /*=FALSE*/)
{
     //Insert an unattached node, pNode, (and children) in the listbox.
     //This updates the data structure by attaching the node to the
     //parent (or sibling) at listbox position PrevIndex>=0.
     //If PrevIndex==-1, or if the listbox is empty, pNode replaces m_pRoot
     //as the listbox's first tree.
     
     //The data are inserted in the listbox according to the current m_bOpen 
     //values of pNode and its subtree. The floating status of the new node is
     //also preserved.
     
     //The return value is the new node's position in the listbox, or
     //HN_ERR_Compare, HN_ERR_MaxLevels, or LB_ERRSPACE upon failure
     //(all are negative values).
     
     //If the new parent node is currently closed, it will be open after a
     //successful insertion. If an error code is returned, the
     //listbox and data structure remain unchanged.
     
     //NOTE: The listbox is updated but SetRedraw(), Invalidate(), and
     //any desired scrolling is left up to the caller.
     
     if(PrevIndex>=0) {
       //For now, simply force PrevIndex to be in range --
       int i=GetCount();
       if(PrevIndex>=i) PrevIndex=i-1;
     }
     
     if(PrevIndex<0) {
	   int i=pNode->Attach(0);
	   if(i<0) return i;
	   if(DisplayNode(pNode,0)<0) return LB_ERRSPACE;
       pNode->m_pNextSibling=m_pRoot;
       m_pRoot=pNode;
       return 0;
     }

     CHierListNode *pPrev=GET_HLNODE(PrevIndex);
     ASSERT_NODE(pPrev);
       
     int i=pNode->Attach(pPrev,bPrevIsSibling);
     if(i<0) return i;
       
     if(!bPrevIsSibling) {
       
         //We attached pNode to pPrev as a new child. If the parent is not open,
         //we complete the insertion by displaying each child, while saving the
         //position of the new node as the return value --
         
	     if(!pPrev->m_bOpen) {
	       int NewIndex=0;
	       pPrev->m_bOpen=TRUE;
	       pPrev=pPrev->m_pFirstChild;
	       i=PrevIndex+1;
	       while(pPrev) {
             if(pPrev==pNode) NewIndex=i;
             if((i=DisplayNode(pPrev,i))<0) {
               OpenToLevel(PrevIndex,0);
               pNode->Detach();
               return LB_ERRSPACE;
             }
             pPrev=pPrev->m_pNextSibling;
           }
           ASSERT(NewIndex);
           return NewIndex;
	     }
	      
	     //Otherwise, we must find its position among siblings. If pNode is
	     //not the first child, we set pPrev and PrevIndex to the previous sibling.
	     //Note: This code is unnecessary if we are not sorting  --
	     if(pPrev->m_pFirstChild!=pNode) {
	       bPrevIsSibling=TRUE;
	       pPrev=pPrev->m_pFirstChild;
	       PrevIndex++;
	       while(pPrev->m_pNextSibling!=pNode) {
	         PrevIndex+=pPrev->GetWidth();
	         pPrev=pPrev->m_pNextSibling;
	         ASSERT(pPrev);
	       }
	     }    
	 } //!bPrevIsSibling
	   
	 //At this point pPrev is non-NULL and either the parent or the previous
	 //sibling. If it is the latter, we must adjust PrevIndex to the new node's
	 //position of insertion --
	 
	 if(bPrevIsSibling) PrevIndex+=pPrev->GetWidth();
	 else PrevIndex++;
	     
     //Although listbox overflow is still possible, HN_ERR_Compare and
     //HN_ERR_MaxLevels are at least ruled out.
     
     if(DisplayNode(pNode,PrevIndex)<0) {
       pNode->Detach(bPrevIsSibling?pPrev:NULL);
       return LB_ERRSPACE;
     }
     return PrevIndex;
}

CHierListNode *CHierListBox::RemoveNode(int index)
{
    //Detaches node at listbox position index from the m_pRoot chain
    //while removing it from the listbox window. Returns pointer to
    //the removed node.
    
	CHierListNode* pNode = GET_HLNODE(index);
	ASSERT_NODE(pNode);
	HideChildren(index);
	DeleteString(index);
	if(pNode->m_pParent) {
      //This may set parent's m_bOpen flag FALSE, in which
      //case how do we invalidate that line in the listbox?
	  pNode->Detach();
	}
	else if(pNode==m_pRoot) m_pRoot=pNode->m_pNextSibling;
	else {
	  CHierListNode *pPrev=m_pRoot;
	  while(pPrev && pPrev->m_pNextSibling!=pNode) pPrev=pPrev->m_pNextSibling;
	  ASSERT(pPrev);
	  pNode->Detach(pPrev);
	}
	return pNode;
}

CHierListNode *CHierListBox::FloatNode(int index,BOOL floating)
{
	CHierListNode *pNode=GET_HLNODE(index);
	
	//A level 0 node is always "floating" --
	if(pNode->Level() && pNode->m_bFloating!=floating) {
		CRect itemRect;
		GetItemRect(index,&itemRect);
		int sibWidth=pNode->SetFloating(floating);
		//If the "last child" status of the previous sibling was changed,
		//we must also invalidate the displayed lines of that sibling's branch --
		if(sibWidth) itemRect.top-=(itemRect.bottom-itemRect.top)*sibWidth;
		InvalidateRect(&itemRect,TRUE); //erase bknd
		return pNode;
	}
	return NULL;
}

CHierListNode * CHierListBox::FloatSelection(BOOL floating)
{
    int index=GetCurSel();
    return (index>0)?FloatNode(index,floating):NULL;
}

BOOL CHierListBox::IsSelectionFloating()
{
    int index=GetCurSel();
    return index>0 && GET_HLNODE(index)->m_bFloating;
}


void CHierListBox::DeleteNode(int index)
{
	CHierListNode* n = RemoveNode(index);
	n->DeleteChildren();
	delete n;
}

void CHierListBox::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int index=ItemFromPoint(point);

	if(index>=0) {
	  CHierListNode* n = GET_HLNODE(index);
	  ASSERT_NODE(n);
 	  OpenNode(n,index);
	}
}	

void CHierListBox::EnableLines(BOOL enable)
{
	if(enable ^ m_bLines)
	{
		m_bLines = enable;
		Invalidate();
	}
}

void CHierListBox::DrawLines(CHierListNode* pNode, CDC* pDC, LPCRECT pRect )
{ 
	//Redraw all the connecting lines between pRect->top and pRect->bottom
	//up to and including the line to the icon for pNode --
	
	WORD mask    = pNode->GetLastChildMask();
	int maxLevel = pNode->m_Level;
	BOOL floating= pNode->m_bFloating;

	if(!maxLevel) return;

	// set line colour
	pDC->SetBkColor(RGB_BLACK);

	WORD levelmask = 1;

	// it looks better having the tees and corners at half bitmap height
	// than text height for large fonts.
	int bmHeight = m_pResources->BitmapHeight();
	
	for( int level=1; level<=maxLevel; level++ )
	{
		int indent = m_RootOffset + m_Offset*(level-1) +m_Offset/2;

		BOOL bLastChild     = (BOOL)(levelmask & mask);

			if(level == maxLevel)
			{
				if( bLastChild )
				{
					if(!pNode->m_bFloating) {
					  // draw corner
					  CRect r(indent,pRect->top,indent+1,pRect->top+bmHeight/2);
					  pDC->ExtTextOut( indent, 0, ETO_OPAQUE, r, NULL, 0, NULL );
#ifdef _GRAYABLE
					  if(pNode->m_bGrayed) pDC->SetBkColor(RGB_WHITE);
#endif
					  CRect r1(indent+1,pRect->top+bmHeight/2,indent+m_Offset/2,pRect->top+bmHeight/2+1);
					  pDC->ExtTextOut(indent, 0, ETO_OPAQUE, r1, NULL, 0, NULL);
					}
				}
				else
				{
					// draw tee intersection --
					
					// first draw vertical line (always black) --
					CRect r(indent,pRect->top,indent+1,pRect->bottom);
					pDC->ExtTextOut( indent, 0, ETO_OPAQUE, r, NULL, 0, NULL );
					
					// next, draw horizontal connecting line if the node is attached --
					if(!pNode->m_bFloating) {
#ifdef _GRAYABLE
					  //pDC->SetBkColor(pNode->m_bGrayed?cGray:cLine);
					  if(pNode->m_bGrayed) pDC->SetBkColor(RGB_WHITE);
#endif
					  CRect r1(indent+1,pRect->top+bmHeight/2,indent+m_Offset/2,pRect->top+bmHeight/2+1);
					  pDC->ExtTextOut(indent, 0, ETO_OPAQUE, r1, NULL, 0, NULL);
					}
				}
			}
			else if(!bLastChild)
			{
				CRect r(indent,pRect->top,indent+1,pRect->bottom);
				pDC->ExtTextOut( indent, 0, ETO_OPAQUE, r, NULL, 0, NULL );
			}

		levelmask <<= 1;
	}
}

void CHierListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	// draw focus rectangle when no items in listbox
	if(lpDIS->itemID == (UINT)-1)
	{
		if(lpDIS->itemAction&ODA_FOCUS)
		{
			// rcItem.bottom seems to be 0 for LBS_OWNERDRAWVARIABLE list boxes 
			// so set it to something sensible
			lpDIS->rcItem.bottom = m_lfHeight;
			if(m_bLines) lpDIS->rcItem.left  += m_Offset+m_RootOffset;
			pDC->DrawFocusRect( &lpDIS->rcItem );
		}
		return;
	}
	else
	{
		ASSERT(m_pResources); //need to attach resources before creation/adding items

		CHierListNode* node = (CHierListNode*)lpDIS->itemData;
		ASSERT_NODE(node);

		int selChange   = lpDIS->itemAction&ODA_SELECT;
		int focusChange = lpDIS->itemAction&ODA_FOCUS;
		int drawEntire  = lpDIS->itemAction&ODA_DRAWENTIRE;

		int indent = m_RootOffset+m_Offset*node->m_Level;

		lpDIS->rcItem.left += indent;

		if(selChange || drawEntire)
		{
			if(m_bLines)
				DrawLines(node,pDC,&lpDIS->rcItem );

			BOOL sel = lpDIS->itemState & ODS_SELECTED;

			pDC->SetBkColor(sel?m_pResources->ColorHighlight():m_pResources->ColorWindow());
			pDC->SetTextColor(sel?m_pResources->ColorHighlightText():m_pResources->ColorWindowText());

			// fill the retangle with the background colour the fast way.
			pDC->ExtTextOut( lpDIS->rcItem.left, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL );

			// derived class can finish things off
 			CListBoxExDrawStruct ds( pDC,&lpDIS->rcItem, sel,(DWORD)(LPVOID)node,lpDIS->itemID,m_pResources);
			DrawItemEx( ds );
		}

		if( focusChange || (drawEntire && (lpDIS->itemState&ODS_FOCUS)) )
			pDC->DrawFocusRect(&lpDIS->rcItem);
	}
}

int CHierListBox::GetBotIndex()
{
	CRect clientrect;
	GetClientRect(clientrect);

	// find index of last visible item in list box --
    int nlines=clientrect.Height()/TextHeight()-1;
	if(nlines<0) nlines=0;

    return GetTopIndex()+nlines;
}

void CHierListBox::MakeChildrenVisible(int parentIndex, int lastChildIndex)
{
	// need to scroll listbox if last item is not visible
	// if the last item will not fit then make sure parent is still visible

	if( lastChildIndex < 0 ) return;

	// find index of last item in list box
	int bottomIndex = GetBotIndex();

	if(( bottomIndex!=LB_ERR ) && ( lastChildIndex>bottomIndex ))
	{
		SetTopIndex(parentIndex);
	}
}

void CHierListBox::OpenNode(CHierListNode *n,int index)
{
	// Open entire listbox to specified level --
	if(n->m_pFirstChild) {
		SetRedraw(FALSE);
		(void)OpenToLevel(index,n->m_bOpen?0:1);
		SetRedraw(TRUE);
		Invalidate();
	}
}

void CHierListBox::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nFlags>=0 && nChar==VK_RETURN) {
	   CHierListNode *pNode;
       int index=GetCurSel();
       pNode=GET_HLNODE(index);
       ASSERT_NODE(pNode);
	   OpenNode(pNode,index);
	   return;
	}	
	CListBoxEx::OnChar(nChar, nRepCnt, nFlags);
}

BOOL CHierListBox::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// TODO: Add your message handler code here and/or call default
	int i=GetTopIndex();
	if(zDelta<0) {
		if(++i<GetCount()) SetTopIndex(i);
	}
	else {
		if(i>0) SetTopIndex(i-1);
	}
	return TRUE;
}
