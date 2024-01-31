#include "log.h"
bool CLog::_bIinitialised;
int CLog::_nLevel;

void CLog::write(int nLevel, const char *szFormat, ...){
    checkInit();
    if (nLevel >= _nLevel)
    {
        va_list args;
        va_start(args, szFormat);
        vprintf(szFormat, args);
        va_end(args);
    }
};
void CLog::setLevel(int nLevel){
    _nLevel = nLevel;
    _bIinitialised = true;
}

void CLog::checkInit(){
    if (!_bIinitialised)
        init();
}

void CLog::init(){
    // todo : get build spec from debug
    int level(CLog::All);
    setLevel(level);
}