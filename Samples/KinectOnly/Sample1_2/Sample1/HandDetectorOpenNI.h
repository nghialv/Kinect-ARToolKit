#ifndef _HANDDETECTOROPENNI_H_
#define _HANDDETECTOROPENNI_H_

#include "Common.h"

class HandDetectorOpenNI
{
public:
	GestureGenerator	gestureGenerator;
	HandsGenerator		handsGenerator;

	XnCallbackHandle	handle;
	XnCallbackHandle	handle2;

	HandDetectorOpenNI();
	~HandDetectorOpenNI(){}

	void Init(Context &context);
	void AddGesture(char *gestureName, XnBoundingBox3D *pArea);

	void (*ClickPointerFunc)();

	void updateHandPosition(HandsGenerator& generator,
								XnUserID nId,
								XnPoint3D pos,
								XnFloat fTime);

	void onGestureRecognized(GestureGenerator& generator,
							const XnChar* strGesture,					//example: Click
							const XnPoint3D* pIDPosition,
							const XnPoint3D* pEndPosition);
	void onGestureProcess(GestureGenerator& generator,
							const XnChar* strGesture,
							const XnPoint3D* pPosition,
							XnFloat fProgress);
};

#endif