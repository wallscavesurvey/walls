// <copyright>
// 
// Copyright (c) 1993-1995
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
// </copyright>

//<file>
//
// Name:       str.C
//
// Purpose:    Character pointer with resource management
//
// Created:     1990    Gerald Pani
//
// Modified:   24 Aug 93    Gerald Pani
// Modified:   14 Sep 94    Gerald Pani
// 		o initLowTable
//
//
//
// Description:
//
//</file>

#include "..\\stdafx.h"

#include <string.h>
#include <memory.h>

#include <ctype.h>
#include <iostream>

#include "str.h"

int class_RString_version_1_2;

static const int lineBufSize = 512;
static char lineBuf[lineBufSize];

static inline char* mmemcpy(char* s1, const char* s2, int len) {
	return (char*)::memcpy(s1, s2, len);
}

static inline const char* mmemchr(const char* s1, char c, int len) {
	return (const char*)::memchr((const void*)s1, c, len);
}

static inline int mmemcmp(const char* s1, const char* s2, int len) {
	return ::memcmp(s1, s2, len);
}


// ---- Delimiter for sgmlIso2LowerAscii() -------
// ---- critical characters ------

// 26 '&', 

static char isoDelimTable[256] = {

	//   0 1 2 3 4 5 6 7 8 9 a b c d e f
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 1
		 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0, // 2
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 3
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 4
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 5
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 6
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1, // 7
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 8
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 9
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // a
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // b
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // c
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // d
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // e
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1  // f
};

static const char* lowTable[256];

// static const char* lowTable[256] = {

// //     0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 0
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 1
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 2
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 3
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 4
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 5
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 6
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // 7
//       nil,"ue", nil, nil,"ae", nil, nil, nil, nil, nil, nil, nil, nil, nil,"ae", nil, // 8
//       nil, nil, nil, nil,"oe", nil, nil, nil, nil,"oe","ue", nil, nil, nil, nil, nil, // 9
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // a
//       nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // b
//       nil, nil, nil, nil,"ae", nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, // c
//       nil, nil, nil, "o", nil, nil,"oe", nil, nil, nil, nil, nil,"ue", nil, nil,"ss", // d
//       nil, "a", nil, nil,"ae", nil, nil, "c", nil, "e", nil, nil, nil, nil, nil, nil, // e
//       nil, nil, nil, nil, nil, nil,"oe", nil, nil, nil, "u", nil,"ue", nil, nil, nil  // f
//      };

// /* lowTable['ä'] = "ae";
//  * lowTable['ö'] = "oe";
//  * lowTable['ü'] = "ue";
//  * lowTable['Ä'] = "ae";
//  * lowTable['Ö'] = "oe";
//  * lowTable['Ü'] = "ue";
//  * lowTable['ß'] = "ss";
//  */

static char whiteTable[256];
static RString wsChars;


RString operator+(const RString & s1, const RString & s2) {
	RString ret(s1, s2);
	return ret;
}

int operator ==(const RString& s1, const char* s2) {
	if (!s2)
		return (s1.length() ? 0 : 1);
	if (!::strcmp(s1.string(), s2))
		return 1;
	return 0;
}

int operator ==(const char* s1, const RString & s2) {
	if (!s1)
		return (s2.length() ? 0 : 1);
	if (!::strcmp(s1, s2.string()))
		return 1;
	return 0;
}

int operator ==(const RString& s1, const RString & s2) {
	if (s1.rep_ == s2.rep_)
		return 1;
	if (s1.length() != s2.length())
		return 0;
	if (!::mmemcmp(s1.string(), s2.string(), s1.length()))
		return 1;
	return 0;
}

int operator !=(const RString& s1, const char* s2) {
	if (!(s1 == s2))
		return 1;
	return 0;
}

int operator !=(const char* s1, const RString & s2) {
	if (!(s1 == s2))
		return 1;
	return 0;
}

int operator !=(const RString& s1, const RString & s2) {
	if (!(s1 == s2))
		return 1;
	return 0;
}

int operator <(const RString& s1, const RString & s2) {
	if (s1.length() < s2.length()) {
		if (::mmemcmp(s1.string(), s2.string(), s1.length() + 1) < 0)
			return 1;
		else
			return 0;
	}
	else {
		if (::mmemcmp(s1.string(), s2.string(), s2.length() + 1) < 0)
			return 1;
		else
			return 0;
	}
}

int operator <(const char* s1, const RString & s2) {
	if (!s1)
		return (s2.length() ? 1 : 0);
	if ((::strcmp((const char *)s1, s2.string())) < 0)
		return 1;
	return 0;
}

int operator <(const RString& s1, const char* s2) {
	if (!s2)
		return 0;
	if ((::strcmp(s1.string(), (const char*)s2)) < 0)
		return 1;
	return 0;
}

StringRep* StringRep::lambda_;

StringRep::StringRep(const char *s) {
	len_ = strlen(s);
	str_ = new char[len_ + 1];
	mmemcpy(str_, s, len_);
	str_[len_] = '\0';
	refs_ = 1;
}

StringRep::StringRep(const char *s, int length) {
	if (length < 0)
		length = 0;
	len_ = length;
	str_ = new char[len_ + 1];
	mmemcpy(str_, s, len_);
	str_[len_] = 0;
	refs_ = 1;
}

StringRep::StringRep(const char* s1, const char* s2) {
	int len1 = strlen(s1);
	int len2 = strlen(s2);
	len_ = len1 + len2;
	str_ = new char[len_ + 1];
	mmemcpy(str_, s1, len1);
	mmemcpy(&str_[len1], s2, len2);
	str_[len_] = '\0';
	refs_ = 1;
}

StringRep::StringRep(StringRep& s1, StringRep& s2) {
	len_ = s1.len_ + s2.len_;
	str_ = new char[len_ + 1];
	mmemcpy(str_, s1.str_, s1.len_);
	mmemcpy(&str_[s1.len_], s2.str_, s2.len_);
	str_[len_] = '\0';
	refs_ = 1;
}


struct ConvertEnt {
	const char* sgml;
	char iso;
	const char* low;
};

static const int numents = 98;

#pragma warning(disable: 4305 4309)

static ConvertEnt convertTable[numents] = { { "AElig",	 16 * 12 + 6,	"ae"	},
					  { "Aacute",	 16 * 12 + 1,	"a"	},
					  { "Acirc",	 16 * 12 + 2,	"a"	},
					  { "Agrave",	 16 * 12 + 0,	"a"	},
					  { "Aring",	 16 * 12 + 5,	"a"	},
					  { "Atilde",	 16 * 12 + 3,	"a"	},
					  { "Auml",	 16 * 12 + 4,	"ae"	},
					  { "Ccedil",	 16 * 12 + 7,	"c"	},
					  { "Dstrok",	 16 * 13 + 0,	"d"	},
					  { "Eacute",	 16 * 12 + 9,	"e"	},
					  { "Ecirc",	 16 * 12 + 10,	"e"	},
					  { "Egrave",	 16 * 12 + 8,	"e"	},
					  { "Euml",	 16 * 12 + 11,	"e"	},
					  { "Iacute",	 16 * 12 + 13,	"i"	},
					  { "Icirc",	 16 * 12 + 14,	"i"	},
					  { "Igrave",	 16 * 12 + 12,	"i"	},
					  { "Iuml",	 16 * 12 + 15,	"i"	},
					  { "Ntilde",	 16 * 13 + 1,	"n"	},
					  { "Oacute",	 16 * 13 + 3,	"o"	},
					  { "Ocirc",	 16 * 13 + 4,	"o"	},
					  { "Ograve",	 16 * 13 + 2,	"o"	},
					  { "Oslash",	 16 * 13 + 8,	"o"	},
					  { "Otilde",	 16 * 13 + 5,	"o"	},
					  { "Ouml",	 16 * 13 + 6,	"oe"	},
					  { "THORN",	 16 * 13 + 14,	""	},
					  { "Uacute",	 16 * 13 + 10,	"u"	},
					  { "Ucirc",	 16 * 13 + 11,	"u"	},
					  { "Ugrave",	 16 * 13 + 9,	"u"	},
					  { "Uuml",	 16 * 13 + 12,	"ue"	},
					  { "Yacute",	 16 * 13 + 13,	"y"	},
					  { "aacute",	 16 * 14 + 1,	"a"	},
					  { "acirc",	 16 * 14 + 2,	"a"	},
					  { "acute",	 16 * 11 + 4,	""	},
					  { "aelig",	 16 * 14 + 6,	"ae"	},
					  { "agrave",	 16 * 14 + 0,	"a"	},
					  { "amp",	 16 * 2 + 6,	"&"	},
					  { "aring",	 16 * 14 + 5,	"a"	},
					  { "atilde",	 16 * 14 + 3,	"a"	},
					  { "auml",	 16 * 14 + 4,	"ae"	},
					  { "brkbar",	 16 * 10 + 6,	""	},
					  { "ccedil",	 16 * 14 + 7,	"c"	},
					  { "cedil",	 16 * 11 + 8,	""	},
					  { "cent",	 16 * 10 + 2,	""	},
					  { "copy",	 16 * 10 + 9,	""	},
					  { "curren",	 16 * 10 + 4,	""	},
					  { "deg",	 16 * 11 + 0,	""	},
					  { "divide",	 16 * 15 + 7,	""	},
					  { "eacute",	 16 * 14 + 9,	"e"	},
					  { "ecirc",	 16 * 14 + 10,	"e"	},
					  { "egrave",	 16 * 14 + 8,	"e"	},
					  { "eth",	 16 * 15 + 0,	""	},
					  { "euml",	 16 * 14 + 11,	"e"	},
					  { "frac12",	 16 * 11 + 13,	""	},
					  { "frac14",	 16 * 11 + 12,	""	},
					  { "frac34",	 16 * 11 + 14,	""	},
					  { "hibar",	 16 * 10 + 15,	""	},
					  { "iacute",	 16 * 14 + 13,	"i"	},
					  { "icirc",	 16 * 14 + 14,	"i"	},
					  { "iexcl",	 16 * 10 + 1,	""	},
					  { "igrave",	 16 * 14 + 12,	"i"	},
					  { "iquest",	 16 * 11 + 15,	""	},
					  { "iuml",	 16 * 14 + 15,	"i"	},
					  { "laquo",	 16 * 10 + 11,	""	},
					  { "lt",		 16 * 3 + 12,	"<"	},
					  { "micro",	 16 * 11 + 5,	""	},
					  { "middot",	 16 * 11 + 7,	""	},
					  { "nbsp",	 16 * 10 + 0,	" "	},
					  { "not",	 16 * 10 + 12,	""	},
					  { "ntilde",	 16 * 15 + 1,	"n"	},
					  { "oacute",	 16 * 15 + 3,	"o"	},
					  { "ocirc",	 16 * 15 + 4,	"o"	},
					  { "ograve",	 16 * 15 + 2,	"o"	},
					  { "ordf",	 16 * 10 + 10,	""	},
					  { "ordm",	 16 * 11 + 10,	""	},
					  { "oslash",	 16 * 15 + 8,	"o"	},
					  { "otilde",	 16 * 15 + 5,	"o"	},
					  { "ouml",	 16 * 15 + 6,	"oe"	},
					  { "para",	 16 * 11 + 6,	""	},
					  { "plusmn",	 16 * 11 + 1,	""	},
					  { "pound",	 16 * 10 + 3,	""	},
					  { "raquo",	 16 * 11 + 11,	""	},
					  { "reg",	 16 * 10 + 14,	""	},
					  { "sect",	 16 * 10 + 7,	""	},
					  { "shy",	 16 * 10 + 13,	""	},
					  { "sup1",	 16 * 11 + 9,	""	},
					  { "sup2",	 16 * 11 + 2,	""	},
					  { "sup3",	 16 * 11 + 3,	""	},
					  { "szlig",	 16 * 13 + 15,	"ss"	},
					  { "thorn",	 16 * 15 + 14,	""	},
					  { "times",	 16 * 13 + 7,	""	},
					  { "uacute",	 16 * 15 + 10,	"u"	},
					  { "ucirc",	 16 * 15 + 11,	"u"	},
					  { "ugrave",	 16 * 15 + 9,	"u"	},
					  { "uml",	 16 * 10 + 8,	""	},
					  { "uuml",	 16 * 15 + 12,	"ue"	},
					  { "yacute",	 16 * 15 + 13,	"y"	},
					  { "yen",	 16 * 10 + 5,	""	},
					  { "yuml",	 16 * 15 + 15,	"y"	}
};



static boolean fdConvEnt(const RString& sgml, int& pos) {
	int low = 0, high = numents;
	for (;;) {
		if (low == high) {
			pos = low;
			return false;
		}
		int mid = (low + high) / 2;
		if (sgml == convertTable[mid].sgml) {
			pos = mid;
			return true;
		}
		if (sgml < convertTable[mid].sgml)
			high = mid;
		else
			low = mid + 1;
	}
}

static boolean lowTableInitialized = false;

static void initLowTable() {
	if (lowTableInitialized)
		return;
	int i;
	for (i = 0; i < 256; i++) {
		lowTable[i] = nil;
	}
	for (i = 0; i < numents; i++) {
		if (convertTable[i].low && *(convertTable[i].low))
			lowTable[(unsigned char)convertTable[i].iso] = convertTable[i].low;
	}
	lowTableInitialized = true;
}


const RString RString::lambda_;

RString::RString() {
	rep_ = StringRep::lambda();
	rep_->refs_++;
}

RString::~RString() {
	if (!--rep_->refs_) {
		delete rep_;
	}
}


RString::RString(const RString &init) {
	rep_ = init.rep_;
	rep_->refs_++;
}

RString::RString(const char* s) {
	if (!s || !*s) {
		rep_ = StringRep::lambda();
		rep_->refs_++;
	}
	else
		rep_ = new StringRep(s);
}

RString::RString(const char* s, int len) {
	if (!s || !len) {
		rep_ = StringRep::lambda();
		rep_->refs_++;
	}
	else
		rep_ = new StringRep(s, len);
}

RString::RString(const RString& s1, const RString& s2) {
	rep_ = new StringRep(*(s1.rep_), *(s2.rep_));
}

RString& RString::operator =(const RString& str) {
	if (rep_ == str.rep_)
		return *this;

	if (rep_) {
		if (!--rep_->refs_) {
			delete rep_;
		}
	}
	else
		cerr << "RString Assignment failed:" << str << endl;

	rep_ = str.rep_;
	rep_->refs_++;
	return *this;
}

RString& RString::operator =(const char* s) {
	RString tmp(s);
	return operator=(tmp);
}

RString& RString::operator +=(const RString& str) {
	StringRep* nr = new StringRep(*rep_, *str.rep_);
	if (!--rep_->refs_) {
		delete rep_;
	}
	rep_ = nr;
	return *this;
}

int RString::index(char c) const {
	return index(0, c);
}

int RString::index(int start, char c) const {
	if (start < 0)
		start = 0;
	else if (start > length())
		start = length();
	const char* p = ::mmemchr(&rep_->str_[start], c, length() - start);
	if (!p)
		return -1;
	return p - rep_->str_;
}

int RString::indexa(const char* str) const {
	return indexa(0, RString(str));
}

int RString::indexa(int start, const char* str) const {
	return indexa(start, RString(str));
}

int RString::indexa(const RString& str) const {
	return indexa(0, str);
}

int RString::indexa(int start, const RString& str) const {
	if (start < 0)
		start = 0;
	else if (start > length())
		start = length();
	// 0 <= start <= length()

	// testing search area
	int len = length() - start;
	if (!len)
		return -1;
	const char* string = &rep_->str_[start];
	// len > 0

	const char* word = str.string();
	int wordlen = str.length();
	if (!wordlen)
		return -1;
	// wordlen > 0

	// search for str in search-area
	const char *s, *p;
	for (;;) {
		// string ... pointer to search-area
		// len    ... length of the search-area
		// word   ... pointer tor the search string
		if (!(p = ::mmemchr(string, *word, len))) // search for the first character of word
			return -1;		      // no, does not occur
		string = ++p;			      // new search-area
		len = length() - (string - rep_->str_); // adjust len
		s = word + 1;			      // compare word
		int i, j;
		for (i = len, j = wordlen - 1;  // max. length to compare
			i > 0 && j > 0 && *p == *s;    // compare and test length
			i--, j--, s++, p++);	      // skip
		if (j == 0)
			return p - rep_->str_; // ok, word found
		if (i == 0)
			return -1;	// no, end of search-area
	}
}

RString RString::gSubstrDelim(int index, char delim) const {
	if (index > rep_->len_)
		return RString();
	if (index < 0)
		index = 0;
	const char* subst = &rep_->str_[index];
	const char* p = ::mmemchr(subst, delim, length() - index);
	if (p)
		return RString(subst, p - subst);
	else
		return RString(subst);
}

RString RString::gSubstrIndex(int begin, int end) const {
	if (!length())
		return RString();
	if (begin > end)
		return RString();
	if (begin >= rep_->len_)
		return RString();
	if (end < 0)
		return RString();
	if (begin < 0)
		begin = 0;
	if (end >= rep_->len_)
		end = rep_->len_ - 1;
	return RString(&rep_->str_[begin], end - begin + 1);
}

RString RString::gRight(int begin) const {
	if (!length())
		return RString();
	if (begin >= rep_->len_)
		return RString();
	if (begin < 0)
		begin = 0;
	return RString(&rep_->str_[begin], rep_->len_ - begin);
}

RString& RString::invert() {
	if (rep_->refs_ > 1) {
		--rep_->refs_;
		rep_ = new StringRep(rep_->str_, rep_->len_);
	}
	for (int i = rep_->len_; i >= 0; i--)
		rep_->str_[i] = ~(rep_->str_[i]) + 1;	// 256 - *p
	return *this;
}

RString& RString::tolower() {
	if (rep_->refs_ > 1) {
		--rep_->refs_;
		rep_ = new StringRep(rep_->str_, rep_->len_);
	}
	for (char* p = rep_->str_; *p; p++)
		*p = ::tolower(*p);
	return *this;
}

RString& RString::subst(char a, char b) { // replace all a with b
	if (rep_->refs_ > 1) {
		--rep_->refs_;
		rep_ = new StringRep(rep_->str_, rep_->len_);
	}
	int i = length();
	for (char* p = rep_->str_; i; p++, i--)
		if (*p == a)
			*p = b;
	return *this;
}

// replace all occurences of "from" with "to"
RString& RString::subst(const RString& from, const RString& to, boolean caseInSens) {
	int fromlength = from.length();
	if (!length() || !fromlength)
		return *this;

	RString comparestring = *this;
	RString fromstring = from;
	if (caseInSens) {
		comparestring.tolower();
		fromstring.tolower();
	}
	int index = 0;
	int where;
	RString tmp;
	while ((where = comparestring.indexa(index, fromstring)) != -1) {
		tmp += gSubstrIndex(index, where - fromlength - 1) + to;
		index = where;
	}
	if (index <= length()) {
		tmp += gRight(index);
	}
	*this = tmp;
	return *this;
}

// replace all occurences of "from" in "in" with "to" if from is surrounded by a delim or 
// the word is at the beginning or the end and a delim on the other side

RString& RString::subst(const RString& from, const RString& to,
	const RString& delim, boolean caseInSens) {
	int fromlength = from.length();
	if (!length() || !fromlength)
		return *this;

	RString comparestring = *this;
	RString fromstring = from;
	RString delimstring = delim;
	if (caseInSens) {
		comparestring.tolower();
		fromstring.tolower();
		delimstring.tolower();
	}

	int index = 0;
	int where;
	RString tmp;
	while ((where = comparestring.indexa(index, fromstring)) != -1) {
		// from found
		// ("from..." || "delim from...") && ("...from" || "...from delim...")
		if (((where - fromlength == 0) ||
			(delimstring.index(comparestring[where - fromlength - 1]) != -1)) &&
			(((where == length()) || (delimstring.index(comparestring[where]) != -1))))
		{
			tmp += gSubstrIndex(index, where - fromlength - 1) + to;
		}
		else
		{
			tmp += gSubstrIndex(index, where - 1);
		}
		index = where;
	}
	if (index <= length())
	{
		tmp += gRight(index);
	}
	*this = tmp;
	return *this;
}

RString& RString::ignMultChars(char c) {
	if (!length())
		return *this;
	if (rep_->refs_ > 1) {
		--rep_->refs_;
		rep_ = new StringRep(rep_->str_, rep_->len_);
	}
	char* from = rep_->str_;
	char* to = from;
	boolean gotIt = false;
	for (int i = length(); i; i--) {
		if (*from == c) {
			if (!gotIt) {
				*to++ = *from;
				gotIt = true;
			}
		}
		else {
			*to++ = *from;
			gotIt = false;
		}
		from++;
	}
	*to = '\0';
	rep_->len_ = to - rep_->str_;
	return *this;
}

RString& RString::sgml2Iso() {
	if (index('&') == -1) // no SGML entities;
		return *this;
	else {                   // replace SGML entities by ISO characters
		RString out, part, ent;
		int pos = 0;
		boolean word = true;
		do {
			out += part;
			if (operator[](pos) == '&') {
				ent = gSubstrDelim(pos + 1, ';');
				int tpos;
				if (length() > ent.length() + pos + 1 && fdConvEnt(ent, tpos)) {
					out += RString(&(convertTable[tpos].iso), 1);
					pos += ent.length() + 2;
				}
				else {
					out += "&";
					pos++;
				}
				part = lambda_;
				continue;
			}
			word = gWordChar(pos, '&', part);
		} while (word);
		operator=(out);
	}
	return *this;
}

RString& RString::sgmlIso2LowerAscii() {
	initLowTable();
	RString out, part, ent;
	int pos = 0;
	boolean word = true;
	do {
		out += part.tolower();
		if (operator[](pos) == '&') {
			ent = gSubstrDelim(pos + 1, ';');
			int tpos;
			if (length() > ent.length() + pos + 1 && fdConvEnt(ent, tpos)) {
				out += convertTable[tpos].low;
				pos += ent.length() + 2;
			}
			else {
				out += "&";
				pos++;
			}
			part = lambda_;
			continue;
		}
		else
			if (operator[](pos)) {
				if (lowTable[(unsigned char)operator[](pos)]) {
					out += lowTable[(unsigned char)operator[](pos)];
					pos++;
					part = lambda_;
					continue;
				}
				else if (isoDelimTable[(unsigned char)operator[](pos)]) {
					pos++;
					part = lambda_;
					continue;
				}
			}
		word = gWordTable(pos, isoDelimTable, part);
	} while (word);
	*this = out;
	return *this;
}

RString& RString::toupper() {
	if (rep_->refs_ > 1) {
		--rep_->refs_;
		rep_ = new StringRep(rep_->str_, rep_->len_);
	}
	for (char* p = rep_->str_; *p; p++)
		*p = ::toupper(*p);
	return *this;
}

RString& RString::dos2Unix() {
	int i = length();
	int crCount = 0;
	for (char* p = rep_->str_; i; p++, i--)
		if (*p == 13)
			crCount++;
	if (crCount) {
		char* out = new char[length() - crCount + 1];
		char* q = out;
		i = length();
		for (char* p = rep_->str_; i; p++, i--)
			if (*p != 13)
				*(q++) = *p;
		*q = '\0';
		i = length();
		if (!--rep_->refs_) {
			delete rep_;
		}
		rep_ = new StringRep(out, i - crCount);
		delete out;
	}
	return *this;
}

RString& RString::unix2Dos() {
	int i = length();
	int nlCount = 0;
	for (char* p = rep_->str_; i; p++, i--)
		if (*p == 10)
			nlCount++;
	if (nlCount) {
		char* out = new char[length() + nlCount + 1];
		char* q = out;
		i = length();
		for (char* p = rep_->str_; i; p++, i--) {
			if (*p == 10)
				*(q++) = 13;
			*(q++) = *p;
		}
		*q = '\0';
		i = length();
		if (!--rep_->refs_) {
			delete rep_;
		}
		rep_ = new StringRep(out, i + nlCount);
		delete out;
	}
	return *this;
}

boolean RString::gWord(int& index, const RString& white, RString& word) const {
	if (white != wsChars) {
		const char* p;
		for (p = wsChars.string(); *p; p++)
			whiteTable[(unsigned char)*p] = 0;
		for (p = white.string(); *p; p++)
			whiteTable[(unsigned char)*p] = 1;
		wsChars = white;
	}

	return gWordTable(index, whiteTable, word);
}

boolean RString::gWordTable(int& index, const char white[], RString& word) const {
	if ((index >= length()) || (index < 0))
		return false;

	const char* p;
	for (p = rep_->str_ + index; *p; p++) {
		if (!(white[(unsigned char)*p]))
			break;
	}

	// p points to end of string or at the first non-whitespace
	if (!*p)
		return false;
	// ok, p points to non-whitespace
	const char* beg = p; // mark the begin
	for (; *p; p++)
		if (white[(unsigned char)*p])
			break;

	// p points to end of string or at the first whitespace after the word.
	index = p - rep_->str_;
	word = RString(beg, p - beg);
	return true;
}

boolean RString::gWordChar(int& index, const char white, RString& word) const {
	if ((index >= length()) || (index < 0))
		return false;

	const char* p;
	for (p = rep_->str_ + index; *p; p++) {
		if (*p != white)
			break;
	}

	// p points to end of string or at the first non-whitespace
	if (!*p)
		return false;
	// ok, p points to non-whitespace
	const char* beg = p; // mark the begin
	for (; *p; p++)
		if (white == *p)
			break;

	// p points to end of string or at the first whitespace after the word.
	index = p - rep_->str_;
	word = RString(beg, p - beg);
	return true;
}

RString& RString::gLine(istream& inp, char delim) {
	RString line;
	while (inp) {
		lineBuf[0] = '\0';
		inp.get(lineBuf, lineBufSize - 1, delim);
		if (inp) {
			char n = inp.get();
			if (!inp || n == delim) {
				line += lineBuf;
				break;
			}
			else {
				lineBuf[lineBufSize - 2] = n;
				lineBuf[lineBufSize - 1] = '\0';
				line += lineBuf;
			}
		}
		else {
			line += lineBuf;
			break;
		}
	}
	return operator=(line);
}
