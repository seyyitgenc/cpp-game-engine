#ifndef TIMER_H
#define TIMER_H
#include <SDL2/SDL.h>
#endif
class Clock
{
public:
    Clock();
    ~Clock();
    // various clock actions
    void start();
    void stop();
    void pause();
    void unpause();
    Uint32 restart();
    // gets the timer's time
    Uint32 getTicks();
    // checks the status of the timer
    bool isStarted() { return m_started; }
    bool isPaused() { return m_paused && m_started; }

private:
    Uint32 m_elapsedTime=0;
    Uint32 m_startTicks=0;
    Uint32 m_pausedTicks=0;

    bool m_paused=false;
    bool m_started=false;
};
