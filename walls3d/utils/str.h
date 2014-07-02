// This may look like C code, but it is really -*- C++ -*-

// <copyright>
// 
// Copyright (c) 1993-1995
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
// </copyright>

//<file>
//
// Name:       str.h
//
// Purpose:    Character pointer with resource management
//
// Created:     1990    Gerald Pani
//
// Modified:   24 Aug 93    Gerald Pani
//
//
//
// Description:
//
//</file>


#ifndef hg_utils_str_h
#define hg_utils_str_h


#include "types.h"
#include <iostream>
using namespace std;

extern int class_RString_version_1_2;
static int class_RString_version = class_RString_version_1_2;

//<class>
//
// Name:       StringRep
//
// Purpose:    Unique representation of a string
// 
//
// Private Interface:
//
// Accessible through class RString.
//
// - StringRep( const char* s)
//   Constructor. Makes a copy of s. s must be null-terminated. The
//   copy is null-terminated too.
//
// - StringRep( const char* s, int len)
//   Constructor. Makes a copy of s. The copy has len + 1 bytes and is
//   null-terminated. 
//
// - StringRep( const char* s1, const char* s2)
//   Constructor. Concatenation of s1 and s2. s1 and s2 must be
//   null-terminated. The copy is null-terminated. 
//
// - StringRep( StringRep& r1, StringRep& r2)
//   Constructor. Concatenation of r1 and r2. r1 or r2 can contain
//   null-characters. The copy is null-terminated. 
//
// - ~StringRep()
//   Destructor. Frees the storage used for the string.
//
// - static StringRep* lambda()
//   Gives a pointer to the empty string.
//   
//
// Description:
//
// 
//</class>


class StringRep {
  public:
     StringRep( const char*);
     StringRep( const char*, int len);
     StringRep( const char*, const char *);
     StringRep( StringRep&, StringRep&);
     ~StringRep() { delete str_; }

     static StringRep* lambda();

     char* str_;		// pointer to the character array
				// (always null-terminated)
     int refs_;			// reference count
     int len_;			// length of the character array
				// (without terminating
				// null-character) 
     static StringRep* lambda_;	// pointer to the representation of
				// the empty string ("")
};

inline StringRep* StringRep::lambda() {
     if (!lambda_)
	  lambda_ = new StringRep("");
     return lambda_;
}


//<class>
//
// Name:       RString
//
// Purpose:   
// 
//
// Public Interface:
//
// - RString()
//   Default constructor. Empty string (lambda).
//
// - RString( const char* s)
//   Constructor. s nil or "" -> lambda; otherwise make a copy of s (s
//   must be null-terminated). 
//
// - RString( const char* s, int len)
//   Constructor. s nil or len = 0 -> lambda; otherwise copy len
//   characters of s and append null-character.
//
// - RString( const RString& r)
//   Constructor. Pointer to the representation of r (increments the
//   reference count).
//
// - ~RString()
//   Destructor. Decrements the reference count and deletes the
//   StringRep if no other RString points to it.
//
// - int indexa( const RString& r) const
// - int indexa( const char* r) const
//   Returns the index of the first character after r (0 <= index <= len) if r is
//   a substring of this or result -1 otherwise.
//   
// - int indexa( int index, const RString& r) const
// - int indexa( int index, const char* r) const
//   Similar to indexa( r) but starting at position index.
//   
// - int index( char c) const
// - int index( int index, char c) const
//   Returns the index of the first occurence of character c (0 <= index < len) or
//   -1 otherwise.
//   
// - RString gSubstrDelim( int index, char delim) const
//   Returns a new RString holding the substring which begins at
//   position index up to the last character before delim or up to the end of
//   string otherwise.
//   
// - RString gSubstrIndex( int begin, int end) const
//   Returns a new RString holding the substring which begins at
//   position begin and ends at position end.
//   
// - RString gRight( int begin) const
//   Returns a new RString holding the substring beginning at
//   position begin up to the end of string.
//   
// - boolean gWord( int& index, const RString& white, RString& word) const
//   Retrieves the next sequence of nonwhitespace characters beginning
//   at position index. The RString white defines the whitespace characters.
//   For a faster lookup they are entered into a table. (This table initialisation
//   takes place only if white differs from the previous used whitespace
//   definition.) If successful the function stores the retrieved character
//   sequence into word, sets index to the next character position after word
//   and returns true.
//   
// - boolean gWordChar( int& index, const char white, RString& word) const
//   Similar to gWord using only one whitespace character.
//   
// - RString& gLine( istream& s, char delim = '\n') const
//   Extracts characters from istream s and stores them in this RString.
//   Extraction stops when delim is encountered. (delim is extracted but not stored.)
//   The function returns a reference to *this.
//   
// - int length() const
//   Returns the length of the stored string.
//   
// - RString& subst( char a, char b)
//   Replaces all occurences of a with b returning a reference to *this.
//
// - RString& subst( const RString& from, const RString& to, boolean caseInSens = true)
//   Replaces all occurences of "from" in "in" with "to" returning a reference to *this.
//
// - RString& subst( const RString& from, const RString& to, const RString& delimiters,
//                   boolean caseInSens = true)
//   Similar to the previous method, but "from" must be either surrounded by
//   a delimiter of delimiters (beginning and end of string is treated as a delimiter).
//
// - RString& invert()
//   Inverts (256 - char) each character returning a reference to *this. 
// 
// - RString& tolower()
//   Converts the string to lower case letters returning a reference to *this. 
// 
// - RString& toupper()
//   Converts the string to upper case letters returning a reference to *this. 
// 
// - RString& sgml2Iso()
//   Converts SGML entities to ISO characters returning a reference to *this. 
// 
// - RString& sgmlIso2LowerAscii()
//   Converts SGML entities and ISO characters to lower case returning
//   a reference to *this. E.g. "AElig" to "ae" or "Aacute" to "a".
// 
// - RString& dos2Unix()
//   Removes all occurances of a ``carriage return'' returning a reference to *this. 
// 
// - RString& unix2Dos()
//   Converts ``newline'' to ``carriage return'' ``newline'' returning a
//   reference to *this.
//
// - RString& ignMultChars( char c)
//   Converts in succession occurences of c to a single c returning a reference to *this.
// 
// - static const RString lambda()
//   Returns an empty RString.
//
// - static const RString& lambdaRef()
//   Returns a reference to an empty RString.
//
// - RString& operator =( const RString& r)
//   Similar to ~RString followed by RString( r) returning a reference to *this.
//
// - RString& operator =( const char* s)
//   Similar to ~RString followed by RString( r) returning a reference to *this.
//
// - RString& operator +=( const RString& r)
//   Sets this to the Concatenation of *this and r returning a reference to *this.
//
// - const char* string() const
//   Returns a pointer to the internal stored string.
//   
// - operator const char*() const
//   Cast operator. Returns a pointer to the internal stored string.
//
// - char operator []( int i) const
//   Array operator. Returns the character at index i.
//
// Friend operators:
//
// - friend RString operator +( const RString& r1, const RString& r2)
//   Resulting RString holds the concatenation of r1 and r2.
//   
// - friend int operator ==( const RString& r1, const RString& r2)
// - friend int operator ==( const RString& r1, const char* s2)
// - friend int operator ==( const char* s1, const RString& r2)
//   Returns true if the arguments are equal. Case sensitive!
//   
// - friend int operator !=( const RString& r1, const RString& r2)
// - friend int operator !=( const RString& r1, const char* s2)
// - friend int operator !=( const char* s1, const RString& r2)
//   Returns true if the arguments are not equal. Case sensitive!
// 
// - friend int operator <( const RString&, const RString&)
// - friend int operator <( const char*, const RString&)
// - friend int operator <( const RString&, const char*)
//   Returns true if the first argument is lexicographically less
//   than the second one. Case sensitive!
//   
// - friend int operator >( const RString& s1, const RString& s2)
// - friend int operator >( const char* s1, const RString& s2)
// - friend int operator >( const RString& s1, const char* s2)
//   Returns true if the first argument is lexicographically greater
//   than the second one. Case sensitive!
//
// - friend ostream& operator <<( ostream& o, const RString& s)
//   Writes s.string() to ostream o.
// 
//   
// Description:
//
// 
//</class>

class RString {
 public:
     // constructors
     RString();
     RString( const char*);
     RString( const char*, int);
     RString( const RString&);
     // destructor
     ~RString();
     // operators
     RString& operator =( const RString&);
     RString& operator =( const char*);
     RString& operator +=( const RString&);
     operator const char*() const;
     const char* string() const;
     char operator []( int i) const;

  private:
     friend RString operator +( const RString&, const RString&);
     friend int operator ==( const RString&, const RString&);
     friend int operator ==( const RString&, const char*);
     friend int operator ==( const char*, const RString&);
     friend int operator !=( const RString&, const RString&);
     friend int operator !=( const RString&, const char*);
     friend int operator !=( const char*, const RString&);
     friend int operator <( const RString&, const RString&);
     friend int operator <( const char*, const RString&);
     friend int operator <( const RString&, const char*);

  public:
     int indexa( const RString& str) const; // index of first character after str in RString.
					    // 0 <= index <= len
     int indexa( const char* str) const;    // index == -1, str not found in RString
     int indexa( int index, const RString& str) const; // indexa starting at index
     int indexa( int index, const char* str) const;
     int index( char c) const;	// index of first occurence of character c in
				// RString. 0 <= index < len.
				// index == -1, c not found in RString
     int index( int index, char c) const;		  // index
							  // starting at index

     RString gSubstrDelim( int index, char delim) const;
     RString GetSubstrDelim( int index, char delim) const { return
							    gSubstrDelim( index, delim); } 
     RString gSubstrIndex( int begin, int end) const;	  // from index begin to end
     RString GetSubstrIndex( int begin, int end) const { return
							 gSubstrIndex(
								      begin, end); }
     RString gRight( int begin) const;			  // from begin to end of string
     RString GetRight( int begin) const { return gRight( begin); }

     boolean gWord( int& index, const RString& white, RString& word) const;
     boolean GetWord( int& index, const RString& white, RString& word) const { 
	  return gWord( index, white, word); }
     boolean gWordChar( int& index, const char white, RString& word) const;
     boolean GetWordChar( int& index, const char white, RString& word) const { 
	  return gWordChar( index, white, word); }
     RString& gLine( istream& s, char delim = '\n');
     RString& GetLine( istream& s) { return gLine( s); }
				
     int length() const { return rep_->len_; }

     RString& ignMultChars( char c);
     RString& subst( char from, char to);
     RString& subst( const RString& from, const RString& to, boolean caseInSens = true);
     RString& subst( const RString& from, const RString& to,
		    const RString& delimiters, boolean caseInSens = true);

     RString& invert();
     RString& tolower();
     RString& toupper();
     RString& sgml2Iso();
     RString& SGMLtoISO() { return sgml2Iso(); }
     RString& sgmlIso2LowerAscii();
     RString& SGMLISOtolowerASCII() { return sgmlIso2LowerAscii(); }
     RString& dos2Unix();
     RString& Dos2Unix() { return dos2Unix(); }
     RString& unix2Dos();
     RString& Unix2Dos() { return unix2Dos(); };

     static const RString lambda() { return lambda_; }
     static const RString& lambdaRef() { return lambda_; }
  protected:
     boolean gWordTable( int& index, const char white[], RString& word) const;
  private:
 friend class HgRegexp;
     RString( const RString&, const RString&);

     StringRep* rep_;

     static const RString lambda_;
};
     
inline int operator >( const RString& s1, const RString& s2) {return (s2 < s1);}
inline int operator >( const char* s1, const RString& s2) {return (s2 < s1);}
inline int operator >( const RString& s1, const char* s2) {return (s2 < s1);}


inline RString::operator const char*() const {
     return (const char*) rep_->str_;
}

inline const char* RString::string() const {
     return (const char*) rep_->str_;
}

inline char RString::operator []( int i) const {
     return rep_->str_[i];
}

inline ostream& operator <<( ostream& o, const RString& s) { return o << s.string(); }

#endif





