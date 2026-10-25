#pragma once

#include <unordered_map>
#include <iostream>
#include <chrono>
#include <string>

template<typename clock = std::chrono::steady_clock>
class Stopwatch
{
public:
    Stopwatch() : _startTime(clock::now()){};

    void reset(){
        _startTime = clock::now();
    }
    // Integral or floating point; floating point works best.
    template<typename Rep = float, typename time_unit = std::ratio<1,1>>
    [[nodiscard]]Rep getElapsedTime(){
        return static_cast<Rep>(std::chrono::duration_cast<std::chrono::duration<Rep, time_unit>>(std::chrono::steady_clock::now() - _startTime).count());
    }

private:
    typename clock::time_point _startTime;
};