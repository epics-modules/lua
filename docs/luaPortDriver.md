---
layout: default
title: luaPortDriver
nav_order: 6
---


# luaPortDriver

There are two ways to create Lua-based asynPortDrivers:

1. **`asyn.driver.new`** (recommended) -- Create a driver using the asyn library's
   Lua API with type constructors, parameter proxies, and callback binding.
   See the [asyn library documentation](libraries/asyn-library) for full details.

2. **`luaPortDriver` iocsh command** (legacy) -- Create a driver using the param
   DSL in a script file, described below.


## asyn.driver.new (Recommended)

The `asyn.driver.new` function creates an asynPortDriver with parameters defined
declaratively and callbacks bound as Lua functions. This is the preferred approach
for new drivers.

```lua
local asyn = require("asyn")
local Int32, Float64 = asyn.Int32, asyn.Float64

local drv = asyn.driver.new(PORT, {
    Float64 "TEMPERATURE" (20.0),
    Float64 "SETPOINT"    (25.0),
    Int32   "ENABLED"     (0),
}, function(self)
    self.scale = tonumber(CONV) or 1.0
end)

drv.TEMPERATURE.read = function(self)
    return drv.SETPOINT.value * self.scale
end

drv.SETPOINT.write = function(value, self)
    drv.SETPOINT.value = value
end
```

For complete documentation of the API, type constructors, parameter proxies,
and driver proxy methods, see the [asyn library documentation](libraries/asyn-library).


## luaPortDriver iocsh Command (Legacy)

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
