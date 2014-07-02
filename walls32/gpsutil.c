#include "gps.h"

// Garmin offset for time
//
#define START		2447892L	// Julian date for 00:00 12/31/1989
#define STARTLOWR	2448623L 	// Julian date for 00:00 01/01/1992 

#define UTMa 6378137.0
#define UTMf 0.0033528106647475
/* -------------------------------------------------------------------------- */
void days2date(long days, short *mm, short *dd, short *yy)
{
	long ja, jalpha, jb, jc, jd, je;

	jalpha = (long)(((float)(days+START-1867216L)-0.25)/36524.25);
	ja = days+START+1+jalpha-(long)(0.25*jalpha);

	jb = ja + 1524L;
	jc = (long)(6680.0 + ((double)(jb - 2439870L) - 122.1) / 365.25);
	jd = (long)(365.25 * jc);
	je = (long)((jb - jd) / 30.6001);
	*dd = (short)(jb - jd - (30.6001 * je));
	*mm = (short)(je - 1);
	if (*mm > 12)
		*mm -= 12;
	*yy = (short)(jc - 4715);
	if (*mm > 2)
		--(*yy);
	if (*yy <= 0)
		--(*yy);
} /* days2date */

/* -------------------------------------------------------------------------- */
long date2days(short mm, short dd, short yy)
{
	long jul;
	short ja, jy, jm;
	
	if (yy < 0)
		++yy;
	if (mm > 2) {
		jy = yy;
		jm = mm + 1;
	}
	else {
		jy = yy - 1;
		jm = mm + 13;
	}
	ja = (short)(0.01 * jy);
	jul = (long)(floor(365.25 * jy) + floor(30.6001 * jm) +
				(long)dd + 1720997L - (long)ja +
				(long)(0.25 * (long)ja));

	return jul-START;
} /* date2days */


/* dt is the date and time in the format "Wed Apr 10 hh:mm:ss 1996" and secs
   is the number of seconds since 12/31/1989 00:00:00, as defined by the
   'define' START
*/
/* -------------------------------------------------------------------------- */

void get_datetime_str(void)
{
	sprintf(record.datetime,"%04u-%02u-%02u %02u:%02u:%02u", 
		 record.year,record.monthn,record.dayofmonth,record.ht,record.mt,record.st);
}

void secs2dt(long secs/*, short offset*/)
{
	//static char dt[50];
	//long daysn = (secs + (long)(offset*3600.0)) / 86400L;
	long daysn = secs / 86400L;
	//long rem = secs + (long)(offset*3600.0) - daysn *  86400L;
	long rem = secs - daysn *  86400L;

	//?? if(rem<0L) rem=-rem; /* saw it once, don't know what it ment */
	
	days2date(daysn,(short*)&record.monthn,
		        (short*)&record.dayofmonth,
		        (short*)&record.year);

	record.ht = (int)(rem/3600L);
	record.mt = (int)((rem-(long)record.ht*3600L)/60L);
	record.st = (int)((rem-(long)record.ht*3600L)-(long)record.mt*60L);

	//get_datetime_str();
	//return dt;
} /* secs2dt */

char *ltrim(char *s)
{
	register char *p;

	p=s;
	while((*p)==' ') p++;
	strcpy(s,p);
	return(s);
}

/* -------------------------------------------------------------------------- */
/*
 * Strips trailing blanks from the string
 */
char *rtrim(char *s)
{
	register int i;

	if(!(i=strlen(s))) return(s);
	i--;
	while((s[i]==' ') && (i>=0)) s[i--]='\0';
	return(s);
}

/* -------------------------------------------------------------------------- */
/*
 * Strips leading and trailing blanks from the string.
 */
char *trim(char *s)
{
	ltrim(s);
	rtrim(s);
	return(s);
}

/* getstrings:
 * char *getstrings(s1,s2,s3)
 * char *s1, *s2;
 * int  s3;
 *
 *  Works somewhat like strtok(3).  In fact, if s3 is 0 this routine
 *  operates *exactly* like strtok3.
 *
 *  When s3 is 1, the static variable ch is bumped up by the value
 *   of s3.  This will allow a linear array of characters separated by
 *   a NULL to be decoded.
 *
 *  Example:
 *
 *
 *char a[100]="One\0Two\0Three\0Four\0Five";
 *main() {
 *
 *	int i;
 *	char *p;
 *
 *	for(i=0;i<30;i++) printf("%c  ",a[i]);
 *
 *	p=getstring(a,"\0",0);
 *	printf("'\n%s'\n",p);
 *
 *	p=getstring(NULL,"",1);
 *	printf("'%s'\n",p);
 *
 *	p=getstring(NULL,"",1);
 *	printf("'%s'\n",p);
 *
 *	p=getstring(NULL,"",1);
 *	printf("'%s'\n",p);
 *
 *	p=getstring(NULL,"",1);
 *	printf("'%s'\n",p);
 *
 *	p=getstring(NULL,"",1);
 *	printf("'%s'\n",p);
 * 
 *}
 *	will print:
 *			'One'
 *			'Two'
 *			'Three'
 *			'Four'
 *			'Five'
 *			'(null)'
 *	
 *
 */

char *getstrings(char *source, char *separators, int bump )
{
	static char *ch;
	register char *sep, *token;

	/* if no source supplied, use the previous one.
	 */
	if( source != (char *) NULL )
		ch = source;

	/* For each character in the source string, see if it is a separator
	 * character.
	 */
	if(bump) ch+=bump;
	for( token = ch; *ch; ++ch ) {
		for( sep = separators; *sep; ++sep ) {

			/* look for a separator
			 */
			if( *ch == *sep ) {
				/* Got one, put a null there and move the
				 * saved source pointer up one for next time.
				 */
				*ch = 0;
				++ch;
				return token;
			}
		}
	}

	/* If nothing was found, then return nothing
	 * else return the token
	 */
	if( token == ch )
		return (char *) NULL;
	else
		return token;
} /* getstrings */

//============================================================================================
//   "WGS 84",                     6378137.0,   298.257223563 },  /* 20   */
//============================================================================================
static void datumParams(double *a, double *es)
{
    double f = UTMf;				// flattening

    *es = 2 * f - f*f;				// eccentricity^2
    *a =  UTMa;						// semimajor axis
}
//============================================================================================

// Equations from "Map Projections -- A Working Manual", USGS Professional Paper 1395

static double M(double phi, double a, double es)
{
	if (phi == 0.0)
		return 0.0;
	else {
		return a * (
			( 1.0 - es/4.0 - 3.0*es*es/64.0 - 5.0*es*es*es/256.0 ) * phi -
			( 3.0*es/8.0 + 3.0*es*es/32.0 + 45.0*es*es*es/1024.0 ) * sin(2.0 * phi) +
			( 15.0*es*es/256.0 + 45.0*es*es*es/1024.0 ) * sin(4.0 * phi) -
			( 35.0*es*es*es/3072.0 ) * sin(6.0 * phi) );
	}
}

/* -------------------------------------------------------------------------- */

static void toTM(double lat, double lon, double lat0, double lon0, double k0, double *x, double *y)
{
	double m, et2, n, t, c, A, a, m0, es, lambda, phi, lambda0, phi0;
	
	datumParams(&a, &es);
	
	lambda = lon * Degree;
	phi = lat * Degree;

	phi0 = lat0 * Degree;
	lambda0 = lon0 * Degree;
	
	m0 = M(phi0, a, es);
	m = M(phi, a, es);
	
	et2 = es / (1 - es);
	n = a / sqrt(1 - es * pow(sin(phi), 2.0));
	t = pow(tan(phi), 2.0);
	c = et2 * pow(cos(phi), 2.0);
	A = (lambda - lambda0) * cos(phi);
	*x = k0*n*(A + (1.0 - t + c)*A*A*A/6.0 
			+ (5.0 - 18.0*t + t*t + 72.0*c - 58.0*et2)*pow(A, 5.0) / 120.0);
	*y = k0*(m - m0 + n*tan(phi)*(A*A/2.0
			+ (5.0 - t + 9.0*c + 4*c*c)*pow(A, 4.0)/24.0
			+ (61.0 - 58.0*t + t*t + 600.0*c - 330.0*et2)*pow(A, 6.0)/720.0) );

}

//============================================================================================

static void toUPS(double lat, double lon, double *x, double *y)
{
	double a, t, e, es, rho;
	const double k0 = 0.994;

	double lambda = lon * Degree;
	double phi = fabs(lat * Degree);
	
	datumParams(&a, &es);
	e = sqrt(es);
	t = tan(Pi/4.0 - phi/2.0) / pow( (1.0 - e * sin(phi)) / (1.0 + e * sin(phi)), (e/2.0) );
	rho = 2.0 * a * k0 * t / sqrt(pow(1.0+e, 1.0+e) * pow(1.0-e, 1.0-e));
	*x = rho * sin(lambda);
	*y = rho * cos(lambda);

	if (lat > 0.0)	// Northern hemisphere
		*y = -(*y);
	*x += 2.0e6;	// Add in false easting and northing
	*y += 2.0e6;
}

//============================================================================================
void DegToUTM(double lat, double lon, char *zone, double *x, double *y)
{
	char nz;
	double lon0;
	const double k0 = 0.9996;
	const double lat0 = 0.0;	// reference transverse mercator latitude
	
	if ((lat >= -80.0) && (lat <= 84.0)) {
		nz = 'C'+((short)(lat + 80.0)) / 8;
		if (nz > 'H') ++nz;					// skip 'I' and 'O'
		if (nz > 'N') ++nz;
		lon0 = 6.0 * floor(lon / 6.0) + 3.0;	
		sprintf(zone, "%02d%c", ((short)lon0 +183) / 6, nz);
		toTM(lat, lon, lat0, lon0, k0, x, y);
		*x += 500000.0;				// false easting
		if (lat < 0.0)				// false northing for southern hemisphere
			*y = 10000000.0 - *y;
	}
	else {
		strcpy(zone, "00x");		
		if (lat > 0.0)
			if (lon < 0.0) zone[2] = 'Y';
					  else zone[2] = 'Z';
		else
			if (lon < 0.0) zone[2] = 'A';
					  else zone[2] = 'B';
		toUPS(lat, lon, x, y);
	}
} // DegToUTM

void getDMS(DEGTYP *p,double val)
{
	/*fdeg --> isgn,ideg,imin,fsec and fmin --*/
	double fmin;
 	
	if(p->bNeg=(val<0.0)) val=-val;

	p->ideg=(short)val;
	if((fmin=60.0*(val-(double)p->ideg))>=59.99995) {
		p->ideg++;
		p->fsec=0.0;
		p->imin=0;
	}
	else {
	    p->imin=(short)fmin;
	    if((p->fsec=(float)(60.0*(fmin-(double)p->imin)))>=59.995) {
			p->fsec=0.0;
			if(++p->imin>=60) {
				p->imin=0;
				p->ideg++;
			}
		}
	}
}

