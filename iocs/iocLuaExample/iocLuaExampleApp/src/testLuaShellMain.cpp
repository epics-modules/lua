#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <epicsThread.h>
#include <epicsExit.h>
#include <luaEpics.h>
#include <luaShell.h>

int main(int argc, char *argv[])
{
	lua_State* state = luaNamedState("shell");
	
    if(argc>=2) {    
        luash(state, argv[1]);
        epicsThreadSleep(.2);
    }
    luash(state, NULL);
	epicsExit(0);
    return(0);
}
