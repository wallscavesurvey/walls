#ifndef HG_PCUTIL_HGDUMMY_H
#define HG_PCUTIL_HGDUMMY_H

#ifdef __AMADEUS__
#error please include the REAL HGObject classes !
#endif

#include "../utils/types.h"
#include "../utils/str.h"
#include "../hyperg/object.h"
#include "../database/objectid.h"

enum HGObjectEnum { 
                    HGObjectType, AnchorType, DocumentType, 
                    CollectionType, ClusterType, TextDocumentType,
                    PICDocumentType, ImageDocumentType, SoundDocumentType, 
                    MovieDocumentType, PostScriptDocumentType, SceneDocumentType,
                    GenericDocumentType, RemoteDocumentType, WWWDocumentType,
                    GopherDocumentType, GopherTextType, GopherImageType, 
                    GopherSoundType, GopherCollectionType, GopherIndexType,
                    WaisDocumentType, WaisSourceType, WaisTextType,
                    WaisImageType, TelnetSessionType, FTPDocumentType,
                    FakeObjectType 
                  };

class LinkList;
class ObjPtrUStringTree;
class ObjectReceiver;
class ReceiverList;

/////////////////////////////////////////////////////////////////////////////
//
class HGObject
{
public:
    static ObjPtrUStringTree cachedObjects_;    // list of already accessed objects

    HGObject(const Object& o);
    virtual ~HGObject()                 { }

    void addReceiver(ObjectReceiver* node) {};     // add an object receiver to list
    void removeReceiver(ObjectReceiver* node) {};  // remove an object receiver from list
    ReceiverList* getReceivers()        { return 0; }

    ObjectID ID() const                 { return _id; }
    const RString& Type() const         { return _type; }
    HGObjectEnum   getType() const      { return objType_; }

    virtual RString IDString() const    { return _id.IDString(); }
    virtual RString Info();

    virtual boolean seen()              { return seen_; }
    virtual void markSeen()             { seen_ = 1; }

    virtual boolean valid()             { return valid_; }
    virtual void invalidate()           { valid_ = 0; }

    virtual boolean existAnnotations()  { return 0; }

protected:
    HGObjectEnum objType_;
    Object origObject_;

private:
    ObjectID _id;
    boolean seen_;          // already seen?
    boolean valid_;         // object still valid (or did some data change since last usage)?
    RString _type;
};


/////////////////////////////////////////////////////////////////////////////
//
class Anchor : public HGObject 
{
public:
    Anchor(const Object& o);

    const RString& Position() const          { return _position; }
    const RString Dest() const               { return _dest; }
    const RString& Hint() const              { return _hint; }
    const RString& LinkType() const          { return _linktype; }
    const boolean  IsSrc() const             { return _issrc; }

protected:
    RString _position;
    RString _dest;
    RString _hint;
    RString _linktype;
    boolean _issrc;
};


/////////////////////////////////////////////////////////////////////////////
//
class Document : public HGObject
{
public:
    Document(const Object& );
    ~Document();

    const RString& DocType() const  { return _doctype; }
    LinkList* linkList()            { return linkList_; }

private:
    RString _doctype;
    LinkList* linkList_;
};


/////////////////////////////////////////////////////////////////////////////
//
class TextDocument : public Document
{
public:
    TextDocument(const Object& o) : Document(o) { objType_ = TextDocumentType; }
};


/////////////////////////////////////////////////////////////////////////////
//
class ImageDocument : public Document
{
public:
    ImageDocument(const Object& o) : Document(o) { objType_ = ImageDocumentType; }
};


/////////////////////////////////////////////////////////////////////////////
//
class PostScriptDocument : public Document
{
public:
    PostScriptDocument(const Object& o) : Document(o) { objType_ = PostScriptDocumentType; }
};


/////////////////////////////////////////////////////////////////////////////
//
class ObjectPtr     // dummy class for stand-alone-viewer
{
public:
    ObjectPtr(HGObject* o = nil)    { _obj = o; }
    ~ObjectPtr()                    { delete _obj; }

    HGObject* Obj() const           { return _obj; }

protected:
    HGObject* _obj;
}; 


#endif
