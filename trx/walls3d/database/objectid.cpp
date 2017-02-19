// **********************************************************
//  File objectid.h                                       
//  Modified:	14 Sep 94	Gerald Pani
//  		o ObjIdArray -> libHyperg
// ---------------------------------------------------------

#include "objectid.h"

// **********************************************************
// implements the treapclass for the keys

Treapimplement(StringKeyRefTree,RString,KeyRefPtr)
Treapimplement(LinkULarecTree,ObjectID,LarecOffset)
Treapimplement(OidUOidTree,ObjectID,UObjectID)
Treapimplement(OidUOidOffsTree,ObjectID,UObjIDOffset)
Treapimplement(KeyRefTree,ObjectID,KeyRefPtr)

// **********************************************************
// KeyRefPtr
//    remove the KeyRef kr from the list of KeyRefs

boolean KeyRefPtr::Remove(const KeyRef& kr) {

     // list is not empty and the first element is to be removed
     if (ref && ref->id() == kr.id()) {
	  KeyRef* help = ref;
	  ref = ref->nextdata;
	  delete help;
	  if (ref) {
	       _count--;
	       return true;
	  }
	  else 
	       return false;
     }
     for (KeyRef* ref1 = ref->nextdata, *ref2 = ref; ref1; ref1 = ref1->nextdata, ref2 = ref2->nextdata) {
	  if (ref1->id() == kr.id()) {
	       ref2->nextdata = ref1->nextdata;
	       delete ref1;
	       _count--;
	       break;
	  }
     }
     return true;
}

boolean KeyRefPtr::RemGet(KeyRef*& out) {
     // list is not empty and the first element is to be removed
     if (ref) {
	  out = ref;
	  ref = ref->nextdata;
	  _count--;
	  if (ref) {
	       return true;
	  }
	  else 
	       return false;
     }
     else {
	  out = nil;
	  return false;
     }
}
	       
boolean KeyRefPtr::RemGet(const KeyRef& kr, KeyRef*& out) {
     out = nil;
     // list is not empty and the first element is to be removed
     if (ref && ref->id() == kr.id()) {
	  out = ref;
	  ref = ref->nextdata;
	  _count--;
	  if (ref) {
	       return true;
	  }
	  else 
	       return false;
     }
     for (KeyRef* ref1 = ref->nextdata, *ref2 = ref; ref1; ref1 = ref1->nextdata, ref2 = ref2->nextdata) {
	  if (ref1->id() == kr.id()) {
	       ref2->nextdata = ref1->nextdata;
	       out = ref1;
	       _count--;
	       break;
	  }
     }
     return true;
}

KeyRefPtr KeyRefPtr::mergeDataList( const KeyRefPtr& data1, const KeyRefPtr& data2) {
     if (!data1.ref)
	  return data2;
     if (!data2.ref)
	  return data1;
     KeyRef* n, *i, *j, *out;
     int dcount = 0;
     if (data1.ref->ID > data2.ref->ID) {
	  n = out = data1.ref;
	  i = n->nextdata;
	  j = data2.ref;
     }
     else if (data2.ref->ID > data1.ref->ID) {
	  n = out = data2.ref;
	  j = n->nextdata;
	  i = data1.ref;
     }
     else {			// data1.ref->ID == data2.ref->ID
	  n = out = data1.ref;
	  i = n->nextdata;
	  j = data2.ref->nextdata;
	  delete data2.ref;
	  dcount++;
     }
	  
     for (; i && j;) {
	  if (i->ID > j->ID) {
	       n->nextdata = i;
	       n = i;
	       i = i->nextdata;
	  }
	  else if (j->ID > i->ID) {
	       n->nextdata = j;
	       n = j;
	       j = j->nextdata;
	  }
	  else {
	       n->nextdata = i;
	       n = i;
	       i = i->nextdata;
	       KeyRef* hilf = j;
	       j = j->nextdata;
	       delete hilf;
	       dcount++;
	  }

     }
     if (i) {
	  n->nextdata = i;
     }
     else {
	  n->nextdata = j;
     }
     KeyRefPtr ret;
     ret._count = data1._count + data2._count - dcount;
     ret.ref = out;
     return ret;
}

int KeyRefPtr::GetArray(ObjIdArray& out) const {
     if (_count) {
	  ObjectID* ids = new ObjectID[_count];
	  ObjectID* r = ids + _count - 1;
	  for (KeyRef* k = ref; k; k = k->nextdata) {
	       *r-- = k->id();
	  }
	  out.init(_count, ids);
     }
     else
	  out.free();
     return _count;
}

void KeyRefPtr::destroy() {
     for (KeyRef* k = ref; k; ) {
	  KeyRef* help = k;
	  k = k->nextdata;
	  delete help;
     }
     _count = 0;
     ref = nil;
}
