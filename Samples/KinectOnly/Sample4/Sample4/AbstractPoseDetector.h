#ifndef _ABSTRACT_POSE_DETECTOR_H_
#define _ABSTRACT_POSE_DETECTOR_H_

#include "Common.h"
#include "Config.h"
#include "UserDetector.h"
#include "TimeTicker.h"
#include "Util.h"

class AbstractPoseDetector
{
protected:
        UserDetector* kn_userDetector;

private:
        // TODO: should be time instead of frame rate
        int kn_requiredPosingStability;
        int kn_posingTicks;

        TimeTicker kn_ticker;

public:
        AbstractPoseDetector(UserDetector* userDetector);
        virtual ~AbstractPoseDetector();

        virtual bool detect();

protected:
        void setRequiredPosingStability(int value) { kn_requiredPosingStability = value; }

        virtual bool isPosing(float dt) = 0;
        virtual void onPoseDetected(float dt) = 0;

		bool isRightArmBend(float minAngle);		//deg
		bool isRightArmExtend(float minAngleExtend);				//check tay duoi thang va ko huong 180 do voi vector up
};

#endif