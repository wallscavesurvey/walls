#ifndef  _QV_WWW_ANCHOR_
#define  _QV_WWW_ANCHOR_

#include "QvSFEnum.h"
#include "QvSFString.h"
#include "QvGroup.h"

#include "../ge3d/vectors.h"

class QvWWWAnchor : public QvGroup {

    QV_NODE_HEADER(QvWWWAnchor);

  public:

    enum Map {			// Map types:
	NONE,				// Leave URL name alone
	POINT				// Add object coords to URL name
    };

    // Fields
    QvSFString		name;		// URL name
    QvSFString		description;	// Useful description of scene
    QvSFEnum		map;		// How to map pick to URL name

    // URL of parent object
    QvString            parentURL_;     // mpichler, 19951103
    // hitpoint in object coordinates (for POINT map)
    point3D             hitpoint_;      // mpichler, 19951122
};

#endif /* _QV_WWW_ANCHOR_ */
