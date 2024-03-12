#pragma once

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

inline std::string RED_TEXT             (std::string text){return "\033[0;31m" + text + "\033[0m";}
inline std::string BLUE_TEXT            (std::string text){return "\033[0;34m" + text + "\033[0m";}
inline std::string CYAN_TEXT            (std::string text){return "\033[0;36m" + text + "\033[0m";}
inline std::string GREEN_TEXT           (std::string text){return "\033[0;32m" + text + "\033[0m";}
inline std::string BLACK_TEXT           (std::string text){return "\033[0;30m" + text + "\033[0m";}
inline std::string WHITE_TEXT           (std::string text){return "\033[0;37m" + text + "\033[0m";}
inline std::string YELLOW_TEXT          (std::string text){return "\033[0;33m" + text + "\033[0m";}
inline std::string MAGENTA_TEXT         (std::string text){return "\033[0;35m" + text + "\033[0m";}
inline std::string LIGHT_RED_TEXT       (std::string text){return "\033[1;31m" + text + "\033[0m";}
inline std::string LIGHT_BLUE_TEXT      (std::string text){return "\033[1;34m" + text + "\033[0m";}
inline std::string LIGHT_CYAN_TEXT      (std::string text){return "\033[1;36m" + text + "\033[0m";}
inline std::string LIGHT_GREEN_TEXT     (std::string text){return "\033[1;32m" + text + "\033[0m";}
inline std::string LIGHT_BLACK_TEXT     (std::string text){return "\033[1;30m" + text + "\033[0m";}
inline std::string LIGHT_WHITE_TEXT     (std::string text){return "\033[1;37m" + text + "\033[0m";}
inline std::string LIGHT_YELLOW_TEXT    (std::string text){return "\033[1;33m" + text + "\033[0m";}
inline std::string LIGHT_MAGENTA_TEXT   (std::string text){return "\033[1;35m" + text + "\033[0m";}

// todo: save log into .log filewh
// todo: add which header file that is included into log
class Log
{
public:
    // todo : reconfigure these properties
    enum {All = 0, Debug, Info, Warning, Error, Fatal, None};
    // note: this doesn't jump to new line
    template<typename... Args>
    static void write(int nLevel, Args... args){
        checkInit();
        if (nLevel >= _nLevel){
            // note: in case if i use multithreadding
            // fixme: this is not thread safe.
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            auto local_time = std::localtime(&time_t_now);
            std::stringstream ss;
            ss << "["<< std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << "] ";
            (std::cout << ss.str() << ... << args);
        }
    }
    static void setLevel(int nLevel){
        _nLevel = nLevel;
        _bIinitialised = true;
    }
protected:
    static void checkInit(){
        if (!_bIinitialised)
            init();
    }
    static void init(){
        // todo : get build spec from debug
        int level(Log::All);
        setLevel(level);
    }
private:
    Log();
    static bool _bIinitialised;
    static int _nLevel;
};
inline bool Log::_bIinitialised;
inline int Log::_nLevel;
