#ifndef _CONFIG_H_
#define _CONFIG_H_

enum ShotStage
{
	STAGE_NORMAL,
	STAGE_READY,
	STAGE_SHOT
};

//-------------------- DETECTOR
#define ONEHANDSHOT_TIME_THRESHOLD			1.0f				//ban 1 tay chi cho phep trong vong 1 khoang thoi gian nao day
#define MIN_ANGLE_ARM_BEND					80.0f				// tay toi thieu phai gap 1 goc lon hon the nay
#define ARM_EXTEND_ANGLE_THRESHOLD			20.0f				//0.85f = 30 do	

#endif
