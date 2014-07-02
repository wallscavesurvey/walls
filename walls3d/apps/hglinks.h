//<copyright>
// 
// Copyright (c) 1993,94,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        hglinks.h
//
// Purpose:     several classes to handle link-lists of different link types
//
// Created:     07 Jan 95    Thomas Dietinger
//                           
//
// Modified:    28 Feb 95    Thomas Dietinger
//
//
//
// Description:
//
//
//</file>

#ifndef HG_APPS_HGLINKS_H
#define HG_APPS_HGLINKS_H

#include "../utils/list.h"
#include "../utils/str.h"

#if defined(__DISPATCHER__) || defined(__AMADEUS__)
//#include "../hgobject.h"
#else
#include "../pcutil/hgdummy.h"
#include "objreceiver.h"
#endif

extern StringKeyRefTree Seen;   // Links and Documents we have already seen

class Anchor;
class Document;
class BaseLinkNode;

enum ImageLinkType { RectType = 0, CircleType = 1, EllipseType = 2 };

/////////////////////////////////////////////////////////////////
// makes a linknode out of an anchor
//
extern BaseLinkNode* makeLinkNode(Anchor* a, Document* d);


//
// the basic (pure virtual) classes for links:
//

/////////////////////////////////////////////////////////////////
// Class BaseLinkData:
// (just a dummy to show how to define derived classes)

class BaseLinkData
{
public:
// every derived class should implement the following operators
// to maintain the right order within a link-list:

    virtual int operator <(BaseLinkData*) = 0;
    virtual int operator >(BaseLinkData*) = 0;
};


/////////////////////////////////////////////////////////////////
// Class BaseLinkNode: basic node for all link types
//

class BaseLinkNode : public DLListNode 
{
public:
    BaseLinkNode(Anchor* src);                              // define dest anchor later if any !
    BaseLinkNode(BaseLinkNode* node);
    ~BaseLinkNode();

    RString getSrcId()              { return srcId_; }
    RString getDestId()             { return destId_; }
    RString getLinkType()           { return linkType_; }   // linktype according to object
                                                            // attribute (e.g. inline)
    ObjectPtr getDestObj()          { return destObj_; };   // destination object if already accessed

    HGObjectEnum getType() const    { return docType_; }    // document type of link

    boolean alreadySeen();
    void markSeen();                                        // mark link as seen
    void updateStatus();

    virtual BaseLinkData* getLinkData() { return srcData_; };
    virtual BaseLinkData* getSrcData()  { return srcData_; };
    virtual BaseLinkData* getDestData() { return destData_; };

    virtual void setDestData(Anchor* dest) { destData_ = makeLinkData(dest); }

protected:

    virtual BaseLinkData* makeLinkData(Anchor* anch) { return 0; };

    RString srcId_;
    RString destId_;
    RString linkType_;
    HGObjectEnum docType_;
    boolean seen_;

    BaseLinkData* srcData_;
    BaseLinkData* destData_;

    ObjectPtr destObj_;
};

/////////////////////////////////////////////////////////////////
// Class BaseLinkList: 
//

DLListdeclare(TmpLinkList, BaseLinkNode)

class LinkList : public TmpLinkList 
{
public:
    BaseLinkNode* find(BaseLinkData* lData);
    virtual void insert(BaseLinkNode* n);
    virtual void insert(Anchor* anchor, Document* doc);

    void updateLinksStatus();
};


//
// the classes for text links:
//

/////////////////////////////////////////////////////////////////
// Class TextLinkData:
//

class TextLinkData : public BaseLinkData
{
public:
    TextLinkData(long startOffset = 0, long endOffset = 0) { startOffset_ = startOffset; endOffset_ = endOffset; };

    long startOffset_;
    long endOffset_;

    virtual int operator <(BaseLinkData*);
    virtual int operator >(BaseLinkData*);
};

/////////////////////////////////////////////////////////////////
// Class TextLinkNode: 
//

class TextLinkNode : public BaseLinkNode 
{
public:
    TextLinkNode(Anchor* a);
    TextLinkNode(BaseLinkNode* node);

protected:
    virtual BaseLinkData* makeLinkData(Anchor* anch);
};


//
// the classes for postscript links:
//

/////////////////////////////////////////////////////////////////
// Class PSLinkData:
//

class PSLinkData : public BaseLinkData
{
public:
    PSLinkData(int iVersion= 1,long p = 0, long x = 0, long y = 0, long w = 0, long h = 0) 
    { iVersion_=iVersion; p_ = p; x_ = x; y_ = y; w_ = w; h_ = h; };
    
    int iVersion_;  // PostScript link version
    long p_;        // postscript page
    long x_;        // upper
    long y_;        // left
    long w_;        // width
    long h_;        // height of link rectangle

    virtual int operator <(BaseLinkData*);
    virtual int operator >(BaseLinkData*);
};

/////////////////////////////////////////////////////////////////
// Class PSLinkNode: 
//

class PSLinkNode : public BaseLinkNode 
{
public:
    PSLinkNode(Anchor* a);
    PSLinkNode(BaseLinkNode* n);

protected:
    virtual BaseLinkData* makeLinkData(Anchor* anch);
};


//
// the classes for image links:
//

/////////////////////////////////////////////////////////////////
// Class ImageLinkData:
//

class ImageLinkData : public BaseLinkData
{
public:
    ImageLinkData(ImageLinkType type = RectType, float x = 0.0, float y = 0.0, 
                    float rw = 0.0, float h = 0.0) 
                    { type_ = type; x_ = x; y_ = y; rw_ = rw; h_ = h; };
    
    ImageLinkType type_;    // type
    float x_;               // upper
    float y_;               // left
    float rw_;              // radius or width
    float h_;               // height of link rectangle

    virtual int operator <(BaseLinkData*);
    virtual int operator >(BaseLinkData*);
};

/////////////////////////////////////////////////////////////////
// Class TextLinkNode: 
//

class ImageLinkNode : public BaseLinkNode 
{
public:
    ImageLinkNode(Anchor* a);
    ImageLinkNode(BaseLinkNode* n);

protected:
    virtual BaseLinkData* makeLinkData(Anchor* anch);
};


#endif
