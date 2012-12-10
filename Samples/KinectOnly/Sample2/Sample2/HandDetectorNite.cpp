#include "HandDetectorNite.h"

static HandDetectorNite* this_cast(void* that) {
        return static_cast<HandDetectorNite*>(that);
}
//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

// Callback for when the focus is in progress
void XN_CALLBACK_TYPE SessionProgress(const XnChar* strFocus, const XnPoint3D& ptFocusPoint, XnFloat fProgress, void* UserCxt)
{
	this_cast(UserCxt)->sessionProgressFunc(strFocus, ptFocusPoint, fProgress, UserCxt);
}
// callback for session start
void XN_CALLBACK_TYPE SessionStart(const XnPoint3D& ptFocusPoint, void* UserCxt)
{
	this_cast(UserCxt)->sessionStartFunc(ptFocusPoint, UserCxt);
}
// Callback for session end
void XN_CALLBACK_TYPE SessionEnd(void* UserCxt)
{
	this_cast(UserCxt)->sessionEndFunc(UserCxt);
}
// Callback for wave detection
void XN_CALLBACK_TYPE OnWaveCB(void* cxt)
{
	this_cast(cxt)->onWaveFunc(cxt);
}
// callback for a new position of any hand
void XN_CALLBACK_TYPE OnPointUpdate(const XnVHandPointContext* pContext, void* cxt)
{
	this_cast(cxt)->onPointUpdateFunc(pContext, cxt);
}

// callback for left swipe
void XN_CALLBACK_TYPE OnSwipeLeftCB(XnFloat fVelocity, XnFloat fAngle, void *pUserCxt)
{
	this_cast(pUserCxt)->onSwipeLeftFunc(fVelocity, fAngle, pUserCxt);
}

// callback for right swipe
void XN_CALLBACK_TYPE OnSwipeRightCB(XnFloat fVelocity, XnFloat fAngle, void *pUserCxt)
{
	this_cast(pUserCxt)->onSwipeRightFunc(fVelocity, fAngle, pUserCxt);
}

//================================================================================================
void HandDetectorNite::sessionProgressFunc(const XnChar *strFocus, const XnPoint3D &ptFocusPoint, XnFloat fProgress, void *UserCxt)
{
	printf("Session progress (%6.2f,%6.2f,%6.2f) - %6.2f [%s]\n", ptFocusPoint.X, ptFocusPoint.Y, ptFocusPoint.Z, fProgress,  strFocus);
}

void HandDetectorNite::sessionStartFunc(const XnPoint3D& ptFocusPoint, void* UserCxt)
{
	printf("Session started. Please wave (%6.2f,%6.2f,%6.2f)...\n", ptFocusPoint.X, ptFocusPoint.Y, ptFocusPoint.Z);
}

void HandDetectorNite::sessionEndFunc(void *UserCxt)
{
	printf("Session ended. Please perform focus gesture to start session\n");
}

void HandDetectorNite::onWaveFunc(void *cxt)
{
	printf("Wave!\n");
}

void HandDetectorNite::onPointUpdateFunc(const XnVHandPointContext* pContext, void* cxt)
{
	//printf("%d: (%f,%f,%f) [%f]\n", pContext->nID, pContext->ptPosition.X, pContext->ptPosition.Y, pContext->ptPosition.Z, pContext->fTime);
}

void HandDetectorNite::onSwipeLeftFunc(XnFloat fVelocity, XnFloat fAngle, void *pUserCxt)
{
	printf("Swipe Left\n");
	if(LeftSwipePointerFunc)
		LeftSwipePointerFunc();
}

void HandDetectorNite::onSwipeRightFunc(XnFloat fVelocity, XnFloat fAngle, void *pUserCxt)
{
	printf("Swipe Right\n");
	if(RightSwipePointerFunc)
		RightSwipePointerFunc();
}

//================================================================================================
HandDetectorNite::HandDetectorNite()
{
	this->pSessionGenerator = NULL;
	this->LeftSwipePointerFunc = NULL;
	this->RightSwipePointerFunc = NULL;
}

bool HandDetectorNite::Init(Context *pContext)
{
	XnStatus rc ;

	// Create the Session Manager
	pSessionGenerator = new XnVSessionManager();
	rc = ((XnVSessionManager*)pSessionGenerator)->Initialize(pContext, "Click", "RaiseHand");
	if (rc != XN_STATUS_OK)
	{
		printf("Session Manager couldn't initialize: %s\n", xnGetStatusString(rc));
		delete pSessionGenerator;
		return false;
	}

	// Initialization done. Start generating
	//context.StartGeneratingAll();

	// Register session callbacks
	pSessionGenerator->RegisterSession(this, &SessionStart, &SessionEnd, &SessionProgress);

	// init & register wave control
	waveDetector = new XnVWaveDetector();
	waveDetector->RegisterWave(this, OnWaveCB);
	waveDetector->RegisterPointUpdate(this, OnPointUpdate);
	pSessionGenerator->AddListener(waveDetector);

	swipeDetector = new XnVSwipeDetector();
	swipeDetector->RegisterSwipeLeft(this, OnSwipeLeftCB);
	swipeDetector->RegisterSwipeRight(this, OnSwipeRightCB);
	pSessionGenerator->AddListener(swipeDetector);

	printf("Please perform focus gesture to start session\n");
	printf("Hit any key to exit\n");

}

void HandDetectorNite::Update(Context *pContext)
{
	((XnVSessionManager*)pSessionGenerator)->Update(pContext);
}


void HandDetectorNite::Exit()
{
	if(waveDetector)
		delete waveDetector;
	if(swipeDetector)
		delete swipeDetector;
	if(pSessionGenerator)
		delete pSessionGenerator;
}
