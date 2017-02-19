#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "expat.h"
#include "trx_str.h"
#include "trxfile.h"

#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

#define BUFFSIZE 16384
#define SIZ_LINEBREAK 80
#define SIZ_PATH_LINEBREAK 130

//Minimum squared hz length for vector used in an adjustment --
//at 1"=30', 25 (5 pts x 5 pts) is ~ 2', 100 is about 4'

#define MIN_VECLEN_SQ 64.0
#define MAX_VEC_SKIPPED 20

#define SVG_MAX_SIZNAME 63

#pragma pack(1)
typedef struct {
	double fr_xy[2];
	double to_xy[2];
} SVG_TYP_TRXREC;

typedef struct {
	double x;
	double y;
	char prefix[SVG_MAX_SIZNAME+1];
	char name[SVG_MAX_SIZNAME+1];
} SVG_ENDPOINT;

typedef struct {
	double x;
	double y;
} SVG_PATHPOINT;

typedef struct { //12*8=96 bytes (coords can be floats!) --
	double B_xy[2]; //SVG version: fr_ = B_, to_ = B_ + M_
	double M_xy[2];
	double Bn_xy[2]; //Shift parameters
	double Mn_xy[2];
	double maxdist;
	int rt,lf;
} SVG_VECNODE;
#pragma pack()

#define NUM_HDR_NODES 7
#define NUM_HDR_LEVELS 3

#define LEN_VECLST 4
#define A_param 0.01
#define B_param 3.0
#define P_param 0.1
static int len_veclst=LEN_VECLST;

enum e_grps {GRP_VECTORS,GRP_WALLS};

static int iGroupType;
static BOOL bAdjust=FALSE;
static SVG_VECNODE *vnod;
static int *vstk;
static int vnod_siz,vnod_len,max_slevel;
static double max_dsquared,max_dsq_x,max_dsq_y;
static UINT node_visits;
static UINT tpoints,closedcnt,pathcnt,tptotal,tppcnt;
static double tlength;
static int max_iv[LEN_VECLST];  //index of nearest vector to last point
static int veclst_i[LEN_VECLST];
static double veclst_d[LEN_VECLST];

static clock_t tm_scan,tm_walls;

CTRXFile *pix=NULL;

static char Buff[BUFFSIZE];
static char buf[BUFFSIZE+2];
static char argv_buf[2*_MAX_PATH];
static char fpin_name[_MAX_PATH],fpout_name[_MAX_PATH];
#define SIZ_REFBUF 384
static char refbuf[SIZ_REFBUF];
static UINT reflen;

typedef struct {
	int bEntity;
	int errcode;
	int errline;
} TYP_UDATA;

static double scale,cosA,sinA,midE,midN,xMid,yMid;

static int veccnt,skipcnt;
static int skiplin[MAX_VEC_SKIPPED];
static TYP_UDATA udata;
static FILE *fpin,*fpout;

static UINT uLevel;
static int Depth,bNoClose,nCol,iLevel;
static XML_Parser parser;
static int parser_ErrorCode;
static int parser_ErrorLineNumber=0;

#define OUTC(c) putc(c,fpout)
#define OUTS(s) fputs(s,fpout)
#define UD(t) ((TYP_UDATA *)userData)->t

enum {
  SVG_ERR_ABORTED=1,
  SVG_ERR_XMLOPEN,
  SVG_ERR_XMLREAD,
  SVG_ERR_XMLPARSE,
  SVG_ERR_SVGCREATE,
  SVG_ERR_WRITE,
  SVG_ERR_VERSION,
  SVG_ERR_NOMEM,
  SVG_ERR_NOFRAME,
  SVG_ERR_NOID,
  SVG_ERR_INDEXINIT,
  SVG_ERR_INDEXWRITE,
  SVG_ERR_BADNAME,
  SVG_ERR_PARSERCREATE,
  SVG_ERR_TRANSFORM,
  SVG_ERR_NOTEXT,
  SVG_ERR_NOTSPAN,
  SVG_ERR_ENDGROUP,
  SVG_ERR_ENDTEXT,
  SVG_ERR_ENDTSPAN,
  SVG_ERR_SIZNAME,
  SVG_ERR_LABELPOS,
  SVG_ERR_NOPATH,
  SVG_ERR_PARSEPATH,
  SVG_ERR_INDEXOPEN,
  SVG_ERR_TEMPINIT,
  SVG_ERR_POLYPATH,
  SVG_ERR_BADIDSTR,
  SVG_ERR_LABELZERO,
  SVG_ERR_SIZREFBUF,
  SVG_ERR_REFFORMAT,
  SVG_ERR_VECNOTFOUND,
  SVG_ERR_NOREF,
  SVG_ERR_VECDIRECTION,
  SVG_ERR_WALLPATH,
  SVG_ERR_NOPATHID,
  SVG_ERR_POINTPATH,
  SVG_ERR_UNKNOWN
};

static NPSTR errstr[]={
  "User abort",
  "Cannot open template",
  "Error reading",
  "Error parsing",
  "Cannot create",
  "Error writing",
  "Incompatible version of wallsvg.dll",
  "Not enough memory or resources",
  "Invalid wallsvg.xml - no w2d_Frame group",
  "Invalid wallsvg.xml - no group id",
  "Error creating index",
  "Error writing index",
  "Bad station name in data set",
  "Error initializing parser",
  "Text transform expected",
  "Text element expected",
  "Tspan element expected",
  "End group expected",
  "End text expected",
  "End tspan expected",
  "Station name too long",
  "Label position out of range",
  "Path element expected",
  "Unexpected path data format",
  "Can't open .._w2d.trx file",
  "Can't create temporary index",
  "Polyline vector with ID",
  "Wrong format for vector path ID",
  "Zero length label",
  "Reference string too long",
  "Unrecognized w2d_Ref format",
  "Vector not found in index",
  "No w2d_Ref group prior to w2d_Vectors",
  "Vector direction mismatch",
  "Wrong format for wall path",
  "Path ID required in vector group",
  "Path with single point",
  "Unknown error"
};

char * ErrMsg(int code)
{
  int len;
  if(code>SVG_ERR_UNKNOWN) code=SVG_ERR_UNKNOWN;
  if(code>SVG_ERR_ABORTED && code<SVG_ERR_SVGCREATE) {
	len=sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code-1],trx_Stpnam(fpin_name));
    if(code==SVG_ERR_XMLPARSE)
		len+=sprintf(argv_buf+len,": %s",XML_ErrorString(parser_ErrorCode));
  }
  else if(code<=SVG_ERR_WRITE) {
	  len=sprintf(argv_buf,"%s file %s",(LPSTR)errstr[code-1],trx_Stpnam(fpout_name));
  }
  else len=strlen(strcpy(argv_buf,errstr[code-1]));
  if(parser_ErrorLineNumber>0) len+=sprintf(argv_buf+len," - Line %d",parser_ErrorLineNumber);
  strcpy(argv_buf+len,"\n");
  return argv_buf;
}

static void finish(int e)
{
	if(pix) {
		pix->Close();
		delete pix;
	}
	if(fpin) fclose(fpin);
	if(fpout) fclose(fpout);
	if(parser) XML_ParserFree(parser);
	if(vstk) free(vstk);
	if(vnod) free(vnod);
	if(e>0) printf(ErrMsg(e));
	exit(e);
}

static double length(double *xy)
{
	return sqrt(xy[0]*xy[0]+xy[1]*xy[1]);
}

static double *get_xy(double *xy,double *xyz)
{
	//Convert to page coordinates and check if in rectange --
	// x'   = (E-midE)*cosA - (N-midN)*sinA
	// y'   = -(E-midE)*sinA - (N-midN)*cosA
	double x=scale*(xyz[0]-midE);
	double y=scale*(xyz[1]-midN);
	//(x,y) = page coordinates of point wrt center of page
	xy[0]=x*cosA-y*sinA+xMid;
	xy[1]=-x*sinA-y*cosA+yMid;
	return xy;
}

static int skip_depth(void)
{
	int i;
	for(i=0;i<Depth;i++) OUTC('\t');
	return Depth;
}

static char * fltstr(char *fbuf,double f,int dec)
{
	//Obtains the minimal width numeric string of f rounded to dec decimal places.
	//Any trailing zeroes (including trailing decimal point) are truncated.
	BOOL bRound=TRUE;

	if(dec<0) {
		bRound=FALSE;
		dec=-dec;
	}
	if(_snprintf(fbuf,20,"%-0.*f",dec,f)==-1) {
	  *fbuf=0;
	  return fbuf;
	}
	if(bRound) {
		char *p=strchr(fbuf,'.');
		for(p+=dec;dec && *p=='0';dec--) *p--=0;
		if(*p=='.') {
		   *p=0;
		   if(!strcmp(fbuf,"-0")) strcpy(fbuf,"0");
		}
	}
	return fbuf;
}

static char * fltstr(double f,int dec)
{
	static char fbuf[40];
	return fltstr(fbuf,f,dec);
}

static char *get_coord(char *p0,double *d)
{
	char *p=p0;
	char c;

	if(*p=='-') p++;
	else{
		p=++p0;
		if(*p=='-') p++;
	}

	while(isdigit((BYTE)*p)) p++;
	if(*p=='.') {
		p++;
		while(isdigit((BYTE)*p)) p++;
	}
	c=*p;
	if(p==p0) return 0;
	*p=0;
	*d=atof(p0);
	*p=c;
	return p;
}

static BOOL is_hex(int c)
{
	return isdigit((BYTE)c) || (c>='a' && c<='f');
}

static int get_name_from_id_component(char *rnam, const char *pnam)
{
	UINT uhex;
	while(*pnam) {
		if(*pnam=='-') {
			if(!is_hex(pnam[1]) || !is_hex(pnam[2]) ||
				1!=sscanf(pnam+1,"%02x",&uhex)) return SVG_ERR_BADIDSTR;
			pnam+=3;
			*rnam++=(byte)uhex;
		}
		else *rnam++=*pnam++;
	}
	*rnam=0;
	return 0;
}

static int get_ep_name(const char *pnam,SVG_ENDPOINT *ep)
{
	int e=0;
	char *p=strchr(pnam,'.');
	if(!p) ep->prefix[0]=0;
	else {
	   *p++=0;
	   e=get_name_from_id_component(ep->prefix,pnam);
	   pnam=p;
	}
	if(!e) e=get_name_from_id_component(ep->name,pnam);
	return e;
}

static int get_ep_names(const char *ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	char idbuf[SVG_MAX_SIZNAME+1];
	char *p1,*p0=idbuf;
	int i;

	if(strlen(ids)>SVG_MAX_SIZNAME) return SVG_ERR_BADIDSTR;
	strcpy(p0,ids);
	if(*p0=='_') p0++;
	if(!(p1=strchr(p0,'_'))) return SVG_ERR_BADIDSTR;
	*p1++=0;
	if(get_ep_name(p0,ep0) || get_ep_name(p1,ep1) ||
		(i=strcmp(ep0->prefix,ep1->prefix))>0 ||
		(i==0 && strcmp(ep0->name,ep1->name)>=0)) return SVG_ERR_BADIDSTR;
	return 0;
}

static int chk_idkey(const char *ids,SVG_ENDPOINT *ep0,SVG_ENDPOINT *ep1)
{
	int e;
	char key[SVG_MAX_SIZNAME+1];
	char *p=key+1;

	if(e=get_ep_names(ids,ep0,ep1)) return e;

	p+=strlen(strcpy(p,ep0->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep0->name)); *p++=0;
	p+=strlen(strcpy(p,ep1->prefix)); *p++=0;
	p+=strlen(strcpy(p,ep1->name));
	*key=(char)(p-key-1);
	if(pix->Find(key)) return SVG_ERR_VECNOTFOUND;
	return 0;
}

static double dist_to_vec(double px,double py, double *B, double *M)
{
	//Vector is defined as B+t*M, 0<=t<=1
	//Minimum squared length is assumed >= MIN_VECLEN_SQ
	double l,t;

	px-=B[0];
	py-=B[1];
	t=M[0]*px+M[1]*py;
	if(t>0) {
		l=M[0]*M[0]+M[1]*M[1];
		if(t>=l) {
			px-=M[0];
			py-=M[1];
		}
		else {
			t/=l;
			px-=t*M[0];
			py-=t*M[1];
		}
	}
	return px*px+py*py;
}

static double dist_to_midpoint(double x,double y,SVG_VECNODE *pn)
{
	double dx=x-pn->B_xy[0]-0.5*pn->M_xy[0];
	double dy=y-pn->B_xy[1]-0.5*pn->M_xy[1];
	return dx*dx+dy*dy;
}

static double init_veclst()
{
	for(int i=0;i<len_veclst;i++) {
		veclst_d[i]=DBL_MAX;
		veclst_i[i]=0;
	}
	return DBL_MAX;
}

static double insert_veclst(int id,double dist)
{
	//Largest distance is at bottom of list --
	int i,j;
	for(i=0;i<len_veclst;i++) {
		if(!(j=veclst_i[i])) break;
		if(j==id) goto _dret;
	}
	for(i=0;i<len_veclst;i++) if(dist<=veclst_d[i]) break;
	if(i<len_veclst) {
		for(j=len_veclst-1;j>i;j--) {
			veclst_i[j]=veclst_i[j-1];
			veclst_d[j]=veclst_d[j-1];
		}
		veclst_i[i]=id;
		veclst_d[j]=dist;
	}
_dret:
	return veclst_d[len_veclst-1];
}

static double adjust_veclst(double x,double y)
{
	int i,j,isav,imax=len_veclst;
	double dsav;

	for(i=0;i<len_veclst;i++) {
		if(!(j=veclst_i[i])) {
			imax=i;
			break;
		}
		veclst_d[i]=dist_to_vec(x,y,vnod[j].B_xy,vnod[j].M_xy);
	}
	for(i=0;i<imax;i++) {
		for(j=i+1;j<imax;j++) {
			if(veclst_d[j]<veclst_d[i]) {
				dsav=veclst_d[i];
				veclst_d[i]=veclst_d[j];
				veclst_d[j]=dsav;
				isav=veclst_i[i];
				veclst_i[i]=veclst_i[j];
				veclst_i[j]=isav;
			}
		}
	}
	return imax?veclst_d[imax-1]:DBL_MAX;
}

static double min_vecdist(double x,double y)
{
	//return squared distance to nearest vector in vnod[] tree --
	double dmin,dmid;
	int ilast;
	int slevel;
	int nvisits;
	SVG_VECNODE *pn;

	//Get maximum distance to set of vectors nearest last point --
	dmin=adjust_veclst(x,y);

	vstk[0]=slevel=ilast=nvisits=0;

	while(slevel>=0) {
		pn=&vnod[vstk[slevel]];
		//Was the last node visited a child of this one?
		if(pn->lf==ilast || pn->rt==ilast) {
			if(pn->rt==ilast || pn->rt==-1) {
				//finished with this node --
				ilast=vstk[slevel--];
				continue;
			}
			//Finally, examine the right child --
			pn=&vnod[vstk[++slevel]=pn->rt];
		}
		//otherwise it was this node's parent, so examine this node --
		ilast=vstk[slevel];

		//Now examine pn's vector where pn=&vnod[ilast=vstk[slevel]] --
		dmid=dist_to_midpoint(x,y,pn);
		if(dmid<=pn->maxdist || //helps significantly!
			sqrt(dmid)-sqrt(pn->maxdist)<sqrt(dmin)) {
			//Search this node and its children --
			if(ilast>=NUM_HDR_NODES) {
				nvisits++;
				dmid=dist_to_vec(x,y,pn->B_xy,pn->M_xy);
				if(dmid<dmin) dmin=insert_veclst(ilast,dmid);
			}
			if((vstk[++slevel]=pn->lf)>0 || (vstk[slevel]=pn->rt)>0) continue;
			slevel--;
		}
		//We are finished with this node --
		slevel--;
	}
	node_visits+=nvisits;
	return veclst_d[0]; //distance to nearest vector --
	//return dmin; //distance to 3rd nearest vector --
}

static SVG_VECNODE *alloc_vnode(double *b_xy,double *m_xy)
{
	SVG_VECNODE *pn,*pnret;
	int slevel;
	int ichild,idx;
	double divider,midp[2];

	midp[0]=b_xy[0]+0.5*m_xy[0];
	midp[1]=b_xy[1]+0.5*m_xy[1];

	vstk[0]=slevel=0;
	pn=&vnod[0];

	while(TRUE) {
		idx=(slevel&1);
		divider=pn->B_xy[idx]+0.5*pn->M_xy[idx];
		if(midp[idx]<divider) ichild=pn->lf;
		else ichild=pn->rt;
		if(ichild==-1) {
			if(vnod_len>=vnod_siz) return NULL;
			if(midp[idx]<divider) pn->lf=vnod_len;
			else pn->rt=vnod_len;
			pn=&vnod[vnod_len++];
			break;
		}
		pn=&vnod[ichild];
		vstk[++slevel]=ichild;
	}
	
	//Set maxdist to the squared distance from the vector's midpoint to an endpoint --
	pn->maxdist=0.25*(b_xy[0]*b_xy[0]+b_xy[1]*b_xy[1]);
	pn->B_xy[0]=b_xy[0]; pn->B_xy[1]=b_xy[1]; 
	pn->M_xy[0]=m_xy[0]; pn->M_xy[1]=m_xy[1];
	pn->lf=pn->rt=-1;
	pnret=pn;

	if(slevel>max_slevel) max_slevel=slevel;

	//Now update maxdist in ancestor nodes, the computed value
	//being the distance between this vector (any point within)
	//and the midpoint of an ancestor's vector.
	for(;slevel>=0;slevel--) {
		pn=&vnod[vstk[slevel]];
		divider=dist_to_vec(pn->B_xy[0]+0.5*pn->M_xy[0],
			pn->B_xy[1]+0.5*pn->M_xy[1],b_xy,m_xy);
		if(divider>pn->maxdist) pn->maxdist=divider;
	}
	return pnret;
}

static BOOL length_small(double *xy)
{
	if(xy[0]*xy[0]+xy[1]*xy[1]<MIN_VECLEN_SQ) {
		if(skipcnt<MAX_VEC_SKIPPED) skiplin[skipcnt]=XML_GetCurrentLineNumber(parser);
		skipcnt++;
		return TRUE;
	}
	return FALSE;
}

static int process_vector(const char *idstr,const char *style,char *data)
{
	int e;
	SVG_ENDPOINT ep0,ep1;
	SVG_TYP_TRXREC rec;
	double bxy[2],mxy[2];

	if(*data!='M') {
		return SVG_ERR_PARSEPATH;
	}
	if(!(data=get_coord(data,&ep0.x)) || !(data=get_coord(data,&ep0.y))) {
		return SVG_ERR_PARSEPATH;
	}
	while(isspace((BYTE)*data)) data++;

	//data is postioned at type char for TO station's coordinates --
	if(*data==0) {
		if(skipcnt<MAX_VEC_SKIPPED) skiplin[skipcnt]=XML_GetCurrentLineNumber(parser);
		skipcnt++;
		return 0;
	}

	if(!strchr("lLvVhH",*data)) {
		return SVG_ERR_PARSEPATH;
	}
	BOOL bRelative=islower(*data);

	switch(toupper(*data)) {
		case 'L' :
			if(!(data=get_coord(data,&ep1.x)) || !(data=get_coord(data,&ep1.y))) {
				return SVG_ERR_PARSEPATH;
			}
			if(bRelative) {
				ep1.x+=ep0.x; ep1.y+=ep0.y;
			}
			break;
		case 'V' :
			ep1.x=ep0.x;
			if(!(data=get_coord(data,&ep1.y))) {
				return SVG_ERR_PARSEPATH;
			}
			if(bRelative) ep1.y+=ep0.y;
			break;
		case 'H' :
			ep1.y=ep0.y;
			if(!(data=get_coord(data,&ep1.x))) {
				return SVG_ERR_PARSEPATH;
			}
			if(bRelative) ep1.x+=ep0.x;
			break;
	}

	//Parametric form --
	ep1.x-=ep0.x; ep1.y-=ep0.y;

 	//Don't use a vector with squared hz length less than MIN_VECLEN_SQ --
	if(length_small(&ep1.x)) return 0;

    if(e=chk_idkey(idstr,&ep0,&ep1)) return e;

	if(e=pix->GetRec(&rec,sizeof(SVG_TYP_TRXREC))) {
		return SVG_ERR_UNKNOWN;
	}

	get_xy(bxy,rec.fr_xy);
	get_xy(mxy,rec.to_xy);
	mxy[0]-=bxy[0];
	mxy[1]-=bxy[1];

	if(length_small(mxy)) return 0;

	//Check vector orientation
	double d=fabs(atan2(ep1.y,ep1.x)-atan2(mxy[1],mxy[0]));
	if(d>PI) d=2*PI-d;
	if(d>PI/2) {
		return SVG_ERR_VECDIRECTION;
	}

	SVG_VECNODE *pn=alloc_vnode(&ep0.x,&ep1.x);

	if(!pn) {
		return SVG_ERR_UNKNOWN;
	}

	for(e=0;e<2;e++) {
		pn->Bn_xy[e]=bxy[e];
		pn->Mn_xy[e]=mxy[e];
	}

	veccnt++;
	return 0;
}

static void adjust_point(double *P)
{
	int i;
	double lsq[LEN_VECLST],lsqr[LEN_VECLST],wt[LEN_VECLST],wtsum;
	double S[2],*B,*M,*Bn,*Mn;
	double t,v;

	wtsum=0.0;
	for(i=0;i<len_veclst;i++) {
		M=vnod[veclst_i[i]].M_xy;
		lsqr[i]=sqrt(lsq[i]=M[0]*M[0]+M[1]*M[1]);
		wtsum+=(wt[i]=pow(pow(lsqr[i],P_param)/(sqrt(veclst_d[i])+A_param),B_param));
	}

	S[0]=S[1]=0.0;
	for(i=0;i<len_veclst;i++) {
		SVG_VECNODE *pn=&vnod[veclst_i[i]];
		M=pn->M_xy;
		B=pn->B_xy;
		Bn=pn->Bn_xy;
		Mn=pn->Mn_xy;

		//relative distance along vector --
		t=(M[0]*(P[0]-B[0])+M[1]*(P[1]-B[1]))/lsq[i];
		//distance from vector (left side positive) --
		v=(M[1]*(P[0]-B[0])-M[0]*(P[1]-B[1]))/lsqr[i];

		v/=sqrt(Mn[0]*Mn[0]+Mn[1]*Mn[1]);
		S[0]+=wt[i]*(Bn[0] + t*Mn[0] + Mn[1]*v - P[0]);
		S[1]+=wt[i]*(Bn[1] + t*Mn[1] - Mn[0]*v - P[1]);
	}
	P[0]+=S[0]/wtsum;
	P[1]+=S[1]/wtsum;
}

static void process_points(SVG_PATHPOINT *pp,int npts)
{
	double d;
	for(int i=(npts?1:0);i<=npts;i++) {
		d=min_vecdist(pp[i].x,pp[i].y);
		if(d>max_dsquared) {
			max_dsquared=d;
			max_dsq_x=pp[i].x;
			max_dsq_y=pp[i].y;
			for(int j=0;j<len_veclst;j++) max_iv[j]=veclst_i[j];
		}
		//The nearest vectors to pp[i] are now contained in veclst_i[] --
		adjust_point(&pp[i].x);
	}
}

static double path_length(SVG_PATHPOINT *pp,int i)
{
	double dx=pp[i].x-pp[0].x;
	double dy=pp[i].y-pp[0].y;
	return sqrt(dx*dx+dy*dy);
}

static void out_point(double x,double y,BOOL bComma)
{
	char fbuf[40];

	fltstr(fbuf,x,2);
	if(bComma && *fbuf!='-') {
		OUTC(','); nCol++;
	}
	OUTS(fbuf);
	nCol+=strlen(fbuf);

	fltstr(fbuf,y,2);
	if(*fbuf!='-') {
		OUTC(','); nCol++;
	}
	OUTS(fbuf);
	nCol+=strlen(fbuf);
}

static int process_path(const char **attr)
{
	int npts;
	char *data=NULL;
	SVG_PATHPOINT bp,pp[6];
	BOOL bRelative;
	char typ;

	if(fpout) {
		nCol=skip_depth();
		OUTS("<path ");
		nCol+=6;
	}

	for(int i=0;attr[i];i+=2) {
		if(fpout) nCol+=fprintf(fpout,"%s=\"",attr[i]);
		if(!strcmp(attr[i],"d")) {
			data=(char *)attr[i+1];
			break;
		}
		if(fpout) nCol+=fprintf(fpout,"%s\" ",attr[i+1]);
	}

	if(!data || *data!='M') {
		return SVG_ERR_WALLPATH;
	}
	if(!(data=get_coord(data,&bp.x)) || !(data=get_coord(data,&bp.y))) {
		return SVG_ERR_WALLPATH;
	}
	pp[0]=bp; //unadjusted base point is saved in bp
	tpoints++;
	pathcnt++;

	if(bAdjust) {
		process_points(pp,0);
		if(fpout) {
			OUTC('M'); nCol++;
			out_point(pp[0].x,pp[0].y,0);
		}
	}

	while(isspace((BYTE)*data)) data++;

	if(!*data) return SVG_ERR_POINTPATH;

	while(*data) {
		//data is postioned at type char for next set of control points --
		if(!strchr("cChHlLsSvVz",*data)) {
			return SVG_ERR_WALLPATH;
		}
		bRelative=islower(*data);

		switch(typ=tolower(*data)) {
			case 'z' :
				closedcnt++;
				while(isspace((BYTE)*++data));
				if(*data) {
					return SVG_ERR_WALLPATH;
				}
				if(fpout) {
					OUTC('z');
					nCol++;
				}
				goto _end;

			case 'l' :
				if(!(data=get_coord(data,&pp[1].x)) || !(data=get_coord(data,&pp[1].y))) {
					return SVG_ERR_WALLPATH;
				}
				npts=1;
				break;

			case 'v' :
				pp[1].x=bRelative?0:bp.x;
				if(!(data=get_coord(data,&pp[1].y))) {
					return SVG_ERR_WALLPATH;
				}
				typ='l';
				npts=1;
				break;

			case 'h' :
				pp[1].y=bRelative?0:bp.y;
				if(!(data=get_coord(data,&pp[1].x))) {
					return SVG_ERR_WALLPATH;
				}
				typ='l';
				npts=1;
				break;

			case 's' :
			case 'c' :
				npts=(typ=='c')?3:2;
				for(int i=1;i<=npts;i++) {
					if(!(data=get_coord(data,&pp[i].x)) || !(data=get_coord(data,&pp[i].y)))
						return SVG_ERR_WALLPATH;
				}
				break;
				
		}

		if(bRelative) {
			for(int i=1;i<=npts;i++) {
				pp[i].x+=bp.x;
				pp[i].y+=bp.y;
			}
		}
		bp=pp[npts]; //save new unadjusted base point

		tpoints+=npts;

		if(bAdjust) {
			process_points(pp,npts);
			if(fpout) {
				if(nCol>SIZ_PATH_LINEBREAK) {
					OUTS("\n\t");
					nCol=skip_depth()+1;
				}
				OUTC(typ); nCol++;
				for(int i=1;i<=npts;i++) {
					out_point(pp[i].x-pp[0].x,pp[i].y-pp[0].y,i>1);
				}
			}
		}
		else tlength+=path_length(pp,npts);

		while(isspace((BYTE)*data)) data++;
		if(!*data) break;
		pp[0]=pp[npts];
	}

_end:

	if(fpout) {
		OUTS("\"/>\n");
		nCol=0;
	}

	if(bAdjust) {
		if((npts=(int)((100.0*tpoints)/tptotal))>(int)tppcnt && (npts%5)==0) {
			printf("%3d%%\b\b\b\b",tppcnt=npts);
		}
	}
	return 0;
}

static int parse_georef()
{
	LPCSTR p;
	//Tamapatz Project  Plan: 90  Scale: 1:75  Frame: 17.15x13.33m  Center: 489311.3 2393778.12
	if(reflen && (p=strstr(refbuf,"  Plan: "))) {
		double view;
		int numval=sscanf(p+8,"%lf  Scale: 1:%lf  Frame: %lfx%lfm  Center: %lf %lf",
			&view,&scale,&xMid,&yMid,&midE,&midN);
		if(numval==6 && scale>0.0) {
			scale=(72*12.0/0.3048)/scale;
			xMid*=(0.5*scale); yMid*=(0.5*scale);
			view*=U_DEGREES;
			sinA=sin(view); cosA=cos(view);

			//Initialize 7 tree header nodes --

			for(UINT n=0;n<NUM_HDR_NODES;n++) {
				vnod[n].M_xy[0]=vnod[n].M_xy[1]=0.0;
			}
			SVG_VECNODE *pn=vnod;
			pn->B_xy[0]=xMid;
			pn->B_xy[1]=yMid;
			pn->lf=1; pn->rt=2;
			pn++; //1
			pn->B_xy[0]=0.5*xMid;
			pn->B_xy[1]=yMid;
			pn->lf=3; pn->rt=4;
			pn++; //2
			pn->B_xy[0]=1.5*xMid;
			pn->B_xy[1]=yMid;
			pn->lf=5; pn->rt=6;
			pn++; //3
			pn->B_xy[0]=0.5*xMid;
			pn->B_xy[1]=0.5*yMid;
			pn->lf=pn->rt=-1;
			pn++; //4
			pn->B_xy[0]=0.5*xMid;
			pn->B_xy[1]=1.5*yMid;
			pn->lf=pn->rt=-1;
			pn++; //5
			pn->B_xy[0]=1.5*xMid;
			pn->B_xy[1]=0.5*yMid;
			pn->lf=pn->rt=-1;
			pn++; //6
			pn->B_xy[0]=1.5*xMid;
			pn->B_xy[1]=1.5*yMid;
			pn->lf=pn->rt=-1;
			vnod_len=7;
			return 0;
		}
	}
	return SVG_ERR_REFFORMAT;
}

//=============================================================================
static void out_xmldecl(void *userData,
                       const XML_Char  *version,
                       const XML_Char  *encoding,
                       int             standalone)
{
	OUTS("<?xml");
	if(version) fprintf(fpout," version=\"%s\"",version);
	if(encoding) fprintf(fpout," encoding=\"%s\"",encoding);
	fprintf(fpout," standalone=\"%s\"?>\n",standalone?"yes":"no");
	nCol=0;
}

static void out_entity(void *userData,
                          const XML_Char *entityName,
                          int is_parameter_entity,
                          const XML_Char *value,
                          int value_length,
                          const XML_Char *base,
                          const XML_Char *systemId,
                          const XML_Char *publicId,
                          const XML_Char *notationName)
{
	if(value) {
		memcpy(buf,value,value_length);
		buf[value_length]=0;
		fprintf(fpout,"  <!ENTITY %s \"%s\">\n",entityName,buf);
	}
}

static void out_startDoctype(void *userData,
                               const XML_Char *doctypeName,
                               const XML_Char *sysid,
                               const XML_Char *pubid,
                               int has_internal_subset)
{
	fprintf(fpout,"<!DOCTYPE %s ",doctypeName);
	if(pubid && *pubid) fprintf(fpout,"PUBLIC \"%s\" ",pubid);
	else if(sysid) OUTS("SYSTEM ");
	if(sysid && *sysid) fprintf(fpout,"\"%s\"",sysid);
	if(has_internal_subset) {
		UD(bEntity)=1;
		OUTS(" [\n");
	}
	nCol=0;
	Depth=1;
}

static void out_endDoctype(void *userData)
{
	if(UD(bEntity)) {
		OUTC(']');
		UD(bEntity)=0;
	}
	OUTS(">\n");
	nCol=Depth=0;
}

static void out_instruction(void *userData,
                                    const XML_Char *target,
                                    const XML_Char *data)
{
	fprintf(fpout,"<?%s %s?>\n",target,data);
}

static int trimctrl(XML_Char *s)
{
	// Replace LFs followed by multiple tabs with single spaces.
	// isgraph(): 1 if printable and not space

	XML_Char *p=s;
	int len=0;
	int bCtl=1;
	while(*s) {
		if(isgraph(*s) || *s==' ') bCtl=0;
		else {
		  if(bCtl) {
		    s++;
		    continue;
		  }
		  bCtl=1;
		  *s=' ';
		}
	    *p++=*s++;
	    len++;
	}
	*p=0;
	return len;
}

static void out_stream(char *p)
{
	//Update nCol while outputting all data --
	OUTS(p);
	while(*p) {
		if(*p++=='\n') nCol=0;
		else nCol++;
	}
}

static void out_chardata(void *userData,const XML_Char *s,int len)
{
	if(bNoClose) {
		OUTC('>');
		nCol++;
		bNoClose=0;
	}
	memcpy(buf,s,len);
	buf[len]=0;
	if(trimctrl(buf)) out_stream(buf);
}

static void out_wraptext(char *p)
{
	for(;*p;p++) {
		if(isspace((BYTE)*p)) {
			if(nCol>SIZ_LINEBREAK) {
			  OUTS("\n");
			  nCol=skip_depth();
			}
			else {
				OUTC(' ');
				nCol++;
			}
			while(isspace((BYTE)p[1])) p++;
		}
		else {
			OUTC(*p);
			nCol++;
		}
	}
}

static void out_comment(void *userData,const XML_Char *s)
{
	if(bNoClose) {
		OUTS(">\n");
		bNoClose=0;
	}
    //else if(nCol) OUTS("\n");
	if(trimctrl((char *)s)) {
		nCol=skip_depth();
		if(nCol+strlen(s)>SIZ_LINEBREAK+20) {
			OUTS("<!--\n");
			Depth++;
			nCol=skip_depth();
			out_wraptext((char *)s);
			OUTS("\n");
			Depth--;
			skip_depth();
			OUTS("-->\n");
		}
		else {
			fprintf(fpout,"<!--%s-->\n",s);
			nCol=0;
		}
	}
	//nCol=0;
}

static int out_vector(const char *idstr,const char *style,char *data)
{
	int e;
	SVG_ENDPOINT ep0,ep1;
	SVG_TYP_TRXREC rec;

    if(e=chk_idkey(idstr,&ep0,&ep1)) return e;
	if(e=pix->GetRec(&rec,sizeof(SVG_TYP_TRXREC))) return SVG_ERR_UNKNOWN;
	get_xy(&ep0.x,&rec.fr_xy[0]);
	get_xy(&ep1.x,&rec.to_xy[0]);

	skip_depth();
	fprintf(fpout,"<path id=\"%s\" style=\"%s\" d=\"M",idstr,style);
	out_point(ep0.x,ep0.y,0);
	OUTC('l');
	ep1.x-=ep0.x;
	ep1.y-=ep0.y;
	out_point(ep1.x,ep1.y,0);
	OUTS("\"/>\n");
	return 0;
}

static void GetGroupType(const char *id)
{
	if(strlen(id)>=11 && !memcmp(id,"w2d_Vectors",11)) {
		iLevel=1;
		iGroupType=GRP_VECTORS;
	}
	else if(strlen(id)>=9 && !memcmp(id,"w2d_Walls",9)) {
		iGroupType=GRP_WALLS;
		iLevel=1;
	}
}

static void out_startElement(void *userData, const char *el, const char **attr)
{
  int i;
  if(UD(errcode)) return;

  if(!iLevel) {
	  if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		  GetGroupType(attr[1]);
	  }
  }
  else if(iLevel>0) {
	  if(!strcmp(el,"g")) iLevel++;
	  else {
		  i=0;
		  if(strcmp(el,"path") || !attr[0]) i=SVG_ERR_NOPATH;
		  else if(iGroupType==GRP_VECTORS) {
			  if(strcmp(attr[0],"id") || !attr[2] ||
				  strcmp(attr[2],"style") || !attr[4] || strcmp(attr[4],"d")) {
				  i=SVG_ERR_NOPATHID;
			  }
			  else i=out_vector(attr[1],attr[3],(char *)attr[5]);
		  }
		  else i=process_path(attr);
		  if(i) {
			  UD(errcode)=i;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
		  }
		  return;
	  }
  }

  if(strcmp(el,"tspan")) {
	  //Normal output --
	  if(bNoClose) {
		OUTS(">\n");
		bNoClose=0;
	  }
	  else if(nCol) OUTS("\n");
	  nCol=skip_depth();
	  Depth++;
  }
  else if(bNoClose) {
	  OUTC('>');
	  nCol++;
	  bNoClose=0;
  }

  nCol+=fprintf(fpout,"<%s", el);

  for (i=0;attr[i];i+=2) {
	el=attr[i];
	if(nCol>SIZ_LINEBREAK) {
		OUTS("\n");
		nCol=skip_depth();
	}
	else {
		OUTC(' ');
		nCol++;
	}
	el=attr[i+1];
	trimctrl((char *)attr[i+1]);
	sprintf(buf,"%s=\"%s\"",attr[i],attr[i+1]);
	out_wraptext(buf);
  }

  if(iLevel || UD(bEntity)) {
	OUTS(">\n");
	nCol=0;
	bNoClose=0;
  }
  else bNoClose=1;
}

static void out_endElement(void *userData, const char *el)
{
  if(UD(errcode)) return;

  if(iLevel) {
	  if(strcmp(el,"g")) {
		  if(strcmp(el,"path")) {
			  UD(errcode)=SVG_ERR_UNKNOWN;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
		  }
		  return;
	  }
	  //leaving the group --
	  iLevel--;
  }

  if(strcmp(el,"tspan")) {
	  Depth--;
	  if(XML_GetCurrentByteCount(parser)==0) OUTS("/>\n");
	  else {
		if(bNoClose) {
			  OUTS(">\n");
			  nCol=0;
		}
		if(!nCol) skip_depth();
		fprintf(fpout,"</%s>\n",el);
	  }
	  bNoClose=nCol=0;
  }
  else {
	  if(bNoClose) {
		  OUTC('>');
		  nCol++;
		  bNoClose=0;
	  }
	  OUTS("</tspan>");
	  nCol+=8;
  }
}

static int parse_file()
{
	int e=0;

	closedcnt=pathcnt=0;
	Depth=iLevel=0;
	tpoints=0;
	bNoClose=nCol=0;
	udata.bEntity=0;
	udata.errcode=0;
	udata.errline=0;
	parser_ErrorLineNumber=0;
	XML_SetUserData(parser,&udata);
	XML_SetParamEntityParsing(parser,XML_PARAM_ENTITY_PARSING_NEVER);
	XML_SetPredefinedEntityParsing(parser,XML_PREDEFINED_ENTITY_PARSING_NEVER);
	XML_SetGeneralEntityParsing(parser,XML_GENERAL_ENTITY_PARSING_NEVER);

	for (;;) {
		int done;
		int len;

		len=fread(Buff, 1, BUFFSIZE, fpin);
		if(ferror(fpin)) {
			e=SVG_ERR_XMLREAD;
			break;
		}
		done=feof(fpin);
		if (!XML_Parse(parser, Buff, len, done)) {
			parser_ErrorLineNumber=XML_GetCurrentLineNumber(parser);
			parser_ErrorCode=XML_GetErrorCode(parser);
			e=SVG_ERR_XMLPARSE;
			break;
		}
		if(udata.errcode) {
			parser_ErrorLineNumber=udata.errline;
			e=udata.errcode;
			break;
		}
		if(done) break;
	}
	XML_ParserFree(parser);
	parser=NULL;
	return e;
}

static int copy_svg()
{
	int e;

	parser = XML_ParserCreate(NULL);
	if (!parser) finish(SVG_ERR_PARSERCREATE);
	XML_SetXmlDeclHandler(parser,out_xmldecl);
	XML_SetElementHandler(parser,out_startElement,out_endElement);
	XML_SetProcessingInstructionHandler(parser,out_instruction);
	XML_SetCharacterDataHandler(parser,out_chardata);
	XML_SetCommentHandler(parser,out_comment);
	//XML_SetDefaultHandler(parser,defhandler);
	XML_SetDoctypeDeclHandler(parser,out_startDoctype,out_endDoctype);
	XML_SetEntityDeclHandler(parser,out_entity);

	if(!(fpout=fopen(fpout_name,"wt"))) {
	  finish(SVG_ERR_SVGCREATE);
	}
	bAdjust=TRUE;
	tppcnt=0;
	node_visits=0;
	init_veclst();
	printf("\nAdjusting wall points.. ");
	tm_walls=clock();
	e=parse_file();
	tm_walls=clock()-tm_walls;
	printf("Completed.\n");
	return e;
}

static void scanv_chardata(void *userData,const XML_Char *s,int len)
{
	if(iLevel==-2) {
		if(reflen+len>=SIZ_REFBUF) {
		  UD(errcode)=SVG_ERR_SIZREFBUF;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
		  return;
		}
		memcpy(refbuf+reflen,s,len);
		refbuf[reflen+=len]=0;
	}
}

static void scanv_startElement(void *userData, const char *el, const char **attr)
{
  int i;
  if(UD(errcode)) return;

  if(!iLevel) {
	if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id")) {
		if(!strcmp(attr[1],"w2d_Ref")) {
			iLevel=-1;
			return;
		}
		GetGroupType(attr[1]);
		if(iLevel && scale==0.0) {
			UD(errcode)=SVG_ERR_NOREF;
			UD(errline)=XML_GetCurrentLineNumber(parser);
		}
	}
	return;
  }

  if(iLevel==-1) {
	  if(!strcmp(el,"text")) {
		  //now look for character data --
		  iLevel=-2;
		  reflen=0;
	  }
	  else {
		  UD(errcode)=SVG_ERR_NOTEXT;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  return;
  }

  if(iLevel>0) {
	  if(!strcmp(el,"g")) {
		  iLevel++;
		  return;
	  }

	  //Path definition
	  if(strcmp(el,"path")) {
		UD(errcode)=SVG_ERR_NOPATH;
		UD(errline)=XML_GetCurrentLineNumber(parser);
		return;
	  }

	  if(iGroupType==GRP_WALLS) {
		  if(i=process_path(attr)) {
			  UD(errcode)=i;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
		  }
		  return;
	  }

	  if(!attr[0] || strcmp(attr[0],"id") || !attr[2] ||
		  strcmp(attr[2],"style") || !attr[4] || strcmp(attr[4],"d")) {
		  UD(errcode)=SVG_ERR_NOPATHID;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  else if(i=process_vector(attr[1],attr[3],(char *)attr[5])) {
		  UD(errcode)=i;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  return;
  }

  if(iLevel!=-2) {
	  UD(errcode)=SVG_ERR_UNKNOWN;
	  UD(errline)=XML_GetCurrentLineNumber(parser);
  }
}

static void scanv_endElement(void *userData, const char *el)
{
  if(UD(errcode)) return;

  //iLevel can be -3,-2 (Ref), -1 (Vec), 0, or >0 (Walls)
  if(!iLevel) return;

  if(iLevel>0) {
	  if(strcmp(el,"g")) {
		  if(strcmp(el,"path")) {
			  UD(errcode)=SVG_ERR_UNKNOWN;
			  UD(errline)=XML_GetCurrentLineNumber(parser);
		  }
		  return;
	  }
	  //leaving the group --
	  iLevel--;
  }
  else if(!strcmp(el,"text")) {
	  int e=parse_georef();
	  if(e) {
		  UD(errcode)=e;
		  UD(errline)=XML_GetCurrentLineNumber(parser);
	  }
	  iLevel=0;
  }
}

static int scan_vectors()
{
  int e=0;
  pix=new CTRXFile();
  strcpy(buf,fpin_name);
  strcpy(trx_Stpext(buf),"_w2d");
  if(pix->Open(buf) || pix->AllocCache(32) || !(vnod_siz=pix->NumRecs()))
	  e=SVG_ERR_INDEXOPEN;
  else {
	pix->SetExact(TRUE);
	vnod_siz+=NUM_HDR_NODES; //room for header nodes
	if(!(vnod=(SVG_VECNODE *)calloc(vnod_siz,sizeof(SVG_VECNODE))) ||
	   !(vstk=(int *)malloc(vnod_siz*sizeof(int)))) return SVG_ERR_NOMEM;
	vnod_len=0;
    veccnt=skipcnt=max_slevel=0;
	scale=0.0;
	tlength=0.0;
	max_dsquared=0.0;
	bAdjust=FALSE;
	parser=XML_ParserCreate(NULL);
	if(!parser) e=SVG_ERR_PARSERCREATE;
	else {
		XML_SetElementHandler(parser,scanv_startElement,scanv_endElement);
		XML_SetCharacterDataHandler(parser,scanv_chardata);
		tm_scan=clock();
		e=parse_file();
		tm_scan=clock()-tm_scan;
	}
  }
  return e;
}

static void scanw_startElement(void *userData,const char *el, const char **attr)
{
  int i;
  if(UD(errcode)) return;

  if(!iLevel) {
	if(!strcmp(el,"g") && attr[0] && !strcmp(attr[0],"id") &&
	  strlen(attr[1])>=9 && !memcmp(attr[1],"w2d_Walls",9)) iLevel=1;
	return;
  }

  if(!strcmp(el,"g")) {
	  iLevel++;
	  return;
  }

  if(strcmp(el,"path")) {
	UD(errcode)=SVG_ERR_NOPATH;
	UD(errline)=XML_GetCurrentLineNumber(parser);
	return;
  }

  if(i=process_path(attr)) {
	  UD(errcode)=i;
	  UD(errline)=XML_GetCurrentLineNumber(parser);
  }
}

static void scanw_endElement(void *userData, const char *el)
{
  if(UD(errcode)) return;

  if(iLevel && !strcmp(el,"g"))  iLevel--;
}

static int scan_walls()
{
	int e=0;

	parser = XML_ParserCreate(NULL);
	if (!parser) e=SVG_ERR_PARSERCREATE;
	else {
		XML_SetElementHandler(parser,scanw_startElement,scanw_endElement);
		bAdjust=TRUE;
		tppcnt=0;
		node_visits=0;
		init_veclst();
		printf("\nProcessing wall points.. ");
		tm_walls=clock();
		e=parse_file();
		tm_walls=clock()-tm_walls;
		printf("Completed.\n");
	}
	return e;
}

void main(int argc, char *argv[])
{
	int e;

	if(argc<2) {
	  printf("w2dwarp v1.0 - Process path points in w2d_Walls group.\n\n"
			 "Command: w2dwarp <input pathname>[.SVG] <output pathname>[.SVG]\n"
			 "Note: Index <input pathname>_w2d.trx must be present.");
	  exit(-1);
	}

	parser=NULL;
	vnod=NULL;
	vstk=NULL;
	fpout=NULL;

	if(!(fpin=fopen(strcpy(fpin_name,argv[1]),"rt"))) {
		finish(SVG_ERR_XMLOPEN);
	}

	//Scan SVG, processing w2d_Ref and w2d_Vector groups, building searchable binary
	//tree of vectors, vnod[], in memory --
	if(e=scan_vectors()) finish(e);
	tptotal=tpoints;

	//Scan file again to update or examine w2d_Walls and w2d_Vector group(s) --

	if(fseek(fpin,0,SEEK_SET)) finish(SVG_ERR_XMLOPEN);

	if(!e && argc>2) {
		strcpy(fpout_name,argv[2]);
		e=copy_svg();
	}
	else e=scan_walls();

	if(e) finish(e);

	printf("Completed - Vectors processed: %d  Skipped: %d\n",veccnt,skipcnt);
	if(pathcnt) printf("Wall segments: %u  Closed: %u  Points: %u  Length %.1f m\n",
		pathcnt,closedcnt,tpoints,tlength/scale);
	if(skipcnt) {
		if(skipcnt>MAX_VEC_SKIPPED) skipcnt=MAX_VEC_SKIPPED;
		printf("Skipped lines:");
		for(e=0;e<skipcnt;e++) printf(" %d",skiplin[e]);
		printf("\n");
	}
	printf("Maximum tree depth: %d\n",max_slevel);
	printf("Average nodes examined per point: %.1f\n",(double)node_visits/tpoints);
	max_dsquared=sqrt(max_dsquared);
	printf("Maximum distance to nearest vector: %.1f or %.1f m\n",max_dsquared,max_dsquared/scale);
	printf("at (%.1f,%.1f). Vector mpts:",max_dsq_x,max_dsq_y);
	for(e=0;e<len_veclst;e++) 
		printf(" (%.1f,%.1f)",vnod[max_iv[e]].B_xy[0]+0.5*vnod[max_iv[e]].M_xy[0],
		vnod[max_iv[e]].B_xy[1]+0.5*vnod[max_iv[e]].M_xy[1]);
	printf("\nTimes - Scan: %.2f  Walls: %.2f\n",
		(double)tm_scan/CLOCKS_PER_SEC,(double)tm_walls/CLOCKS_PER_SEC);
	finish(0);
}
