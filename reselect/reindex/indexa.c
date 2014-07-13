#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <trx_str.h>
#include <trx_file.h>

#define EXPLICIT_SPC 0xA0

//Functions in rebuild.c or reindex.c --

apfcn_v logmsg(nptr p, nptr s);
acfcn_v abortmsg(nptr format,...);
apfcn_v AddKeyword(byte *keybuf);
apfcn_v IndexDate(int date);

static UINT nParseKW;

static int trx;
static UINT reftotal;

BOOL is_xspace(byte c)
{
	return isspace(c) || c==EXPLICIT_SPC;
}

nptr skip_space_types(nptr p)
{
	while((is_xspace(*p) && (++p)) || (!memcmp(p,"&nbsp;",6) && (p+=6)));
	return p;
}

static BOOL iskeydelim(byte c)
{
	return (is_xspace(c) || c=='.' || c==';' || c==',');
}

static BOOL convert_ansi(byte *p)
{
	UINT c=*p;
	switch(c) {
		case 0x91:
		case 0x92: c='\''; break;
		case 0x93:
		case 0x94: c='"'; break;
		case 231:
		case 199: c='C'; break;
		case 192:
		case 193:
		case 194:
		case 195:
		case 196:
		case 197:
		case 198:
		case 224:
		case 225:
		case 226:
		case 227:
		case 228:
		case 229:
		case 230: c='A'; break;
		case 200:
		case 201:
		case 202:
		case 203:
		case 232:
		case 233:
		case 234:
		case 235: c='E'; break;
		case 204:
		case 205:
		case 206:
		case 207:
		case 236:
		case 237:
		case 238:
		case 239: c='I'; break;
		case 210:
		case 211:
		case 212:
		case 213:
		case 214:
		case 242:
		case 243:
		case 244:
		case 245:
		case 246: c='O'; break;
		case 217:
		case 218:
		case 219:
		case 220:
		case 249:
		case 250:
		case 251:
		case 252: c='U'; break;
		case 209:
		case 241: c='N'; break;
		case 221:
		case 253:
		case 255: c='Y'; break;
		case 0x97: c='-'; break;
		case 0xBA: c='d'; break;

		default: return FALSE;
	}
	*p=(byte)c;
	return TRUE;
}

enum e_keys {KY_ASCII=1,KY_SPACE=2,KY_COLON=4,KY_PERIOD=8,KY_QUOTES=16};

static void condition_key(nptr p)
{
	int c;
	UINT i,j,nonspc,len=*p;
	UINT eMsg=0;
	byte keybuf[TRX_MAX_KEYLEN+1];

	nonspc=0;
	for(i=j=1;i<=len;i++) {
		c=p[i];
		nonspc++;
		if(!isascii(c)) {
			if(!convert_ansi((nptr)&c)) eMsg |= KY_ASCII;
			else p[i]=c;
		}

		if(c=='\"') continue;

		if(isspace(c)) {
			p[i]=' ';
			if(!nonspc) {
				//Last char was a space --
				eMsg |= KY_SPACE;
			    nonspc=(UINT)-1;
				continue;
			}
			nonspc=(UINT)-1;
		}
		else if(c==':') {
			if(p[i+1]!=' ' && !(i>1 && i<len && isdigit(p[i+1]) && isdigit(p[i-1]))) eMsg |= KY_COLON;
		}
		else if(c=='.') {
			if(i<len-1 && isalpha(p[i+1]) && isalpha(p[i+2])) {
				p[0]=(byte)(j-1);
				AddKeyword(p);
				nonspc=0;
				j=1;
				continue;
			}
		}

		p[j++]=p[i];
	}

	p[0]=(byte)(j-1);

	if(eMsg) {
	  trx_Stcpc(keybuf,p);
	  if(eMsg&KY_ASCII) logmsg(keybuf,"Keyword had non-ascii character(s)");
	  if(eMsg&KY_COLON) logmsg(keybuf,"Keyword has colon followed by non-space");
	  if(eMsg&KY_SPACE) logmsg(keybuf,"Keyword had embedded space pair");
	}
}

static BOOL add_author_key(char *cKey)
{
	UINT i,len;
	byte *p;
	byte pKey[46];

	if((len=strlen(cKey))>40) {
		logmsg(cKey,"Note: Indexed as ASSOCIATION (length > 40 chars)");
		trx_Stccp(pKey,"ASSOCIATION");
		goto _index;
	}

	p=(byte *)trx_Stccp(pKey,cKey);

	//Remove spaces after embedded periods and commas and convert non-ascii codes --
	p=pKey;
	for(i=1;i<=len;i++) {
		if(pKey[i]==' ' && (*p==',' || *p=='.' || p==pKey || i==len)) continue;
		if(pKey[i]==',' && *p=='.') {
			*p=',';
			continue;
		}
		*++p=pKey[i];
		if(!isascii(*p) && !convert_ansi(p)) logmsg(cKey,"Name has unknown non-ascii char");
	}

	while(p>pKey && !isalpha(*p)) p--;
	*pKey=(p-pKey);

	while(p>pKey) if(*p--==',') break;

	if(p==pKey && memcmp(p+1,"ANONYMOUS",9)) {
		if(!memcmp(p+1,"AUTHOR UNKNOWN",14)) {
			trx_Stccp(pKey,"UNKNOWN,AUTHOR");
		}
		else {
			p=trx_Stcpc(cKey,pKey);
			for(;*p;p++) if(*p==' ') break;
			//Note -- Indexing trx_Stxpc(pKey) exposes error in index format!!
			if(*p && strcmp(p," PSEUD")) {
				logmsg(cKey,"Note: Indexed as ASSOCIATION (no comma found)");
				trx_Stccp(pKey,"ASSOCIATION");
			}
			else {
				if(*p) {
					(*pKey)-=6;
					*p=0;
				}
				else {
					if(!strcmp(cKey,"EDITORS") || !strcmp(cKey,"DIRECTORS") ||
					   !strcmp(cKey,"MODERATORS") || !strcmp(cKey,"ASSOCIATES")) return TRUE;
				}
				logmsg(cKey,"Note: No first name or initials (indexed)");
			}
		}
	}

	if(!pKey) {
		logmsg(cKey,"Empty name - remaining chars ignored");
		return FALSE;
	}

_index:

	if(trx_InsertKey(trx,&reftotal,pKey)) abortmsg("Error writing author index");
	return TRUE;

}

static UINT chk_abbrev(char *p,char *s)
{
	UINT len=strlen(s);
	if((!p[len] || p[len]=='.' || p[len]==',') && !memcmp(p,s,len)) return len;
	return 0;
}

static UINT abbrev_length(char *p)
{
	UINT len;
	if((len=chk_abbrev(p,"JR")) || 
		(len=chk_abbrev(p,"SR")) ||
		(len=chk_abbrev(p,"II")) ||
		(len=chk_abbrev(p,"IV")) ||
		(len=chk_abbrev(p,"III"))) return len;

	if(len=chk_abbrev(p,"M.D")) {
		if(len==3) return len;
	}
	return 0;
}

static BOOL reverse_author_key(char *px,char *pxnext)
{
	char *p,*p0=px,*psuffix;
	UINT len;
	char namebuf[256];

	psuffix=strchr(p0,',');
	if(!psuffix) psuffix=p0+strlen(p0);

	//Now it's possible JR, III, etc., is not preceeded by a comma --
	for(p=psuffix-1;p>p0;p--) if(p[-1]==' ') break;
	if(p>p0 && abbrev_length(p)) {
		*--p=',';
		psuffix=p;
	}
	else p=psuffix;

	if(p>p0 && p[-1]=='.') p--;

	len=0;

	for(;p>p0;p--) {
		if(p[-1]==' ' || p[-1]=='.') {
			if(len>1) break;
		}
		len++;
	}

	//p points to last name of length len, p0 points to first name of length (p-p0)-1:

	if(len+strlen(psuffix)+(p-p0)+4>=sizeof(namebuf)) return FALSE;
	memcpy(namebuf,p,len);
	if((p-p0)>1) {
		namebuf[len++]=',';
		memcpy(namebuf+len,p0,(p-p0)-1);
		len+=(p-p0)-1;
	}
	if(*psuffix) strcpy(namebuf+len,psuffix);
	else namebuf[len]=0;
	if(pxnext && (UINT)(pxnext-px)<=len) return FALSE;
	strcpy(px,namebuf);
	return TRUE;
}

static int isdate(nptr *p0)
{
	//return 0 (no date or unknown), years>0, or -1 (no date phrase found)
	int len,date;
	nptr p=*p0+2;

	if(*p=='[' || *p=='?') p++;

	if(*p=='1' || *p=='2') {
		date=atoi(p);
		for(len=0;len<4;len++) if(!isdigit(*++p)) break;
		return (len==3 || *p==' ')?date:-1;
	}

	if(!memcmp(p,"N.D.",4) || !memcmp(p,"N. D.",5) ||
		!memcmp(p,"NO DATE",7) || !memcmp(p,"DATE UNKNOWN",12) ||
		!memcmp(p,"IN PRESS",8)) return 0;

	len=0; p=*p0;
	if(isdigit(p[--len]) && isdigit(p[--len]) &&
		isdigit(p[--len]) && isdigit(p[--len]) && p[--len]==' ') {
		date=atoi(p+len+1);
		//Either the period is missing or a comma was used instead of a period --
		if(p[len-1]==',') len--;
		*p0=p+len;
		return date;
	}

	return -1;
}

static char *skip_abbrev(char *p)
{
	UINT len;

	p+=2;
	if((len=abbrev_length(p))) {
		p+=len;
		if(*p) {
		    *p++=0;
			if(*p=='.') p++; //shouldn't be required
			if(*p==',') p++;
			if(*p==' ') p++;
		}
	}
	else p[-2]=0;
	if(!*p) p=0;
	return p;
}

static char *skip_ed(char *px)
{
	char *p;

	if(!*px) return 0;

	p=0;
	if(!memcmp(px,"ED",2)) {
		p=px+2;
		if(*p=='S') p++;
	}
	else if(!memcmp(px,"COMP",4)) {
		p=px+4;
		if(*p=='S') p++;
	}
	else if(!memcmp(px,"ET AL",5)) p=px+5;

	if(p && (*p=='.' || *p==0)) {
		if(*p) {
		  p++;
		  if(*p==',') p++;
		  if(*p==' ') p++;
		}
		px=p;
		if(!*px) px=0;
	}
	return px;
}


static char *advance_px(char *p)
{
	//*ppx points to the next key to be added, possibly needing to be reversed.
	//This function terminates this key and advances px to the next name.

	char *px=strstr(p,", ");

	if((p=strstr(p," AND ")) && (!px || p<=px+1)) {
		if(px && p==px+1) *px=0;
		else *p=0;
		return p+5;
	}

	if(px) px=skip_abbrev(px);

	//string has now been terminated. Now move px to the next author,
	//skipping "ET AL.", "COMP.", "COMPS.", "ED.", or "EDS."
	if(px) {
		px=skip_ed(px);
		if(px && !memcmp(px,"AND ",4)) px=skip_ed(px+4);
	}
	return px;
}

static nptr cleanCopy(nptr p0,nptr px)
{
	nptr p;
	UINT lenb,lenc;

	strncpy(p0,px,506);
	p0[506]=0;

	lenb=lenc=0;
	for(px=p=p0;*p;p++) {
		if(*p=='<') {
			lenb++;
			continue;
		}
		if(*p=='>') {
			if(lenb) lenb--;
			continue;
		}

		if(*p=='(') {
			if(!lenc && px>p0 && px[-1]==' ') px--;
			lenc++;
			continue;
		}

		if(*p==')') {
			if(lenc) {
				lenc--;
				if(p[1]=='.' && px>p0 && px[-1]==' ') p++;
				continue;
			}
		}

		if(lenb || lenc || *p=='[' || *p==']' || *p=='?') continue;

		if(is_xspace(*p)) {
			*px++=' ';
			continue;
		}

		if(*p=='&') {
			if(!memcmp(p,"&amp;",5)) {
				strcpy(px,"and"); px+=3;
				p+=4;
			}
			else {
				p=strchr(p,';');
				if(!p) break;
			}
			continue;
		}

		if(*p=='"') continue;

		if(!isascii(*p)) {
			*px=*p;
			convert_ansi(px);
			if(*px!='"') px++;
			continue;
		}

		*px++=*p;
	}

	*px=0;

	//Now eliminate multiple spaces --
	for(px=p=p0;*p;p++) {
		if(*p==' ') {
			*px++=' ';
			while(p[1]==' ') p++;
			continue;
		}
		*px++=*p;
	}
	*px=0;

	return p0;
}

void index_author(int trxno,UINT refno,char *pRef)
{
	byte *px,*p,*p0;
	UINT len;
	int date=-1;
	byte buffer[512];

	trx=trxno;
	reftotal=refno;

	p0=_strupr(cleanCopy((nptr)buffer,(nptr)pRef));

	for(p=p0;p && *p;p=strstr(++p,". ")) {
		if((date=isdate(&p))>=0) break;
	}
	
	IndexDate(date);

	if(date>0 && (date<1800 || date>2020)) logmsg(p0,"Date is questionable");

	if(!p) {
		len=strlen(p0);
		if(len>=9 && !memcmp(p0,"ANONYMOUS",9)) len=9;
		else if(len>=14 && !memcmp(p0,"AUTHOR UNKNOWN",14)) len=14;
		else {
			logmsg(p0,"Date unrecognized - no names indexed");
			return;
		}
	}
	else {
		len=p-p0;
		if(len && p0[len-1]=='.') len--;
	}

	if(date<0) logmsg(p0,"Date missing or unrecognized");

	p0[len]=0;
	p=p0;

	if(!(px=strchr(p,','))) {
		add_author_key(p);
		return;
	}

	if(*++px==' ') px++;

	px=advance_px(px);
	if(!*p) goto _err;

	while(TRUE) {
		if(!add_author_key(p)) break;
		if(!px) break;
		if(!*px) {
			abortmsg("Unexpected parse error: Reference %u",reftotal);
		}
		p=px;
		px=advance_px(px);
		if(!reverse_author_key(p,px)) goto _err;
	}
	return;

_err:
	logmsg(cleanCopy((nptr)buffer,(nptr)pRef),"Unrecognized format for name(s)");
	return;
}

apfcn_n skip_tag(nptr p)
{
	if(*p=='<') {
		while(*++p && *p!='>');
		if(*p) p++;
	}
	return p;
}

nptr elim_all_st(nptr p0,int n)
{
	nptr pn=p0;
	nptr p,pp;
	char buf[88];
	char buf0[32];
	int lenb;

	sprintf(buf0,"</st%d:",n);
	sprintf(buf,"<st%d:",n);
	lenb=strlen(buf);

	while(pn=strstr(pn,buf)) {
		p=strchr(pn+lenb,'>');
		if(!p) return 0;
		for(pp=pn+lenb;pp<p;pp++) {
			if(is_xspace(*pp)) break;
		}
		p++;
		memcpy(buf0+lenb+1,pn+lenb,pp-pn-lenb);
		n=pp-pn+1;
		buf0[n++]='>';
		buf0[n]=0;
		if(!(pp=strstr(p,buf0))) {
			return 0;
		}
		memcpy(pn,p,pp-p);
		pn+=(pp-p);
		strcpy(pn,skip_tag(pp));
		pn=p0;
	}
	return skip_space_types(p0);
}

nptr elim_all_ins(nptr p0)
{
	nptr pn=p0;
	nptr p,pp;
	while(pn=strstr(pn,"<ins ")) {
		p=strchr(pn+5,'>');
		if(!p) return 0;
		p++;
		if(!(pp=strstr(p,"</ins>"))) {
			return 0;
		}
		memcpy(pn,p,pp-p);
		pn+=(pp-p);
		strcpy(pn,pp+6);
		pn=p0;
	}
	return skip_space_types(p0);
}

apfcn_v parse_keywords(nptr p0)
{
	nptr p;
	UINT keylen,toklen;
	int c;
	byte keybuf[TRX_MAX_KEYLEN+2];
	BOOL bLastSpc;

	p=p0;

	nParseKW++;

	while(*p) {
		keylen=toklen=0;
		while(iskeydelim(*p)) p++;
		bLastSpc=TRUE;

		for(;*p;p++) {

			if(*p=='<') {
				p=skip_tag(p);
				p--;
				continue;
			}

			if(is_xspace(*p)) {
				if(bLastSpc) continue;
				bLastSpc=TRUE;
				c=' ';
				goto _next;
			}

			if(*p=='&') {
				if(!memicmp(p,"&nbsp;",6)) {
					p+=5;
					if(bLastSpc) continue;
					bLastSpc=TRUE;
					c=' ';
					goto _next;
				}
				if(!memicmp(p,"&quot;",6)) {
					c='"';
					p+=5;
				}
				else if(!memicmp(p,"&amp;",5)) {
					c='&';
					p+=4;
				}
				else if(!memicmp(p,"&mdash;",7)) {
					c=0xBE;
					p+=6;
				}
				else c='&';
				bLastSpc=FALSE;
				goto _next;
			}

			bLastSpc=FALSE;

			if(*p==':' && keylen>=6 && !memcmp(keybuf+keylen-5,"COUNTY",6)) {
				logmsg(p,"Note: Colon following COUNTY in KW list changed to period");
				*p='.';
			}

			if(*p=='.' && (is_xspace(p[1]) || p[1]=='<')  && toklen!=1) {
				if(toklen!=2 || 
					(
					!(keybuf[keylen-1]=='N' && keybuf[keylen]=='O') &&
					!(keybuf[keylen-1]=='F' && keybuf[keylen]=='T') &&
					!(keybuf[keylen-1]=='M' && keybuf[keylen]=='I') &&
					!(keybuf[keylen-1]=='M' && keybuf[keylen]=='R') &&
					!(keybuf[keylen-1]=='M' && keybuf[keylen]=='T') &&
					!(keybuf[keylen-1]=='S' && keybuf[keylen]=='T') &&
					!(keybuf[keylen-1]=='J' && keybuf[keylen]=='R')
					))
				{
					if(toklen!=3 ||
						(
						!(keybuf[keylen-2]=='N' && keybuf[keylen-1]=='O' && keybuf[keylen]=='S') &&
						!(keybuf[keylen-2]=='B' && keybuf[keylen-1]=='L' && keybuf[keylen]=='K') &&
						!(keybuf[keylen-2]=='S' && keybuf[keylen-1]=='E' && keybuf[keylen]=='C') &&
						!(keybuf[keylen-2]=='H' && keybuf[keylen-1]=='W' && keybuf[keylen]=='Y') &&
						!(keybuf[keylen-2]=='M' && keybuf[keylen-1]=='R' && keybuf[keylen]=='S')
						))
					{
						p++;
						break;
					}
				}
			}

			c=*p;
_next:
			if(isalpha(c)) toklen++;
			else toklen=0;

			if(keylen<TRX_MAX_KEYLEN) keybuf[++keylen]=toupper(c);
		}

		while(keylen && iskeydelim(keybuf[keylen])) keylen--;
		if(keylen) {
			*keybuf=(byte)keylen;
			keybuf[keylen+1]=0;
#ifdef _DEBUG
			if(strstr(keybuf,"BROWNWOOD")) {
				c=0;
			}
#endif
			condition_key(keybuf);
			AddKeyword(keybuf);
		}
	}
}


