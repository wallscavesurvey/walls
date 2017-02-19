#ifndef  _QV_ASCII_TEXT_
#define  _QV_ASCII_TEXT_

#include "QvMFFloat.h"
#include "QvMFString.h"
#include "QvSFEnum.h"
#include "QvSFFloat.h"
#include "QvSubNode.h"

class QvAsciiText : public QvNode {

    QV_NODE_HEADER(QvAsciiText);

  public:

    enum Justification {	// Text justification:
	LEFT,				// Align left edge of text to origin
	CENTER,				// Align center of text to origin
	RIGHT				// Align right edge of text to origin
    };

    // Fields
    QvMFString		string;		// Text string
    QvSFFloat		spacing;	// Inter-string spacing
    QvSFEnum		justification;	// Text justification
    QvMFFloat		width;		// Suggested width constraint
};

#endif /* _QV_ASCII_TEXT_ */
