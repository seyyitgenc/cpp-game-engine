#include "utils.h"

#include "clock.h"
#include "globals.h"

Clock capTimer;
// for pyhsics calculatio f
float getTimeStep() {
    static Clock stepTimer;
    static float timeStep;
    timeStep = stepTimer.getTicks() / 1000.f;
    stepTimer.start();
    return timeStep;
}
float getDeltaTime() {
    static Uint64 NOW = SDL_GetPerformanceCounter();
    static Uint64 LAST = 0;
    static float dt = 0;
    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    dt = (NOW - LAST) / (double)SDL_GetPerformanceFrequency();
    return dt;
}

constexpr void setFrameRate(const int &SCREEN_FPS) { SCREEN_TICKS_PER_FRAME = 1000.0f / SCREEN_FPS; }

void limitFrameRate() {
    capTimer.start();
    int frameTicks = capTimer.getTicks();
    if (frameTicks < SCREEN_TICKS_PER_FRAME) SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
}

float getFrameRate(const int &countedFrames, const Uint64 &fpsTimer) {
    float avgFps = countedFrames / (fpsTimer / 1000.f);
    if (avgFps > 2000000) avgFps = 0;
    return avgFps;
}