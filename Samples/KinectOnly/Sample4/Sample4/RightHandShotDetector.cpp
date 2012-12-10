#include "RightHandShotDetector.h"

RightHandShotDetector::RightHandShotDetector(UserDetector *userDetector)
:AbstractPoseDetector(userDetector)
{
	kn_stage = STAGE_NORMAL;
	kn_timeShot = ONEHANDSHOT_TIME_THRESHOLD;
}

bool RightHandShotDetector::isRightHandBend()
{
	return isRightArmBend(MIN_ANGLE_ARM_BEND);
}

bool RightHandShotDetector::isRightHandExtend()
{
	//check right arm extend and angle of (up vector, hand) < 150 deg
	return isRightArmExtend( ARM_EXTEND_ANGLE_THRESHOLD);
}


bool RightHandShotDetector::isPosing(float dt)
{
	if(!kn_userDetector->isPlayerVisible())
		return false;

	switch (kn_stage) 
	{
        case STAGE_NORMAL:
			if(isRightHandBend())
			{
				kn_stage = STAGE_READY;
				kn_ticker.lock();
			}
			break;
        case STAGE_READY:
			kn_timeShot = kn_ticker.tick();
			if(kn_timeShot > ONEHANDSHOT_TIME_THRESHOLD)						//change
				kn_stage = STAGE_NORMAL;
			else
				if(isRightHandExtend())					//this time can used to caculate velocity of shot
					return true;
     }
     return false;
}

void RightHandShotDetector::onPoseDetected(float dt)
{
	//kn_stage = STAGE_SHOT;
	//printf("-------------  SHOT------------\n");
	kn_stage = STAGE_NORMAL;
}