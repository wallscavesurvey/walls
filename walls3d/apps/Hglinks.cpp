//<copyright>
// 
// Copyright (c) 1993,94, 95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:        hglinks.cpp
//
// Purpose:     classes to store and handle links
//              
//
// Created:     07 Jan 95    Thomas Dietinger
//
// Modified:    28 Feb 95    Thomas Dietinger
//
//
//
// Description:
//
//
//</file>

#include "../stdafx.h"

#include "hglinks.h"
#include "treaps.h"

//StringKeyRefTree Seen;      // Links and Documents we have already seen

const char* invisibleLink = "invisible"; 


/////////////////////////////////////////////////////////////////
// makes a linknode out of an anchor
//
BaseLinkNode* makeLinkNode(Anchor* a, Document* d)
{
/*
    if (!a->LinkType().length() || a->LinkType() == "inline")
        return new TextLinkNode(a);
    else
    if (a->LinkType() == "PostScript")
        return new PSLinkNode(a);
*/
    BaseLinkNode* n = 0;

    switch(d->getType())
    {
        case TextDocumentType       : n = new TextLinkNode(a); break;
        case PostScriptDocumentType : n = new PSLinkNode(a); break;
        case ImageDocumentType      : n = new ImageLinkNode(a); break;

        default : n = 0;
    }

    return n;
}


/////////////////////////////////////////////////////////////////
// Class BaseLinkNode: basic node for all link types
//
BaseLinkNode::BaseLinkNode(Anchor* a) : DLListNode()
{
    docType_  = HGObjectType;
    seen_     = 0;

    if (a)
    {
        srcId_    = a->IDString();
        destId_   = a->Dest();
        if (!destId_.length())
            destId_ = a->Hint().gRight(4);  // not a Hyper-G link, but a URL!
        linkType_ = a->LinkType();
        if (linkType_ == "intag")
        {
            if (Object(a->Info()).field("TagAttr=") == "SRC")
                linkType_ = "inline";
        }

        UObjectPtr destUObj;
        HGObject::cachedObjects_.find(destId_, destUObj);
        destObj_ = destUObj;
        if (destObj_.Obj())
            seen_ = destObj_.Obj()->seen();
//        else
//        KeyRefPtr dummy;
//        seen_     = Seen.find( destId_, dummy);
    }

    srcData_ = destData_ = 0;
}

BaseLinkNode::BaseLinkNode(BaseLinkNode* node) : DLListNode()
{
    if (node)
    {
        srcId_ = node->srcId_;
        destId_ = node->destId_;
        linkType_ = node->linkType_;
        docType_ = node->docType_;
        seen_ = node->seen_;
        destObj_ = node->destObj_;
    }
    else
        seen_     = 0;

    srcData_ = 0;
    destData_ = 0;
}

BaseLinkNode::~BaseLinkNode()
{
    delete srcData_;
    delete destData_;
}

void BaseLinkNode::markSeen()
{
    seen_ = 1;
    if (destObj_.Obj())
        destObj_.Obj()->markSeen();
}

void BaseLinkNode::updateStatus()
{
/*
    KeyRefPtr dummy;
    seen_     = Seen.find( destId_, dummy);
*/
    UObjectPtr destUObj;
    HGObject::cachedObjects_.find(destId_, destUObj);
    destObj_ = destUObj;
    if (destObj_.Obj())
        seen_ = destObj_.Obj()->seen();
}


boolean BaseLinkNode::alreadySeen()
{
    if (seen_)              // required if we manually want to mark link as seen!
        return seen_;       // (pre-marking)

    if (destObj_.Obj())
        return destObj_.Obj()->seen();

    UObjectPtr destUObj;
    HGObject::cachedObjects_.find(destId_, destUObj);
    destObj_ = destUObj;
    if (destObj_.Obj())
        return destObj_.Obj()->seen();
    else
        return 0;
}


/////////////////////////////////////////////////////////////////
// Class LinkList: 
//

BaseLinkNode* LinkList::find(BaseLinkData* lData) 
{
    // find first node with offset >= this offset
	BaseLinkNode* n = getFirst();
    for (; n && n->getLinkData()->operator <(lData); n = getNext(n)) ;
    return n;
}

void LinkList::insert(BaseLinkNode* n) 
{
    if (!n)
        return;

    // insert at (AFTER) correct position so that ordering remains intact
    BaseLinkNode* t = find(n->getLinkData());
    if (t) 
    {
        for (; t && t->getLinkData()->operator <(n->getLinkData()); 
             t = getNext(t))
        {
            if (n->getLinkData() == t->getLinkData() &&
                n->getDestId() == t->getDestId())
                return; // same link already included!
        }

        if (t)
            insertBefore(n, t);
    }
    else
        addTail( n);
}


void LinkList::insert(Anchor* anchor, Document* doc)
{
    if (anchor->IsSrc())
        insert(makeLinkNode(anchor, doc));
    else    // we have a destination anchor !
    {
        // find out whether we have a source anchor which points to this destination anchor !
        for (BaseLinkNode* n = getFirst(); n; n = getNext(n))
            if (n && n->getDestId() == anchor->IDString()) // ok, we found a source anchor that fits !
                n->setDestData(anchor);
    }
}


void LinkList::updateLinksStatus()
{
    for (BaseLinkNode* n = getFirst(); n; n = getNext(n))
        n->updateStatus();
}


/////////////////////////////////////////////////////////////////
// Class TextLinkData:
//

int TextLinkData::operator <(BaseLinkData* d)
{
    return startOffset_ < ((TextLinkData*)d)->startOffset_ ? 1 : 0;
}

int TextLinkData::operator >(BaseLinkData* d)
{
    return startOffset_ > ((TextLinkData*)d)->startOffset_ ? 1 : 0;
}


/////////////////////////////////////////////////////////////////
// Class TextLinkNode: 
//

TextLinkNode::TextLinkNode(Anchor* src) : BaseLinkNode(src)
{
    docType_ = TextDocumentType;
    srcData_ = makeLinkData(src);
}


TextLinkNode::TextLinkNode(BaseLinkNode* node) : BaseLinkNode(node)
{
    if (docType_ == TextDocumentType)
    {
        if (node->getSrcData())
            srcData_ = new TextLinkData(((TextLinkData*)node->getSrcData())->startOffset_, 
                                        ((TextLinkData*)node->getSrcData())->endOffset_);

        if (node->getDestData())
            destData_ = new TextLinkData(((TextLinkData*)node->getDestData())->startOffset_, 
                                         ((TextLinkData*)node->getDestData())->endOffset_);
    }
}

BaseLinkData* TextLinkNode::makeLinkData(Anchor* anch)
{
    RString pos = anch->Position();
    long startpos = 0;
    long endpos = 0;

    if (pos != invisibleLink)
        if (sscanf(pos.string(), "%ld %ld", &startpos, &endpos) != 2)
            sscanf(pos.string(), "0x%lx 0x%lx", &startpos, &endpos);

    BaseLinkData* lData = new TextLinkData(startpos, endpos);

    if (lData)
        TRACE("TextLinkNode::TextLinkNode: source: %s, dest.: %s, type: %s, offset:%d-%d, seen:%d\n", 
              (const char*)srcId_, (const char*)destId_, (const char*)linkType_, 
              ((TextLinkData*)lData)->startOffset_, ((TextLinkData*)lData)->endOffset_, (int)alreadySeen());

    return lData;
}

/////////////////////////////////////////////////////////////////
// Class PSLinkData:
//

int PSLinkData::operator <(BaseLinkData* d)
{
    if (p_ < ((PSLinkData*)d)->p_)  // check page !
        return 1;
    else
    if (p_ > ((PSLinkData*)d)->p_)
        return 0;
    else
    if (x_ < ((PSLinkData*)d)->x_)  // same page, so check x !
        return 1;
    else
    if (x_ > ((PSLinkData*)d)->x_)
        return 0;
    else
    if (y_ < ((PSLinkData*)d)->y_)  // same x, so check y !
        return 1;
    else
    if (y_ > ((PSLinkData*)d)->y_)
        return 0;
    else
    if (w_ < ((PSLinkData*)d)->w_)  // same y, so check w !
        return 1;
    else
    if (w_ > ((PSLinkData*)d)->w_)
        return 0;
    else
    if (h_ < ((PSLinkData*)d)->h_)  // same w, so check h !
        return 1;
    else
        return 0;                   // equal or less, so take less (we can't express eq.)
}

int PSLinkData::operator >(BaseLinkData* d)
{
    return PSLinkData::operator <(d) ? 0 : 1;
}


/////////////////////////////////////////////////////////////////
// Class TextLinkNode: 
//

PSLinkNode::PSLinkNode(Anchor* a) : BaseLinkNode(a)
{
    docType_ = PostScriptDocumentType;
    srcData_ = makeLinkData(a);
}

PSLinkNode::PSLinkNode(BaseLinkNode* node) : BaseLinkNode(node)
{
    if (docType_ == PostScriptDocumentType)
    {
        if (node->getSrcData())
            srcData_ = new PSLinkData(((PSLinkData*)node->getSrcData())->p_, 
                                      ((PSLinkData*)node->getSrcData())->x_, 
                                      ((PSLinkData*)node->getSrcData())->y_, 
                                      ((PSLinkData*)node->getSrcData())->w_, 
                                      ((PSLinkData*)node->getSrcData())->h_);

        if (node->getDestData())
            destData_ = new PSLinkData(((PSLinkData*)node->getDestData())->p_, 
                                       ((PSLinkData*)node->getDestData())->x_, 
                                       ((PSLinkData*)node->getDestData())->y_, 
                                       ((PSLinkData*)node->getDestData())->w_, 
                                       ((PSLinkData*)node->getDestData())->h_);
    }
}

BaseLinkData* PSLinkNode::makeLinkData(Anchor* a)
{
    long p = 0;         // postscript page
    long x = 0;         // upper
    long y = 0;         // left
    long w = 0;         // width
    long h = 0;         // height of link rectangle
    int ret = 0;
    int iVersion=1;

    // is the link version 2.0?
    if(!strncmp((const char*)a->Position(),"V20@",4))
    {
      // V2.0
      iVersion=2;
      if(a->Position() != invisibleLink)
        ret=sscanf(((const char*)a->Position())+4, "%d:%d,%d,%d,%d", &p, &x, &y, &w, &h);
    }
    else
      // V1.0
      if (a->Position() != invisibleLink)
        ret = sscanf((const char*)a->Position(), "%d:%d,%d,%d,%d", &p, &x, &y, &w, &h);



    BaseLinkData* lData = new PSLinkData(iVersion,p, x, y, w, h);
    if (ret == 5)
    {
        TRACE("PSLinkNode::PSLinkNode: source: %s, dest.: %s, type: %s, version:%d.0 position:%d:%d,%d,%d,%d, seen:%d\n", 
              (const char*)srcId_, (const char*)destId_, (const char*)linkType_, 
              ((PSLinkData*)lData)->iVersion_,
              ((PSLinkData*)lData)->p_, ((PSLinkData*)lData)->x_, ((PSLinkData*)lData)->y_, 
              ((PSLinkData*)lData)->w_, ((PSLinkData*)lData)->h_, (int)alreadySeen());
        return lData;
    }
    else
    {
        TRACE("Error: PSLinkNode::PSLinkNode: source: %s, dest.: %s, type: %s, position:%s, seen:%d\n", 
              (const char*)srcId_, (const char*)destId_, (const char*)linkType_, (const char*)a->Position(), (int)alreadySeen());
        return lData;
    }
}

/////////////////////////////////////////////////////////////////
// Class ImageLinkData:
//

int ImageLinkData::operator <(BaseLinkData* d)
{
    float myX, yourX, myY, yourY;
    if (type_ == CircleType)
    {
        myX = x_ - rw_;
        myY = y_ + rw_;;
    }
    else
    {
        myX = x_;
        myY = y_;
    }

    if (((ImageLinkData*)d)->type_ == CircleType)
    {
        yourX = ((ImageLinkData*)d)->x_ + ((ImageLinkData*)d)->rw_;
        yourY = ((ImageLinkData*)d)->y_ - ((ImageLinkData*)d)->rw_;
    }
    else
    {
        yourX = ((ImageLinkData*)d)->x_;
        yourY = ((ImageLinkData*)d)->y_;
    }

    if (myX < yourX)   // check x !
        return 1;
    else
    if (myX > yourX)
        return 0;
    else
    if (myY < yourY)   // same x, so check y !
        return 1;
    else
    if (myY > yourY)
        return 0;
    else
    if (rw_ < ((ImageLinkData*)d)->rw_) // same y, so check rw !
        return 1;
    else
    if (rw_ > ((ImageLinkData*)d)->rw_)
        return 0;
    else
    if (h_ < ((ImageLinkData*)d)->h_)   // same rw, so check h !
        return 1;
    else
        return 0;                       // equal or less, so take less (we can't express eq.)
}

int ImageLinkData::operator >(BaseLinkData* d)
{
    return ImageLinkData::operator <(d) ? 0 : 1;
}


/////////////////////////////////////////////////////////////////
// Class TextLinkNode: 
//

ImageLinkNode::ImageLinkNode(Anchor* a) : BaseLinkNode(a)
{
    docType_ = ImageDocumentType;
    srcData_ = makeLinkData(a);
}

ImageLinkNode::ImageLinkNode(BaseLinkNode* node) : BaseLinkNode(node)
{
    if (docType_ == ImageDocumentType)
    {
        if (node->getSrcData())
            srcData_ = new ImageLinkData(((ImageLinkData*)node->getSrcData())->type_, 
                                      ((ImageLinkData*)node->getSrcData())->x_, 
                                      ((ImageLinkData*)node->getSrcData())->y_, 
                                      ((ImageLinkData*)node->getSrcData())->rw_, 
                                      ((ImageLinkData*)node->getSrcData())->h_);

        if (node->getDestData())
            destData_ = new ImageLinkData(((ImageLinkData*)node->getDestData())->type_, 
                                       ((ImageLinkData*)node->getDestData())->x_, 
                                       ((ImageLinkData*)node->getDestData())->y_, 
                                       ((ImageLinkData*)node->getDestData())->rw_, 
                                       ((ImageLinkData*)node->getDestData())->h_);
    }
}

BaseLinkData* ImageLinkNode::makeLinkData(Anchor* a)
{
    // default dummy values for invisible link !
    float x  = 0.0f;
    float y  = 0.0f;
    float rw = 0.0f;
    float h  = 0.0f;
    ImageLinkType type = RectType;
    RString pos = a->Position();

    if (pos != invisibleLink)
    {
        RString typeStr = pos.gSubstrDelim(0, ' ');
        pos = pos.gRight(pos.index(' ')+1);
        if (typeStr == "Circle")
        {
            type = CircleType;
            sscanf((const char*)pos, "%f %f %f", &x, &y, &rw);
        }
        else
        {
            if (typeStr == "Ellipse")
                type = EllipseType;
            else
                type = RectType;
            sscanf((const char*)pos, "%f %f %f %f", &x, &y, &rw, &h);
        }
    }

    BaseLinkData* lData = new ImageLinkData(type, x, y, rw, h);
    TRACE("ImageLinkNode::ImageLinkNode: source: %s, dest.: %s, type: %s, position:%d %f,%f,%f,%f, seen:%d\n", 
          (const char*)srcId_, (const char*)destId_, (const char*)linkType_, 
          ((ImageLinkData*)lData)->type_, ((ImageLinkData*)lData)->x_, ((ImageLinkData*)lData)->y_, 
          ((ImageLinkData*)lData)->rw_, ((ImageLinkData*)lData)->h_, (int)alreadySeen());
    return lData;
}

