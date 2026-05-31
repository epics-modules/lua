---
layout: default
title: Included Libraries
nav_order: 7
has_children: true
---


# Included Libraries

In addition to the standard Lua libraries, the following libraries are
available in the Lua runtime to help with common EPICS tasks. Each
library is loaded on demand via `require()`:

```lua
local epics = require("epics")
local db    = require("db")
local asyn  = require("asyn")
```

The built-in C libraries (epics, db, asyn, osi, iocsh, lpeg) are
available in every Lua state without any path configuration. Pure Lua
libraries (bytestream, re) are installed to the module's `lib/<arch>/`
directory and require `luaAddModule` to be called first.


EPICS Libraries
---------------

**asyn** -- [Documentation](asyn-library) --
Communicate with asyn ports via octet read/write, get and set
asynPortDriver parameters, create new asynPortDrivers from Lua
with `asyn.driver.new`, and manage client connections with
`asyn.client`.

**db** -- [Documentation](database-library) --
Create and inspect EPICS database records from Lua using
`db.record`, `db.loadRecords`, and `db.loadTemplate`. Provides
static database access functions and record enumeration with
`db.list`.

**epics** -- [Documentation](epics-library) --
Read and write PV values with `epics.get` and `epics.put`,
including array support and an options table for timeout, count,
and string formatting. Create PV proxy objects with `epics.pv`
for convenient dot-syntax field access.

**iocsh** -- [Documentation](iocsh-library) --
Access environment variables and iocsh-registered functions from
Lua. In the Lua shell, this functionality is embedded into the
global environment automatically.

**bytestream** -- [Documentation](bytestream-library) --
Scanf-style parsing (`bytestream.match`) and printf-style formatting
(`bytestream.format`) for byte stream device communication.
Includes a `bytestream.client` wrapper around `asyn.client` for
structured write/read I/O with format specifiers following
StreamDevice conventions.

**event** -- [Documentation](event-library) --
Synchronization primitives for inter-thread communication.
Provides event flags that can be shared across Lua states by
name, with set, clear, test, testAndClear, and wait operations.

**osi** --
OS-independent utilities provided by the EPICS OSI layer.
Functions: `osi.sleep(seconds)`, `osi.startRedirectOut(filename)`,
`osi.endRedirectOut()`.


Pattern Matching Libraries
--------------------------

**lpeg** --
The LPeg 1.1.0 library (Parsing Expression Grammars for Lua) is
compiled into the module and available via `require("lpeg")`.
LPeg is used internally by the bytestream library.
See the [LPeg documentation](http://www.inf.puc-rio.br/~roberto/lpeg/)
for details.

**re** --
LPeg's regex-style interface, available via `require("re")`.
Installed to `lib/<arch>/re.lua`.


Custom Libraries
----------------

You can add your own Lua or C libraries to extend the environment.
See [Adding Additional Libraries](adding-libraries) for details on
registering libraries and configuring search paths.
