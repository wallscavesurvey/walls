// ftracker.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "plotview.h"
#include "ftracker.h"
#include "review.h"

void CFixedTracker::AdjustRect(int nHandle,LPRECT pRect)
{
	int r=0,b=0;
	
	CRect R(pRect);
		
    if(R.left>R.right) {
      r=R.right;
      R.right=R.left;
      R.left=r;
      r=1;
      
    }
    if(R.top>R.bottom) {
      b=R.bottom;
      R.bottom=R.top;
      R.top=b;
      b=1;
    }
    
	int Rh=R.bottom-R.top;
	int Rw=R.right-R.left;
    
	long diff=(long)Rw*m_pr->Height()-(long)Rh*m_pr->Width();
	
	if(nHandle<4) {
	    //We are dragging a corner --
		if(diff>0) {
		  //Increase height --
		  int incY=(int)(diff/m_pr->Width());
		  if(b!=(nHandle>1)) {
		    //Increment bottom --
		    R.bottom+=incY;
		  }
		  else {
		    //Decrement top --
		    R.top-=incY;
		  }
		}
		else if(diff<0) {
		  //Increase width --
		  int incX=(int)(-diff/m_pr->Height());
		  if(r!=(nHandle==1||nHandle==2)) {
		    //Increment right --
		    R.right+=incX;
		  }
		  else {
		    //Decrement left --
		    R.left-=incX;
		  }
		}
	}
	else {
		//We are dragging a side --
		if(nHandle&1) {
			//We are using a right or left handle.
			//Expand or shrink height --
			R.InflateRect(0,(int)(diff/(2*m_pr->Width())));
		}
		else {
			//We are using a top or bottom handle.
			//Expand or shrink width --
			R.InflateRect((int)(-diff/(2*m_pr->Height())),0);
		}	
	}
	
	//The rectangle is now at the correct aspect ratio --	
	if(R.Width()>=m_pr->Width() || R.Height()>=m_pr->Height()) {
	  R.CopyRect(m_pr);
	}
	else {	
		//At this point, the rectangle is of the correct size and shape,
		//but may be out of range --
		if(R.left<m_pr->left) R.OffsetRect(m_pr->left-R.left,0);
		else if(R.right>m_pr->right) R.OffsetRect(m_pr->right-R.right,0);
		if(R.top<m_pr->top) R.OffsetRect(0,m_pr->top-R.top);
		else if(R.bottom>m_pr->bottom) R.OffsetRect(0,m_pr->bottom-R.bottom);
	}
	
    if(r) {
      r=R.right;
      R.right=R.left;
      R.left=r;
    }
    if(b) {
      b=R.bottom;
      R.bottom=R.top;
      R.top=b;
    }
    
    *pRect=R;
}
