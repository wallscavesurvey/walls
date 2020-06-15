#ifndef  _QV_TEXTURE_2_
#define  _QV_TEXTURE_2_

#include "QvSFEnum.h"
#include "QvSFImage.h"
#include "QvSFString.h"
#include "QvSubNode.h"

class QvTexture2 : public QvNode {

	QV_NODE_HEADER(QvTexture2);

public:
	enum Wrap {			// Texture wrap type
		REPEAT,				// Repeats texture outside 0-1
						//  texture coordinate range
						CLAMP				// Clamps texture coordinates
										//  to lie within 0-1 range
	};

	// Fields.
	QvSFString		filename;	// file to read texture from
	QvSFImage		image;		// The texture
	QvSFEnum		wrapS;
	QvSFEnum		wrapT;

	virtual QvBool	readInstance(QvInput *in);
	QvBool		readImage();

	// mpichler, 19950905
	int handle_;  // for ge3d
	static void(*freeTexture_) (int);
	int reps_, rept_;  // s/t repeat flags, mpichler, 19960216
};

#endif /* _QV_TEXTURE_2_ */
