/* testMain.c */
/* Author:  Ron Sluiter */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <epicsThread.h>
#include <iocsh.h>
#include <epicsExit.h>

int main(int argc,char *argv[])
{
    if(argc>=2) {    
        iocsh(argv[1]);
        epicsThreadSleep(.2);
    }
    iocsh(NULL);
	epicsExit(0);
    return(0);
}
