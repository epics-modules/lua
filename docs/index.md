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
- **iocsh** -- Access environment variables and iocsh-registered
  functions from Lua.
- **osi** -- OS-independent utilities: sleep and stdout redirection.
- **lpeg** -- Parsing Expression Grammars for Lua (LPeg 1.1.0).

Custom libraries can be added to extend the Lua environment.

[Library Documentation](libraries/epics-functions)


luaPortDriver
-------------

Lua-based asynPortDrivers can be created entirely from Lua scripts using
the `asyn.driver.new` API. Parameters are defined with type constructors,
and read/write callbacks are bound as Lua functions. A legacy
`luaPortDriver` iocsh command is also available.

[Full Documentation](luaPortDriver)
