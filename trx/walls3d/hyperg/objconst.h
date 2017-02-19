// This may look like C code, but it is really -*- C++ -*-

/*
 * File :       objconst.h
 *
 * Purpose :    RString constants for Objects
 *
 * Created :    22 Jul 92    Keith Andrews, IICM
 *
 * Modified :    1 Sep 92    Gerald Pani, IICM
 *
 */


#ifndef objconst_h
#define objconst_h

#include "../utils/str.h"

//
// Hyper-G Objects
//
// Objects in Hyper-G have both a C++ class representation
// and an ASCII string representation.
// This file contains the ASCII string representation of
// the various Hyper-G objects.
//
//
// All objects must have the following two fields:
//
// ObjectID=0xnnnnnnnn                              // unique id in hex
// Type= Anchor | Document | Link                   // object type
//


extern const RString rsObjectIDEq ;                 // "ObjectID="
extern const RString rsGOidEq ;                     // "GOid="
extern const RString rsTypeEq ;                     // "Type="

// Type field values
extern const RString rsLink ;                       // "Link"
extern const RString rsAnchor ;                     // "Anchor"
extern const RString rsDocument ;                   // "Document"





//
// Anchors
//

//
// ObjectID=0xnnnnnnnn                              // unique id in hex
// Type=Anchor                                      // object type
// Position=
// [ Title= ]
// [ TimeCreated= ]                                 // e.g. 92/07/03 12:01:02
// [ TimeOpen= ]
// [ TimeExpire= ]
// { Keyword=[key key ...] }
//



// general field identifiers

extern const RString rsAuthorEq ;                 // "Author="
extern const RString rsTitleEq ;                  // "Title="
extern const RString rsTimeCreatedEq ;            // "TimeCreated="
extern const RString rsTimeModifiedEq ;           // "TimeModified="
extern const RString rsTimeOpenEq ;               // "TimeOpen="
extern const RString rsTimeExpireEq ;             // "TimeExpire="
extern const RString rsKeywordEq ;                // "Keyword="

extern const RString rsTitlePt ;                  // "Title."
extern const RString rsEngEq ;                      // "eng="
extern const RString rsGerEq ;                      // "ger="


// extra fields for Anchors

extern const RString rsPositionEq ;               // "Position="







// extra fields for Links

extern const RString rsHintEq ;                   // "Hint="



// extra fields for Documents

extern const RString rsPathEq ;                   // "Path="
extern const RString rsDocumentTypeEq ;           // "DocumentType="



// DocumentType field values

extern const RString rsCollection ;                 // "collection"


// extra fields for collections

extern const RString rsNameEq ;                   // "Name="
extern const RString rsCollectionTypeEq ;         // "CollectionType="


// CollectionType field values

extern const RString rsCluster ;                    // "Cluster"




// extra fields for Frank !!

extern const RString rsProtocolEq ;               // "Protocol="
extern const RString rsHostEq ;                   // "Host="
extern const RString rsPortEq ;                   // "Port="
extern const RString rsGopherTypeEq ;             // "GopherType="



#endif
