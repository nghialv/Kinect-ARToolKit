#ifndef _RIGHTHANDSHOTDETECTOR_H_
#define _RIGHTHANDSHOTDETECTOR_H_

#include "AbstractPoseDetector.h"

class RightHandShotDetector: public AbstractPoseDetector
{
private:
	TimeTicker	kn_ticker;
	ShotStage	kn_stage;
	float		kn_timeShot;

public:
	RightHandShotDetector(UserDetector *userDetector);

	virtual bool isPosing(float dt);
    virtual void onPoseDetected(float dt);

	bool isRightHandBend();
	bool isRightHandExtend();
};

#endif