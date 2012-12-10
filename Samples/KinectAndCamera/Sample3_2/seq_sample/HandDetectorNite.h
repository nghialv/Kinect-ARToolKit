#ifndef _HANDDETECTORNITE_H_
#define _HANDDETECTORNITE_H_

#include "Common.h"

// NITE headers
#include <XnVSessionManager.h>
#include "XnVMultiProcessFlowClient.h"
#include <XnVWaveDetector.h>
#include <XnVSwipeDetector.h>

class HandDetectorNite
{
public:
	XnVSessionGenerator*	pSessionGenerator;
	XnVWaveDetector*		waveDetector;
	XnVSwipeDetector*		swipeDetector;

	HandDetectorNite();
	~HandDetectorNite();

	bool Init(Context *pContext);
	void Update(Context *pContext);
	void Exit();

	void (*LeftSwipePointerFunc)();
	void (*RightSwipePointerFunc)();

	void sessionProgressFunc(const XnChar* strFocus, const XnPoint3D& ptFocusPoint, XnFloat fProgress, void* UserCxt);
	void sessionStartFunc(const XnPoint3D& ptFocusPoint, void* UserCxt);
	void sessionEndFunc(void* UserCxt);

	void onSwipeLeftFunc(XnFloat fVelocity, XnFloat fAngle, void *pUserCxt);
	void onSwipeRightFunc(XnFloat fVelocity, XnFloat fAngle, void *pUserCxt);
};

#endif