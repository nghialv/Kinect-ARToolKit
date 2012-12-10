#ifndef _USER_DETECTOR_H_
#define _USER_DETECTOR_H_

#include "Common.h"
#include "PlayerStatus.h"
#include "MyVector.h"

class UserDetector
{
private:
	PlayerStatus	*kn_playerStatus;
    UserGenerator	*kn_userGen;
    XnChar			kn_pose[20];								// pose name for calibration, or null if posing is not required
	XnBool			kn_needPose;

    XnCallbackHandle kn_hUserCallbacks;
    XnCallbackHandle kn_hCalibrationCallbacks;
    XnCallbackHandle kn_hPoseCallbacks;

	XnSkeletonJointPosition kn_skeletonPositions[24];			//takai seido no tameni
public:
    UserDetector(UserGenerator* userGen, PlayerStatus *playerStatus);
	~UserDetector(){}

	bool isPlayerVisible(){ return kn_playerStatus->isPlayerVisible();}
	XnUserID getPlayerID(){ return kn_playerStatus->getPlayerID();}

	XV3 getForwardVector();		//returns the vector to the direction of player
	XV3 getUpVector();			//returns the vector to the up of player

	XV3 getSkeletonJointPosition(XnSkeletonJoint jointID);
    void getSkeletonJointPosition(XnSkeletonJoint eJoint, XnSkeletonJointPosition* pJointPosition);

    void onNewUser(XnUserID userID);
    void onLostUser(XnUserID userID);
    void onCalibrationStart(XnUserID userID);
    void onCalibrationEnd(XnUserID userID, bool isSuccess);
    void onPoseDetected(XnUserID userID, const XnChar* pose);
};

#endif

