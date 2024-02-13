#pragma once

#include <unordered_map>
#include <type_traits>
#include <iostream>
#include <chrono>
#include <string>

// yes i really need to hold several timers. like frame time, render time, model generation time etc.
// but what is the most optimal way to do it ?
// in my map i hold several clocks with names. i need most of them for debug purposes.
// for example if i write down a manager i need to add timers to the list first. but what happens if i deleted a reference to it ?
// it's actually easy to solve this. we can check if the 

template<typename clock = std::chrono::steady_clock>
class Stopwatch
{
public:
    Stopwatch() : _startTime(clock::now()){};
    void start(){
        _startTime = clock::now();
    }

    void reset(){
        _startTime = clock::now();
    }
    // this function supports integral and floating point types. 
    // note: providing floating points to this function is the best way to use it.
    template<typename Rep = float, typename time_unit = std::ratio<1,1>>
    [[nodiscard]]Rep getElapsedTime(){
        return static_cast<Rep>(std::chrono::duration_cast<std::chrono::duration<Rep, time_unit>>(std::chrono::steady_clock::now() - _startTime).count());
    }

private:
    using time_point = clock::time_point;
    time_point _startTime;
};