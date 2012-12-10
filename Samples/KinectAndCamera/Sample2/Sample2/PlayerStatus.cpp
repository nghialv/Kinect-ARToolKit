#include "PlayerStatus.h"

PlayerStatus::PlayerStatus()
{
	playerID = 0;
	isVisible = FALSE;
}

void PlayerStatus::setPlayerID(XnUserID id)
{
	playerID = id;
}

bool PlayerStatus::isPlayerVisible()
{ 
	return isVisible;
}

void PlayerStatus::setPlayerVisible()
{ 
	isVisible = TRUE;
	printf("Player %d is VISIBLE\n", playerID);
}

void PlayerStatus::setPlayerNotVisible()
{ 
	isVisible = FALSE;
	playerID = 0;
	printf("Player %d is OUT\n", playerID);
}
