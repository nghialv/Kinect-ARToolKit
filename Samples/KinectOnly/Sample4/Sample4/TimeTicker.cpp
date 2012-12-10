#include "TimeTicker.h"

TimeTicker::TimeTicker()
{
        kn_lastTime = 0;
        kn_isLocked = false;
}

float TimeTicker::tick()
{
        DWORD currentTime = GetTickCount();													//get elapsed time since the system was started
        float dt = (kn_lastTime == 0) ? 0.0f : (currentTime - kn_lastTime) / 1000.0f;		//get elapsed time since last call this function
        if (!kn_isLocked) {
                kn_lastTime = currentTime;
        }
        return dt;
}

float TimeTicker::lock()
{
        kn_isLocked = false;
        float dt = tick();
        kn_isLocked = true;
        return dt;
}

float TimeTicker::unlock()
{
        kn_isLocked = false;
        return tick();
}