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

void XN_CALLBACK_TYPE HandCreate(HandsGenerator& generator,
								 XnUserID nId,
								 const XnPoint3D* position,
								 XnFloat fTime,
								 void *pCookie)
{
	printf("New Hand\n");
}

void XN_CALLBACK_TYPE HandUpdate(HandsGenerator& generator,
								 XnUserID nId,
								 const XnPoint3D* position,
								 XnFloat fTime,
								 void *pCookie)
{
	this_cast(pCookie)->updateHandPosition(generator,
											nId,
											*position,
											fTime);
}

void XN_CALLBACK_TYPE HandDestroy(HandsGenerator& generator,
								 XnUserID nId,
								 XnFloat fTime,
								 void *pCookie)
{
	printf("Lost Hand : %d\n", nId);
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
	handsGenerator.StartTracking(*pEndPosition);
}

void HandDetectorOpenNI::onGestureProcess(GestureGenerator& generator,
										  const XnChar* strGesture,
										  const XnPoint3D* pPosition,
										  XnFloat fProgress)
{
}

void HandDetectorOpenNI::updateHandPosition(HandsGenerator& generator,
								 XnUserID nId,
								 XnPoint3D pos,
								 XnFloat fTime)
{
	printf("Hand Position %d: %.2f, %.2f, %.2f\n",nId, pos.X, pos.Y, pos.Z);
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
	handsGenerator.Create(context);
	gestureGenerator.RegisterGestureCallbacks(GestureRecognized,
												GestureProcess,
												this,
												handle);
	handsGenerator.RegisterHandCallbacks(HandCreate,
										HandUpdate,
										HandDestroy,
										this,
										handle2);
}

//-----------------------------------------------------------------------------
// AddGesture
//-----------------------------------------------------------------------------
void HandDetectorOpenNI::AddGesture(char *gestureName, XnBoundingBox3D *pArea)
{
	gestureGenerator.AddGesture(gestureName, pArea);
}