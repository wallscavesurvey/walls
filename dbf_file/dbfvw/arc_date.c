/*arc_date.c -- Routines to manipulate dates*/
#include <trx_str.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static int DC_days_in_year_[2][14] =
{
    { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

static int DC_days_in_month_[2][13] =
{
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static long DC_year_to_days(int year)
{
    long days;

    days = year * 365L;
    days += year >>= 2;
    days -= year /= 25;
    days += year >>  2;
    return(days);
}

static BOOL DC_leap_year(int year)
{
    int yy;

    return( ((year & 0x03) == 0) &&
            ( (((yy = (int) (year / 100)) * 100) != year) ||
              ((yy & 0x03) == 0) ) );
}

/*
static BOOL DC_check_date(int year, int month, int day)
{
    if ((year >= 1) &&
        (month >= 1) && (month <= 12) &&
        (day >= 1) &&
        (day <= DC_days_in_month_[DC_leap_year(year)][month]))
      return(1);
    else return(0);
}
*/

static long DC_date_to_days(int year, int month, int day)
{
    BOOL leap;

    if ((year >= 1) &&
        (month >= 1) && (month <= 12) &&
        (day >= 1) &&
        (day <= DC_days_in_month_[leap=DC_leap_year(year)][month]))
      return( DC_year_to_days(--year) +
              DC_days_in_year_[leap][month] + day );
    else return(0L);
}


static BOOL DC_add_delta_days(int *year, int *month, int *day, long Dd)
{
	/*==================================================================
	This function has two principal uses:

	First, it can be used to calculate a new date, given an initial date and
	an offset (which may be positive or negative) in days, in order to answer
	questions like "today plus 90 days -- which date gives that?".

	(In order to add a weeks offset, simply multiply the weeks offset with
	7 and use that as your days offset.)

	Second, it can be used to convert the canonical representation of a date,
	i.e., the number of that day (where counting starts at the 1st of January
	in 1 A.D.), back into a date given as year, month and day.

	Because counting starts at 1, you will actually have to subtract 1
	from the canonical date in order to get back the original date:

	  $canonical = Date_to_Days($year,$month,$day);

	  ($year,$month,$day) = Add_Delta_Days(1,1,1, $canonical - 1);

	Moreover, this function is the inverse of the function Delta_Days():

	  Add_Delta_Days(@date1, Delta_Days(@date1, @date2))

	yields @date2 again, whereas

	  Add_Delta_Days(@date2, -Delta_Days(@date1, @date2))

	yields @date1, and

	  Delta_Days(@date1, Add_Delta_Days(@date1, $delta))

	yields $delta again.
	=======================================================================*/
    long  days;
    BOOL leap;

    if (((days = DC_date_to_days(*year,*month,*day)) > 0L) &&
        ((days += Dd) > 0L))
    {
        *year = (int) ( days / 365.2425 );
        *day  = (int) ( days - DC_year_to_days(*year) );
        if (*day < 1)
        {
            *day = (int) ( days - DC_year_to_days(*year-1) );
        }
        else (*year)++;
        leap = DC_leap_year(*year);
        if (*day > DC_days_in_year_[leap][13])
        {
            *day -= DC_days_in_year_[leap][13];
            leap  = DC_leap_year(++(*year));
        }
        for ( *month = 12; *month >= 1; (*month)-- )
        {
            if (*day > DC_days_in_year_[leap][*month])
            {
                *day -= DC_days_in_year_[leap][*month];
                break;
            }
        }
        return(1);
    }
    else return(0);
}
/*===============================================================*/
/*date and time fcns --*/

#define v_year ((UINT)(((BYTE *)v)[1]+1900-32))
#define v_mon ((BYTE)(((BYTE *)v)[2]-32))
#define v_day ((BYTE)(((BYTE *)v)[3]-32))
#define _d1900 693596
#define _d1970 719163
#define _d1978 722085

BOOL datechk(LPCSTR v)
{
	int y=v_year;
	int m=v_mon;
	int d=v_day;

	return *v==3 && y>=1900 && m>=0 && m<=12 && d>=0 &&
		d<=DC_days_in_month_[DC_leap_year(y)][m];
}

int datedays(LPCSTR v)
{
	/* Compute days since start of 1 Jan 1900 from date variable:
	   v[1]=year-1900+32; v[2]=month+32; v[3]=days+32

           n=DC_date_to_days(1900,1,1); (n=693596=_d1900)
	   n=DC_date_to_days(1970,1,1); (n=719163=_d1970)
           n=DC_date_to_days(1978,1,1); (n=722085=_d1978)
        */

        return DC_date_to_days(v_year,v_mon,v_day)-_d1900;
}

time_t datetime_t(LPCSTR v)
{
	static struct tm t;
	time_t tt;

	if(!t.tm_mday) {
		tt=time(0);
		t=*localtime(&tt);
		/*This should set t.tm_gmtoff=-6*60*60=-21600*/
	}
	t.tm_hour=t.tm_min=t.tm_sec=0;
	t.tm_year=(BYTE)v[1]-32;
	t.tm_mon=(BYTE)v[2]-32-1;
	t.tm_mday=(BYTE)v[3]-32;
	t.tm_isdst=-1; /*unknown if daylight savings is in effect*/
	tt=mktime(&t);
	return tt;
}

TRXFCN_S datestr(LPSTR s8,LPCSTR v)
{
	UINT y=v_year;
	UINT m=v_mon;
	UINT d=v_day;

	if(m>=100) m=99;
	if(d>=100) d=99;
	sprintf(s8,"%02u/%02u/%02u",m,d,y%100);
	if(!m) s8[0]=s8[1]=' ';
	if(!d) s8[3]=s8[4]=' ';
	if(y==1900) s8[6]=s8[7]=' ';
	return trx_Stxcp(s8);
}

TRXFCN_S datestr10(LPSTR s10,LPCSTR v)
{
	UINT m=v_mon;
	UINT d=v_day;
	UINT y=v_year;

	if(m>=100) m=0;
	if(d>=100) d=0;
	sprintf(s10,"%02u/%02u/%04u",m,d,y);
	if(!m) s10[0]=s10[1]=' ';
	if(!d) s10[3]=s10[4]=' ';
	if(y==1900) s10[6]=s10[7]=s10[8]=s10[9]=' ';
	return trx_Stxcp(s10);
}

TRXFCN_S dateval(LPSTR v,LPCSTR p)
{
  int m,d,y;

  memcpy(v,p+7,2);
  if(v[0]==' ' && v[1]==' ') y=0;
  else {
	  v[2]=0;
	  y=atoi(v);
	  if(y<ARC_MINYEAR) {
		  if(y<0) y=0;
		  else y+=100;
	  }
  }
  d=atoi(p+4); if(d<0) d=0;
  m=atoi(p+1); if(m<0) m=0;
  v[0]=3; v[1]=y+32; v[2]=m+32; v[3]=d+32;
  return v;
}

TRXFCN_S daysdate(LPSTR v,UINT days)
{
	int y,m,d;

	v[0]=3;
        y=m=d=1;
        if(!DC_add_delta_days(&y,&m,&d,days+_d1900-1)) v[1]=v[2]=v[3]=32;
	else {
	   v[1]=(BYTE)(y-1900+32);
	   v[2]=(BYTE)(m+32);
	   v[3]=(BYTE)(d+32);
	}
	return v;
}

time_t daystime_t(int datedays)
{
   /*datedays==days since 1900 in UTC*/
   /*This fcn returns seconds since 1970 in UTC irrespective of daylight savings*/

   return (datedays-(_d1970-_d1900))*60*60*24+timezone;
}

TRXFCN_S trx_FileDate(LPSTR v,UINT days)
{
  return daysdate(v,days+(_d1978-_d1900));
}

int filedays(LPCSTR v)
{
  /*Old CDOS filedate: Days since start of 1 Jan 1978*/

  return datedays(v)-(_d1978-_d1900); /*datedays-28489*/
}

TRXFCN_S systime(LPSTR s3)
{
	time_t tt=time(0);
	struct tm *t=localtime(&tt);
	s3[0]=3;
	s3[1]=(BYTE)t->tm_hour;
	s3[2]=(BYTE)t->tm_min;
	s3[3]=(BYTE)t->tm_sec;
	return s3;
}

TRXFCN_S sysdate(LPSTR s3)
{
	time_t tt=time(0);
	struct tm *t=localtime(&tt);
	s3[0]=3;
	s3[1]=(BYTE)(t->tm_year+32);
	s3[2]=(BYTE)(t->tm_mon+33);
	s3[3]=(BYTE)(t->tm_mday+32);
	return s3;
}

TRXFCN_S timestr(LPSTR s8,LPCSTR s3)
{
	UINT h=s3[1];
	UINT m=s3[2];
	UINT s=s3[3];

	if(h>=100) h=99;
	if(m>=100) m=99;
	if(s>=100) s=99;

	sprintf(s8,"%02u:%02u:%02u",h,m,s);
	return trx_Stxcp(s8);
}

