# Lua Shell

The lua shell is exposed as both a c function and is registered as an ioc shell function.
Thus, the shell can either be invoked in a startup script or be run as the startup program
in general.

## Lua Shell Inside The IOC Shell

Once the dbd has been loaded and the registerRecordDeviceDriver command has been called on
an IOC, you can call the *luash* command. Luash takes in two parameters, the first, the
name of a lua script to run, and the second a set of macros to be set as global variables
in the lua shell's state. The macros are defined in the same "aaa=bbb,ccc=ddd" form that
the ioc shell uses for iocshLoad/iocshRun and that is used for dbLoadRecords/dbLoadTemplate.
One difference, however, is that strings need to be quoted in order to be recognized as
string variables, any non-quoted value will be interpreted as a number. So to create a
global numeric variable X with the value 5 and a string variable TEXT with the value
"Hello, World!" you would have your second parameter as "X=5,TEXT='Hello,World'".

The luash command will then attempt to find the script you have given it. It does this by
searching for a file with the given name in a set of folders as given by the environment
variable LUA_SCRIPT_PATH. If this variable is undefined, the command will search within the
current directory. As well, if no script name is given, the shell will take commands from
the standard input, with a prompt set by the environment variable LUASH_PS1. Additionally,
within the shell, you can find and include other scripts by using the '<' character. Putting

    < script.lua

Will attempt to find script.lua by the same process as detailed above and include all the text
of that file at the insertion point. When running any script the lua shell will exit at the end
of the file being read. For both files and commands from the standard input, a single line with
only the word 'exit' will close the session (Currently, this doesn't work within conditionals).

## Lua Shell As A Replacement For The IOC Shell

The lua shell can also be called as a c function, which allows it to be used on vxWorks or as
a replacement for the IOC shell in a soft IOC. The same parameters apply as when calling the
command inside the IOC shell. So your main.cpp file might look like:

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

When using the lua shell as a replacement for the IOC shell, you can call all the
same commands as the IOC shell by using iocsh.<command_name>(*args*).
