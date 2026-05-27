---
layout: default
title: Device Support
nav_order: 5
---

# DTYP "lua" Device Support
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}


Introduction
------------

DTYP "lua" device support allows standard EPICS record types to use Lua
callback functions for their read and write operations. This provides a
way to interface with hardware or perform custom logic without writing
C code or creating a custom record type.

The Lua function is specified in the record's INP or OUT field, and is
called each time the record processes.


Supported Record Types
----------------------

| Record Type | Direction | INP/OUT | Return Type |
| - | - | - | - |
| ai | Input | INP | number (double) |
| ao | Output | OUT | nil |
| bi | Input | INP | number or string label (ZNAM/ONAM) |
| bo | Output | OUT | nil |
| longin | Input | INP | integer |
| longout | Output | OUT | nil |
| mbbi | Input | INP | number or string label |
| mbbo | Output | OUT | nil |
| stringin | Input | INP | string |
| stringout | Output | OUT | nil |


INP/OUT Field Format
--------------------

The INP or OUT field specifies the Lua script and function to call:

```
field(INP, "@<source> <function>([params])")
field(OUT, "@<source> <function>([params])")
```

The source can be either a **script file** or a **named state**:

### File-based

```
field(INP, "@myscript.lua read_value(10)")
```

The file is located using `LUA_SCRIPT_PATH` and any paths registered via
`luaAddPath`/`luaAddModule`. A new Lua state is created and the file is
loaded into it. All records that reference the same file share the same
Lua state.

### Named state

```
field(INP, "@MYSTATE read_value()")
```

If the source name does not resolve to a file on disk, it is treated as
the name of a Lua state registered with `luaRegisterState`. This allows
records to call functions in a state created by `luaLoadFile`, sharing
variables, upvalues, and module-level state with the code that created
the records.

This is the recommended pattern when using `db.record` from Lua to
create records and define their callbacks in the same file:

```lua
luaRegisterState(PORT)

local client = bs.client(PORT)

db.record("ai", P .. "temperature") {
    DTYP = "lua",
    INP  = "@" .. PORT .. " read_temp()",
    SCAN = "1 second",
}

function read_temp()
    return client:write("MEAS:TEMP?"):read("%f")
end
```


Callback Functions
------------------

Every callback function receives a **PV object** as its first argument,
followed by any parameters specified in the INP/OUT field. The PV object
represents the record being processed and provides dot-syntax access to
its fields.

### Input records

Input callbacks return a value that becomes the record's reading. The
type of the return value should match what the record expects:

```lua
-- ai: return a number
function read_temperature(record)
    return 25.3
end

-- longin: return an integer
function read_count(record)
    return math.tointeger(os.time())
end

-- stringin: return a string
function read_status(record)
    return os.date("%Y-%m-%d %H:%M:%S")
end

-- bi: return a string matching ZNAM or ONAM
function read_enable(record)
    if some_condition then
        return "Enabled"
    else
        return "Disabled"
    end
end
```

Returning `nil` from an input callback leaves the record's value
unchanged.

### Output records

Output callbacks read the value being written from `record.VAL` (or
`record.RVAL` for multi-bit records). They should return `nil` to
indicate success:

```lua
-- ao: read the value being written
function on_setpoint(record)
    print("New setpoint: " .. record.VAL .. " " .. record.EGU)
end

-- stringout: read the string being written
function on_command(record)
    print("Command: " .. record.VAL)
end
```

### Parameters

Parameters specified in the INP/OUT field are passed as additional
arguments after the PV object:

```
field(INP, "@myscript.lua read_channel(3, 'volts')")
```

```lua
function read_channel(record, channel, unit)
    -- channel = 3, unit = "volts"
    return get_reading(channel)
end
```


The PV Object
-------------

The PV object passed to callbacks is the same type of object created by
`epics.pv()`. It provides dot-syntax access to record fields:

```lua
function my_callback(record)
    -- Read fields
    local val  = record.VAL
    local desc = record.DESC
    local egu  = record.EGU
    local sevr = record.SEVR

    -- The .name property returns the record name
    print(record.name)    -- e.g., "IOC:sensor:temperature"
end
```

Field writes are also supported via dot syntax (`record.VAL = x`),
though for output records the value is typically read rather than
written by the callback.


Error Handling
--------------

If a callback function raises an error (via `error()` or a runtime
fault), the record is placed into alarm. For input records, a
`READ_ALARM` with `INVALID` severity is set. For output records, a
`WRITE_ALARM` with `INVALID` severity is set. The error message is
printed to the IOC console.

This means library functions that raise errors on failure (such as
`bytestream.client:read()` or `asyn.client:write()`) will automatically
put the calling record into alarm if the operation fails, without
requiring explicit error handling in the callback.


Examples
--------

### Simple file-based callbacks

**dtyp.db:**
```
record(ai, "$(P)counter") {
    field(DTYP, "lua")
    field(INP,  "@dtyp.lua next_int(10)")
}

record(bi, "$(P)toggle") {
    field(DTYP, "lua")
    field(INP,  "@dtyp.lua next_bool")
    field(ZNAM, "Off")
    field(ONAM, "On")
}
```

**dtyp.lua:**
```lua
function next_int(record, step)
    return record.VAL + step
end

function next_bool(record)
    if record.VAL == 0 then return "On"
    else                    return "Off"
    end
end
```

The `07-LuaDTYPSupport` example IOC demonstrates this pattern with
additional record types.

### Named state with bytestream client

The `09-StreamCommunications` example IOC demonstrates using named
states with `bytestream.client` for structured device I/O. Each load
of the device script creates a separate Lua state registered under the
port name, allowing multiple device instances without collisions:

```lua
luaRegisterState(PORT)

local client = bs.client(PORT)
client.OutTerminator = "\n"
client.InTerminator  = "\n"

db.record("ai", P .. "temperature") {
    DTYP = "lua",
    INP  = "@" .. PORT .. " read_temp()",
    SCAN = "1 second",
    EGU  = "degC",
    PREC = "2",
}

function read_temp()
    return client:write("MEAS:TEMP?"):read("%f")
end
```

**Startup:**
```lua
luaAddModule("$(LUA)")
drvAsynIPPortConfigure("SENSOR1", "192.168.1.100:5025")
luaLoadFile("device.lua", {P="dev1:", PORT="SENSOR1"})

drvAsynIPPortConfigure("SENSOR2", "192.168.1.101:5025")
luaLoadFile("device.lua", {P="dev2:", PORT="SENSOR2"})
```
