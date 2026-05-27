---
layout: default
title: Lua Port Drivers
nav_order: 6
---


# Lua Port Drivers
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}


Introduction
------------

Lua-based asynPortDrivers can be created entirely from Lua scripts,
with parameters defined declaratively and read/write callbacks bound
as Lua functions. There are two approaches:

1. **`asyn.driver.new`** (recommended) -- Create a driver using the asyn
   library's Lua API with type constructors, parameter proxies, and
   callback binding.

2. **`luaPortDriver` iocsh command** (legacy) -- Create a driver using
   a parameter DSL in a script file.

Both approaches produce a standard asynPortDriver that works with all
asyn device support record types (asynInt32, asynFloat64, asynOctet).


Creating a Driver
-----------------

### asyn.driver.new
---

```
asyn.driver.new (portName, paramTable [, initFunc])
```

Creates a new asynPortDriver with parameters defined in the parameter
table, and optional initialization code. Returns a driver proxy object.

The parameter table is an array of parameter specifications created using
the type constructors `asyn.Int32`, `asyn.Float64`, and `asyn.Octet`.
Each type constructor takes a parameter name and an optional default value:

```lua
local asyn = require("asyn")
local Int32, Float64, Octet = asyn.Int32, asyn.Float64, asyn.Octet

local drv = asyn.driver.new(PORT, {
    Float64 "TEMPERATURE" (0.0),
    Float64 "SETPOINT"    (25.0),
    Int32   "ENABLED"     (0),
    Octet   "STATUS"      ("OK"),
}, function(self)
    -- Initialization code: runs once at creation time
    -- 'self' is the driver proxy object
    self.port = asyn.client(DEVICE_PORT, 0, "")
    self.conversion = tonumber(CONV) or 1.0
end)
```

| Parameter  |   Type   | Description |
|------------|----------|-|
| portName   |  string  | The asyn port name for the new driver |
| paramTable |  table   | Array of parameter specs created with `asyn.Int32`, `asyn.Float64`, or `asyn.Octet` |
| initFunc   | function | Optional initialization function. Receives the driver proxy as its argument. |

<br>

### Type Constructors
---

```
asyn.Int32 "PARAM_NAME" [(default_value)]
asyn.Float64 "PARAM_NAME" [(default_value)]
asyn.Octet "PARAM_NAME" [(default_value)]
```

Create parameter specifications for use with `asyn.driver.new`. Each returns a
table describing the parameter's name and type. The optional parenthesized
default value sets the parameter's initial value.

```lua
local Int32, Float64, Octet = asyn.Int32, asyn.Float64, asyn.Octet

Int32   "COUNTER"              -- integer param, default 0
Float64 "TEMPERATURE" (20.0)   -- float param, default 20.0
Octet   "MESSAGE" ("Ready")    -- string param, default "Ready"
```


Parameter Access
----------------

### Parameter Proxy
---

When you access a parameter name on a driver proxy (e.g., `drv.TEMPERATURE`),
you get a parameter proxy object with the following properties:

```lua
drv.PARAM.value          -- read the current parameter value
drv.PARAM.value = x      -- write a new value + call param callbacks
drv.PARAM.name           -- the parameter name string
```

<br>

### driver:callParamCallbacks
---

```
driver:callParamCallbacks ()
```

Triggers asyn parameter callbacks on the driver. This is automatically called
when setting a parameter value via `drv.PARAM.value = x`, but can also be
called explicitly.

<br>

### driver:writeParam / driver:readParam
---

```
driver:writeParam (paramName, value)
driver:readParam (paramName)
```

Write or read a parameter value by name using the asyn parameter library.
These are convenience methods that work on both `find` and `new` drivers.

| Parameter |   Type   | Description |
|-----------|----------|-|
| paramName |  string  | The name of a parameter in the driver |
| value     |  varies  | The new value to write (for writeParam) |


Read and Write Callbacks
-------------------------

For drivers created with `asyn.driver.new`, you can bind read and write
callbacks to individual parameters. These callbacks are called when
asyn device support records read from or write to the parameter.

```lua
drv.PARAM.read = function(self)
    -- Called when a record reads this parameter
    -- 'self' is the driver proxy (access internal state via self.xxx)
    -- Return the value to send back to the reader
    return some_value
end

drv.PARAM.write = function(value, self)
    -- Called when a record writes to this parameter
    -- 'value' is the incoming value from the writer
    -- 'self' is the driver proxy
    drv.PARAM.value = value  -- store the value
end
```

Callbacks cannot be bound on drivers found with `asyn.driver.find` -- only
on drivers created with `asyn.driver.new`.

<br>

### Driver Internal State
---

The init function can set arbitrary fields on the driver proxy via `self`.
These fields persist and are accessible from all callbacks:

```lua
local drv = asyn.driver.new(PORT, {
    Float64 "READING" (0.0),
}, function(self)
    self.scale = 2.0
    self.offset = 10.0
end)

drv.READING.read = function(self)
    local raw = get_raw_value()
    return raw * self.scale + self.offset
end
```

Internal state fields do not conflict with parameter names. Parameters
are accessed via the parameter proxy (`drv.PARAM.value`), while internal
state is accessed directly (`self.fieldname`).


Finding Existing Drivers
------------------------

Existing asynPortDrivers (whether created from Lua or C++) can be
accessed using `asyn.driver.find`. See the
[asyn library documentation](libraries/asyn-library) for details.

```lua
local drv = asyn.driver("MYPORT")

-- Read a parameter value
print(drv.TEMPERATURE.value)

-- Write a parameter value (also calls callParamCallbacks)
drv.SETPOINT.value = 25.0
```

The parameter proxy, `callParamCallbacks`, `writeParam`, and `readParam`
methods documented above all work on drivers obtained via `find` as well.
Callback binding (`.read` / `.write`) is only available on drivers
created with `asyn.driver.new`.


Creating Records
----------------

Lua port drivers are typically paired with `db.record` calls to create
the asyn device support records that connect to the driver's parameters.
Using `luaRegisterState` and `luaLoadFile`, the driver, callbacks, and
records can all be defined in a single Lua file:

```lua
local asyn = require("asyn")
local db = require("db")
local Int32, Float64, Octet = asyn.Int32, asyn.Float64, asyn.Octet

luaRegisterState(PORT)

local drv = asyn.driver.new(PORT, {
    Float64 "TEMPERATURE" (0.0),
    Float64 "SETPOINT"    (25.0),
    Int32   "ERRORS"      (0),
    Octet   "STATUS"      ("OK"),
}, function(self)
    self.port = asyn.client(DEVICE_PORT, 0, "")
    self.port.InTerminator = "\r\n"
    self.port.OutTerminator = "\r\n"
    self.conversion = tonumber(CONV) or 1.0
end)

drv.TEMPERATURE.read = function(self)
    local response = self.port:writeread("READ:TEMP?")
    if response then
        drv.STATUS.value = "OK"
        return tonumber(response) * self.conversion
    end
    drv.ERRORS.value = drv.ERRORS.value + 1
    drv.STATUS.value = "Read error"
    return drv.TEMPERATURE.value
end

drv.SETPOINT.write = function(value, self)
    drv.SETPOINT.value = value
end

db.record("ai", P .. "Temperature") {
    DTYP = "asynFloat64",
    INP  = "@asyn(" .. PORT .. ",0,0)TEMPERATURE",
    SCAN = "I/O Intr",
    PINI = "1",
    EGU  = "degC",
    PREC = "2",
}

db.record("ao", P .. "Setpoint") {
    DTYP = "asynFloat64",
    OUT  = "@asyn(" .. PORT .. ",0,0)SETPOINT",
    EGU  = "degC",
}
```

Startup:
```lua
luaSpawn("device.lua", {P="dev1:", PORT="DEV1", DEVICE_PORT="serial1", CONV="0.01"})
luaSpawn("device.lua", {P="dev2:", PORT="DEV2", DEVICE_PORT="serial2", CONV="0.1"})
```

The `08-PortDriver` example IOC demonstrates this pattern.


Legacy: luaPortDriver iocsh Command
------------------------------------

The `luaPortDriver` iocsh command takes the name of an asyn port, a lua script,
and a string of macro definitions:

```
luaPortDriver("EXAMPLE", "exampleDriver.lua", "VAL=10")
```

An asynPortDriver is created with the given asyn port name and the lua script
is run with the defined macro values. Within the script, parameters can be
implemented using the following convention:

```
param.<param_type> "<NAME>"
```

The parameter type can be any of Int32, Float64, or Octet, each corresponding
to the equivalent asynParamType that shares their name. You can also use
String as an alternative for an Octet definition. None of these definitions
are case sensitive. Name defines the name that the parameter is created with.

The above, basic definition of a parameter only creates a correctly typed
parameter in the port driver. Values can be written or read from the parameter,
but nothing else is actually done. Instead, a slightly more advanced form is
used to bind lua code to reading and writing callbacks.

```
param.<param_type>.<read/write> "NAME" [[ 
    CODE 
]]
```

The same parameter type and name conventions remain, but the definition now
also includes a specifier on whether you are providing a function for the 
reading of a parameter or the writing of a parameter. The code is then lua
code as a multi-line string. 

Implicitly defined within the code is a variable named "self". This is a
driver proxy object representing the luaPortDriver you are creating.
This is useful for being able to read from and write to other parameters
in the driver during execution. As well, for code that implements a write
callback, you also have the variable "value" that contains the value to
write coming from the asyn callback. For read callbacks, a value that is
returned by the code will be written out to the value parameter in the 
asyn callback.

Put together, here is a small example of a functional luaPortDriver for a simple
calculation of the length of a hypotenuse:

```lua
param.int32 "BASE"
param.int32 "SIDE"

param.float64.read "HYPOTENUSE" [[
    return math.sqrt(self["BASE"]^2 + self["SIDE"]^2)
]]
```
