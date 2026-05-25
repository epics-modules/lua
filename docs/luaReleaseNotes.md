---
layout: default
title: Release Notes
nav_order: 2
---


# lua Release Notes

Release 4-0
-----------

### Major Changes

- **luaaa dependency removed.** The third-party luaaa C++ binding library has been 
  completely eliminated. All Lua libraries now use the standard Lua C API directly.
  This removes approximately 2000 lines of template-heavy C++11 code from the build.

- **C++11 no longer required.** All C++11 features have been removed from the codebase.
  The module now builds with C++98/03 compilers, restoring compatibility with older
  toolchains such as vxWorks and legacy MSVC versions. The `CXX11_SUPPORT` build flag
  has been removed.

- **Basic and main library versions unified.** The separate `libs/basic/` implementations 
  have been replaced with `#include` forwarders to the main versions. All platforms now 
  get the same full-featured library code.

### New Features

- **`luaLoadFile` command.** New iocsh command that loads and executes a Lua script as
  a single chunk in a new state. Unlike `luash`, the entire file is compiled at once 
  (local variables work across lines). Unlike `luaSpawn`, execution is synchronous. 
  Accepts table macros when called from Lua.

- **`luash` file-mode interleaved output.** When `luash` runs a script file, it now
  executes each statement as it reads it, with output appearing immediately after the
  echoed line that produced it. This matches iocsh behavior and makes it easier to trace
  execution in IOC startup scripts. Multi-line constructs (function/if/for/do blocks)
  are accumulated until complete.

- **`POPT`/`PCAL` fields added to luascript record.** The Process Option field controls
  whether CODE runs every time (`Always`, default) or only when a condition is met
  (`Conditional`). The Process Condition field holds a Lua expression that gates CODE
  execution. Changed flags (`_A`-`_J`, `_AA`-`_JJ`) are provided as boolean Lua globals
  indicating which inputs changed since the last process cycle. PCAL is pre-compiled
  for efficiency and recompiled when changed at runtime.

- **`IVOA`/`IVOV` fields added to luascript record.** The Invalid Output Action field
  controls behavior when Lua script execution fails: "Continue normally" (default),
  "Don't drive outputs" (suppress output write and forward link), or "Set output to
  IVOV" (write the IVOV value instead). Uses the standard `menuIvoa` from EPICS base,
  matching the calcout/scalcout/acalcout convention.

- **luascript record string fields expanded.** AA-JJ, SVAL, PSVL fields expanded from
  40 to 256 characters. ERR field expanded from 200 to 256 characters.

- **`info()` function for shell discoverability.** New global function available in all
  Lua states. Call `info(library)` to list available functions, or `info(object)` to
  list methods and properties of a userdata object.

- **`db.entry()` with method syntax.** The entry object returned by `db.entry()` now 
  supports both method syntax (`ent:findRecord("name")`) and module function syntax
  (`db.findRecord(ent, "name")`). The entry is proper userdata with `__gc` for 
  automatic cleanup.

- **`db.record()` with `:field()` and `:info()` methods.** The record object now 
  supports `rec:field("VAL", "test")` and `rec:info("autosave", "VAL")` for setting
  fields and info tags. The record is proper userdata with `__gc` for automatic cleanup.

- **`epics.get` and `epics.put` array support.** Both functions now support array/waveform
  PVs. `epics.get` returns a Lua table for multi-element PVs; `epics.put` accepts a Lua
  table to write arrays. Char waveforms are returned as Lua strings by default.

- **`epics.get` and `epics.put` options table.** Both functions accept an options table
  as the second/third argument with named parameters: `timeout`, `count` (max elements
  to fetch), and `string` (force string vs numeric return for enums and char arrays).

- **`epics.get` returns `nil, "error"` on failure.** Returns the value on success, or
  `nil, "error message"` on failure instead of silently returning nil.

- **`epics.put` returns nothing on success.** Returns an error string on failure. The
  same convention applies to `luaSpawn`, `luash`, `luaCmd`, and `luaLoadFile` -- all
  action commands return nothing on success and an error string on failure.

- **`epics.get`/`epics.put` integer distinction.** Integer types (DBF_SHORT, DBF_LONG,
  DBF_CHAR, DBF_ENUM) are now returned with `lua_pushinteger` instead of `lua_pushnumber`,
  preserving integer semantics. Scalar integer puts use `DBR_LONG` instead of `DBR_DOUBLE`.

- **CA context cached per Lua state.** The CA context is now created once per Lua state
  and reused for all subsequent `epics.get`/`epics.put` calls, eliminating the overhead
  of context creation/destruction on every call.

- **Local PV fast path.** `epics.get` and `epics.put` now automatically detect PVs that
  exist in the local IOC database and use direct database access (`dbGetField`/`dbPutField`)
  instead of Channel Access. This eliminates network and CA overhead for local PVs,
  providing orders-of-magnitude faster access. Remote PVs transparently fall through to CA.

- **`epics.pv` redesigned as proper userdata.** The PV object returned by `epics.pv()`
  is now a proper Lua userdata with a `__gc` metamethod (ready for future channel
  caching), `__tostring` support, and type-safe access via `luaL_checkudata`. The
  `pv.name` property returns the PV name. The old `pv:getName()` method and `pv.pv_name`
  raw field are removed.

- **`epics.pv` get/put methods.** The PV object supports `pv:get(field, options)` and
  `pv:put(field, value, options)` for field access with custom timeout, count, and
  string options.

- **`epics.sleep` removed.** Use `osi.sleep` instead.

- **`#-` silent comments.** When hash comments are enabled, lines starting with `#-`
  are skipped silently without being echoed, matching the iocsh convention. Blank lines
  in file mode are also elided from the output.

- **LPeg pattern matching library embedded.** The LPeg 1.1.0 library (Parsing
  Expression Grammars for Lua) is now included and automatically available via
  `require("lpeg")`. The companion `re.lua` module is installed to the lib directory.
  LPeg is Copyright (c) 2007-2023 Lua.org, PUC-Rio (MIT License).

- **luascript record async rework.** Asynchronous mode (`SYNC=Async`) now uses the
  EPICS callback system instead of creating a new thread per process cycle. The Lua
  script runs in an EPICS callback thread, and completion is signaled via `dbProcess`
  with proper `dbScanLock`. This follows the standard EPICS two-pass asynchronous
  record processing model.

- **Multiple `asyn.driver.new()` per Lua state.** Each driver now stores its callback
  functions and proxy reference independently using `luaL_ref`, so multiple drivers 
  can coexist in the same Lua state without interfering.

### Bug Fixes

#### Memory Leaks
- `luaLoadMacros`: `macParseDefns` pairs now freed after use
- `parseHelp`: buffer properly null-terminated; `ftell` error checked
- `myDatabaseHook`: `macEnvExpand` and `macParseDefns` results now freed; macro pairs 
  pointer reset for each hook invocation
- `luaSpawn`: lua_State closed on file-not-found and load-failure error paths
- `createTable` (luascriptRecord): data array freed on `dbGetLink` failure
- `luaLoadLibrary`: error message popped from stack on failure
- `luaPopScope`: stack imbalance fixed; crash prevented when no metatable exists
- `readInt32`/`readFloat64`/`readOctet` (asyn driver callbacks): nil return value 
  now always popped from the Lua stack
- `parseINPOUT`: broken Protocol no longer returned on script load failure; state 
  and Protocol cleaned up
- `db.entry()`: now returns full userdata with `__gc` (previously returned light
  userdata that leaked the DBENTRY)
- `db.record()`: `__gc` added to free DBENTRY on garbage collection

#### Thread Safety
- `shell_state` changed from a static global to thread-local storage 
  (`epicsThreadPrivateId`), preventing races between concurrent shell sessions
- `default_state` access protected with `epicsMutex`
- `previousFunctionHook` installation guarded to prevent infinite recursion on 
  repeated `luash` calls
- `shellStateId` lazily initialized to prevent segfault when `luash` is called
  before registrars fire

#### String and Null Safety
- `logError` (luascriptRecord): guards against `lua_tostring` returning NULL
- `luaLocateFile`: early return on empty filename (prevents `at(0)` exception)
- `luaLoadString`: NULL check without constructing `std::string(NULL)`
- `luaMacrosFromTable`: value duplicated before `lua_tostring` to prevent 
  corrupting `lua_next` iteration
- `l_pvgetval`/`l_pvsetval`: NULL guard on field name
- `l_caget`/`l_caput`: changed from `lua_tostring` to `luaL_checkstring` for 
  proper argument validation
- `strcpy` replaced with `strncpy` on `record->call` and `record->pcode`
- `devLuaBi`/`devLuaMbbi`/`devLuaStringin`: `lua_pop` moved after string 
  comparison to prevent dangling pointer

#### Logic Bugs
- `checkSvalUpdate` Transition_To_Non_zero: condition corrected from 
  `(!prev.empty() && !curr.empty())` to `(prev.empty() && !curr.empty())`
- `setLinks` callback setup moved outside the per-link loop to prevent 
  redundant callback requests and `wd_id_1_LOCK` reset issues
- Async thread names now use `"luascript:" + record->name` for unique 
  identification (previously truncated to "Script")
- `luashSetCommonState`: missing `return` after NULL name check

#### Device Support
- `devLuaLongin`/`devLuaStringin`: `return 2` changed to `return 0` (longin 
  and stringin record support do not give special meaning to return value 2)
- `parseINPOUT`: returns NULL on script load failure instead of a broken Protocol

#### Asyn Library
- `luaL_error`/`longjmp` no longer called from C++ catch blocks in `asyn_read`, 
  `asyn_write`, `asyn_writeread`, and `push_clientproxy`. Error messages are 
  captured inside the catch and `luaL_error` is called after the catch exits.
- `asyn_writeparam`: argument type validation moved before `asynUser` allocation
  to prevent resource leak on type errors
- `getIntegerParam`/`getDoubleParam`/`getStringParam`: return value checked 
  before calling `luaL_check*` to prevent accessing wrong stack position
- Octet read parameter buffer increased from 256 to 1024 bytes

#### Old Port Driver
- `readOctet`: output buffer now properly null-terminated
- `writeOctet`/`writeFloat64`/`writeInt32`: `callParamCallbacks()` called after
  successful writes so parameter changes propagate to clients

#### Record Support
- Asynchronous processing reworked to use EPICS callback system instead of 
  per-process thread creation, with proper `dbScanLock`/`dbProcess` completion
- `processCallback` stack leak fixed for non-incomplete syntax errors; 
  `lua_settop` safety net added
- `checkValUpdate` "On Change" now uses `fabs(pval - val) > 0.0` instead of 
  `MDEL` deadband (matches calcout behavior)
- `loadStrings` no longer returns early on first error; preserves first error 
  while processing all remaining string inputs (matches `loadNumbers` pattern)
- ERR field only cleared and posted when it contains a previous error message, 
  eliminating unnecessary monitor traffic
- `writeValue` no longer sets `pact = TRUE` on DSET error (was dead code)
- `lua_close(NULL)` guarded in `initState`
- `recGblGetTimeStamp` called on all process paths including early error returns
- `OUTV` field now defaults to "Ext PV OK" matching all input validity fields
- `printf` replaced with `errlogPrintf` for all error/diagnostic messages
- `checkLinks` and `setLinks` declared `static`
- Unused `#include <algorithm>` removed
- DBD pointer fields changed from `size(4)` to `size(8)` for 64-bit correctness

#### Other
- `all_records` (db library): `lua_call` changed to `lua_pcall` for controlled
  error handling
- Protocol char arrays increased from 62 to 256 bytes
- VLAs in `luaSoft.c` replaced with `malloc`/`free` for portability and to 
  prevent stack overflow with large arrays


Release 3-1
-----------

### Version Changes
- Lua language version updated to 5.4.6
- luaaa library version updated to 1.0

### Library Changes:

Included libraries now have extended capabilities when built with a compiler that 
supports C++11. To enable these capabilites, define CXX11_SUPPORT=YES in a 
CONFIG_SITE.local file. Changes are noted below.

#### asyn

- Added setOption function
- New include file, 'lasynlib.h' to allow external creation of lua asynPortDriver
and asynOctetClient classes

- **C++11 Only:** asynOctetClient and asynPortDriver changed to be proper c++ classes.
Use asynOctetClient.find and asynPortDriver.find to construct instead of client and driver
functions. 

#### epics

- New include file, 'lepicslib.h' to allow external creation of lua PV class
- sleep function deprecated

#### db

- All static database access functions implemented
- Can specify callback function that is triggered whenever a database is loaded

#### osi

- New library for os independent functions
- sleep function moved here


### Other Changes:

- print function now uses epicsStdoutPrintf behind the scenes to allow output redirection
- Added luaLoadLibrary function to trigger library load in a given lua_state


Release 3-0-2
-------------

- Various bug fixes relating to proper link handling
- Fixed another windows build issue relating to order of includes
- Enum input field support added, fields are grabbed as string value.


Release 3-0-1
-------------

-  Fix issues for windows builds.


Release 3-0
-----------

-  Lua language version updated to 5.4.0
-  "db" library added. Allows users to generate records within lua scripts
   during IOC startup.
-  luaPortDriver added. Generates an asynPortDriver based off of a lua
   script. Each parameter is defined within lua and links to a snippet
   of lua code, with the code being run whenever the parameter is read
   or written (depending on how the parameter is defined).
-  Named lua states added. Can create lua_States that can be shared between
   instances of lua scripts by name.
-  The command exit in the lua shell changed from a specially recognized
   word to a lua function so that it can be properly parsed within a chunk.
   Allows it to be used in a conditional or loop.
-  luascriptRecord AA-JJ inputs can now take in arrays and the record can
   also write out to array records.
-  luaShell.h API changed, luashBegin renamed to luashLoad to match IOC shell
   naming conventions, C++ overloads of luash command to allow for lua_States
   to be given to shell to set the environment.
-  Added functions to luaEpics.h to provide scoped environment variables,
   changed luaLoadMacros to use scopes.
-  Fixed an issue with too many temp files being created and deleted by the
   iocsh library.


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


Release 1-1
-----------

-  luash function now runs as a command shell or an interpreter
-  Lua asyn library function to access ports as lua objects
-  LUASH_PS1 variable for lua shell prompt

Release 1-0
-----------

-  First public release on github

| Suggestions and Comments to:  
| [Keenan Lang](mailto:klang@anl.gov)
