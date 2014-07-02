/*
 * File :       object.C
 *
 * Purpose :    Implementation of classes Object and ObjectID
 *
 * Created :    16 Jul 92    Keith Andrews, IICM
 *
 * Modified :    1 Sep 92    Gerald Pani, IICM
 * Modified :   14 Sep 94    Gerald Pani, IICM
 *		     o ObjIdArray
 *
 */

//be sure to NOT include several Windows-header files !!
#define WIN32_LEAN_AND_MEAN

#include "object.h"

#include <stdio.h>

Arraysimplement(RStringArray,RString)
Arraysimplement(ObjIdArray,ObjectID)
Fieldsimplement(ObjIdField,ObjectID)
Fieldsimplement(ObjectField,Object)

//
// class ObjectID
//

ObjectID::ObjectID( const char* s )
{
    ID = 0;
    if (s)
    {
	    if (!sscanf( s, "0x%x", &ID ))
            sscanf( s, "%ld", &ID );
    }
}

ObjectID::ObjectID( const RString& s )
{
    ID = 0;
    if (!sscanf( s.string(), "0x%x", &ID ))
        sscanf( s.string(), "%ld", &ID );
}

ObjectID& ObjectID::operator =( const char* s ) {
    ID = 0;
    if (s)
    {
	    if (!sscanf( s, "0x%x", &ID ))
            sscanf( s, "%ld", &ID );
    }
    return *this;
}

ObjectID& ObjectID::operator =( const RString& s ) {
    ID = 0;
    if (!sscanf( s.string(), "0x%x", &ID ))
        sscanf( s.string(), "%ld", &ID );
    return *this;
}

RString ObjectID::IDString() const
{
     char buf[11];
     sprintf( buf, "0x%08x", ID ) ;
     return RString( buf ) ;
}

istream& operator >> ( istream& s, ObjectID &id )
{
  s.setf(0,ios::basefield) ;
  return s >> id.ID ;
  // return s >> resetiosflags(ios::basefield) >> id.ID ;
}

ostream& operator << ( ostream& s, const ObjectID &id )
{
     char buf[10];
     register char *buf_ptr = buf+10; // End of buf.
     
     // Now do the actual conversion, placing the result at the *end* of buf.
     // Note that we use separate code for decimal, octal, and hex,
     // so we can divide by optimizable constants.
     char *xdigs = "0123456789abcdef";
     register unsigned long val = id.ID;
     do {
	  *--buf_ptr = xdigs[val & 15];
	  val = val >> 4;
     } while (val != 0);
     
     while (buf_ptr > buf +2)
	  *--buf_ptr = '0';
     buf[0] = '0';
     buf[1] = 'x';
     
     s.write( buf, 10);
     return s;
}


// 
// class Object
//
boolean Object::ID( ObjectID& i ) const
{
  int tindex = indexa( rsObjectIDEq ) ;
  if ( tindex != -1 ) {
    RString tmp = gSubstrDelim( tindex, '\n');
    ObjectID onlyForGnu_HaHaHa( tmp);
    i = onlyForGnu_HaHaHa;
    return true ;
  }
  i = 0L ;
  return false ;
}


boolean Object::GOid( ObjectID& sid, ObjectID& oid ) const
{
  int tindex = indexa( rsGOidEq ) ;
  if ( tindex != -1 ) {
    RString tmp = gSubstrDelim( tindex, '\n');
    int i = 0 ;
    RString id1 ;
    RString id2 ;
    if (! gWord (i, " ", id1))
       return false ;
    if (! gWord (i, "\n", id2))
       return false ;

    ObjectID gnu1 (id1) ;
    ObjectID gnu2 (id2) ;

    sid = gnu1 ;
    oid = gnu2 ;

    return true ;
  }
  sid = 0L ;
  oid = 0L ;
  return false ;
}

/*
int Object::fields(const RString& fieldEq, RStringArray& values) const {
     int ndx = 0;
     int len = fieldEq.length();
     for(ndx = indexa(fieldEq); 
	 ndx != -1 && (ndx == len || operator[](ndx - len - 1) == '\n'); 
	 ndx = indexa( ndx, fieldEq)
	 ) {
	  int end = index(ndx, '\n');
	  if (end != -1) {
	       values.insert( gSubstrIndex( ndx, end - 1));
	       ndx = end + 1;
	  }
	  else 
	       break;
     } 
     return values.count();
}
*/

int Object::fields(const RString& fieldEq, RStringArray& values) const {
     int ndx = 0;
     int len = fieldEq.length();
     for(ndx = indexa(fieldEq); 
	 ndx != -1; 
	 ndx = indexa( ndx, fieldEq)
	 ) {
	  if (ndx == len || operator[](ndx - len - 1) == '\n')
	       values.insert( gSubstrDelim( ndx, '\n'));
	  int end = index(ndx, '\n');
	  if (end != -1)
	       ndx = end + 1;
	  else 
	       break;
     }
     return values.count();
}


boolean Object::field( const RString& fieldEq, RString& value) const {
     int ndx = 0;
     int len = fieldEq.length();
     for(ndx = indexa(fieldEq); 
	 ndx != -1; 
	 ndx = indexa( ndx, fieldEq)
	 ) {
	  if (ndx == len || operator[](ndx - len - 1) == '\n') {
	       value = gSubstrDelim( ndx, '\n');
	       return true;
	  }
	  int end = index(ndx, '\n');
	  if (end != -1)
	       ndx = end + 1;
	  else 
	       break;
     }
     return false;
}


////////////////////////////////////////////////////////////////////

boolean DbObject::GetField(int& ndx, RString& name, RString& value) const {
     RString line;
     if (gWordChar(ndx, '\n', line)) {
	  int i;
	  if ((i = line.index('=')) != -1) {
	       name = line.gSubstrIndex(0, i-1);
	       value = line.gRight(i+1);
	       return true;
	  }
     }
     return false;
}
