#include "../stdafx.h"
#include "hgdummy.h"
#include "../apps/hglinks.h"
#include "../apps/treaps.h"

// HGObject statics:
// ~~~~~~~~~~~~~~~~~
ObjPtrUStringTree HGObject::cachedObjects_;    // list of already accessed objects

/////////////////////////////////////////////////////////////////////////////
//

HGObject::HGObject(const Object& o)
{
	objType_ = HGObjectType;
	_id = o.ID();
	_type = o.type();
	origObject_ = o;    // store original object information at a safe place
}

RString HGObject::Info()
{
	return origObject_;
}

/////////////////////////////////////////////////////////////////
// Class Anchor:

Anchor::Anchor(const Object& o) : HGObject(o)
{
	objType_ = AnchorType;
	_position = o.position();
	_dest = o.field("Dest=");
	_hint = o.field("Hint=");
	_linktype = o.field("LinkType=");
	_issrc = (o.field("TAnchor=") == "Src");
}


/////////////////////////////////////////////////////////////////
// Class Document; abstract:

Document::Document(const Object& o) : HGObject(o)
{
	objType_ = DocumentType;
	_doctype = o.documenttype();
	linkList_ = new LinkList();
}

Document::~Document()
{
	delete linkList_;
}
