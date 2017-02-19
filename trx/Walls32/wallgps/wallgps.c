//Wallgps.c

#include "gps.h"
#include <trx_str.h>

enum e_gpsopts {GPS_PORTMASK=15,GPS_GETWP=16,GPS_GETTRK=32,GPS_LOCALTIME=64,GPS_WRITECSV=128,GPS_RESTRICT=256};
//enum e_gpsopts {GPS_PORTMASK=7,GPS_GETWP=8,GPS_GETTRK=16,GPS_LOCALTIME=32,GPS_WRITECSV=64};

#define GPS_LENNAME 15
#define GPS_LENCOMMENT 31
#define WAYPT_INC 200
#define TRACKPT_INC 600

enum {PRJ_CFG_BOOK=1,
	  PRJ_CFG_ENDBOOK,
	  PRJ_CFG_NAME,
      PRJ_CFG_OPTIONS,
	  PRJ_CFG_PATH,
	  PRJ_CFG_REF,
	  PRJ_CFG_STATUS,
      PRJ_CFG_SURVEY
};

enum e_ref {REF_FMTDM=1,REF_FMTDMS=2,REF_FMTMASK=3,REF_LONDIR=4,REF_LATDIR=8};

static LPSTR prjtypes[]={
   "BOOK",
   "ENDBOOK",
   "NAME",
   "OPTIONS",
   "PATH",
   "REF",
   "STATUS",
   "SURVEY"
};

static char gps_badchrs[]={' ',':',',',';','#',0};
static char gps_goodchrs[]={'_','|','.','@','$',0};

#pragma pack(1)
typedef struct { //8+8+4+2+16+32+2=72 bytes
	double east;
	double north;
	float  up;
	WORD icon_smbl;
	char name[GPS_LENNAME+1];
	char comment[GPS_LENCOMMENT+1];
	WORD zone; //index to zone array
} GPS_WAYPT;

typedef struct { //24+8=32 bytes
	double east;
	double north;
	float up;
	float conv;
	BYTE year; //year minus 1900
	BYTE month;
	BYTE day;
	BYTE hr;
	BYTE min;
	BYTE sec;
	BYTE start;
	BYTE zone;
} GPS_TRACKPT;

#pragma pack()

typedef struct {
  int uTrkDist;
  int uLimitDist;
  char name[GPS_LENNAME+1];
} GPS_AREA;

static int waypt_bound,waypts;
static int trackpt_bound,trackpts;
static GPS_WAYPT *waypt;
static GPS_TRACKPT *trackpt;
static GPS_AREA *pArea;
static double dLimitDist2,dTrkDist2,dLimitX,dLimitY;
static int dLimitZone;
static BOOL	bGetTRK,bGetWP;
static char limitZone[16];

static jmp_buf jmpbuf;
LPFN_GPS_CB Walls_CB;

char gps_errmsg[128];
char dl_time[24]; // mm/dd/yyyy hh:mm:ss
char prj_path[MAX_PATH];
char *prj_stem; //position of '-' in prj_path for creating SRV pathname: <pfx>-<zone>.srv

struct RECORD record;
INTTYP	gotID;			// set to 1 when the connected GPS has been ID'd
BOOL	bLocalTime,bWptAlt,bTrkAlt;
BOOL	bWriteCSV;
BOOL	bWptFound;

WORD	SWVersion;		/* version * 100				     */
int		VersionSubStrings;
char	VersionString[99];	/* model/version in string format    */
char	*VersionSubString[10];// version substrings

BYTE 	LastGarminGPSMsg[MAX_LENGTH];
short	LastGarminLen;
BYTE	SerialMessage[MAX_LENGTH];
BYTE	SerialMessageOut[MAX_LENGTH];

typedef struct {
	DEGTYP la;
	DEGTYP lo;
	double east;
	double north;
	float up;
	float conv;
} ZONE_DATA;

#define LEN_TRACKNAME 50
#define MAX_TRACKNAMES 11
static char trackName[MAX_TRACKNAMES][LEN_TRACKNAME+1];
static int numTrackNames;

#define SIZ_ZONELIST 20
#define SIZ_ZONESTR 4
static char	zoneList[SIZ_ZONELIST][SIZ_ZONESTR];
static ZONE_DATA zoneData[SIZ_ZONELIST];

static int numZones,tp_namecnt;
static UINT tp_lastdatecode;

static INTTYP com_open;

static FILE	*fpprj,*fpsrv,*fpcsv;		// used when creating walls project

//=====================================================================
static void Catcher(int dd)
{
	char *pms;
	switch(dd) {
		case SIGFPE : pms="Floating point trap";break;
		case SIGILL : pms="Illegal instruction"; break;
		case SIGINT : pms="Break or CNTL-C pressed";break;
		case SIGSEGV: pms="Memory access violation";break;
		case SIGTERM: pms="Termination signal received";break;
		case SIGABRT: pms="Abort signal";
		default: pms="Unknown reason";
	}
	sprintf(gps_errmsg,"Exception: %s",pms);
	byebye(-1);	
} /* Catcher */

/* -------------------------------------------------------------------------- */

void delay(long ms)
{
   struct _timeb timebuffer;
   __time64_t t1,t2;
   
   _ftime( &timebuffer );
   t1=timebuffer.millitm+timebuffer.time*1000;
   _ftime( &timebuffer );
   t2=timebuffer.millitm+timebuffer.time*1000;
   while((long)(t2-t1)<ms){
		_ftime( &timebuffer );
		t2=timebuffer.millitm+timebuffer.time*1000;
   }
}

/*
 * exits closing COM port if necessary
 */
void byebye(INTTYP rt)
{
	int i;
	if(com_open) AsyncStop();
	if(waypt) free(waypt);
	if(trackpt) free(trackpt);
	for(i=0;i<VersionSubStrings;i++) free(VersionSubString[i]);
	if(fpsrv) fclose(fpsrv);
	if(fpcsv) fclose(fpcsv);
	if(fpprj) fclose(fpprj);
	if(rt) longjmp(jmpbuf,rt);
} /* byebye */

/* -------------------------------------------------------------------------- */
//  set_sign works when the values of record.ns and record.ew are correct
//    it sets the values of record.la (N+,S-), record.lo (E+,W-)
//
//void set_sign(void)
//{
//	record.la=fabs(record.la);
//	record.lo=fabs(record.lo);
//	if(record.ns=='S') record.la=-record.la; // restore sign
//	if(record.ew=='W') record.lo=-record.lo; // restore sign
//} // set_sign(void)

/* -------------------------------------------------------------------------- */
//
// set_ns works when the sign of record.la and record.lo are correct.  It sets
//   the value of record.ns and record.ew
//
/*
void set_ns(void)
{
	if(record.la<0.0) record.ns='S'; else record.ns='N';
	if(record.lo<0.0) record.ew='W'; else record.ew='E';
} // set_ns
*/
/* -------------------------------------------------------------------------- */
void set_record_icon(void)
{
	record.icon_smbl=default_Garmin_icon;
	record.icon_dspl=1;
	record.datetime[0]=0;
} /* set_record_icon */

/* -------------------------------------------------------------------------- */
/*
 *  returns GARMIN time in seconds.
 * Note:  unix_time = garmin_time + 631065600L(==UnixSecsMinusGpsSecs)
 *        garmin_time = unix time - 631065600L 
 *
 * Note:  unix_time = Lowrance_time + 694224000L
 *        Lowrance_time = unix time - 694224000L
 */
/*
long dt2secs(short offset)
{
	return (86400L*date2days(record.monthn, record.dayofmonth, record.year) +
			  3600L*(long)record.ht + 
			  60L*(long)record.mt   +
			  (long)record.st       -
			  ((long)offset*3600L));
}*/

/* -------------------------------------------------------------------------- */
//		COM1 = 03f8 IRQ 4
//		COM2 = 02f8 IRQ 3
//		COM3 = 03e8 IRQ 4
//		COM4 = 02e8 IRQ 3
static void open_COM(int ComPort)
{
	if(ComPort<COM1 || ComPort>COM15) ComPort=COM1;
	signal(SIGABRT,Catcher);
	signal(SIGFPE,Catcher);
	signal(SIGILL,Catcher);
	signal(SIGINT,Catcher);
	signal(SIGSEGV,Catcher);
	signal(SIGTERM,Catcher);
	AsyncInit(ComPort);
	com_open=1;
	AsyncSet(GARMIN_BAUD_RATE, BYTESIZE, ONESTOPBIT, NOPARITY);
} /* open_COM */

/* -------------------------------------------------------------------------- */
/*
enum e_errmsg {
	GPS_ERR_NONE=0,
	GPS_ERR_ALLOC,
	GPS_ERR_SENDRECORD,
	GPS_ERR_NOWPEOF,
	GPS_ERR_NOTRKEOF,
	GPS_ERR_PROTOCOL,
	GPS_ERR_NORESPOND,
	GPS_ERR_PROTVIOL,
	GPS_ERR_PROTUNK,
	GPS_ERR_MAXNAKS,
	GPS_ERR_NUMRECS,
	GPS_ERR_BADWP,
	GPS_ERR_PORTOUT,
	GPS_ERR_ZONECOUNT,
	GPS_ERR_NODATA,
	GPS_ERR_SRVCREATE,
	GPS_ERR_PRJCREATE,
	GPS_ERR_CSVCREATE,
	GPS_ERR_CANCELLED
};
*/
static char *errmsg[]={
	"No error",
	"Memory allocation failed",
	"Illegal send record",
	"No Waypoint EOF received",
	"No Track EOF received",
	"No protocol data sent by GPS",
	"GPS receiver not responding",
	"Link protocol violation",
	"Unknown link protocol",
	"Too many NAKS sent by GPS",
	"No record count received",
	"Unknown waypoint protocol",
	"Error writing to serial port",
	"Too many different UTM zones",
	"No waypoints or tracks to download",
	"Error creating SRV file",
	"Error creating PRJ file",
	"Error creating CSV file",
	"Operation cancelled"
};

static GPS_TRACKPT *alloc_trackpt(void)
{
	void *ptr;
	if(trackpts>=trackpt_bound) {
		ptr=realloc(trackpt,(trackpt_bound+=TRACKPT_INC)*sizeof(GPS_TRACKPT));
		if(!ptr) return NULL;
		trackpt=(GPS_TRACKPT *)ptr;
	}
	return &trackpt[trackpts++];
}

static GPS_WAYPT *alloc_waypt(void)
{
	void *ptr;
	if(waypts>=waypt_bound) {
		ptr=realloc(waypt,(waypt_bound+=WAYPT_INC)*sizeof(GPS_WAYPT));
		if(!ptr) return NULL;
		waypt=(GPS_WAYPT *)ptr;
	}
	return &waypt[waypts++];
}

static double get_conv(void)
{
    double d = 6.0 * floor(record.lo/6.0) + 3.0;
	return (double)(sin(Degree*record.la)*(record.lo-d));
}

static BYTE getZoneIndex(char *zone,double east,double north)
{
	int i;
	static int lastIndex=0;
	ZONE_DATA *pz;

	if(numZones && !strcmp(zone,zoneList[lastIndex])) return (BYTE)lastIndex;
	for(i=0;i<numZones;i++) if(!strcmp(zone,zoneList[i])) goto _ret;
	if(i==SIZ_ZONELIST) byebye(GPS_ERR_ZONECOUNT);
	if(strlen(zone)>3) {
		sprintf(gps_errmsg,"Unrecognized zone id: %s",zone);
		byebye(-1);
	}
	pz=&zoneData[numZones];
	getDMS(&pz->la,record.la);
	getDMS(&pz->lo,record.lo);
	pz->conv=(float)get_conv();
	pz->up=record.altitude;
	pz->east=east;
	pz->north=north;
	strcpy(zoneList[numZones++],zone);

_ret:
	return (BYTE)(lastIndex=i);
}

static int GetTrackNameIndex(void)
{
	record.ident[40]=0;
	if(numTrackNames && (!strcmp(record.ident,trackName[numTrackNames-1]) ||
		numTrackNames==MAX_TRACKNAMES)) return numTrackNames;

	strcpy(trackName[numTrackNames++],record.ident);
	return numTrackNames;
}

static BOOL break_test(int i)
{
	double x2,y2;
	if(!i || trackpt[i].zone!=trackpt[i-1].zone) return TRUE;
	//Break track when pt separation exceeds ... m
	x2=trackpt[i].east-trackpt[i-1].east;
	x2*=x2;
	if(x2>dTrkDist2) return TRUE;
	y2=trackpt[i].north-trackpt[i-1].north;
	y2*=y2;
	return (x2+y2>dTrkDist2);
}

void push_track()
{
	char zone[16];
	double x,y;
	GPS_TRACKPT	*tp;
	static BOOL bGetting=FALSE;
	static int iTrackName=0;

	if(record.type!='T') iTrackName=GetTrackNameIndex();

	DegToUTM(record.la,record.lo,zone,&x,&y);

	if(pArea) {
		if(!strcmp(zone,limitZone) &&
			(((x-dLimitX)*(x-dLimitX)+(y-dLimitY)*(y-dLimitY))<=dLimitDist2)) {
			//Point is in range --
			if(!bGetting) {
				record.type='N';
				bGetting=TRUE;
			}
		}
		else {
			bGetting=FALSE;
			return;
		}
	}

	if(!(tp=alloc_trackpt())) byebye(GPS_ERR_ALLOC);
	tp->zone=(BYTE)getZoneIndex(zone,x,y);
	tp->east=x;
	tp->north=y;
	tp->conv=(float)get_conv();
	tp->up=record.altitude;
	if(tp->month=(BYTE)record.monthn) {
		tp->year=(BYTE)(record.year-1900);
		tp->day=(BYTE)record.dayofmonth;
		tp->hr=(BYTE)record.ht;
		tp->min=(BYTE)record.mt;
		tp->sec=(BYTE)record.st;
	}
	if(record.type!='T' || break_test(trackpts-1)) tp->start=(BYTE)iTrackName;
	else tp->start=0;
}

/* -------------------------------------------------------------------------- */

static void fix_name(char *s)
{
	char *pos,*p=gps_errmsg;
	int i,len=strlen(s);
	for(i=0;i<len;i++,s++,p++) {
		if(i && i==(len-8)) *p++=':';
		if(pos=strchr(gps_badchrs,*s)) *p=gps_goodchrs[pos-gps_badchrs];
		else *p=*s;
	}
	*p=0;
}

void push_waypoint()
{
	char zone[16];
	double x,y;
	GPS_WAYPT *wp;
	ZONE_DATA *pz;

	DegToUTM(record.la,record.lo,zone,&x,&y);

	if(pArea) {
		if(!bWptFound) {
			if(!strcmp(pArea->name,record.ident)) {
				strcpy(limitZone,zone);
				dLimitX=x; dLimitY=y;
				bWptFound++;
				pz=&zoneData[dLimitZone=getZoneIndex(zone,x,y)];
				//Override geographical reference --
				getDMS(&pz->la,record.la);
				getDMS(&pz->lo,record.lo);
				pz->conv=(float)get_conv();
				pz->up=record.altitude;
				pz->east=x;
				pz->north=y;
			}
			if(!bGetWP) return;
		}
		else if(!bGetWP || strcmp(zone,limitZone) ||
				(((x-dLimitX)*(x-dLimitX)+(y-dLimitY)*(y-dLimitY))>dLimitDist2)) return;
	}
	
	if(!(wp=alloc_waypt())) byebye(GPS_ERR_ALLOC);
	wp->zone=getZoneIndex(zone,x,y);
	record.ident[GPS_LENNAME]=0;
	strcpy(wp->name,record.ident);
	record.comment[GPS_LENCOMMENT]=0;
	strcpy(wp->comment,record.comment);
	wp->east=x;
	wp->north=y;
	wp->up=record.altitude;
	wp->icon_smbl=(WORD)record.icon_smbl;
}

DLLExport LPSTR PASCAL ErrMsg(int code)
{
	if(code<0) return gps_errmsg;
	else return errmsg[code];
}

static UINT datecode(GPS_TRACKPT *tp)
{
	return (((tp->year+1900)%100)<<26)+(tp->month<<22)+(tp->day<<17)+(tp->hr<<12)+
		(tp->min<<6)+tp->sec;
}

static void get_tp_name(char *prefix,char *name,GPS_TRACKPT *tp)
{
	UINT code;
	static GPS_TRACKPT *tplast;

	if(tp->month) {
		sprintf(prefix,"%02u-%02u-%02u",(tp->year+1900)%100,tp->month,tp->day);

		if(tp_lastdatecode < (code=datecode(tp))) {
			sprintf(name,"%02u.%02u.%02u",tp->hr,tp->min,tp->sec);
			tp_lastdatecode=code;
			tplast=tp;
		}
		else {
			sprintf(name,".%u",++tp_namecnt);
		}
	}
	else {
		memcpy(prefix,dl_time+2,8);
		prefix[8]=0;
		sprintf(name,".%u",++tp_namecnt);
	}
}

static double Round(double s)
{
	return floor(100.0*s+0.5)/100.0;
}

static void CDECL PrjWrite(UINT nType,const char *format,...)
{
	char buf[256];
	int len=0;
	  
	if(nType) len=sprintf(buf,".%s\t",prjtypes[nType-1]);
	if(format) {
		va_list marker;
		va_start(marker,format);
		len+=_vsnprintf(buf+len,254-len,format,marker);
		va_end(marker);
	}
	else if(len) len--;
	strcpy(buf+len,"\n");
	fprintf(fpprj,buf);
}

static void open_csv(void)
{
	char buf[MAX_PATH];

	strcpy(buf,prj_path);
	strcpy(trx_Stpext(buf),".csv");
	if(!(fpcsv=fopen(buf,"wt"))) byebye(GPS_ERR_CSVCREATE);
	fprintf(fpcsv,"GPS Download %s%s\nDatum: WGS84\n"
		"TYPE,NAME,ZONE,EASTING,NORTHING,ELEV,DATE,TIME,SYMBOL,COMMENT\n",
		dl_time,bLocalTime?"":" (UTC)");

}

static void write_prj_csv(void)
{
	char buf[MAX_PATH];

	strcpy(buf,trx_Stpnam(prj_path));
	strcpy(trx_Stpext(buf),".csv");
	PrjWrite(PRJ_CFG_SURVEY,"%s: GPS Download %.10s",buf,dl_time);
	PrjWrite(PRJ_CFG_NAME,buf);
	PrjWrite(PRJ_CFG_STATUS,"327690");
}

static void write_prj_ref(int z)
{
	ZONE_DATA *zd=&zoneData[z];
	int flag=REF_FMTDMS;

	if(zd->lo.bNeg) flag |= REF_LONDIR;
	if(zd->la.bNeg) flag |= REF_LATDIR;
	PrjWrite(PRJ_CFG_REF,"%.2f %.2f %d %.3f %d %u %u %u %.3f %u %u %.3f 27 \"WGS 1984\"",
	zd->north,
	zd->east,
	atoi(zoneList[z]),
	zd->conv,
	(zd->up<NO_ALTITUDE)?(int)zd->up:0,
	flag,
	zd->la.ideg,
	zd->la.imin,
	zd->la.fsec,
	zd->lo.ideg,
	zd->lo.imin,
	zd->lo.fsec);
}

static void write_prj_survey(int z)
{
	PrjWrite(PRJ_CFG_SURVEY,"GPS Download %.10s, Zone %s",dl_time,zoneList[z]);
	*trx_Stpext(prj_stem)=0;
	PrjWrite(PRJ_CFG_NAME,trx_Stpnam(prj_path));
	PrjWrite(PRJ_CFG_STATUS,(pArea||z==0)?"2056":"2058");
	write_prj_ref(z);
}

static void write_csv_wp(GPS_WAYPT *wp,BOOL bElev)
{
	//"TYPE,NAME,ZONE,EASTING,NORTHING,ELEV,DATE,TIME,SYMBOL#,COMMENT\n"
	fprintf(fpcsv,"W,%s,%s,%.3f,%.3f,",
			wp->name,zoneList[wp->zone],wp->east,wp->north);

	if(bElev && wp->up<NO_ALTITUDE) fprintf(fpcsv,"%.3f",wp->up);
	fprintf(fpcsv,",%.10s,%s,%u,%s\n",dl_time,dl_time+11,wp->icon_smbl,wp->comment);
}

static void write_csv_tp(GPS_TRACKPT *tp,BOOL bElev)
{
	//"TYPE,NAME,ZONE,EASTING,NORTHING,ELEV,DATE,TIME\n"
	if(tp->start) fprintf(fpcsv,"N,%s,",trackName[tp->start-1]);
	else fprintf(fpcsv,"T,,");
	
	fprintf(fpcsv,"%s,%.3f,%.3f,",zoneList[tp->zone],tp->east,tp->north);
	if(bElev && tp->up<NO_ALTITUDE) fprintf(fpcsv,"%.3f",tp->up);
	if(tp->month) fprintf(fpcsv,",%04u-%02u-%02u,%02u:%02u:%02u\n",
		tp->year+1900,tp->month,tp->day,tp->hr,tp->min,tp->sec);
	else fprintf(fpcsv,",,\n");
}

static void outZoneFile(int z)
{
	int i;
	GPS_WAYPT *wp;
	GPS_TRACKPT	*tp;
	BOOL bNewTrack,bHeader=FALSE;
	//BOOL bElev;
	double last_e,last_n,last_u,u;
	char tp_name[10],tp_name_last[10];
	char tp_prefix[10],tp_prefix_last[10];
	int trackNameIndex=0;

	sprintf(prj_stem,"-%s.srv",zoneList[z]);
	fpsrv=fopen(prj_path,"wt");
	if(!fpsrv) byebye(GPS_ERR_SRVCREATE);

	write_prj_survey(z);

	fprintf(fpsrv,";GPS Download %.10s, Zone %s\n;Datum: WGS84 (NAD83)\n;Time of download: %s%s\n",
		dl_time,zoneList[z],dl_time+11,bLocalTime?"":" (UTC)");

	if(pArea) fprintf(fpsrv,";Area of coverage: Locations within %u meters of waypoint %s\n",
		pArea->uLimitDist,pArea->name);

	//bElev=zoneData[z].up<NO_ALTITUDE;

	for(i=0;i<waypts;i++) {
		wp=&waypt[i];
		if(wp->zone!=(BYTE)z) continue;
		if(pArea && (((wp->east-dLimitX)*(wp->east-dLimitX)+
			(wp->north-dLimitY)*(wp->north-dLimitY))>dLimitDist2)) continue;
		if(bWriteCSV) write_csv_wp(wp,bWptAlt);
		if(!bHeader) {
			fprintf(fpsrv,";\n;Waypoint data --\n#seg /Waypoints\n#units order=EN%s prefix=WP-%s\n",
				bWptAlt?"U":"",
				zoneList[z]);
			bHeader=TRUE;
		}
		fix_name(wp->name); //puts name in gps_errmsg[]
		fprintf(fpsrv,"#flag\t%s\t/Waypt-%u\n#fix\t%s\t%.2lf\t%.2lf",
			gps_errmsg,wp->icon_smbl,gps_errmsg,wp->east,wp->north);

		if(bWptAlt) fprintf(fpsrv,"\t%.2f",(wp->up<NO_ALTITUDE)?wp->up:0.0f);
		else fprintf(fpsrv,"\t(0,?)");
		
		if(*wp->comment) {
			fprintf(fpsrv,"\t/%s",wp->comment);
		}
		fprintf(fpsrv,"\n");
	}

	bHeader=FALSE;
	tp_lastdatecode=0; //Check for time reversals within zone;

	for(i=0;i<trackpts;i++) {
		tp=&trackpt[i];
		if(tp->zone!=(BYTE)z) {
			bNewTrack=TRUE;
			continue;
		}
		if(!bHeader) {
			fprintf(fpsrv,";\n;Track data --\n");
			bHeader=bNewTrack=TRUE; //force new track
		}
		if(bWriteCSV) write_csv_tp(tp,bTrkAlt);
		if(tp->start || bNewTrack) {
			if(tp->start) trackNameIndex=tp->start;
			if(!tp->month)
				fprintf(fpsrv,";Undated track points. Name prefix is download date%s.\n",
					bLocalTime?"":" (UTC)");
			else if(!bLocalTime) fprintf(fpsrv,";Track point times are UTC.\n");
			*tp_name=0;
			get_tp_name(tp_prefix,tp_name,tp);
			fprintf(fpsrv,"#units rect order=EN%s rect=%.4f grid=%.4f prefix=%s\n#seg\t/Tracks",
				bTrkAlt?"U":"",tp->conv,tp->conv,tp_prefix);
			if(trackNameIndex && *trackName[trackNameIndex-1]) fprintf(fpsrv,"/%s",trackName[trackNameIndex-1]);
			if(tp->month) fprintf(fpsrv,"/%s",tp_prefix);

			fprintf(fpsrv,"\n#flag\t%s\t/Track start\n#fix\t%s\t%.2lf\t%.2lf",
				tp_name,tp_name,tp->east,tp->north);
			last_e=Round(tp->east);
			last_n=Round(tp->north);
			if(bTrkAlt) {
				last_u=(tp->up<NO_ALTITUDE)?tp->up:0.0;
				fprintf(fpsrv,"\t%.2lf",last_u);
				if(last_u) last_u=Round(last_u);
			}
			bNewTrack=FALSE;
		}
		else {
			strcpy(tp_name_last,tp_name);
			strcpy(tp_prefix_last,tp_prefix);
			get_tp_name(tp_prefix,tp_name,tp);
			if(strcmp(tp_prefix,tp_prefix_last))
				fprintf(fpsrv,"#prefix\t%s\n%s:",tp_prefix,tp_prefix_last);
			fprintf(fpsrv,"%s\t%s\t%.2lf\t%.2lf",tp_name_last,tp_name,tp->east-last_e,tp->north-last_n);
			last_e+=Round(tp->east-last_e);
			last_n+=Round(tp->north-last_n);
			if(bTrkAlt) {
				u=(tp->up<NO_ALTITUDE)?tp->up:0.0;
				fprintf(fpsrv,"\t%.2lf",u-last_u);
				last_u+=Round(u-last_u);
			}
		}
		if(!bTrkAlt) fprintf(fpsrv,"\t(0,?)");
		if(tp->month && *tp_name=='.') fprintf(fpsrv,"\t;%02u.%02u.%02u",tp->hr,tp->min,tp->sec);

		fprintf(fpsrv,"\n");
	}

	if(fclose(fpsrv)) byebye(GPS_ERR_SRVCREATE);
	fpsrv=NULL;
}

static void init_prj_path(LPCSTR p)
{
	int i;
	trx_Stcext(prj_path,p,"wpj",sizeof(prj_path));
	p=trx_Stpnam(prj_path);
	for(i=0;i<4;i++,p++) {
	  if(*p=='.' || *p==' ') break;
	}
	prj_stem=(LPSTR)p;
}

DLLExport int PASCAL Import(LPCSTR lpPrjPath,GPS_AREA *pA,UINT flags,LPFN_GPS_CB pCB)
{
	int e,port;

	Walls_CB=pCB;
	bLocalTime=(flags&GPS_LOCALTIME)!=0;
	bWriteCSV=(flags&GPS_WRITECSV)!=0;
	bGetTRK=(flags&GPS_GETTRK)!=0;
	bGetWP=(flags&GPS_GETWP)!=0;
	port=(flags&GPS_PORTMASK);

	fpprj=fpsrv=fpcsv=NULL;

	com_open=0;
	gotID=0;
	VersionSubStrings=0;
	waypts=waypt_bound=trackpts=trackpt_bound=0;
	bWptFound=0;
	pArea=(pA && pA->uLimitDist>=0)?pA:NULL;
	numZones=tp_namecnt=0;
	waypt=NULL; trackpt=NULL;

	if(e=setjmp(jmpbuf)) {
		return e;
	}

	open_COM(port);
	getID();

	//Verify connection in all cases --
	getGPSTime();

	if(!lpPrjPath) {
		sprintf(gps_errmsg,"OK: UTC time is %s",record.datetime);
		Walls_CB(gps_errmsg);
		byebye(0);
		return 0;
	}
	if(bLocalTime) {
		_tzset();
		settime_record(time(NULL)-UnixSecsMinusGpsSecs); //Fcn assumes GPS seconds UTC
		get_datetime_str();
	}
	strcpy(dl_time,record.datetime);

	init_prj_path(lpPrjPath);

	if(pArea) {
		dLimitDist2=(double)pArea->uLimitDist;
		dLimitDist2*=dLimitDist2;
	}
	if(bGetWP || pArea) {
		getWaypoints();
		if(pArea && !bWptFound) {
			sprintf(gps_errmsg,"No waypoint named %s was found in GPS.",pArea->name);
			byebye(-1);	
		}
	}
	if(bGetTRK) {
		dTrkDist2=(double)pA->uTrkDist;
		dTrkDist2*=dTrkDist2;
		numTrackNames=0;
		record.ident[0]=0;
		getTrack();
	}

	AsyncStop(); com_open=0;

	//Write project files --

	if(!waypts && !trackpts) {
		if(pArea && bWptFound) {
			sprintf(gps_errmsg,"No tracks found near waypoint %s.",pArea->name);
			byebye(-1);	
		}
		byebye(GPS_ERR_NODATA);
	}

	if(bWriteCSV) open_csv();

	if(!(fpprj=fopen(prj_path,"wt"))) byebye(GPS_ERR_PRJCREATE);

	fprintf(fpprj,";WALLS Project File\n");
	*trx_Stpext(prj_path)=0;
	PrjWrite(PRJ_CFG_BOOK,"GPS Project: %s",trx_Stpnam(prj_path));
	PrjWrite(PRJ_CFG_NAME,"PROJECT");
	PrjWrite(PRJ_CFG_STATUS,"2051");
	write_prj_ref(pArea?dLimitZone:0);

	if(bWriteCSV) write_prj_csv();

	for(e=0;e<numZones;e++) {
		if(!pArea || e==dLimitZone) outZoneFile(e);
	}
	PrjWrite(PRJ_CFG_ENDBOOK,NULL);

	byebye(0);
	return 0;
}