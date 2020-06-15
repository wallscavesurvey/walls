// This may look like C code, but it is really -*- C++ -*-

// **********************************************************
//  File objectid.h                                       
// ---------------------------------------------------------
//  Encapsulation of Object ID. Currently implemented as  
//  32-bit integer, but could be changed anytime.         
//                                                        
//  Created: 20-Aug-91 F. Kappe                           
//  Changed: 21-Jul-92 Gerald Pani
//  Modified:   14 Sep 94   Gerald Pani
//          o ObjIdArray -> libHyperg
// **********************************************************

#ifndef have_objectid_h
#define have_objectid_h

#ifdef __PC__
#include "../pcutil/pcstuff.h"
#endif

// #include <hyperg/utils/arrays.h>
#include "../utils/gentreap.h"
#include "../hyperg/object.h"

// **********************************************************
// class LARecord
// ----------------------------------------------------------
// LinkAnchorRecord
// consists of source-, destination- and textanchor
// **********************************************************

class LARecord {
public:
	LARecord() {}
	LARecord(const ObjectID s, const ObjectID d, const ObjectID t) : _src(s), _dst(d), _txt(t) {}
	LARecord(const LARecord& r) {
		_src = r._src; _dst = r._dst; _txt = r._txt;
	}

	ObjectID src() const;
	ObjectID dst() const;
	ObjectID txt() const;
	void src(long s) { _src = s; }
	void dst(long s) { _dst = s; }
	void txt(long s) { _txt = s; }
private:
	ObjectID _src;
	ObjectID _dst;
	ObjectID _txt;
};

inline ObjectID LARecord::src() const {
	return _src;
}

inline ObjectID LARecord::dst() const {
	return _dst;
}

inline ObjectID LARecord::txt() const {
	return _txt;
}

// **********************************************************
// class LarecOffset
// ----------------------------------------------------------
class LarecOffset : public LARecord {
public:
	LarecOffset()
		: LARecord() { }
	LarecOffset(const ObjectID s, const ObjectID d, const ObjectID t, long offs = 0)
		: LARecord(s, d, t), _offset(offs) { }
	LarecOffset(const LARecord& r, long offs = 0)
		: LARecord(r), _offset(offs) { }

	long offset() const;

private:
	friend class LinkULarecTree;
	static LarecOffset& mergeDataList(const LarecOffset&, const LarecOffset&);

	ObjectID _offset;
};

inline long LarecOffset::offset() const {
	return _offset.id();
}

inline LarecOffset& LarecOffset::mergeDataList(const LarecOffset&, const LarecOffset&) {
	cerr << "Fatal Error in Relation: two Link-Anchor-Records with identical LinkID!\n";
	exit(1);
	return *(new LarecOffset());
}

// **********************************************************
// class UObjectID
// ----------------------------------------------------------
// UniqueObjectID for TreapNameNode::Data (see gentreap.h).
// Unique means, that no other Data can be appended.
// **********************************************************
class UObjectID : public ObjectID {
public:
	UObjectID() : ObjectID() { }
	UObjectID(int id) : ObjectID(id) { }
	UObjectID(long id) : ObjectID(id) { }
	UObjectID(const char* s) : ObjectID(s) { }
	UObjectID(const ObjectID& oid) : ObjectID(oid) { } /* gpani */

protected:
	friend inline istream& operator >> (istream& s, UObjectID& uo);
private:
	friend class OidUOidTree;
	friend class RStringUOidTree;
	static UObjectID& mergeDataList(const UObjectID& a, const UObjectID& b) {
		cerr << "Fatal Error in UObjectID: two Objects with identical ObjectID!\n";
		cerr << "Offset a:" << a << " Offset b:" << b << endl;
		exit(1);
		return *(new UObjectID());
	};
};

inline istream& operator >>(istream& s, UObjectID& uo)
{
	s.setf(0, ios::basefield);
	s >> uo.ID;
	return s;
}

// **********************************************************
// class UObjIDOffset
// ----------------------------------------------------------
// UniqueObjIDOffset for TreapNameNode::Data (see gentreap.h).
// Unique means, that no other Data can be appended.
// **********************************************************
class UObjIDOffset : public UObjectID {
public:
	UObjIDOffset() : UObjectID(), _offset(0) { }
	UObjIDOffset(int id, long offs = 0) : UObjectID(id), _offset(offs) { }
	UObjIDOffset(long id, long offs = 0) : UObjectID(id), _offset(offs) { }
	UObjIDOffset(const char* s, long offs = 0) : UObjectID(s), _offset(offs) { }
	UObjIDOffset(const ObjectID& oid, long offs = 0) : UObjectID(oid), _offset(offs) { }

	long offset() const { return _offset.id(); }
	void offset(long offs) { _offset = offs; }
protected:
	friend inline istream& operator >> (istream& s, UObjIDOffset &uo);
private:
	friend class OidUOidOffsTree;
	static UObjIDOffset& mergeDataList(const UObjIDOffset&, const UObjIDOffset&) {
		cerr << "Fatal Error in UObjIDOffset: two Objects with identical ObjectID!\n";
		exit(1);
		return *(new UObjIDOffset());
	};

	ObjectID _offset;
};

inline istream& operator >>(istream& s, UObjIDOffset &uo)
{
	s.setf(0, ios::basefield);
	s >> uo.ID;
	return s;
}


// **********************************************************
// class FLARecord
// ----------------------------------------------------------
// FullLinkAnchorRecord
// Consists of link and LinkAnchorRecord
// **********************************************************
class FLARecord {
public:
	FLARecord() {}
	FLARecord(const ObjectID l, const LARecord r) : _link(l), _rest(r) {}

	ObjectID link() const { return _link; }
	void link(long s) { _link = s; }
	const LARecord& larecord() const { return _rest; }
	void larecord(LARecord& rec) { _rest = rec; }
private:
	ObjectID _link;
	LARecord _rest;
};

// **********************************************************
// class LinkULarecTree, class LinkULarecTreeNode
// ----------------------------------------------------------
// link ----> LarecOffset
// **********************************************************
Treapdeclare(LinkULarecTree, ObjectID, LarecOffset)

// **********************************************************
// class KeyRef
// ----------------------------------------------------------
// List for ObjectID's in a Treap
// **********************************************************
class KeyRef : public ObjectID {
public:
	KeyRef() : ObjectID() { nextdata = nil; }
	KeyRef(int id) : ObjectID(id) { nextdata = nil; }
	KeyRef(const char* s) : ObjectID(s) { nextdata = nil; }
	KeyRef(const ObjectID& oid) : ObjectID(oid) { nextdata = nil; }

	ObjectID id() const { return *this; }
protected:
	KeyRef* nextdata;
	friend class KeyRefPtr;
};

// **********************************************************
// class KeyRefOffs
// ----------------------------------------------------------
// List for ObjectID's in a Treap
// **********************************************************
class KeyRefOffs : public KeyRef {
public:
	KeyRefOffs() : KeyRef(), _offset(0) { nextdata = nil; }
	KeyRefOffs(int id, long offs = 0) : KeyRef(id), _offset(offs) { nextdata = nil; }
	KeyRefOffs(const char* s, long offs = 0) : KeyRef(s), _offset(offs) { nextdata = nil; }
	KeyRefOffs(const ObjectID& oid, long offs = 0) : KeyRef(oid), _offset(offs) { nextdata = nil; }

	long   offset()    const { return _offset.id(); }
	void   offset(long offs) { _offset = offs; }
protected:
	friend class KeyRefPtr;
	ObjectID _offset;
};

// **********************************************************
// class KeyRefPtr
// ----------------------------------------------------------
// Used if only a pointer is stored in a Treap
// **********************************************************
class KeyRefPtr {
public:
	KeyRefPtr() { ref = nil; _count = 0; }
	KeyRefPtr(ObjectID id) { ref = new KeyRef(id); _count = 1; }
	KeyRefPtr(ObjectID id, long offs) { ref = new KeyRefOffs(id, offs); _count = 1; }
	//     KeyRefPtr (KeyRef* r) {ref = r;}

	boolean Remove(const KeyRef&);
	boolean RemGet(const KeyRef&, KeyRef*& out);
	boolean RemGet(KeyRef*& out);
	void destroy();
	static KeyRefPtr mergeDataList(const KeyRefPtr& data1, const KeyRefPtr& data2);
	int GetArray(ObjIdArray&) const;
	int count() const { return _count; }

protected:
	KeyRef* ref;
	int _count;
};

// **********************************************************
// class OidUOidTree, OidUOidTreeNode
// ----------------------------------------------------------
// ObjectID -----> offset
// **********************************************************
Treapdeclare(OidUOidTree, ObjectID, UObjectID)


// **********************************************************
// class OidUOidOffsTree, OidUOidOffsTreeNode
// ----------------------------------------------------------
// ObjectID -----> ObjIDOffset
// **********************************************************
Treapdeclare(OidUOidOffsTree, ObjectID, UObjIDOffset)


// **********************************************************
// class KeyRefTree, KeyRefTreeNode
// ----------------------------------------------------------
// ObjectID ----> ObjectID -> ... -> ObjectID
// **********************************************************
Treapdeclare(KeyRefTree, ObjectID, KeyRefPtr)


// **********************************************************
// class StringKeyRefTree, StringKeyRefTreeNode
// ----------------------------------------------------------
// String ----> ObjectID -> ... -> ObjectID
// **********************************************************
Treapdeclare(StringKeyRefTree, RString, KeyRefPtr)

#endif
