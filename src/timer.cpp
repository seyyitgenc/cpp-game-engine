#include <timer.hpp>

LTimer::LTimer()
{
    m_startTicks = 0;
    m_pausedTicks = 0;
    m_paused = false;
    m_started = false;
}
LTimer::~LTimer(){
    
}
void LTimer::start()
{
    // start the timer
    m_started = true;
    m_paused = false;
    // get the current clock time
    m_startTicks = SDL_GetTicks();
    m_pausedTicks = 0;
}
void LTimer::stop()
{
    m_started = false;
    m_paused = false;

    m_startTicks = 0;
    m_pausedTicks = 0;
}
void LTimer::pause()
{
    // timer is runing and isn't already paused
    if (m_started && !m_paused)
    {
        // pause timer
        m_paused = true;
        // calculate pause ticks
        m_pausedTicks = SDL_GetTicks() - m_startTicks;
        m_startTicks = 0;
    }
}
void LTimer::unpause()
{
    if (m_started && m_paused)
    {
        m_paused = false;
        m_startTicks = SDL_GetTicks() - m_pausedTicks;
        m_pausedTicks = 0;
    }
}
Uint32 LTimer::getTicks()
{
    Uint32 time = 0;

    if (m_started)
        if (m_paused)
            time = m_pausedTicks;
        else
            time = SDL_GetTicks() - m_startTicks;

    return time;
}