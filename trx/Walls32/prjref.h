#pragma once

#define REF_SIZDATUMNAME 20

struct PRJREF { //2*8 + 4*4 + 2 + 6 + 20 = 60 bytes
	PRJREF() { *datumname=0; }
	PRJREF(
		double dnorth,
		double deast,
		int ielev,
		float fconv,
		float flatsec,
		float flonsec,
		short szone,
		BYTE blatmin,
		BYTE blonmin,
		BYTE blatdeg,
		BYTE blondeg,
		BYTE bflags,
		BYTE bdatum,
		char *pdatumname)
		: north(dnorth)
		, east(deast)
		, elev(ielev)
		, conv(fconv)
		, latsec(flatsec)
		, lonsec(flonsec)
		, zone(szone)
		, latmin(blatmin)
		, lonmin(blonmin)
		, latdeg(blatdeg)
		, londeg(blondeg)
		, flags(bflags)
		, datum(bdatum)
	{
		strcpy(datumname, pdatumname);
	}
	double north;
	double east;
	int elev;
	float conv;
	float latsec;
	float lonsec;
	short zone;
	BYTE latmin;
	BYTE lonmin;
	BYTE latdeg;
	BYTE londeg;
	BYTE flags;
	BYTE datum;
	char datumname[REF_SIZDATUMNAME];
};
