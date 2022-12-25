#pragma once
#include <SDL.h>

// some general utilities
float &getTimeStep();
float getFrameRate(const int &countedFrames, const Uint64 &fpsTimer);
void setFrameRate(const int &SCREEN_FPS);
void limitFrameRate();