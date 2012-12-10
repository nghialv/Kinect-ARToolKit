#include "UserDetector.h"
#include "Util.h"

static UserDetector* this_cast(void* that) {
        return static_cast<UserDetector*>(that);
}

static void XN_CALLBACK_TYPE s_onNewUser(UserGenerator& userGen, XnUserID userID, void* that)
{
        this_cast(that)->onNewUser(userID);
}

static void XN_CALLBACK_TYPE s_onLostUser(UserGenerator& userGen, XnUserID userID, void* that)
{
        this_cast(that)->onLostUser(userID);
}

static void XN_CALLBACK_TYPE s_onCalibrationStart(SkeletonCapability& capability, XnUserID userID, void* that)
{
        this_cast(that)->onCalibrationStart(userID);
}

static void XN_CALLBACK_TYPE s_onCalibrationEnd(SkeletonCapability& capability, XnUserID userID, XnBool isSuccess, void* that)
{
        this_cast(that)->onCalibrationEnd(userID, i2b(isSuccess));
}

static void XN_CALLBACK_TYPE s_onPoseDetected(PoseDetectionCapability& capability, const XnChar* pose, XnUserID userID, void* that)
{
        this_cast(that)->onPoseDetected(userID, pose);
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
UserDetector::UserDetector(UserGenerator* userGen, PlayerStatus *playerStatus)
{
	kn_userGen = userGen;
	kn_playerStatus = playerStatus;
	kn_needPose = FALSE;

	if (!kn_userGen->IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
		exit(1);
	}
	kn_userGen->RegisterUserCallbacks(s_onNewUser, s_onLostUser, this, kn_hUserCallbacks);
	kn_userGen->GetSkeletonCap().RegisterCalibrationCallbacks(s_onCalibrationStart, s_onCalibrationEnd, this, kn_hCalibrationCallbacks);

	if (kn_userGen->GetSkeletonCap().NeedPoseForCalibration())
	{
		kn_needPose = TRUE;
		if (!kn_userGen->IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
			exit(0);
		}
		kn_userGen->GetPoseDetectionCap().RegisterToPoseCallbacks(s_onPoseDetected, NULL, this, kn_hPoseCallbacks);
		kn_userGen->GetSkeletonCap().GetCalibrationPose(kn_pose);
	}

	kn_userGen->GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void UserDetector::onNewUser(XnUserID userID)
{
	printf("New User %d\n", userID);
	// New user found
	//check if player is not visible
	if(!kn_playerStatus->isPlayerVisible())										//nghia
	{
		if (kn_needPose)
		{
			kn_userGen->GetPoseDetectionCap().StartPoseDetection(kn_pose, userID);
		}
		else
		{
			kn_userGen->GetSkeletonCap().RequestCalibration(userID, TRUE);
		}
	}
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void UserDetector::onLostUser(XnUserID userID)
{
	printf("Lost user %d\n", userID);
	if(kn_playerStatus->isPlayerVisible() && (kn_playerStatus->getPlayerID() == userID))		//when player out
	{
		kn_playerStatus->setPlayerNotVisible();
	}
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void UserDetector::onCalibrationStart(XnUserID userID)
{
	printf("Calibration started for user %d\n", userID);
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void UserDetector::onCalibrationEnd(XnUserID userID, bool isSuccess)
{
	if (isSuccess)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", userID);
		kn_userGen->GetSkeletonCap().StartTracking(userID);
		kn_playerStatus->setPlayerID(userID);										//set player is visible now
		kn_playerStatus->setPlayerVisible();
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", userID);
		if (kn_needPose)
		{
			kn_userGen->GetPoseDetectionCap().StartPoseDetection(kn_pose, userID);
		}
		else
		{
			kn_userGen->GetSkeletonCap().RequestCalibration(userID, TRUE);
		}
	}
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void UserDetector::onPoseDetected(XnUserID userID, const XnChar* pose)
{
	printf("Pose %s detected for user %d\n", pose, userID);
	kn_userGen->GetPoseDetectionCap().StopPoseDetection(userID);
	kn_userGen->GetSkeletonCap().RequestCalibration(userID, TRUE);
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
XV3 UserDetector::getSkeletonJointPosition(XnSkeletonJoint eJoint)
{
    XnSkeletonJointPosition j;
    getSkeletonJointPosition(eJoint, &j);

    if (isConfident(j)) {
            kn_skeletonPositions[eJoint-1] = j;
    }
	return  kn_skeletonPositions[eJoint-1].position;
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
void UserDetector::getSkeletonJointPosition(XnSkeletonJoint eJoint, XnSkeletonJointPosition* pJointPosition)
{
	if (kn_playerStatus->isPlayerVisible()){
            kn_userGen->GetSkeletonCap().GetSkeletonJointPosition(getPlayerID(), eJoint, *pJointPosition);
    } else {
            pJointPosition->fConfidence = 0;
    }
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
XV3 UserDetector::getForwardVector()
{
	if (kn_playerStatus->isPlayerVisible())
	{
        // OpenNI's orientation does not work very well.
        /*
        XnSkeletonJointTransformation t;
        m_userGen->GetSkeletonCap().GetSkeletonJoint(userID, XN_SKEL_TORSO, t);
        float* e = t.orientation.orientation.elements;
        return XV3(e[6], e[7], -e[8]);
        */
		XnUserID userID = getPlayerID();
        XnSkeletonJointPosition p0, p1, p2;
        kn_userGen->GetSkeletonCap().GetSkeletonJointPosition(userID, XN_SKEL_RIGHT_SHOULDER, p0);
        kn_userGen->GetSkeletonCap().GetSkeletonJointPosition(userID, XN_SKEL_TORSO, p1);
        kn_userGen->GetSkeletonCap().GetSkeletonJointPosition(userID, XN_SKEL_LEFT_SHOULDER, p2);
        XV3 v0(p0.position), v1(p1.position), v2(p2.position);
        return ((v1 - v0).cross(v2 - v1)).normalize();
    } else {
        return XV3();
    }
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
XV3 UserDetector::getUpVector()
{
	if (kn_playerStatus->isPlayerVisible())
	{
		XnUserID userID = getPlayerID();
        XnSkeletonJointPosition p0, p1;
        kn_userGen->GetSkeletonCap().GetSkeletonJointPosition(userID, XN_SKEL_TORSO, p0);
        kn_userGen->GetSkeletonCap().GetSkeletonJointPosition(userID, XN_SKEL_NECK, p1);
        XV3 v0(p0.position), v1(p1.position);
        return (v1 - v0).normalize();
    } else {
            return XV3();
    }
}