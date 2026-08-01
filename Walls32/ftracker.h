//CFixedTracker  interface

#ifndef __FTRACKER_H
#define __FTRACKER_H 

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#define MIN_TRACKERWIDTH 8

class CFixedTracker : public CRectTracker
{
public:
	CRect *m_pr;

protected:
	virtual void AdjustRect(int nHandle, LPRECT pRect);
};
#endif

