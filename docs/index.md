---
layout: default
title: Home
nav_order: 1
---


# Lua EPICS Module

The Lua EPICS Module embeds the Lua language interpreter into an EPICS
IOC, providing a scriptable alternative to the traditional ioc shell and
calcout/scalcout record types. Lua scripts can be used for IOC startup,
record processing logic, device communication, and asynPortDriver
creation.

The module uses Lua version 5.4.6. A reference manual describing the
details of the language can be [found here](https://www.lua.org/manual/5.4/).


Adding lua to an IOC
--------------------

Point your IOC's `configure/RELEASE` at the built lua module:

```
# configure/RELEASE
LUA = /path/to/modules/lua
```

Then make the support library and its database definition available to
the IOC application. There are two ways to wire this up in the IOC's
`src/Makefile`.

**Automatic dependencies (recommended).** Include the module's
`CONFIG_MODULE` from `configure/CONFIG_SITE` (or `RELEASE`) and use the
variables it provides. This pulls in the correct DBD, library, and their
dependencies (such as asyn) automatically:

```makefile
# configure/CONFIG_SITE
-include $(LUA)/cfg/CONFIG_MODULE
```

```makefile
# xxxApp/src/Makefile
xxx_DBD  += $(LUA_IOC_DBDS)
xxx_LIBS += $(LUA_IOC_LIBS)
```

**Explicit form.** Name the DBD and library directly. The lua library
depends on asyn, so list asyn as well:

```makefile
# xxxApp/src/Makefile
xxx_DBD  += luaSupport.dbd
xxx_LIBS += lua
xxx_LIBS += asyn
```

{: .note }
> The pure-Lua libraries (`bytestream`, `seq`, and `re`) are installed to
> the module's `lib/<arch>/` directory. Call `luaAddModule("$(LUA)")` in
> your startup script so `require()` can find them. See
> [Adding Additional Libraries](libraries/adding-libraries) for details.


Example IOCs
------------

A full, numbered set of runnable example IOCs lives in
`iocs/iocLuaExample/iocBoot/`. Each is a self-contained startup script;
build the example application and run one with the `testLuaShell` binary,
e.g. from inside an example directory:

```
../../bin/<arch>/testLuaShell st.lua
```

The examples are ordered as a learning progression:

| Example | Demonstrates |
|---------|--------------|
| `01-LuaShell` | Using the Lua shell as a replacement for the ioc shell. |
| `02-LuascriptRecord` | The luascript record with inline code and script files. |
| `03-ArrayHandling` | Array/waveform inputs and table (array) output. |
| `04-AsynLibrary` | Communicating with asyn ports via the asyn library. |
| `05-EpicsLibrary` | Reading and writing PVs with the epics library. |
| `06-DatabaseLibrary` | Creating and inspecting records with the db library. |
| `07-EventLibrary` | Inter-thread signaling with the event library. |
| `08-LuaDTYPSupport` | DTYP "lua" device support callbacks. |
| `09-PortDriver` | Creating an asynPortDriver from Lua. |
| `10-StreamCommunications` | Structured device I/O with the bytestream library. |
| `11-Sequencer` | State machines with the seq library. |


Lua Shell
---------

The Lua shell is an alternative to the ioc shell for IOC startup scripts
and interactive use. It can call all iocsh-registered functions directly,
while also providing variables, conditionals, loops, and functions within
startup scripts. The shell can be used alongside or as a complete
replacement for the ioc shell.

[Full Documentation](using-lua-shell)


luascript Record
----------------

The luascript record type provides scriptable record behavior, similar
to the calcout or scalcout record. Each time the record processes, Lua
code is executed and any returned value is stored in the record's output
fields. The record supports double and string input links exposed as
global variables, external Lua script files, conditional processing via
POPT/PCAL, and asynchronous execution.

[Full Documentation](luascriptRecord)


Device Support
--------------

DTYP "lua" device support allows standard EPICS record types to use Lua
callback functions for their read and write operations. Supported record
types include ai, ao, bi, bo, longin, longout, mbbi, mbbo, stringin,
and stringout. Each callback receives a PV object for accessing the
record's fields, and errors in the callback are reported as record
alarms.

[Full Documentation](device-support)


Included Libraries
------------------

In addition to the standard Lua libraries, several EPICS-specific
libraries are available via `require()`:

- **epics** -- Read and write PV values with local-PV fast path, array
  support, and PV proxy objects.
- **db** -- Create and inspect EPICS database records from Lua,
  replacing .db files and substitution files.
- **asyn** -- Communicate with asyn ports, access parameters, and create
  asynPortDrivers with read/write callbacks.
- **bytestream** -- Scanf-style parsing and printf-style formatting for
  byte stream device communication.
- **event** -- Synchronization primitives for inter-thread signaling.
- **seq** -- State machine sequencer, a Lua alternative to SNL.
- **iocsh** -- Access environment variables and iocsh-registered
  functions from Lua.
- **osi** -- OS-independent utilities: sleep and stdout redirection.
- **lpeg** -- Parsing Expression Grammars for Lua (LPeg 1.1.0).

Custom libraries can be added to extend the Lua environment.

[Library Documentation](libraries/epics-functions)


Lua Port Drivers
-----------------

Lua-based asynPortDrivers can be created entirely from Lua scripts using
the `asyn.driver.new` API. Parameters are defined with type constructors,
and read/write callbacks are bound as Lua functions. A legacy
`luaPortDriver` iocsh command is also available.

[Full Documentation](luaPortDriver)
