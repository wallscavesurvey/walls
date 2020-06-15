//georef.cpp
//#include <windows.h>
#include <georef.h>

static int format;

#define FLTYP double

static double getDEG(DEGTYP *p)
{
	/*isgn,ideg,imin,fsec --> fdeg and fmin --*/
	double val;
	switch (format) {
	case 0: val = p->fdeg; break;
	case 1: val = (double)p->ideg + p->fmin / 60.0; break;
	case 2: val = (double)p->ideg + p->imin / 60.0 + p->fsec / 3600.0;
	}
	val *= (double)p->isgn;
	//if(val==180.0) val=-180.0; //DMcK 4/1998
	return val;
}

static void getDMS(DEGTYP *p, double val)
{
	/*fdeg --> isgn,ideg,imin,fsec and fmin --*/

	if (val) {
		//if(val==180.0) val=-180.0;  //DMcK 4/1998
		if (val < 0.0) {
			val = -val;
			p->isgn = -1;
		}
		else p->isgn = 1;
	}
	else p->isgn = 1;

	p->fdeg = val;
	if (!format) return;

	p->ideg = (int)val;
	if ((p->fmin = 60.0*(val - (double)p->ideg)) >= 59.99995) {
		p->ideg++;
		p->fmin = 0.0;
		p->fsec = 0.0;
		p->imin = 0;
	}
	else if (format > 1) {
		p->imin = (int)p->fmin;
		if ((p->fsec = 60.0*(p->fmin - (double)p->imin)) >= 59.995) {
			p->fsec = 0.0;
			if (++p->imin >= 60) {
				p->imin = 0;
				p->fmin = 0.0;
				p->ideg++;
			}
		}
	}
}

int geo_GetData(GEODATA *pD)
{
	int   ix = pD->fcn;
	double lat, lon, lon0;

	format = pD->format;

	if (ix < GEO_CVTDATUM) {
		switch (ix) {
		case GEO_GETVERSION: return GEO_VERSION;
		case GEO_GETDATUMCOUNT: return geo_nDatums;

		case GEO_GETDATUMNAME: {
			if (pD->datum >= geo_nDatums) {
				pD->datumname = "";
				return 0;
			}
			pD->datumname = geo_gDatum[pD->datum].name;
			return 1;
		}
		case GEO_GETNAD27:
			return geo_NAD27_datum;

		case GEO_FIXFORMAT:
			lat = getDEG(&pD->lat);
			lon = getDEG(&pD->lon);
			format = 2; //get all
			getDMS(&pD->lat, lat);
			getDMS(&pD->lon, lon);
			return 0;

		case GEO_CVTUTM2LL2:
		case GEO_CVTUTM2LL:
			geo_UTM2LatLon(pD->zone, pD->east, pD->north, &lat, &lon, pD->datum);
			if (fabs(lat) > 90.0 || fabs(lon) > 180.0) return GEO_ERR_CVTUTM2LL;
			getDMS(&pD->lat, lat);
			getDMS(&pD->lon, lon);
			if (ix == GEO_CVTUTM2LL2) {
				//Let's see if UTM coordinates are out of the zone --
				pD->zone2 = 0;
				geo_LatLon2UTM(lat, lon, &pD->zone2, &pD->east2, &pD->north2, pD->datum);
				if (pD->zone2 != pD->zone) return GEO_ERR_OUTOFZONE;
			}
			break;

		case GEO_CVTLL2UTM:
		case GEO_CVTLL2UTM2:
			lat = getDEG(&pD->lat);
			lon = getDEG(&pD->lon);
			if (fabs(lat) >= 90.0 || fabs(lon) > 180.0) return GEO_ERR_LLRANGE;
			pD->zone = 0;
			geo_LatLon2UTM(lat, lon, &pD->zone, &pD->east, &pD->north, pD->datum);
			/*Also fix signs so calling program can note any change --*/
			//pD->lat.isgn=(lat>=0)?1:-1;
			//pD->lon.isgn=(lon>=0)?1:-1;
			break;
		}
		lon0 = 6.0 * floor(lon / 6.0) + 3.0;
		pD->conv = (float)(sin((GEO_PI / 180)*lat)*(lon - lon0));
		return 0;
	}

	//Datum conversion --
	//The existing datum ID is assumed to be pD->datum.
	//
	//The new datum ID will be:
	//  ix-GEO_CVTDATUM2 if the coordinates in pD are in UTM, or
	//  ix-GEO_CVTDATUM  if the coordinates in pD are in Lat/Long.
	//
	//If coordinates are Lat/Long, pD stores them before and after
	//conversion depending on pD->format.
	//  0:  D   based on lat.fdeg and lat.isgn
	//  1:  DM  based on lat.ideg, lat.fmin, and lat.isgn
	//  2:  DMS based on lat.ideg, lat.imin, lat.fsec, and lat.isgn
	//
	//NOTE: Lat/Long (in pD->format style) is available in pD after conversion.
	//      UTM will also be available if ix>=GEO_CVTDATUM2 **and** a conversion
	//      is required.

	if (ix >= GEO_CVTDATUM2) {
		if ((ix - GEO_CVTDATUM2) == pD->datum) return 0;
		geo_UTM2LatLon(pD->zone, pD->east, pD->north, &lat, &lon, pD->datum);
		if (fabs(lat) > 90.0 || fabs(lon) > 180.0) return GEO_ERR_CVTUTM2LL;
	}
	else {
		if ((ix - GEO_CVTDATUM) == pD->datum) return 0;
		lat = getDEG(&pD->lat);
		lon = getDEG(&pD->lon);
	}

	if (pD->datum != geo_WGS84_datum) {
		//Convert to WGS84 --
		geo_FromToWGS84(false, &lat, &lon, pD->datum);
	}
	geo_datum = (ix >= GEO_CVTDATUM2) ? (ix - GEO_CVTDATUM2) : (ix - GEO_CVTDATUM);

	//Convert from WGS84 if necessary --
	if (geo_datum != geo_WGS84_datum)
		geo_FromToWGS84(true, &lat, &lon, geo_datum);

	if (fabs(lat) >= 90.0 || fabs(lon) > 180.0)
		return GEO_ERR_TRANSLATE;

	pD->datum = geo_datum;

	getDMS(&pD->lat, lat);
	getDMS(&pD->lon, lon);
	if (ix >= GEO_CVTDATUM2) {
		pD->zone = 0;
		geo_LatLon2UTM(lat, lon, &pD->zone, &pD->east, &pD->north, geo_datum);
	}

	return 0;
}
