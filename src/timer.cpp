#include <headers/timer.h>

// constract timer
Timer::Timer()
{
    m_startTicks = 0;
    m_pausedTicks = 0;
    m_paused = false;
    m_started = false;
}
Timer::~Timer()
{
}
// start timer
void Timer::start()
{
    // start the timer
    m_started = true;
    m_paused = false;
    // get the current clock time
    m_startTicks = SDL_GetTicks();
    m_pausedTicks = 0;
}
// this function will get time when it's called
Uint32 Timer::getTicks()
{
    Uint32 time = 0;

    if (m_started)
    {
        m_paused ? time = m_pausedTicks : time = SDL_GetTicks() - m_startTicks;
    }

    return time;
}
// stop timer
void Timer::stop()
{
    m_started = false;
    m_paused = false;

    m_startTicks = 0;
    m_pausedTicks = 0;
}
// pause timer
void Timer::pause()
{
    if (m_started && !m_paused)
    {
        m_paused = true;
        m_pausedTicks = SDL_GetTicks() - m_startTicks;
        m_startTicks = 0;
    }
}
// unpause timer
void Timer::unpause()
{
    if (m_started && m_paused)
    {
        m_paused = false;
        m_startTicks = SDL_GetTicks() - m_pausedTicks;
        m_pausedTicks = 0;
    }
}
// reset timer
void Timer::reset()
{
    m_startTicks = 0;
    m_pausedTicks = 0;
}