#include "AbstractPoseDetector.h"

AbstractPoseDetector::AbstractPoseDetector(UserDetector* userDetector)
{
        kn_userDetector = userDetector;

        kn_requiredPosingStability = 0;
        kn_posingTicks = 0;
}

AbstractPoseDetector::~AbstractPoseDetector()
{
}

bool AbstractPoseDetector::detect()
{
    float dt = kn_ticker.tick();

	if(!kn_userDetector->isPlayerVisible())
		return false;

    // TODO: should be time instead of frame rate
    if (isPosing(dt))
	{
        if (kn_posingTicks < kn_requiredPosingStability){
			kn_posingTicks++;
        }
        if (kn_posingTicks >= kn_requiredPosingStability){					//pose is detected
			onPoseDetected(dt);
			return true;
		}
	} 
	else
	{
		if (kn_posingTicks > 0) {
			kn_posingTicks--;
        }
    }
	return false;
}


// util function-----------------------------------------------------------------------------------------------
//check tay phai co lai. phai co lon hon minAngle (vi du co hon 90 do)
//-------------------------------------------------------------------------------------------------------------
bool AbstractPoseDetector::isRightArmBend(float minAngle)
{
	XnSkeletonJointPosition rightShoulder, rightElbow, rightHand;

    kn_userDetector->getSkeletonJointPosition(XN_SKEL_RIGHT_SHOULDER, &rightShoulder);
    kn_userDetector->getSkeletonJointPosition(XN_SKEL_RIGHT_ELBOW, &rightElbow);
    kn_userDetector->getSkeletonJointPosition(XN_SKEL_RIGHT_HAND, &rightHand);

    if (!isConfident(rightShoulder) || !isConfident(rightElbow) || !isConfident(rightHand)) {
		return false;
    }

	XV3 prShoulder(rightShoulder.position), prElbow(rightElbow.position), prHand(rightHand.position);
    XV3 vectorSE(prElbow - prShoulder), vectorEH(prHand - prElbow);
	
	if(vectorSE.dotNormalized(vectorEH) < cos(DEG_TO_RADIAN(minAngle)))
		return true;
	return false;
}

//---------------------------------------------------------------------------------------------------
//check tay duoi thang. Nhung ma tay khi duoi hop voi upvector 1 goc be hon maxAngleWithUpVector
//---------------------------------------------------------------------------------------------------
bool AbstractPoseDetector::isRightArmExtend(float minAngleExtend)
{
	XnSkeletonJointPosition rightShoulder, rightElbow, rightHand;

    kn_userDetector->getSkeletonJointPosition(XN_SKEL_RIGHT_SHOULDER, &rightShoulder);
    kn_userDetector->getSkeletonJointPosition(XN_SKEL_RIGHT_ELBOW, &rightElbow);
    kn_userDetector->getSkeletonJointPosition(XN_SKEL_RIGHT_HAND, &rightHand);

    if (!isConfident(rightShoulder) || !isConfident(rightElbow) || !isConfident(rightHand)) {
		return false;
    }

	XV3 prShoulder(rightShoulder.position), prElbow(rightElbow.position), prHand(rightHand.position);
    XV3 vectorSE(prElbow - prShoulder), vectorEH(prHand - prElbow);
	
	if(vectorSE.dotNormalized(vectorEH) > cos(DEG_TO_RADIAN(minAngleExtend)))
		return true;
	return false;
}