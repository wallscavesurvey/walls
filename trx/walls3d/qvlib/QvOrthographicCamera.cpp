#include "QvOrthographicCamera.h"
#include "QvSwitch.h"

QV_NODE_SOURCE(QvOrthographicCamera);

QvOrthographicCamera::QvOrthographicCamera()
{
    QV_NODE_CONSTRUCTOR(QvOrthographicCamera);
    isBuiltIn = TRUE;

    QV_NODE_ADD_FIELD(position);
    QV_NODE_ADD_FIELD(orientation);
    QV_NODE_ADD_FIELD(focalDistance);
    QV_NODE_ADD_FIELD(height);

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
    height.value = 2.0f;

    // mpichler, 19951024
    registered_ = 0;
    camswitch_ = 0;
}

QvOrthographicCamera::~QvOrthographicCamera()
{
}


// mpichler, 19951024
void QvOrthographicCamera::switchto ()
{
  if (camswitch_)
    camswitch_->whichChild.value = camswindex_;
}
