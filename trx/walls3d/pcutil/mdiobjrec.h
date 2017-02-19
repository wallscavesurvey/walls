//<class>
//
// Name: MDIObjReceiver
//
// Purpose: Class representing an MDI object receiving data from the database
//
// Public Interface:
//
// Description:
//
//</class>


#ifndef HG_PCUTIL_MDIOBJREC_H
#define HG_PCUTIL_MDIOBJREC_H


#include "../apps/hglinks.h"
//#include <pcutil/mfcutil.h>

struct CMDIObjectInfo;

class MDIObjReceiver : public ObjectReceiver  
{
public:
    MDIObjReceiver(ObjectPtr docPtr, 
                   ObjectPtr destAnch = ObjectPtr())
    {
        docPtr_ = docPtr;
        destAnch_ = destAnch;
        viewerInfo_ = 0;
    }

    CMDIObjectInfo* getMDIObjInfo()  { return viewerInfo_; }

protected:
    ObjectPtr docPtr_;
    ObjectPtr destAnch_;            // destination anchor
    CMDIObjectInfo* viewerInfo_;    // internal viewer information 
};

#endif