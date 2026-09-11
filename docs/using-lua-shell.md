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

All run/load commands share the same argument shape:

```
command "target" ["macros"] ["options"]
```

- **target** -- a Lua code string (`luaRunString`) or a script filename
  (`luaRunFile`, `luaShell`).
- **macros** -- a `"KEY=val,KEY2=val2"` string (strings quoted, unquoted
  values interpreted as numbers/booleans), set as global variables in
  the target state. When called from Lua, a table (`{P="dev1:"}`) is
  also accepted.
- **options** -- a `"key=val"` string of execution-control flags (same
  grammar as macros). Currently only `async` is defined, for
  `luaRunFile`. From Lua a table (`{async=true}`) is also accepted.

### luaRunString
---

Execute a string of Lua code as a single chunk, printing the result of
the final expression.

```
luaRunString "lua code" ["macros"]
```

```lua
luaRunString("print(P .. ' started')", {P="dev1:"})
```

Runs in a **new state** which is closed after the code completes.

**Returns:** nothing on success, error string on failure.

<br>

### luaRunFile
---

Load and execute a Lua script file in a new state.

```
luaRunFile "file.lua" ["macros"] ["options"]
```

The entire file is compiled as a single chunk, so `local` variables
work across lines. By default execution is **synchronous** (the command
blocks until the script completes). With the option `async=true` the
script runs in a **background thread** and the command returns
immediately.

```lua
luaRunFile("driver.lua", {P="dev1:", PORT="SENSOR1"})
luaRunFile("tick.lua", nil, {async=true})     -- background thread
```

```
# iocsh: async as a keyword option
luaRunFile "tick.lua", "", "async=true"
```

The new state has access to all registered libraries and paths. If the
script calls `luaNameState`, the state is kept alive after execution
(see Named States below). Otherwise, the state is closed when the
script finishes. `async=true` is commonly used for long-running scripts
such as device polling loops or port-driver definitions.

**Returns:** nothing on success, error string on failure.

<br>

### luaShell
---

Run a Lua script in the calling shell's state, line-by-line
(interactive/REPL style).

```
luaShell "file.lua" ["macros"]
```

The script shares variables, loaded modules, and function definitions
with the shell session. If no filename is given, the shell reads from
standard input with a prompt set by `LUASH_PS1`.

```lua
luaShell("config.lua", {P="dev1:", PORT="serial1", ADDR=0})
```

The file is located by searching `LUA_SCRIPT_PATH`, paths registered
via `luaAddPath`/`luaAddModule`, and the current directory.

**File mode:** each statement is executed as it is read, with lines
echoed and output interleaved (matching iocsh behavior). Multi-line
constructs (function definitions, if blocks, loops) are accumulated
until the statement is complete. Return values from expressions are
printed automatically. Because each line is a separate chunk, `local`
variables do **not** persist between lines -- use global variables, or
use `luaRunFile` which compiles the whole file as one chunk.

**Include directive:** the `<` command includes another script at the
current point:

```
< other_script.lua
```

**Exit:** a line containing only `exit` ends the current script and
returns to the caller.

**Returns:** nothing on success, error string on failure.

<br>

### Command Comparison
---

| Command | Runs in | Execution | Line echoing |
| ------- | ------- | --------- | ------------ |
| `luaShell "file"` | Calling shell | Synchronous, line-by-line | Yes |
| `< file` | Calling shell | Synchronous, line-by-line | Yes |
| `luaRunFile "file"` | New state | Synchronous, whole file | No |
| `luaRunFile "file","","async=true"` | New state, background thread | Whole file | No |
| `luaRunString "code"` | New state | Synchronous, single chunk | No |

Commands that run in the **calling shell** share variables, loaded
modules, and function definitions with the shell session. Commands
that run in a **new state** start fresh -- pass configuration via
macros.

### Deprecated command names
---

The following older names still work but are **deprecated**; each emits
a one-time warning on first use naming its replacement. Migrate to the
canonical names above.

| Deprecated | Use instead |
| ---------- | ----------- |
| `luash` / `luashLoad` | `luaShell` |
| `luaLoadFile` | `luaRunFile` |
| `luaSpawn` | `luaRunFile` with option `async=true` |
| `luaCmd` | `luaRunString` |
| `luaRegisterState` | `luaNameState` |


Named States
------------

By default, a Lua state created by `luaRunFile` is
closed when the script finishes. A **named state** is one that has been
registered with a name, making it persistent and referenceable from
other parts of the IOC.

### Creating a Named State

Call `luaNameState` from within a script to register the current
state under a name:

```lua
-- Inside a script loaded via luaRunFile
luaNameState("mydevice")
```

The state will not be closed when the script finishes. All global
variables and function definitions in the state remain available.

### Referencing Named States

Named states are referenced using the `@name` syntax in luascript
record CODE fields and DTYP device support INP/OUT fields. When the
name after `@` does not resolve to a file on disk, it is looked up
as a named state:

```lua
-- In a script loaded with luaRunFile:
luaNameState(PORT)

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

The most common pattern is a single file loaded via `luaRunFile` that:

1. Registers its state under a unique name (typically a port name or prefix)
2. Creates records using `db.record`
3. Defines the callback functions those records reference

```lua
-- device.lua: loaded before iocInit via luaRunFile
local db = require("db")

luaNameState(PORT)

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
luaRunFile("device.lua", {P="dev1:", PORT="DEV1"})
luaRunFile("device.lua", {P="dev2:", PORT="DEV2"})
iocInit()
```

Each call to `luaRunFile` creates a separate named state, so the two
instances do not interfere with each other.


luaAddPath / luaAddModule
-------------------------

The `luaAddPath` and `luaAddModule` commands register directories for
both `require()` (Lua's module loader) and script file resolution (used
by `luaRunFile`, luascript `@file`, and DTYP `@file`).

### luaAddPath
---

Add a directory to the library and script search paths.

```
luaAddPath "directory"
```

The directory is used for:

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
---

Register an EPICS module's library and script directories.

```
luaAddModule "module_top"
```

Reads the `EPICS_HOST_ARCH`
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
