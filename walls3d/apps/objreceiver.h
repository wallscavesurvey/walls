//<class>
//
// Name: ObjectReceiver
//
// Purpose: Class representing an object receiving data from the database
//
// Public Interface:
//
// Description:
//
//</class>

#ifndef HG_APPS_OBJRECEIVER_H
#define HG_APPS_OBJRECEIVER_H

#include "../stdafx.h"
#include "../utils/list.h"
#include "../utils/str.h"

class ObjectReceiver : public DLListNode  
{
public:
    enum Status 
    {
      RECOK,
      RECBREAK,
      RECERROR
    };

    virtual int blockReceived(void* block) { return 0; }
    virtual int objectReceived(void* object_data, Status status) 
    {
        return 0;
    }

    virtual int objectReceived(RString objFileName, Status status) 
    {
        return 0;
    }
};



DLListdeclare(ReceiverList, ObjectReceiver)


#endif
