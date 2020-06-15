// This may look like C code, but it is really -*- C++ -*-

// **********************************************************
// File treaps.h
// ----------------------------------------------------------
// Created: F. Kappe
// **********************************************************

#ifndef treaps_h
#define treaps_h

#include "../utils/str.h"
#include "../utils/gentreap.h"
#include "../database/objectid.h"

#if defined(__DISPATCHER__) || defined(__AMADEUS__)
//#include "../hgobject.h"
#else
#include "../pcutil/hgdummy.h"
#endif

// **********************************************************
// class URString
// ----------------------------------------------------------
// Unique RString for TreapNameNode::Data (see gentreap.h).
// Unique means, that no other Data can be appended.
// **********************************************************
class URString : public RString
{
public:
	URString() : RString() { }
	URString(const char* s) : RString(s) { }
	URString(const char* s, int len) : RString(s, len) { }
	URString(const RString& s) : RString(s) { }

private:
	friend class OidUStringTree;
	friend class ObjPtrUStringTree;
	static URString& mergeDataList(const URString& a, const URString& b)
	{
		//        cerr << "Fatal Error in URString: two Objects with identical ObjectID!\n";
		//        cerr << "String a:" << a << " String b:" << b << endl;
		exit(1);
		return *(new URString());
	};
};


// **********************************************************
// class OidUStringTree, OidUStringTreeNode
// ----------------------------------------------------------
// ObjectID ----> URString
// **********************************************************
Treapdeclare(OidUStringTree, ObjectID, URString)

// **********************************************************
// class RStringUOidTree, RStringUOidNode
// ----------------------------------------------------------
// RString -> UObjectID
// **********************************************************
Treapdeclare(RStringUOidTree, RString, UObjectID)


/////////////////////////////////////////////////////////////////////////////
// class FlushAction : used for copying a OidUStringTree 

class FlushAction : public OidUStringTreeAction
{
public:
	FlushAction(OidUStringTree* treap) : OidUStringTreeAction(),
		treap_(treap) {}
	OidUStringTree* getTreap() { return treap_; }

protected:
	void actionMiddle(const ObjectID& key, URString& data) {
		treap_->insert(key, data);
	}
	void actionBefore(const ObjectID&, URString&) {}
	void actionAfter(const ObjectID&, URString&) {}

	OidUStringTree* treap_;
};


/////////////////////////////////////////////////////////////////////////////
// 

class UObjectPtr : public ObjectPtr
{
public:
	UObjectPtr(HGObject* o = nil) : ObjectPtr(o) {};
	UObjectPtr(const ObjectPtr& ptr) : ObjectPtr(ptr) {};

private:
	friend class ObjPtrUStringTree;
	static UObjectPtr& mergeDataList(const UObjectPtr& a, const UObjectPtr& b)
	{
		return *(new UObjectPtr());
	};
};

Treapdeclare(ObjPtrUStringTree, RString, UObjectPtr)

/////////////////////////////////////////////////////////////////////////////
// class InvalidateAction : used for invalidating a whole ObjPtrUStringTree 

class InvalidateAction : public ObjPtrUStringTreeAction
{
public:
	InvalidateAction(ObjPtrUStringTree* treap) : ObjPtrUStringTreeAction(),
		treap_(treap) {}
	ObjPtrUStringTree* getTreap() { return treap_; }

protected:
	void actionMiddle(const RString& key, UObjectPtr& data)
	{
		if (data.Obj())
			data.Obj()->invalidate();
	}
	void actionBefore(const RString&, UObjectPtr&) {}
	void actionAfter(const RString&, UObjectPtr&) {}

	ObjPtrUStringTree* treap_;
};


#endif
