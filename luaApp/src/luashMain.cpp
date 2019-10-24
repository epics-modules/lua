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
	luaNamedState("default");
	luashSetCommonState("default");
	
    if(argc>=2) {    
        luash(argv[1]);
        epicsThreadSleep(.2);
    }
    luash(NULL);
	epicsExit(0);
    return(0);
}
