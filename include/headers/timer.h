#pragma once
#include <SDL2/SDL.h>
class LTimer
{
public:
    LTimer();
    ~LTimer();
    // various clock actions
    void start();
    void stop();
    void pause();
    void unpause();
    void reset();
    // gets the timer's time
    Uint32 getTicks();
    // checks the status of the timer
    bool isStarted() { return m_started; }
    bool isPaused() { return m_paused && m_started; }

private:
    Uint32 m_startTicks;
    Uint32 m_pausedTicks;
    int m_countedFrames;
    bool m_paused;
    bool m_started;
};
