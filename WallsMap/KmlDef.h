#pragma once

#define GE_MAX_POINTS 500

class CShpLayer;

struct GE_POINT {
	GE_POINT(LPCSTR label,DWORD rec,double lat,double lon)
		: m_csLabel(label)
		, m_rec(rec)
		, m_lat(lat)
		, m_lon(lon)
	{}
	DWORD m_rec;
	CString m_csLabel;
	double m_lat,m_lon;
};

typedef std::vector<GE_POINT> VEC_GE_POINT;

BOOL GE_Launch(CShpLayer *pShp,LPCSTR pathName,GE_POINT *pt,UINT numpts,BOOL bFly);
bool GE_IsInstalled();
