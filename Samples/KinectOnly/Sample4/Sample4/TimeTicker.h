#ifndef _TIME_TICKER_H_
#define _TIME_TICKER_H_

#include <windows.h>

class TimeTicker
{
private:
        DWORD	kn_lastTime;
        BOOL	kn_isLocked;

public:
        TimeTicker();
        float tick();		//get elapsed time since last call this function
        float lock();		//get elapsed time since last call this function and lock ticker (so m_lastTime not change)
        float unlock();		//get elapsed time since last call this function and unlock ticker
};

#endif