/*
 * File :       object.h
 *
 * Purpose :    Interface to classes Object and ObjectID
 *
 * Created :    16 Jul 92    Keith Andrews, IICM
 *
 * Modified :    Mar 18 1993    Gerald Pani, IICM
 *      class GlobOid
 *
 * Modified :    Aug 23 1993    Gerald Pani, IICM
 *      title -> anyTitle, titles (new)
 * Modified :   14 Sep 94    Gerald Pani
 *      o ObjIdArray
 *
 */


#ifndef object_h
#define object_h

#include "../utils/arrays.h"
#include "../utils/fields.h"
#include "objconst.h"

Arraysdeclare(RStringArray,RString)

//
// class ObjectID
//
// Encapsulation of Object ID.
// Currently implemented as 32-bit integer, but could be changed anytime.
//

class ObjectID
{
protected:
    long ID ;

public:
    ObjectID()                        { ID = 0 ; }
    ObjectID( int id )                { ID = id ; }
    ObjectID( long id )               { ID = id ; }
    ObjectID( const char* s ) ;
    ObjectID( const RString& s ) ;
    ObjectID( const ObjectID& oid )   { ID = oid.ID ; }

    ObjectID& operator =( long s ) { ID = s; return *this;};
    ObjectID& operator =( const char* s );
    ObjectID& operator =( const RString& s );
    ObjectID& operator =( const ObjectID& s );

    int operator == ( const ObjectID i ) const { return (ID == i.ID) ; }
    int operator != ( const ObjectID i ) const { return (ID!= i.ID) ; }
    int operator <= ( const ObjectID i ) const { return (ID <= i.ID) ; }
    int operator <  ( const ObjectID i ) const { return (ID <  i.ID) ; }
    int operator >= ( const ObjectID i ) const { return (ID>= i.ID) ; }
    int operator >  ( const ObjectID i ) const { return (ID>  i.ID) ; }

    long id() const { return ID ; }
    RString IDString() const ;

protected:
    friend istream& operator >> ( istream& s, ObjectID &id ) ;
    friend ostream& operator << ( ostream& s, const ObjectID &id ) ;
} ;

inline ObjectID& ObjectID::operator =( const ObjectID& s ) {
    ID = s.ID;
    return *this;
}


Arraysdeclare(ObjIdArray,ObjectID)
Fieldsdeclare(ObjIdField,ObjectID)


//
// class Object
//
// Hyper-G Objects, e.g. documents, anchors, etc.
//

class Object : public RString 
{
public:

    Object() : RString() {}
    Object( const char* s ) : RString( s ) {}
    Object( const char* s, int l ) : RString( s, l ) {}
    Object( const RString& r ) : RString( r ) {}
    const Object& operator =( const RString& o) { RString::operator =( o); return *this;}
    const Object& operator =( const char* o) { RString::operator =( o); return *this;}

    const Object& operator =( const Object& o) { RString::operator =( o); return *this;}

    boolean field( const RString& fieldEq, RString& value ) const ;
    RString field( const RString& fieldEq ) const ;

    int fields(const RString& fieldEq, RStringArray& values) const;


    boolean ID( ObjectID& i ) const ;
    ObjectID ID() const ;

    boolean GOid( ObjectID& sid, ObjectID& oid ) const ;

    boolean anyTitle( RString& t) const;
    RString anyTitle() const;
    int titles( RStringArray& values) const { return fields( rsTitleEq, values); }

    boolean hint( RString& t ) const ;
    RString hint() const ;

    boolean anyKeyword( RString& t) const;           // returns 1st keyword (if more than 1)
    RString anyKeyword() const;
    int keywords( RStringArray& values) const { return fields( rsKeywordEq, values); }

    boolean type( RString& t ) const ;
    RString type() const ;

    boolean documenttype( RString& t ) const ;
    RString documenttype() const ;

    boolean collectiontype( RString& t ) const ;
    RString collectiontype() const ;

    boolean name( RString& t ) const ;
    RString name() const ;

    boolean author( RString& t ) const ;
    RString author() const ;

    boolean created( RString& t ) const ;
    RString created() const ;

    boolean modified( RString& t ) const ;
    RString modified() const ;

    boolean open( RString& t ) const ;
    RString open() const ;

    boolean expire( RString& t ) const ;
    RString expire() const ;

    boolean path( RString& t ) const ;
    RString path() const ;

    boolean position( RString& t ) const ;
    RString position() const ;

    boolean protocol( RString& t ) const ;
    RString protocol() const ;

    boolean host( RString& t ) const ;
    RString host() const ;

    boolean port( RString& t ) const ;
    RString port() const ;

    boolean gophertype( RString& t ) const ;
    RString gophertype() const ;

};


inline RString Object::field(                       // get value of field
  const RString& name
) const
{
    RString ret ;
    field( name, ret ) ;                              // ignore return status
    return ret ;
}

inline ObjectID Object::ID() const
{
    ObjectID i ;
    ID( i ) ;
    return i ;
}

inline boolean Object::protocol( RString& t ) const
{
    return field( rsProtocolEq, t ) ;
}

inline RString Object::protocol() const
{
    return field( rsProtocolEq ) ;
}

inline boolean Object::host( RString& t ) const
{
    return field( rsHostEq, t ) ;
}

inline RString Object::host() const
{
    return field( rsHostEq ) ;
}

inline boolean Object::port( RString& t ) const
{
    return field( rsPortEq, t ) ;
}

inline RString Object::port() const
{
    return field( rsPortEq ) ;
}

inline boolean Object::gophertype( RString& t ) const
{
    return field( rsGopherTypeEq, t ) ;
}

inline RString Object::gophertype() const
{
    return field( rsGopherTypeEq ) ;
}

inline boolean Object::position( RString& t ) const
{
    return field( rsPositionEq, t ) ;
}

inline RString Object::position() const
{
    return field( rsPositionEq ) ;
}

inline boolean Object::path( RString& t ) const
{
    return field( rsPathEq, t ) ;
}

inline RString Object::path() const
{
    return field( rsPathEq ) ;
}

inline boolean Object::name( RString& t ) const
{
    return field( rsNameEq, t ) ;
}

inline RString Object::name() const
{
    return field( rsNameEq ) ;
}

inline boolean Object::author( RString& t ) const
{
    return field( rsAuthorEq, t ) ;
}

inline RString Object::author() const
{
    return field( rsAuthorEq ) ;
}

inline boolean Object::created( RString& t) const
{
    return field( rsTimeCreatedEq, t ) ;
}

inline RString Object::created() const {
    return field( rsTimeCreatedEq ) ;
}

inline boolean Object::modified( RString& t) const
{
    return field( rsTimeModifiedEq, t ) ;
}

inline RString Object::modified() const {
    return field( rsTimeModifiedEq ) ;
}

inline boolean Object::open( RString& t) const
{
    return field( rsTimeOpenEq, t ) ;
}

inline RString Object::open() const
{
    return field( rsTimeOpenEq ) ;
}

inline boolean Object::expire( RString& t ) const
{
    return field( rsTimeExpireEq, t ) ;
}

inline RString Object::expire() const
{
    return field( rsTimeExpireEq ) ;
}

inline boolean Object::documenttype( RString& t ) const
{
    return field( rsDocumentTypeEq, t ) ;
}

inline RString Object::documenttype() const
{
    return field( rsDocumentTypeEq ) ;
}

inline boolean Object::collectiontype( RString& t) const 
{
    return field( rsCollectionTypeEq, t ) ;
}

inline RString Object::collectiontype() const
{
    return field( rsCollectionTypeEq ) ;
}

inline boolean Object::type( RString& t ) const
{
    return field( rsTypeEq, t ) ;
}

inline RString Object::type() const
{
    return field( rsTypeEq ) ;
}

inline boolean Object::anyTitle( RString& t) const
{
    return field( rsTitleEq, t ) ;
}

inline RString Object::anyTitle() const
{
    return field( rsTitleEq ) ;
}

inline boolean Object::hint( RString& t ) const
{
    return field( rsHintEq, t ) ;
}

inline RString Object::hint() const
{
    return field( rsHintEq ) ;
}

inline boolean Object::anyKeyword( RString& t) const
{
    return field( rsKeywordEq, t ) ;
}

inline RString Object::anyKeyword() const
{
    return field( rsKeywordEq ) ;
}


Fieldsdeclare(ObjectField,Object)



class DbObject : public Object 
{
public:
    DbObject() : Object() {}
    DbObject( const char* s ) : Object( s ) {}
    DbObject( const char* s, int l ) : Object( s, l ) {}
    DbObject( const RString& r ) : Object( r ) {}
    DbObject( const Object& r ) : Object( r ) {}
    DbObject& operator =( const RString& r) { Object::operator =( r); return *this;}
    DbObject& operator =( const char* s) { Object::operator =( s); return *this;}
    boolean GetField(int& ndx, RString& name, RString& value) const;
};

#endif
