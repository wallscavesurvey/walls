#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <trx_str.h>
#include <trx_file.h>

#define EXPLICIT_SPC 0xA0
#define MAX_NAMEKEYLEN 80

//Functions in rebuild.c or reindex.c --

acfcn_v abortmsg(nptr format, ...);
apfcn_v AddKeyword(byte *keybuf);
apfcn_v IndexDate(int date);
apfcn_v openlog(nptr hdr);

#define MAX_ORGKEYS 1000
static LPSTR orgkeys[MAX_ORGKEYS];
static UINT numOrgkeys;

static UINT nParseKW;

static int trx;
static UINT reftotal;

//globals --
char filename[128];
UINT logtotal;
FILE *fplog;

apfcn_v logmsg(LPSTR p, LPCSTR s)
{
#define SIZ_MSG 70

	char prefix[SIZ_MSG + 6];
	int i;

	if (!fplog) {
		openlog("QUESTIONABLE ITEMS");
		putc('\n', fplog);
	}
	strncpy(prefix, p, SIZ_MSG);
	if (prefix[SIZ_MSG - 1] != 0) strcpy(prefix + SIZ_MSG - 1, "..");
	if (*prefix != '<' && (p = strchr(prefix, '<'))) *p = 0;
	if ((i = strlen(prefix)) < SIZ_MSG + 1) while (i++ < SIZ_MSG + 1) strcat(prefix, " ");
	fprintf(fplog, "%s:%5u. %s - %s.\n", filename, reftotal, prefix, s);
	logtotal++;
}

BOOL pfxcmp(LPCSTR p, LPCSTR pfx)
{
	for (; *p && *pfx; p++, pfx++) {
		if (*p != *pfx) return TRUE;
	}
	return *pfx != 0;
}


BOOL is_xspace(byte c)
{
	return isspace(c) || c == EXPLICIT_SPC;
}

nptr skip_space_types(nptr p)
{
	while ((is_xspace(*p) && (++p)) || (!pfxcmp(p, "&nbsp;") && (p += 6)));
	return p;
}

static BOOL iskeydelim(byte c)
{
	return (is_xspace(c) || c == '.' || c == ';' || c == ',');
}

static BOOL convert_ansi(byte *p)
{
	UINT c = *p;
	switch (c) {
	case 0x91:
	case 0x92: c = '\''; break;
	case 0x93:
	case 0x94: c = '"'; break;
	case 0x96: //N-Dash
	case 0x97: c = '-'; break; //M-Dash
	case 231:
	case 199: c = 'C'; break;
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
	case 230: c = 'A'; break;
	case 200:
	case 201:
	case 202:
	case 203:
	case 232:
	case 233:
	case 234:
	case 235: c = 'E'; break;
	case 204:
	case 205:
	case 206:
	case 207:
	case 236:
	case 237:
	case 238:
	case 239: c = 'I'; break;
	case 210:
	case 211:
	case 212:
	case 213:
	case 214:
	case 242:
	case 243:
	case 244:
	case 245:
	case 246: c = 'O'; break;
	case 217:
	case 218:
	case 219:
	case 220:
	case 249:
	case 250:
	case 251:
	case 252: c = 'U'; break;
	case 209:
	case 241: c = 'N'; break;
	case 221:
	case 253:
	case 255: c = 'Y'; break;
	case 0xBA: c = 'd'; break;

	default: return FALSE;
	}
	*p = (byte)c;
	return TRUE;
}

enum e_keys { KY_ASCII = 1, KY_SPACE = 2, KY_COLON = 4, KY_PERIOD = 8, KY_QUOTES = 16 };

static void condition_key(nptr p)
{
	int c;
	UINT i, j, nonspc, len = *p;
	UINT eMsg = 0;
	byte keybuf[TRX_MAX_KEYLEN + 1];

	nonspc = 0;
	for (i = j = 1; i <= len; i++) {
		c = p[i];
		nonspc++;
		if (!isascii(c)) {
			if (!convert_ansi((nptr)&c)) eMsg |= KY_ASCII;
			else p[i] = c;
		}

		if (c == '\"') continue;

		if (isspace(c)) {
			p[i] = ' ';
			if (!nonspc) {
				//Last char was a space --
				eMsg |= KY_SPACE;
				nonspc = (UINT)-1;
				continue;
			}
			nonspc = (UINT)-1;
		}
		else if (c == ':') {
			if (p[i + 1] != ' ' && !(i > 1 && i < len && isdigit(p[i + 1]) && isdigit(p[i - 1]))) eMsg |= KY_COLON;
		}
		else if (c == '.') {
			if (i < len - 1 && isalpha(p[i + 1]) && isalpha(p[i + 2])) {
				p[0] = (byte)(j - 1);
				AddKeyword(p);
				nonspc = 0;
				j = 1;
				continue;
			}
		}

		p[j++] = p[i];
	}

	p[0] = (byte)(j - 1);

	if (eMsg) {
		trx_Stcpc(keybuf, p);
		if (eMsg&KY_ASCII) logmsg(keybuf, "Keyword had non-ascii character(s)");
		if (eMsg&KY_COLON) logmsg(keybuf, "Keyword has colon followed by non-space");
		if (eMsg&KY_SPACE) logmsg(keybuf, "Keyword had embedded space pair");
	}
}

static BOOL add_author_key(char *cKey, BOOL bAssoc)
{
	byte pkey[MAX_NAMEKEYLEN + 6];
	byte *p = pkey, *psrc = cKey;

	//Copy key, removing spaces after embedded periods and commas and converting non-ascii codes --
	for (; *psrc; psrc++) {
		if (!bAssoc) {
			if (*psrc == ' ' && (*p == ',' || *p == '.' || *p == ' ' || psrc == cKey)) continue;
			//also remove any period followed by a comma --
			if (*psrc == '.' && psrc[1] == ',') continue;
		}
		*++p = *psrc;
		if (!isascii(*p) && !convert_ansi(p)) logmsg(cKey, "Name has unknown non-ascii char");
		if (p - pkey == MAX_NAMEKEYLEN) {
			logmsg(cKey, "Truncated 80-char key placed in name index");
			break;
		}
	}
	//p==pkey or *p==last character stored

	while (p > pkey && !isalpha(*p)) p--;
	p[1] = 0;  //pkey+1 is now a c-str
	*pkey = (byte)(p - pkey); //pkey isnow a p-str with last char *p
	while (p > pkey) if (*p-- == ',') break;

	if (p == pkey) {
		//no comma in pkey --
		if (!pfxcmp(p + 1, "ANONYMOUS")) {
			*p = 9; p[10] = 0;
		}
		else if (!pfxcmp(p + 1, "AUTHOR UNKNOWN")) {
			trx_Stccp(pkey, "UNKNOWN,AUTHOR");
		}
		else {
			//drop certain trailing words --
			for (p = pkey + *pkey; p > pkey; p--) if (*p == ' ') break;
			if (p > pkey) {
				p++;
				if (!strcmp(p, "INC") || !strcmp(p, "LLC") || !pfxcmp(p, "PSEUD") || !pfxcmp(p, "EDITOR") || !pfxcmp(p, "DIRECTOR") ||
					!pfxcmp(p, "MODERATOR")) {
					*--p = 0;
					*pkey = (byte)(p - pkey - 1);
				}
			}
			if (*pkey) {
				bAssoc = TRUE;
			}
		}
	}

	if (!*pkey) {
		logmsg(cKey, "Empty author name - remaining chars ignored");
		return FALSE;
	}

	if (bAssoc) {
		//check if assoc already encountered --
		if (trx_Find(trx, pkey))
			if (strchr(pkey + 1, ' ')) logmsg(pkey + 1, "Organization assumed");
			else logmsg(pkey + 1, "Pseudonym assumed");
	}


	if (trx_InsertKey(trx, &reftotal, pkey)) abortmsg("Error writing author index");
	return TRUE;

}

static UINT chk_abbrev(char *p, char *s)
{
	UINT len = strlen(s);
	if ((!p[len] || p[len] == '.' || p[len] == ',') && !memcmp(p, s, len)) return len;
	return 0;
}

static UINT abbrev_length(char *p)
{
	UINT len;
	if ((len = chk_abbrev(p, "JR")) ||
		(len = chk_abbrev(p, "SR")) ||
		(len = chk_abbrev(p, "II")) ||
		(len = chk_abbrev(p, "IV")) ||
		(len = chk_abbrev(p, "III"))) return len;

	if (len = chk_abbrev(p, "M.D")) {
		if (len == 3) return len;
	}
	return 0;
}

static BOOL reverse_author_key(char *px, char *pxnext)
{
	char *p, *p0 = px, *psuffix;
	UINT len;
	char namebuf[512];

	//first eliminate spaces preceded by periods or commas
	len = 0;
	for (p = px; *p && (!pxnext || p < pxnext); p++) {
		if (*p == ' ' && (!len || px[len - 1] == '.' || px[len - 1] == ',')) continue;
		px[len++] = *p;
	}
	p0[len] = 0;

	//set p and psuffix to comma-prefixed suffix, which will be stored after reversed name --
	psuffix = strrchr(p0, ',');
	if (!psuffix) {
		psuffix = p0 + strlen(p0);
		//see if "last name" is actually a known suffix: JR, III, etc.
		for (p = psuffix - 1; p > p0; p--) if (p[-1] == ' ') break;
		if (p > p0 && abbrev_length(p)) {
			//found a known suffix, so set p to last name past earlier space below
			*--p = ',';
			psuffix = p;
		}
		else {
			p = psuffix;
			assert(!*p);
		}
	}
	else p = psuffix;

	if (p > p0 && p[-1] == '.') p--;
	//p points one past last character of last name -
	//p0 could be AM ENDE

	len = 0;

	for (; p > p0; p--) {
		if (p[-1] == '.' || (p[-1] == ' ' && !(p - p0 > 4 && !memcmp(p - 4, " AM", 3)))) {
			if (len > 1) break;
		}
		len++;
	}

	//p now points to last name of length len, p0 points to first name of length (p-p0)-1:

	if (len + strlen(psuffix) + (p - p0) + 4 >= sizeof(namebuf)) return FALSE;
	memcpy(namebuf, p, len);
	if ((p - p0) > 1) {
		namebuf[len++] = ',';
		memcpy(namebuf + len, p0, (p - p0) - 1);
		len += (p - p0) - 1;
	}
	if (*psuffix) strcpy(namebuf + len, psuffix);
	else namebuf[len] = 0;
	if (pxnext && (UINT)(pxnext - px) <= len) return FALSE;
	//strcpy(px,namebuf);
	for (p = namebuf; *p; p++) {
		if (*p != '.' || p[1] != ',') *px++ = *p;
	}
	*px = 0;
	return TRUE;
}

static int isdate(nptr *p0)
{
	// *p0 == ". ", does it precede a date indicator or follow a numeric date?
	// return 0 (no date or unknown), years>0, or -1 (no date phrase found)
	int len, date;
	nptr p = *p0 + 2;

	if (*p == '[' || *p == '?') p++;

	if (*p == '1' || *p == '2') {
		date = atoi(p);
		for (len = 0; len < 4; len++) if (!isdigit(*++p)) break;
		return (len == 3 || *p == ' ') ? date : -1;
	}

	if (!memcmp(p, "N.D.", 4) || !memcmp(p, "N. D.", 5) ||
		!memcmp(p, "NO DATE", 7) || !memcmp(p, "DATE UNKNOWN", 12) ||
		!memcmp(p, "IN PRESS", 8)) return 0;

	len = 0; p = *p0;
	assert(*p == '.');
	if (isdigit(p[--len]) && isdigit(p[--len]) &&
		isdigit(p[--len]) && isdigit(p[--len]) && p[--len] == ' ') {
		//p=". " immediately follows a numeric date, p+len == " nnnn"
		date = atoi(p + len + 1);
		*p0 = p + len; //back up *p0 5 characters so that **p0 == " " 
		return date;
	}

	return -1;
}

static char *skip_abbrev(char *p)
{
	UINT len;

	p += 2;
	if ((len = abbrev_length(p))) {
		p += len;
		if (*p) {
			*p++ = 0;
			if (*p == '.') p++; //shouldn't be required
			if (*p == ',') p++;
			if (*p == ' ') p++;
		}
	}
	else p[-2] = 0;
	if (!*p) p = 0;
	return p;
}

static char *skip_ed(char *px)
{
	char *p;

	if (!*px) return 0;

	p = 0;
	if (!memcmp(px, "ED", 2)) {
		p = px + 2;
		if (*p == 'S') p++;
	}
	else if (!memcmp(px, "COMP", 4)) {
		p = px + 4;
		if (*p == 'S') p++;
	}
	else if (!memcmp(px, "ET AL", 5)) p = px + 5;

	if (p && (*p == '.' || *p == 0)) {
		if (*p) {
			p++;
			if (*p == ',') p++;
			if (*p == ' ') p++;
		}
		px = p;
		if (!*px) px = 0;
	}
	return px;
}


static nptr advance_px(nptr p)
{
	//p points to the next key to be added, possibly needing to be reversed.
	//This function terminates this key and returns a pointer to the next name.

	char *px = strstr(p, ", ");

	if ((p = strstr(p, " AND ")) && (!px || p <= px + 1)) {
		if (px && p == px + 1) *px = 0;
		else *p = 0;
		return p + 5;
	}

	if (px) px = skip_abbrev(px);

	//string has now been terminated. Now move px to the next author,
	//skipping "ET AL.", "COMP.", "COMPS.", "ED.", or "EDS."
	if (px) {
		px = skip_ed(px);
		if (px && !memcmp(px, "AND ", 4)) px = skip_ed(px + 4);
	}
	return px;
}

static nptr cleanCopy(nptr p0, nptr px)
{
	nptr p, pn;
	UINT lenb, lenc;

	strncpy(p0, px, 506);
	p0[506] = 0;

	lenb = lenc = 0;
	pn = NULL;
	for (px = p = p0; *p; p++) {
		if (*p == '?') continue;
		if (*p == '<') {
			lenb++;
			continue;
		}
		if (*p == '>') {
			if (lenb) lenb--;
			continue;
		}
		if (lenb) continue;

		if (*p == '(') {
			if (!lenc && px > p0 && px[-1] == ' ') px--;
			lenc++;
			continue;
		}

		if (*p == ')') {
			if (lenc) {
				lenc--;
				if (p[1] == '.' && px > p0 && px[-1] == ' ') {
					px[-1] = '.';
					p++;
				}
			}
			continue;
		}

		if (lenc) continue;

		if (*p == '[') {
			if (p[1] != '=') continue;
			while (px > p0 && *(px - 1) == ' ') px--;
			while (px > p0 && *(px - 1) != ' ') px--;
			for (++p; *++p && *p != ']'; px++) {
				*px = is_xspace(*p) ? ' ' : *p;
				if (*px == '"' || *px == '&' || !isascii(*px)) {
					logmsg(p0, "Bracketed group has \", &, or non-ascii character");
				}
			}
			if (!*p) break;
			//*p==']'
		}

		if (*p == ']') {
			if (p[-1] == '.' && p[1] == '.') p++;
			continue;
		}

		if (is_xspace(*p)) {
			if (px > p0 && px[-1] != ' ') {
				*px++ = ' ';
			}
			continue;
		}

		if (*p == '.') {
			if (px > p0 && px[-1] == ' ') px--;
			*px++ = '.';
			continue;
		}

		if (*p == '&') {
			if (!memcmp(p, "&quot;", 6)) {
				p += 5;
			}
			else if (!memcmp(p, "&amp;", 5)) {
				strcpy(px, "and"); px += 3;
				p += 4;
			}
			else if (!memcmp(p, "&nbsp;", 6)) {
				if (px > p0 && px[-1] != ' ') *px++ = ' ';
				p += 5;
			}
			else if (!memcmp(p, "&#8209;", 7)) {
				*px++ = '-';
				p += 6;
			}
			continue;
		}

		if (*p == '"') continue; //skip quotes

		if (!isascii(*p)) {
			*px = *p;
			convert_ansi(px);
			if (*px != '"') px++;
			continue;
		}

		*px++ = *p;
	}

	*px = 0;

	return p0;
}

void index_authors(int trxno, UINT refno, char *pRef)
{
	byte *px, *p, *p0;
	UINT len;
	int date = -1;
	byte buffer[512];

	trx = trxno;
	reftotal = refno;

	//only need upper-case to handle authors and trailing date -
	p0 = _strupr(cleanCopy((nptr)buffer, (nptr)pRef));

	for (p = p0; *p && (p = strstr(++p, ". ")); p++) {
		if ((date = isdate(&p)) >= 0) break;
	}

	IndexDate(date); //date<=0 treated as no date

	if (!p) {
		// no recognizable date or no ". " found in reference --
		logmsg(p0, "Error: Date (\" nnnn.\" or \" n.d.\") not found - no names indexed");
		return;
	}

	len = p - p0;
	while (len && (p0[len - 1] == ',' || p0[len - 1] == '.' || p0[len - 1] == ' ')) len--;

	assert(date >= 0);
	if (date && (date < 1800 || date>2020)) {
		logmsg(p0, "Date is questionable");
	}

	if (!len) {
		logmsg(p0, "Error: No names found to index");
		return;
	}

	p0[len] = 0; //remove trailing period and date

#if 1
	if (!pfxcmp(p0, "HAUWERT. N. [=NICO] M., D.")) {
		int jj = 0;
	}
#endif

	//Author string is an association if name following "AND" suggests it is --
	assert(*p0 != ' ');
	if ((px = strstr(p0, " AND ")) && !pfxcmp(px + 5, "ASSOCIATES") && px[15] != ',') {
		if (px[-1] != ',') {
			add_author_key(p0, TRUE);
			return;
		}
		*--px = 0;
	}

	if (!(px = strrchr(p0, ','))) {
		if (!pfxcmp(p0, "ANONYMOUS")) {
			p0[9] = 0;
			add_author_key(p0, FALSE);
			return;
		}
		//No commas - compound association?
	_retry:
		for (p = p0; *p;) {
			px = strchr(p, '.');
			if (!px) px = p + strlen(p);
			if (px - p < 4) {
				add_author_key(p0, TRUE);
				return;
			}
			if (!*px) break;
			p = px + 1;
			if (*p == ' ') p++;
		}

		if (len == px - p) {
			add_author_key(p0, TRUE);
			return;
		}

		logmsg(p0, "Multiple organizations indexed");

		for (p = p0; *p;) {
			px = strchr(p, '.');
			if (!px) px = p + strlen(p);
			else *px++ = 0;
			add_author_key(p, TRUE);
			p = px;
			if (*p == ' ') p++;
		}

		return;
	}

	//*px is last comma in colplete author string
	if (*++px == ' ') px++;

	if (!strcmp(px, "EDS") || !pfxcmp(px, "EDITOR") || !pfxcmp(px, "DIRECTOR") ||
		!pfxcmp(px, "MODERATOR") || !pfxcmp(px, ", AND ASSOCIATES")) {
		while (px > p0 && (px[-1] == ' ' || px[-1] == ',' || px[-1] == '.')) px--;
		*px = 0;
	}
	else if (!strcmp(px, "INC") || !strcmp(px, "LLC")) {
		while (px > p0 && (px[-1] == ' ' || px[-1] == ',' || px[-1] == '.')) px--;
		*px = 0;
		add_author_key(p0, 2); //don't note in log?
		return;
	}

	if (!*p0) goto _err;

	if (!(px = strchr(p0, ','))) {
		add_author_key(p0, TRUE);
		return;
	}

	//count words in first name --
	len = 1;
	for (p = p0; p < px; p++) if (*p == ' ') len++;
	if (len > 3) {
		//index multiple associations --
		goto _retry;
	}

	if (*++px == ' ') px++;

	px = advance_px(px); //advance past common modifyers, "EDS", etc.

	while (TRUE) {
		//p0 is current name, px is next name --
		if (!add_author_key(p0, FALSE)) break;
		if (!px) break;
		if (!*px) {
			abortmsg("Unexpected parse error: Reference %u", reftotal);
		}
		p0 = px;
		px = advance_px(px);
		if (!reverse_author_key(p0, px)) goto _err;
	}
	return;

_err:
	logmsg(cleanCopy((nptr)buffer, (nptr)pRef), "Invalid format for name(s)");
	return;
}

apfcn_n skip_tag(nptr p)
{
	if (*p == '<') {
		while (*++p && *p != '>');
		if (*p) p++;
	}
	return p;
}

nptr elim_all_st(nptr p0, int n)
{
	nptr pn = p0;
	nptr p, pp;
	char buf[88];
	char buf0[32];
	int lenb;

	sprintf(buf0, "</st%d:", n);
	sprintf(buf, "<st%d:", n);
	lenb = strlen(buf);

	while (pn = strstr(pn, buf)) {
		p = strchr(pn + lenb, '>');
		if (!p) return 0;
		for (pp = pn + lenb; pp < p; pp++) {
			if (is_xspace(*pp)) break;
		}
		p++;
		memcpy(buf0 + lenb + 1, pn + lenb, pp - pn - lenb);
		n = pp - pn + 1;
		buf0[n++] = '>';
		buf0[n] = 0;
		if (!(pp = strstr(p, buf0))) {
			return 0;
		}
		memcpy(pn, p, pp - p);
		pn += (pp - p);
		strcpy(pn, skip_tag(pp));
		pn = p0;
	}
	return skip_space_types(p0);
}

nptr elim_all_ins(nptr p0)
{
	nptr pn = p0;
	nptr p, pp;
	while (pn = strstr(pn, "<ins ")) {
		p = strchr(pn + 5, '>');
		if (!p) return 0;
		p++;
		if (!(pp = strstr(p, "</ins>"))) {
			return 0;
		}
		memcpy(pn, p, pp - p);
		pn += (pp - p);
		strcpy(pn, pp + 6);
		pn = p0;
	}
	return skip_space_types(p0);
}

nptr elim_all_ole(nptr p, BOOL bNote, UINT *uLinksOLE)
{
	//remove any <a name=xxx></a> or <a name=xxx>...</a>
	nptr p0, p1;
	while (p0 = strstr(p, "<a name=")) {
		*uLinksOLE = *uLinksOLE + 1;
		if (bNote) logmsg(p0, "Note: Link removed from paragraph");
		if (!(p1 = strchr(p0, '>'))) return NULL;
		p1++;
		strcpy(p0, p1);
		if (!(p1 = strstr(p0, "</a>"))) return NULL;
		strcpy(p1, p1 + 4);
	}
	return p;
}

nptr elim_all_spans(nptr p)
{
	//remove any <a name=xxx></a> or <a name=xxx>...</a>
	nptr p0, p1;
	while (p0 = strstr(p, "<span ")) {
		if (!(p1 = strchr(p0, '>'))) return NULL;
		p1++;
		strcpy(p0, p1);
		if (!(p1 = strstr(p0, "</span>"))) return NULL;
		strcpy(p1, p1 + 7);
	}
	return p;
}

static BOOL is_abbrev2(LPSTR p)
{
	static LPCSTR abbrev2[] = { "NO","FT","MI","MR","MT","ST","JR" };
	int i = sizeof(abbrev2) / sizeof(LPCSTR) - 1;
	p -= 1;
	for (; i >= 0; i--) {
		if (!memicmp(p, abbrev2[i], 2)) break;
	}
	return i >= 0;
}

static BOOL is_abbrev3(LPSTR p)
{
	static LPCSTR abbrev3[] = { "NOS","BLK","SEC","HWY","MRS" };
	int i = sizeof(abbrev3) / sizeof(LPCSTR) - 1;
	p -= 2;
	for (; i >= 0; i--) {
		if (!memicmp(p, abbrev3[i], 2)) break;
	}
	return i >= 0;
}

apfcn_v parse_keywords(nptr p0)
{
	nptr p;
	UINT keylen, toklen;
	int c;
	byte keybuf[TRX_MAX_KEYLEN + 2];
	BOOL bLastSpc;

	p = p0;

	nParseKW++;

#ifdef _DEBUG
	if (reftotal == 3102) {
		int jj = 0;
	}
#endif

	while (*p) {
		keylen = toklen = 0;
		while (iskeydelim(*p)) p++;
		bLastSpc = TRUE;

		for (; *p; p++) {
			if (*p == '<') {
				p = skip_tag(p);
				p--;
				continue;
			}

			if (is_xspace(*p)) {
				if (bLastSpc) continue;
				bLastSpc = TRUE;
				if (!keylen) continue;
				c = ' ';
				goto _next;
			}

			if (*p == '&') {
				if (!memicmp(p, "&nbsp;", 6)) {
					p += 5;
					if (bLastSpc) continue;
					bLastSpc = TRUE;
					if (!keylen) continue;
					c = ' ';
					goto _next;
				}
				if (!memicmp(p, "&quot;", 6)) {
					c = '"';
					p += 5;
				}
				else if (!memicmp(p, "&amp;", 5)) {
					c = '&';
					p += 4;
				}
				else if (!memicmp(p, "&mdash;", 7)) {
					c = '-';
					p += 6;
				}
				else if (!memcmp(p, "&#8209;", 7)) {
					c = '-';
					p += 6;
				}
				else c = '&';
				bLastSpc = FALSE;
				goto _next;
			}

			bLastSpc = FALSE;

			if (*p == ':' && keylen >= 6 && !memcmp(keybuf + keylen - 5, "COUNTY", 6)) {
				logmsg(p, "Note: Colon following COUNTY in KW list changed to period");
				*p = '.';
			}

			if (*p == '.') {
				if (!p[1]) break;
				if (is_xspace(p[1]) || p[1] == '<' || !pfxcmp(p + 1, "&nbsp;")) {
					if (!((toklen == 1 && isalpha(keybuf[keylen])) || toklen == 2 && is_abbrev2(keybuf + keylen) || toklen == 3 && is_abbrev3(keybuf + keylen))) {
						break;
					}
				}
			}

			c = *p;
		_next:
			if (isalpha(c) || isdigit(c)) {
				toklen++;
			}
			else toklen = 0;

			if (keylen < TRX_MAX_KEYLEN) keybuf[++keylen] = toupper(c);
		}

		while (keylen && iskeydelim(keybuf[keylen])) keylen--;
		if (keylen) {
			*keybuf = (byte)keylen;
			keybuf[keylen + 1] = 0;
			condition_key(keybuf);
			AddKeyword(keybuf);
		}
	}
}
