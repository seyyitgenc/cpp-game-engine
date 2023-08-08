// #include "Engine/clock.h"
// // constract Clock
// Clock::Clock() {
//     m_startTicks = 0;
//     m_pausedTicks = 0;
//     m_paused = false;
//     m_started = false;
// }
// Clock::~Clock() {}
// // start Clock
// void Clock::start() {
//     // start the Clock
//     m_started = true;
//     m_paused = false;
//     // get the current clock time
//     m_startTicks = SDL_GetTicks();
//     m_pausedTicks = 0;
// }
// // this function will get time when it's called
// Uint32 Clock::getTicks() {
//     Uint32 time = 0;

//     if (m_started) {
//         m_paused ? time = m_pausedTicks : time = SDL_GetTicks() - m_startTicks;
//     }

//     return time;
// }
// // stop Clock
// void Clock::stop() {
//     m_started = false;
//     m_paused = false;

//     m_startTicks = 0;
//     m_pausedTicks = 0;
// }
// // pause Clock
// void Clock::pause() {
//     if (m_started && !m_paused) {
//         m_paused = true;
//         m_pausedTicks = SDL_GetTicks() - m_startTicks;
//         m_startTicks = 0;
//     }
// }
// // unpause Clock
// void Clock::unpause() {
//     if (m_started && m_paused) {
//         m_paused = false;
//         m_startTicks = SDL_GetTicks() - m_pausedTicks;
//         m_pausedTicks = 0;
//     }
// }
// // reset Clock
// void Clock::restart() { m_startTicks = SDL_GetTicks(); }