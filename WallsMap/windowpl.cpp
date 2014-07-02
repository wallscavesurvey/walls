#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "windowpl.h"

////////////////////////////////////////////////////////////////
// CWindowPlacement 1996 Microsoft Systems Journal.
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.

CWindowPlacement::CWindowPlacement()
{
   // Note: "length" is inherited from WINDOWPLACEMENT
   length = sizeof(WINDOWPLACEMENT);
}

CWindowPlacement::~CWindowPlacement()
{
}

//////////////////
// Restore window placement from profile key
BOOL CWindowPlacement::Restore(CWnd* pWnd)
{
   GetProfileWP();
   // Only restore if window intersets the screen.
   //
   CRect rcTemp, rcScreen(0,0,GetSystemMetrics(SM_CXSCREEN),
      GetSystemMetrics(SM_CYSCREEN));
   if (!::IntersectRect(&rcTemp, &rcNormalPosition, &rcScreen))
      return FALSE;

   pWnd->SetWindowPlacement(this);  // set placement
   return TRUE;
}

static int _tok_count;
static char *_tok_ptr;
static void tok_init(char *p)
{
	_tok_count=0;
	_tok_ptr=p;
}

static int tok_int()
{
	char *p=strtok(_tok_ptr," ");
	_tok_ptr=NULL;
	if(p) {
	  _tok_count++;
	  return atoi(p);
	}
	return 0;
}
//////////////////
// Get window placement from profile.
void CWindowPlacement::GetProfileWP()
{
    CString str;
   
	str=theApp.GetProfileString("Screen","rect",NULL);
	
	showCmd=SW_SHOWNORMAL;
	flags=0;

	if(!str.IsEmpty()) {
		tok_init(str.GetBuffer(1));
		rcNormalPosition.left=tok_int();
		rcNormalPosition.top=tok_int();
		rcNormalPosition.right=tok_int();
		rcNormalPosition.bottom=tok_int();
		if(tok_int()) { 
			showCmd=SW_SHOWMAXIMIZED;
			flags=WPF_RESTORETOMAXIMIZED;
		}
	}
	else {
		RECT rc;
		UINT nWidth;
		::GetWindowRect(::GetDesktopWindow(), &rc);
		nWidth=(rc.right-rc.left)/10;
		rcNormalPosition.left=nWidth;
		rcNormalPosition.right=rc.right-nWidth;
		nWidth=(rc.bottom-rc.top)/10;
		rcNormalPosition.top=nWidth;
		rcNormalPosition.bottom=rc.bottom-nWidth;
	}
	ptMinPosition=CPoint(0,0);
	ptMaxPosition=CPoint(-::GetSystemMetrics(SM_CXBORDER),-::GetSystemMetrics(SM_CYBORDER));
}

////////////////
// Save window placement in app profile
void CWindowPlacement::Save(CWnd* pWnd)
{
   pWnd->GetWindowPlacement(this);
   WriteProfileWP();
}

//////////////////
// Write window placement to app profile
void CWindowPlacement::WriteProfileWP()
{
	char str[80];
	sprintf(str,"%d %d %d %d %d",
		rcNormalPosition.left,
		rcNormalPosition.top,
		rcNormalPosition.right,
		rcNormalPosition.bottom,
		showCmd==SW_SHOWMAXIMIZED);
	theApp.WriteProfileString("Screen","rect",str);
}
