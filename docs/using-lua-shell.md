---
layout: default
title: Lua Shell
nav_order: 5
---


# Using the Lua Shell

The lua shell is exposed as both a c function and is registered as a function
with iocsh. Thus, the shell can either be invoked in a startup script or be 
run as the startup program in general.

The shell has been set up so as to be as backwards compatible with the iocsh
style startup scripts as possible. While within the lua shell, the global
environment is set up so that lookups of names that don't have an associated
variable will attempt to pull values from, first, the running epics environment,
and then, if no environment variable is found, try to find a matching function
name. This means that functions that are registered with the ioc shell can
be treated as if they were defined lua functions.


```
   luash> EPICS_VERSION_MAJOR
   7
   luash>
   luash> epicsEnvShow
   func_meta: 0x673150
   luash>
   luash> epicsEnvShow("EPICS_VERSION_MAJOR")
   EPICS_VERSION_MAJOR=7
```
    
As well, the special directive '#ENABLE_HASH_COMMENTS' is provided. While lua normally
reserves the '#' character for determining the length of a table, putting the line
'#ENABLE_HASH_COMMENTS' in your scripts will set the shell to accept iocsh style
comments that use '#'. This setting only applies to lines where '#' is the first
non-empty character in the line, it will not affect the use of '#' in normal lua
operations.


```
   luash> #ENABLE_HASH_COMMENTS
   Accepting iocsh-style comments
   luash>
   luash> #print("This won't print")
   luash> print(#"Check len")
   9
```
    
Calling the Lua Shell From Inside The IOC Shell
-----------------------------------------------

Once the dbd has been loaded and the registerRecordDeviceDriver command
has been called on an IOC, you can call the *luash* command. Luash takes
in two parameters, the first, the name of a lua script to run, and the
second a set of macros to be set as global variables in the lua shell’s
state. The macros are defined in the same “aaa=bbb,ccc=ddd” form that
the ioc shell uses for iocshLoad/iocshRun and that is used for
dbLoadRecords/dbLoadTemplate. One difference, however, is that strings
need to be quoted in order to be recognized as string variables, any
non-quoted value will be interpreted as a number. So to create a global
numeric variable X with the value 5 and a string variable TEXT with the
value “Hello, World!” you would have your second parameter as
“X=5,TEXT=‘Hello,World’”.

The luash command will then attempt to find the script you have given
it. It does this by searching for a file with the given name in a set of
folders as given by the environment variable LUA_SCRIPT_PATH. If this
variable is undefined, the command will search within the current
directory. As well, if no script name is given, the shell will take
commands from the standard input, with a prompt set by the environment
variable LUASH_PS1. Additionally, within the shell, you can find and
include other scripts by using the ‘<’ character. Putting


```
   < script.lua
```
    
Will attempt to find script.lua by the same process as detailed above
and include all the text of that file at the insertion point. When
running any script the lua shell will exit at the end of the file being
read. 

For both files and commands from the standard input, a single line
with only the word ‘exit’ will break from the current level of shell
execution. In example, if you were to load the file script.lua as seen
above, and the file had the following code within it


```lua
   if (true) then
       exit
   end
```

That exit will end the reading of just the script.lua file and flow would
resume to the shell that called script.lua.

As a note, the use of 'exit' is replaced automatically with an actual call
to a function 'exit()' by the shell. This is done to make the code proper
in regards to lua syntax, while still being able to provide syntactic sugar.

Lua Shell As A Replacement For The IOC Shell
--------------------------------------------

The lua shell can also be called as a c function, which allows it to be
used on vxWorks or as a replacement for the IOC shell in a soft IOC. The
same parameters apply as when calling the command inside the IOC shell.
So your main.cpp file might look like:

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

In the above code, there are two different lua shell calls. One to read in
a given script, and one to provide an interactive shell. As the code stands,
any objects that are created in the input script aren't available to the
interactive shell. Say, for example, you have a library that provides useful
functions that are used in scripts, you would have to include that library
again once you are in the interactive shell, and there wouldn't be a way to
automate that.

Therefore, there is a way to define that you want different calls to luash
to use a common environment. The luashSetCommonState command is included
in the luaShell.h file and sets a default environment that subsequent
luash calls will use. The command takes a c-string, if the string matches
the name of an environment created using luaNamedState(), it will use that
environment, otherwise it will create a new state with that name. So the
above code changed to allow the interactive shell to reference code from
the interpreted script would look like:

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
