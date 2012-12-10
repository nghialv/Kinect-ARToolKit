#include "HandDetectorOpenNI.h"

static HandDetectorOpenNI* this_cast(void* that) {
        return static_cast<HandDetectorOpenNI*>(that);
}

//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------
void XN_CALLBACK_TYPE GestureRecognized(GestureGenerator& generator,
										const XnChar* strGesture,					//example: Click
										const XnPoint3D* pIDPosition,
										const XnPoint3D* pEndPosition,
										void* pCookie)
{
	printf("Gesture recognized : %s\n", strGesture);
	this_cast(pCookie)->onGestureRecognized(generator, strGesture, pIDPosition, pEndPosition);
}

void XN_CALLBACK_TYPE GestureProcess(GestureGenerator& generator,
									 const XnChar* strGesture,
									 const XnPoint3D* pPosition,
									 XnFloat fProgress,
									 void* pCookie)
{
	this_cast(pCookie)->onGestureProcess(generator, strGesture, pPosition, fProgress);
}

//-----------------------------------------------------------------------------
// HandDetectorOpenNI functions
//-----------------------------------------------------------------------------
void HandDetectorOpenNI::onGestureRecognized(xn::GestureGenerator &generator,
											 const XnChar *strGesture,
											 const XnPoint3D *pIDPosition,
											 const XnPoint3D *pEndPosition)
{
	printf("HandDetectorOpenNI: Gesture recognized: %s\n", strGesture);
	if(ClickPointerFunc)
		ClickPointerFunc();
}

void HandDetectorOpenNI::onGestureProcess(GestureGenerator& generator,
										  const XnChar* strGesture,
										  const XnPoint3D* pPosition,
										  XnFloat fProgress)
{
}


HandDetectorOpenNI::HandDetectorOpenNI()
{
	ClickPointerFunc = NULL;
}
//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------
void HandDetectorOpenNI::Init(xn::Context &context)
{
	gestureGenerator.Create(context);
	gestureGenerator.RegisterGestureCallbacks(GestureRecognized,
												GestureProcess,
												this,
												handle);
}

//-----------------------------------------------------------------------------
// AddGesture
//-----------------------------------------------------------------------------
void HandDetectorOpenNI::AddGesture(char *gestureName, XnBoundingBox3D *pArea)
{
	gestureGenerator.AddGesture(gestureName, pArea);
}