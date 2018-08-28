#ifndef INC_LUASHELL_H
#define INC_LUASHELL_H

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif


epicsShareFunc int epicsShareAPI luash(const char* pathname);
epicsShareFunc int epicsShareAPI luashBegin(const char* pathname, const char* macros);

epicsShareFunc int epicsShareAPI luaSpawn(const char* pathname, const char* macros);

#ifdef __cplusplus
}
#endif


#endif
