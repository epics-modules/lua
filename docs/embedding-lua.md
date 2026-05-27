---
layout: default
title: Embedding the Lua Shell
nav_order: 8
---


# Embedding the Lua Shell
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}


Lua Shell As A Replacement For The IOC Shell
--------------------------------------------

The Lua shell can be called as a C function, which allows it to be used
on vxWorks or as a replacement for the IOC shell in a soft IOC. The
same parameters apply as when calling the command inside the IOC shell.
A main.cpp file might look like:

```c++
#include "luaShell.h"

int main(int argc,char *argv[])
{
    if(argc>=2) {
        luash(argv[1]);
        epicsThreadSleep(.2);
    }
    luash(NULL);
    epicsExit(0);
    return(0);
}
```


Common Lua Environments
-----------------------

In the above code, there are two different Lua shell calls. One to read
in a given script, and one to provide an interactive shell. As the code
stands, any objects that are created in the input script aren't available
to the interactive shell. Say, for example, you have a library that
provides useful functions that are used in scripts, you would have to
include that library again once you are in the interactive shell, and
there wouldn't be a way to automate that.

Therefore, there is a way to define that you want different calls to
luash to use a common environment. The `luashSetCommonState` command is
included in the luaShell.h file and sets a default environment that
subsequent luash calls will use. The command takes a C string; if the
string matches the name of a state created using `luaNamedState()`, it
will use that state, otherwise it will create a new state with that
name. So the above code changed to allow the interactive shell to
reference code from the interpreted script would look like:

```c++
#include "luaShell.h"

int main(int argc,char *argv[])
{
    luashSetCommonState("default");

    if(argc>=2) {
        luash(argv[1]);
        epicsThreadSleep(.2);
    }
    luash(NULL);
    epicsExit(0);
    return(0);
}
```
