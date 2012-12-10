#ifndef _PLAYERSTATUS_H_
#define _PLAYERSTATUS_H_

#include "Common.h"

class PlayerStatus
{
private:
	XnUserID	playerID;
	bool		isVisible;

public:
	PlayerStatus();
	~PlayerStatus(){}

	XnUserID getPlayerID(){ return playerID;}
	void setPlayerID(XnUserID id);
	bool isPlayerVisible();
	void setPlayerVisible();
	void setPlayerNotVisible();
};

#endif