/*julian.c -- Routines to manipulate Julian dates*/
#include <trx_str.h>
#include <stdio.h>

/*
http://www.hermetic.ch/cal_stud/jdn.htm#comp

Mathematicians and programmers have naturally interested themselves in mathematical
and computational algorithms to convert between Julian day numbers and Gregorian dates.
The following conversion algorithm is due to Henry F. Fliegel and Thomas C. Van Flandern:
The Julian day (jd) is computed from Gregorian day, month and year (d, m, y) as follows:


     jd = ( 1461 * ( y + 4800 + ( m - 14 ) / 12 ) ) / 4 +
          ( 367 * ( m - 2 - 12 * ( ( m - 14 ) / 12 ) ) ) / 12 -
          ( 3 * ( ( y + 4900 + ( m - 14 ) / 12 ) / 100 ) ) / 4 +
          d - 32075

Converting from the Julian day number to the Gregorian date is performed thus:

        l = jd + 68569
        n = ( 4 * l ) / 146097
        l = l - ( 146097 * n + 3 ) / 4
        i = ( 4000 * ( l + 1 ) ) / 1461001
        l = l - ( 1461 * i ) / 4 + 31
        j = ( 80 * l ) / 2447
        d = l - ( 2447 * j ) / 80
        l = j / 11
        m = j + 2 - ( 12 * l )
        y = 100 * ( n - 49 ) + i + l

Days are integer values in the range 1-31, months are integers in the range 1-12,
and years are positive or negative integers. Division is to be understood as in
integer arithmetic, with remainders discarded. In these algorithms Julian day
number 0 corresponds to -4713-11-24 (Gregorian), which is -4712-01-01 (Julian).

These algorithms are valid only in the Gregorian Calendar and the proleptic
Gregorian Calendar. They do not correctly convert dates in the Julian Calendar.
It seems that the designers of these algorithms intended them to be used only with
non-negative Julian day numbers (corresponding to Gregorian dates on and after
-4713-11-24 G). In fact they are valid (only) for dates from -4900-03-01 G
onward when converting from a Julian day number to a date, and (only) from
-4800-03-01 G onward when converting from a date to a Julian day number.
*/

TRXFCN_I trx_GregToJul(int y,int m,int d)
{
	return ( 1461 * ( y + 4800 + ( m - 14 ) / 12 ) ) / 4 +
          ( 367 * ( m - 2 - 12 * ( ( m - 14 ) / 12 ) ) ) / 12 -
          ( 3 * ( ( y + 4900 + ( m - 14 ) / 12 ) / 100 ) ) / 4 +
          d - 32075;
/*
	int M1 = (m-14)/12;
	int Y1 = y + 4800;

	return 1461*(Y1+M1)/4 + 367*(m-2-12*M1)/12 - (3*((Y1+M1+100)/100))/4 + d - 32075;
*/
}

TRXFCN_V trx_JulToGreg(int jd,int *y,int *m, int *d)
{
 	int i,l,n;

    l = jd + 68569;
    n = ( 4 * l ) / 146097;
    l = l - ( 146097 * n + 3 ) / 4;
    i = ( 4000 * ( l + 1 ) ) / 1461001;
    l = l - ( 1461 * i ) / 4 + 31;
    jd = ( 80 * l ) / 2447;
    *d = l - ( 2447 * jd ) / 80;
    l = jd / 11;
    *m = jd + 2 - ( 12 * l );
    *y = 100 * ( n - 49 ) + i + l;
/*
	int p = jd + 68569;
	int q = 4*p/146097;
	int r = p - (146097*q + 3)/4;
	int s = 4000*(r+1)/1461001;
	int t = r - 1461*s/4 + 31;
	int u = 80*t/2447;
	int v = u/11;

	*y = 100*(q-49)+s+v;
	*m = u + 2 - 12*v;
	*d = t - 2447*u/80;
*/
}

TRXFCN_I trx_DateToJul(void *pDate)
{
	DWORD dw=*(DWORD *)pDate;
	if(!(dw&0xFFFFFF)) dw=(1000<<9)+(1<<5)+1;
	return trx_GregToJul((dw>>9)&0x7FFF,(dw>>5)&0xF,dw&0x1F);
}

TRXFCN_V trx_JulToDate(int jd,void *pDate)
{
	int y,m,d;
	trx_JulToGreg(jd,&y,&m,&d);
	d+=(y<<9)+(m<<5); // 15:4:5
	memcpy(pDate,&d,3);
}

TRXFCN_S trx_JulToDateStr(int jd,char *buf)
{
	int y,m,d;
	trx_JulToGreg(jd,&y,&m,&d);
	if(y<1000 || y>=3000 || m<1 || m>12 || d<1 || d>31) *buf=0;
	else sprintf(buf,"%04d-%02d-%02d",y,m,d);
	return buf;
}

TRXFCN_S trx_Strxchg(char *p,char cOld,char cNew)
{
   char *p0=p;
   for(;*p;p++) if(*p==cOld) *p=cNew;
   return p0;
}

TRXFCN_I trx_DateStrToJul(LPCSTR p)
{
    //Allow mm-dd-yy, mm-dd-yyyy, or yyyy-mm-dd
    char *p1,*p2;
    int y,m,d;
	char buf[16];

    p=trx_Strxchg(trx_Stncc(buf,p,sizeof(buf)),'-','/');
    if((p1=strchr(p,'/')) && (p2=strchr(p1+1,'/'))) {
       if(p1-p==4) {
       	  //yyyy-mm-dd
          y=atoi(p); m=atoi(p1+1); d=atoi(p2+1);
       }
       else {
          //mm-dd-yyyy
          y=atoi(p2+1); m=atoi(p); d=atoi(p1+1);
       }
	   if(y<100) y+=(y<30)?2000:1900;
       if(y>=1000 && y<3000 && m>0 && m<13 && d>0 && d<32) return trx_GregToJul(y,m,d);
   }
   return -1;
}

