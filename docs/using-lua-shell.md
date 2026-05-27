---
layout: default
title: Lua Shell
nav_order: 3
---


# Using the Lua Shell
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}


Introduction
------------

The Lua shell is an alternative to the ioc shell for IOC startup scripts
and interactive use. It can be invoked from an iocsh startup script or
used as the startup program directly.

The shell is designed for compatibility with iocsh-style startup scripts.
The global environment is set up so that name lookups fall through to
EPICS environment variables and iocsh-registered functions. This means
iocsh functions can be called directly without any special syntax:

```
luash> EPICS_VERSION_MAJOR
7
luash>
luash> epicsEnvShow("EPICS_VERSION_MAJOR")
EPICS_VERSION_MAJOR=7
```

### Hash Comments

The directive `#ENABLE_HASH_COMMENTS` configures the shell to accept
iocsh-style `#` comments. This only applies to lines where `#` is the
first non-whitespace character -- it does not affect the use of `#` for
table length in normal Lua expressions.

```
luash> #ENABLE_HASH_COMMENTS
Accepting iocsh-style comments
luash>
luash> #print("This won't print")
luash> print(#"Check len")
9
```

Comments starting with `#-` are silent -- they are not echoed when
running scripts in file mode. This matches the iocsh convention:

```lua
#ENABLE_HASH_COMMENTS
# This comment will be echoed
#- This comment will be silent
```

Blank lines in file mode are also elided from the output.


info() Function
---------------

The `info()` function is available in all Lua states for discovering
available functions and methods. It can be called on any library table
or userdata object:

```
luash> info(epics)
  .get(PV [, timeout | {timeout, count, string}])
  .put(PV, value [, timeout | {timeout}])
  .pv(PV) -- create PV proxy object

luash> pv = epics.pv("IOC:m1")
luash> info(pv)
  .name                       -- PV name (property)
  .FIELD                      -- read field value
  .FIELD = value              -- write field value
  :get(field [, {timeout, count, string}])
  :put(field, value [, {timeout}])
```

Calling `info()` with no arguments prints usage help. Calling it with
a nil argument reports that the input is nil, which helps diagnose cases
where a library hasn't been loaded yet.


Shell Commands
--------------

### luash
---

```
luash "file.lua" ["macros"]
```

Runs a Lua script in the calling shell's state. The script shares
variables, loaded modules, and function definitions with the shell
session.

Macros are set as global variables in the shell's state. They use the
same `"KEY=val,KEY2=val2"` format as iocsh. Strings must be quoted;
unquoted values are interpreted as numbers:

```
luash "config.lua" "P='dev1:',PORT='serial1',ADDR=0"
```

When called from Lua, table macros are also accepted:

```lua
luash("config.lua", {P="dev1:", PORT="serial1", ADDR=0})
```

The file is located by searching `LUA_SCRIPT_PATH`, paths registered
via `luaAddPath`/`luaAddModule`, and the current directory. If no
filename is given, the shell reads from standard input with a prompt
set by `LUASH_PS1`.

**File mode:** When running a script file, each statement is executed as
it is read, with lines echoed and output interleaved (matching iocsh
behavior). Multi-line constructs such as function definitions, if blocks,
and loops are accumulated until the statement is complete. Return values
from expressions are printed automatically.

**Include directive:** The `<` command includes another script at the
current point:

```
< other_script.lua
```

**Exit:** A line containing only `exit` ends the current script and
returns to the caller.

<br>

### luaLoadFile
---

```
luaLoadFile "file.lua" ["macros"]
```

Loads and executes a Lua script in a **new state**. The entire file is
compiled as a single chunk, so `local` variables work across lines.
Execution is synchronous -- the command blocks until the script
completes.

```lua
luaLoadFile("driver.lua", {P="dev1:", PORT="SENSOR1"})
```

The new state has access to all registered libraries and paths. If the
script calls `luaRegisterState`, the state is kept alive after execution
(see Named States below). Otherwise, the state is closed when the script
finishes.

<br>

### luaSpawn
---

```
luaSpawn "file.lua" ["macros"]
```

Loads and executes a Lua script in a **new state** on a background
thread. The command returns immediately. The file is compiled as a
single chunk.

```lua
luaSpawn("tick.lua", "INTERVAL=1.0")
```

Commonly used for long-running scripts such as device polling loops
or port driver definitions.

<br>

### luaCmd
---

```
luaCmd "lua code" ["macros"]
```

Executes a single Lua statement in a **new state**. The state is closed
after the statement completes.

```lua
luaCmd("print(P .. ' started')", {P="dev1:"})
```

<br>

### Command Comparison
---

| Command | Runs in | Execution | Line echoing |
| ------- | ------- | --------- | ------------ |
| `luash "file"` | Calling shell | Synchronous, line-by-line | Yes |
| `< file` | Calling shell | Synchronous, line-by-line | Yes |
| `luaLoadFile "file"` | New state | Synchronous, whole file | No |
| `luaSpawn "file"` | New state, background thread | Whole file | No |
| `luaCmd "code"` | New state | Synchronous, single statement | No |

Commands that run in the **calling shell** share variables, loaded
modules, and function definitions with the shell session. Commands
that run in a **new state** start fresh -- pass configuration via
macros.

Because `luash` executes each line as a separate statement, `local`
variables do not persist between lines. Use global variables, or use
`luaLoadFile` which compiles the entire file as one chunk.


Named States
------------

By default, a Lua state created by `luaLoadFile` or `luaSpawn` is
closed when the script finishes. A **named state** is one that has been
registered with a name, making it persistent and referenceable from
other parts of the IOC.

### Creating a Named State

Call `luaRegisterState` from within a script to register the current
state under a name:

```lua
-- Inside a script loaded via luaLoadFile or luaSpawn
luaRegisterState("mydevice")
```

The state will not be closed when the script finishes. All global
variables and function definitions in the state remain available.

### Referencing Named States

Named states are referenced using the `@name` syntax in luascript
record CODE fields and DTYP device support INP/OUT fields. When the
name after `@` does not resolve to a file on disk, it is looked up
as a named state:

```lua
-- In a script loaded with luaLoadFile:
luaRegisterState(PORT)

function read_temperature()
    return client:write("MEAS:TEMP?"):read("%f")
end
```

```lua
-- Record references the named state and function:
db.record("ai", P .. "temperature") {
    DTYP = "lua",
    INP  = "@" .. PORT .. " read_temperature()",
    SCAN = "1 second",
}
```

### Typical Pattern

The most common pattern is a single file loaded via `luaLoadFile` that:

1. Registers its state under a unique name (typically a port name or prefix)
2. Creates records using `db.record`
3. Defines the callback functions those records reference

```lua
-- device.lua: loaded before iocInit via luaLoadFile
local db = require("db")

luaRegisterState(PORT)

-- ... set up clients, define functions ...

db.record("ai", P .. "reading") {
    DTYP = "lua",
    INP  = "@" .. PORT .. " read_value()",
}

function read_value()
    -- this function runs in the named state
    return get_reading()
end
```

```lua
-- st.lua: startup script
luaLoadFile("device.lua", {P="dev1:", PORT="DEV1"})
luaLoadFile("device.lua", {P="dev2:", PORT="DEV2"})
iocInit()
```

Each call to `luaLoadFile` creates a separate named state, so the two
instances do not interfere with each other.


luaAddPath / luaAddModule
-------------------------

The `luaAddPath` and `luaAddModule` commands register directories for
both `require()` (Lua's module loader) and script file resolution (used
by `luaLoadFile`, `luaSpawn`, luascript `@file`, and DTYP `@file`).

### luaAddPath

```
luaAddPath "directory"
```

Adds a single directory to the search paths. The directory is used for:

- `require()`: appends `dir/?.lua` and `dir/?/init.lua` to `package.path`,
  and `dir/?.so` (or `dir/?.dll` on Windows) to `package.cpath`
- Script loading: the directory is searched after `LUA_SCRIPT_PATH`
  directories but before the current directory

```lua
luaAddPath("/usr/local/share/lua/5.4")

local mylib = require("mylib")  -- finds /usr/local/share/lua/5.4/mylib.lua
```

Duplicate paths are silently ignored. When called from Lua, the path
takes effect immediately in the calling state. When called from iocsh,
the path takes effect for all future Lua states.

### luaAddModule

```
luaAddModule "module_top"
```

Convenience wrapper for EPICS modules. Reads the `EPICS_HOST_ARCH`
environment variable and adds two directories via `luaAddPath`:

- `module_top/lib/<arch>/` -- for libraries installed via `LIB_INSTALLS`
- `module_top/bin/<arch>/` -- for scripts installed via `SCRIPTS` or `BIN_INSTALLS`

```lua
-- From a Lua startup script:
luaAddModule("../..")
luaAddModule(MYMODULE)

local bs = require("bytestream")
```

```
# From iocsh:
< envPaths
luaAddModule("$(LUA)")
luaAddModule("$(MYMODULE)")
```

If `EPICS_HOST_ARCH` is not set, `luaAddModule` prints a warning and
does nothing.

### Path Search Order

Paths are searched in the order they are registered:

```lua
luaAddPath("/first")
luaAddPath("/second")
-- package.path: /first/?.lua;...;/second/?.lua;...;<defaults>
-- script search: LUA_SCRIPT_PATH dirs, /first/, /second/, .
```
