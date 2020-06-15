#include "QvPerspectiveCamera.h"
#include "QvSwitch.h"
#include "QvBasic.h"
#include <math.h>  /* mpichler, 19950616 */

QV_NODE_SOURCE(QvPerspectiveCamera);

QvPerspectiveCamera::QvPerspectiveCamera()
{
	QV_NODE_CONSTRUCTOR(QvPerspectiveCamera);
	isBuiltIn = TRUE;

	QV_NODE_ADD_FIELD(position);
	QV_NODE_ADD_FIELD(orientation);
	QV_NODE_ADD_FIELD(focalDistance);
	QV_NODE_ADD_FIELD(heightAngle);

	// mpichler, 19950713
	QV_NODE_ADD_FIELD(nearDistance);
	QV_NODE_ADD_FIELD(farDistance);
	nearDistance.value = -1.0f;
	farDistance.value = -1.0f;

	position.value[0] = 0.0f;
	position.value[1] = 0.0f;
	position.value[2] = 1.0f;
	orientation.axis[0] = 0.0f;
	orientation.axis[1] = 0.0f;
	orientation.axis[2] = 1.0f;
	orientation.angle = 0.0f;
	focalDistance.value = 5.0f;
	heightAngle.value = (float)M_PI_4; // 45 degrees

	// mpichler, 19951024
	registered_ = 0;
	camswitch_ = 0;
}

QvPerspectiveCamera::~QvPerspectiveCamera()
{
}

// mpichler, 19951024
void QvPerspectiveCamera::switchto()
{
	if (camswitch_)
		camswitch_->whichChild.value = camswindex_;
}
