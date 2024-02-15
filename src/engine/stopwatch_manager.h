#pragma once
#include "util/stopwatch.hpp"
#include <memory>

class StopwatchManager
{
    /* data */
public:
    StopwatchManager(/* args */);
    ~StopwatchManager();
    void add();
    void remove();
private:
    std::unordered_map<std::string,std::unique_ptr<Stopwatch<>>> timeList;
};