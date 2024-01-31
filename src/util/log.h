#pragma once

#include <cstdio>
#include <cstdarg>

// todo: modift this to use std::cout 

// note : this is C implementation of Log
class CLog
{
public:
    // todo : reconfigure these properties
    enum {All = 0, Debug, Info, Warning, Error, Fatal, None};
    static void write(int nLevel, const char *szFormat, ...);
    static void setLevel(int nLevel);
protected:
    static void checkInit();
    static void init();
private:
    CLog();
    static bool _bIinitialised;
    static int _nLevel;
};