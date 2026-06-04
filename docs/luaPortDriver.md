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

Create a new asynPortDriver with Lua callbacks.

```
asyn.driver.new (portName, paramTable [, initFunc])
```

Creates a driver with parameters defined in the parameter table and
optional initialization code.

```lua
local asyn = require("asyn")
local Int32, Float64, Octet = asyn.Int32, asyn.Float64, asyn.Octet

local drv = asyn.driver.new(PORT, {
    Float64 "TEMPERATURE" (0.0),
    Float64 "SETPOINT"    (25.0),
    Int32   "ENABLED"     (0),
    Octet   "STATUS"      ("OK"),
}, function(self)
    self.port = asyn.client(DEVICE_PORT, 0, "")
    self.conversion = tonumber(CONV) or 1.0
end)
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | The asyn port name for the new driver. |
| paramTable | table | Array of parameter specs from `asyn.Int32`, `asyn.Float64`, or `asyn.Octet`. |
| initFunc | function | Optional. Initialization function, receives the driver proxy as its argument. |

**Returns:** a driver proxy object.

<br>

### Type Constructors
---

Create parameter specifications for `asyn.driver.new`.

```
asyn.Int32 "PARAM_NAME" [(default_value)]
asyn.Float64 "PARAM_NAME" [(default_value)]
asyn.Octet "PARAM_NAME" [(default_value)]
```

The optional parenthesized default value sets the parameter's initial
value.

```lua
Int32   "COUNTER"              -- integer param, default 0
Float64 "TEMPERATURE" (20.0)   -- float param, default 20.0
Octet   "MESSAGE" ("Ready")    -- string param, default "Ready"
```

**Returns:** a parameter specification table.


Parameter Access
----------------

### Parameter Proxy
---

Access parameters through the driver proxy by name.

When you access a parameter name on a driver proxy (e.g.,
`drv.TEMPERATURE`), you get a parameter proxy object:

```lua
drv.PARAM.value          -- read the current parameter value
drv.PARAM.value = x      -- write a new value + call param callbacks
drv.PARAM.name           -- the parameter name string
```

<br>

### driver:callParamCallbacks
---

Trigger asyn parameter callbacks on the driver.

```
driver:callParamCallbacks ()
```

This is called automatically when setting a parameter value via
`drv.PARAM.value = x`, but can also be called explicitly.

<br>

### driver:writeParam / driver:readParam
---

Read or write a parameter value by name.

```
driver:writeParam (paramName, value)
driver:readParam (paramName)
```

Convenience methods that work on both `find` and `new` drivers.

| Parameter | Type | Description |
| - | - | - |
| paramName | string | The name of a parameter in the driver. |
| value | varies | The new value to write (for writeParam). |

**Returns:** the parameter value (for readParam).


Read and Write Callbacks
-------------------------

### Binding Callbacks
---

Bind Lua functions to parameter read/write events.

For drivers created with `asyn.driver.new`, you can bind callbacks
to individual parameters. These are called when asyn device support
records read from or write to the parameter.

```lua
drv.PARAM.read = function(self)
    -- 'self' is the driver proxy
    -- Return the value to send back to the reader
    return some_value
end

drv.PARAM.write = function(value, self)
    -- 'value' is the incoming value from the writer
    -- 'self' is the driver proxy
    drv.PARAM.value = value
end
```

{: .note }
> Callbacks can only be bound on drivers created with
> `asyn.driver.new`, not on drivers found with `asyn.driver.find`.

<br>

### Driver Internal State
---

Store persistent state on the driver proxy.

The init function can set arbitrary fields on the driver proxy via
`self`. These fields persist and are accessible from all callbacks:

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

print(drv.TEMPERATURE.value)
drv.SETPOINT.value = 25.0
```

The parameter proxy, `callParamCallbacks`, `writeParam`, and `readParam`
documented above all work on drivers obtained via `find` as well.


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

The `09-PortDriver` example IOC demonstrates this pattern.


Legacy: luaPortDriver iocsh Command
------------------------------------

The `luaPortDriver` iocsh command takes the name of an asyn port, a
Lua script, and a string of macro definitions:

```
luaPortDriver("EXAMPLE", "exampleDriver.lua", "VAL=10")
```

An asynPortDriver is created with the given asyn port name and the
Lua script is run with the defined macro values. Within the script,
parameters are defined using the following convention:

```
param.<param_type> "<NAME>"
```

The parameter type can be any of Int32, Float64, or Octet (String is
an alias for Octet). Names are case insensitive.

To bind callbacks to reading or writing:

```
param.<param_type>.<read/write> "NAME" [[
    CODE
]]
```

Inside the code, `self` is a driver proxy object and `value` (for
write callbacks) contains the incoming value. Read callbacks return
a value to send back to the reader.

```lua
param.int32 "BASE"
param.int32 "SIDE"

param.float64.read "HYPOTENUSE" [[
    return math.sqrt(self["BASE"]^2 + self["SIDE"]^2)
]]
```
