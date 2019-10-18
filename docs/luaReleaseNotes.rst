lua Release Notes
=================

Release 2-1
-----------

-  LUA_SCRIPT_PATH now always includes the current directory. Makes more
   sense when using '<' in the lua shell.
-  Previously, the "iocsh" library was used only to lookup ioc shell functions,
   now it will now also check for environment variables that match the given name.
-  "iocsh" library lookups now also fixed to return nil when it can't find a
   matching element (in R2-0 it was returning functions that, when called, stated
   nothing was found).
-  Added setOption function to the "asyn" library. works the same way as
   asynSetOption. The asyn.client class received a matching function.
-  Fixed bug in "asyn" library where writeread requests were attempting to read
   twice, causing timeout waits.
-  Added luashCmd function to ioc shell. Useful for running one-liners of lua code.
-  lua shell now specially recognizes the line '#ENABLE_HASH_COMMENTS', 
   when put into a lua shell script, the shell will ignore lines where
   the first non-whitespace line is a '#' character. Allowing scripts to
   appear more like regular ioc shell scripts.
-  lua shell now ignores leading whitespace on lines, was only an issue
   with the 'exit' and '<' commands.
-  Fixed an issue where I was leaving a metatable reference on the lua
   stack when luaCreateState was called.
-  Documentation has been switched to use ReStructured text, now hosted
   on https://epics-lua.readthedocs.io/en/latest/

Release 2-0
-----------

-  "iocsh" library now available for any version of base
-  Calls to the "iocsh" library can omit the library name while within
   the luash interpreter, this makes the luash almost fully backwards
   compatible with iocsh scripts. The only problems come from comments
   (due to "#" being a command in lua) and macros being different than
   global variables.
-  Better error handling in "asyn" library
-  lua Script file location now allows full paths
-  loadRegistered function automatically triggered and no longer needed
   for lua startup scripts
-  Lua static library registration now setup to work with standard lua
   "require" functionality
-  'asyn' lua library function "port" changed to "client" to better
   represent that it is creating an asynOctetClient not an
   asynPortDriver. InTerminator and OutTerminator changed to member
   fields rather than get/set functions.
-  'asyn' lua library new function "driver" creates an object
   representing an asynPortDriver. Allows you to get/set the value of
   parameters in the paramList and trigger the read and write functions
   of parameters.
-  Parameters supplied in luaRecord CODE field and macros provided to
   luaSpawn are now evaluated using a lua sandbox environment rather
   than a custom parser.

Release 1-3
-----------

-  Fixes compilation issues for Visual Studio 2010
-  Dynamic library loading enabled for Linux architectures
-  C functions can now be registered into lua libraries that will
   automatically load on lua shell startup.
-  Also set up a quick way to bind functions already loaded into the IOC
   shell
-  All libraries loaded into shell, device support, and luascript record
-  luaEpics functions are now able to be used in C files
-  Added luaSpawn function to allow for running scripts in the
   background
-  Full Documentation

Release 1-2-2
-------------

-  Fixed a compilation bug on vxWorks
-  First official synApps release

Release 1-2-1
-------------

-  Fixed a bug where softChannel support wasn't working
-  luascript's CODE field is no longer saved by autosave on every
   luascript record, just for the user luascripts.

Release 1-2
-----------

-  Fixed building for Windows
-  Added global reference to the pdbbase variable, allows lua shell to
   be used as a full replacement for iocsh
-  Added '<' command to luash to include the contents of other scripts
-  luaScript input fields now can have descriptions of their contents
-  Fixed a bug where forward link processing wasn't happening
-  

Release 1-1
-----------

-  luash function now runs as a command shell or an interpreter
-  Lua asyn library function to access ports as lua objects
-  LUASH_PS1 variable for lua shell prompt

Release 1-0
-----------

-  First public release on github

| Suggestions and Comments to:
| `Keenan Lang <mailto:klang@aps.anl.gov>`__: (klang@aps.anl.gov)
