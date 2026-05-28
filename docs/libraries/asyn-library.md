---
layout: default
title: asyn
parent: Included Libraries
nav_order: 1
---


# Asyn Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}


Getting / Setting Parameters
----------------------------


### asyn.get____
---

```
asyn.getParam (portName[, addr], paramName)  
asyn.getStringParam (portName[, addr], paramName)  
asyn.getDoubleParam (portName[, addr], paramName)  
asyn.getIntegerParam (portName[, addr], paramName)  
```
Fetches the value of an asyn parameter. These work like the asynPortDriver functions of the same name, retrieving the value from the param list.

Returns the value of the asyn parameter as the type specified, if no type was specified, uses the asynParamType of the parameter to determine       

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | The registered asyn port name that contains the parameter you are getting. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| paramName |  string  | The name of the parameter to fetch. |

<br>

### asyn.set____
---

```
asyn.setParam (portName[, addr], paramName)  
asyn.setStringParam (portName[, addr], paramName, value)  
asyn.setDoubleParam (portName[, addr], paramName, value)  
asyn.setIntegerParam (portName[, addr], paramName, value)  
```

Sets the value of an asyn parameter. These work like the asynPortDriver functions of the same name, saving the value in the param list.

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | The registered asyn port name that contains the parameter you are setting. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| paramName |  string  | The name of the parameter to set. |
| value     |  varies  | The value to set the parameter to. Type should match the type of the parameter you are setting. |

<br>

### asyn.callParamCallbacks
---

```
asyn.callParamCallbacks (portName[, addr, parameter])
```

Tells an asyn port to call parameter callbacks on changed values.

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The index of the parameter list to do callbacks on. Optional, default value is 0. |
| parameter |  string  | A specific parameter to do callbacks on. Optional, default is to perform callbacks on all values that have been changed. |


						
Reading / Writing Values
------------------------

### asyn.readParam
---

```
asyn.readParam (portName[, addr], paramName)
```

Calls the read function of the correct asyn interface

Returns the value of the asyn parameter as the type specified, if no type was specified, uses the asynParamType of the parameter to determine

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | The registered asyn port name that contains the parameter you are getting. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| paramName |  string  | The name of the parameter to fetch. |

<br>

### asyn.writeParam
---

```
asyn.writeParam (portName[, addr], paramName, value)
```

Calls the write function of the correct asyn interface

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | The registered asyn port name that contains the parameter you are setting. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| paramName |  string  | The name of the parameter to set. |
| value     |  varies  | The value to write to the parameter. Type should match the type of the parameter you are setting. |



Configuration Parameters
------------------------


### asyn.setOutTerminator
---

```
asyn.setOutTerminator (terminator)
```

Sets the global variable OutTerminator, which controls asyn write commands

| Parameter  |   Type   | Description |
|------------|----------|-|
| terminator |  string  | The string value to append to the end of all asyn write calls. |

<br>

### asyn.getOutTerminator
---

```
asyn.getOutTerminator ()
```

Returns the value of the global variable OutTerminator

<br>

### asyn.setInTerminator
---

```
asyn.setInTerminator (terminator)
```

Sets the global variable InTerminator, which controls asyn read commands

| Parameter  |   Type   | Description |
|------------|----------|-|
| terminator |  string  | The string value to wait for when reading from an asyn port. |

<br>

### asyn.getInTerminator
---

```
asyn.getInTerminator ()
```

Returns the value of the global variable InTerminator

<br>

### asyn.setWriteTimeout
---

```
asyn.setWriteTimeout (timeout)
```

Sets the global variable WriteTimeout, which controls asyn write commands

| Parameter |   Type   | Description |
|-----------|----------|-|
|  timeout  |  number  | The number of milliseconds for an asyn write command to wait before failure. |

<br>

### asyn.getWriteTimeout
---

```
asyn.getWriteTimeout ()
```

Returns the value of the global variable WriteTimeout

<br>

### asyn.setReadTimeout
---

```
asyn.setReadTimeout (timeout)
```

Sets the global variable ReadTimeout, which controls asyn read commands

| Parameter |   Type   | Description |
|-----------|----------|-|
|  timeout  |  number  | The number of milliseconds for an asyn read command to wait before failure. |

<br>

### asyn.getReadTimeout
---

```
asyn.getReadTimeout ()
```

Returns the value of the global variable ReadTimeout



Debug Information
-----------------

### asyn.setTrace
---

```
asyn.setTrace (portName[, addr], key, val)  
asyn.setTrace (portName[, addr], {key1=val1, ...})
```

Turns on or off asyn's tracing for a mask on a given port. Valid keys are  "error", "device", "filter", "driver", "flow", and "warning", case insensitive.
       
| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| key       |  string  | Which mask to change |
| val       |  boolean | Whether to turn on or off the mask |

<br>

### asyn.setTraceIO
---

```
asyn.setTraceIO (portName[, addr], key, val)  
asyn.setTraceIO (portName[, addr], {key1=val1, ...})
```

Turns on or off asyn's tracing for a mask on a given port. Valid keys are "nodata", "ascii", "escape", and "hex", case insensitive.

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| key       |  string  | Which mask to change |
| val       |  boolean | Whether to turn on or off the mask |



Octet Communications
--------------------

### asyn.write
---

```
asyn.write (data, portName[, addr, parameter])
```

Write a string to a given asynOctet port

| Parameter |   Type   | Description |
|-----------|----------|-|
| data      |  string  | The string to write to the port. This string will automatically have the value of the global variable OutTerminator appended to it. |
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |     
| parameter |  string  | An asyn parameter to write to. Optional. |

<br>

### asyn.read
---

```
asyn.read (portName[, addr, parameter])
```

Read a string from a given asynOctet port

Returns a string containing all data read from the asynOctet port until encountering
the input terminator set by the global variable InTerminator, or until the timeout set
by the global variable ReadTimeout is reached.

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |     
| parameter |  string  | An asyn parameter to read from. Optional. |

<br>

### asyn.writeread
---

```
asyn.writeread (data, portName[, addr, parameter])
```

Writes data to a port and then reads data from that same port.

Returns a string containing all data read from the asynOctet port until encountering
the input terminator set by the global variable InTerminator, or until the timeout set
by the global variable ReadTimeout is reached.

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |     
| parameter |  string  | An asyn parameter to read to and write from. Optional. |

<br>

### asyn.setOption
---

```
asyn.setOption (portName[, addr], key, val)
```

Sets driver-specific options

Returns the asynStatus value of the asynSetOption call.

   
| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |   string | A registered asyn port name. |
| addr      |   number | The asyn address of the parameter. Optional, default value is 0. |
| key       |   string | The name of the option you are setting. |
| val       |   string | The value to set the option to. |


   
asynOctetClient Object
----------------------

### asyn.client
---

```
asyn.client (portName[, addr, parameter])
asyn.client.find (portName[, addr, parameter])
```

Creates an asynOctetClient object connected to the specified port. The object
provides methods for reading, writing, and configuring the connection.

`asyn.client(...)` and `asyn.client.find(...)` are equivalent.

```lua
local port = asyn.client("SERIAL1")
local port = asyn.client("SERIAL1", 0, "PARAM")
local port = asyn.client.find("SERIAL1")
```

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asyn port name. |
| addr      |  number  | The asyn address of the parameter. Optional, default value is 0. |
| parameter |  string  | A specific asyn parameter. Optional, default is empty string. |

<br>

### client.portName
---

Read-only property. Returns the port name this client is connected to.

### client.addr
---

Read-only property. Returns the address this client is connected to.

### client.InTerminator
---

Get or set the input end-of-string terminator for this client.

```lua
port.InTerminator = "\r\n"
print(port.InTerminator)
```

### client.OutTerminator
---

Get or set the output end-of-string terminator for this client.

```lua
port.OutTerminator = "\r\n"
print(port.OutTerminator)
```

### client.ReadTimeout
---

Get or set the read timeout in seconds for this client. Used by
`:read()` and `:writeread()`. Default is 1.0.

```lua
port.ReadTimeout = 5.0
print(port.ReadTimeout)
```

### client.WriteTimeout
---

Get or set the write timeout in seconds for this client. Used by
`:write()`. Default is 1.0.

```lua
port.WriteTimeout = 0.5
print(port.WriteTimeout)
```

### client[addr]
---

Returns a new client connected to the same port but at a different address.

```lua
local port0 = asyn.client("SERIAL1")
local port1 = port0[1]   -- same port, address 1
```

<br>

### client:read
---

```
client:read ()
```

Reads a string from the port. Returns the data read, or nil on timeout.

<br>

### client:write
---

```
client:write (data)
```

Writes a string to the port.

| Parameter |   Type   | Description |
|-----------|----------|-|
| data      |  string  | The string to write to the port. |

<br>

### client:writeread
---

```
client:writeread (data)
```

Writes data to the port and reads the response as an atomic operation.

Returns the data read from the port.

| Parameter |   Type   | Description |
|-----------|----------|-|
| data      |  string  | The string to write to the port. |

<br>

### client:flush
---

```
client:flush ()
```

Flushes the input buffer on the port.

<br>

### client:trace
---

```
client:trace (key, val)  
client:trace ({key1=val1, ...})
```

Turns on or off asyn's tracing for a given mask on the port this client is connected 
to. Valid keys are "error", "device", "filter", "driver", "flow", and "warning", case
insensitive.

| Parameter |   Type   | Description |
|-----------|----------|-|
|    key    |  string  | Which mask to change |
|    val    |  boolean | Whether to turn on or off the mask |

<br>

### client:traceio
---

```
client:traceio (key, val)  
client:traceio ({key1=val1, ...})
```

Turns on or off asyn's tracing for a given mask on the port this client is connected 
to. Valid keys are "nodata", "ascii", "escape", and "hex", case insensitive.

| Parameter |   Type   | Description |
|-----------|----------|-|
|    key    |  string  | Which mask to change |
|    val    |  boolean | Whether to turn on or off the mask |

<br>

### client:setOption
---

```
client:setOption (key, val)
```

Sets an asynOption for the port this client is connected to.

| Parameter |   Type   | Description |
|-----------|----------|-|
|    key    |  string  | The name of the option you are setting. |
|    val    |  string  | The value to set the option to. |



asynPortDriver
--------------

### asyn.driver / asyn.driver.find
---

```
asyn.driver (portName)
asyn.driver.find (portName)
```

Finds an existing asynPortDriver and returns a driver proxy object. You can
access parameters by name through the proxy, which returns parameter proxy
objects for reading and writing values.

`asyn.driver(portName)` and `asyn.driver.find(portName)` are equivalent.

```lua
local drv = asyn.driver("MYPORT")

-- Read a parameter value
print(drv.TEMPERATURE.value)

-- Write a parameter value (also calls callParamCallbacks)
drv.SETPOINT.value = 25.0

-- Access driver properties
print(drv.portName)
print(drv.maxAddr)
```

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asynPortDriver port name |

For creating new asynPortDrivers from Lua, binding read/write callbacks,
and defining parameters with type constructors, see the
[Lua Port Drivers](../luaPortDriver) documentation.
asyn.driver (portName)
asyn.driver.find (portName)
```

Finds an existing asynPortDriver and returns a driver proxy object. You can 
access parameters by name through the proxy, which returns parameter proxy
objects for reading and writing values.

`asyn.driver(portName)` and `asyn.driver.find(portName)` are equivalent.

```lua
local drv = asyn.driver("MYPORT")

-- Read a parameter value
print(drv.TEMPERATURE.value)

-- Write a parameter value (also calls callParamCallbacks)
drv.SETPOINT.value = 25.0

-- Access driver properties
print(drv.portName)
print(drv.maxAddr)
```

| Parameter |   Type   | Description |
|-----------|----------|-|
| portName  |  string  | A registered asynPortDriver port name |

<br>

### asyn.driver.new
---

```
asyn.driver.new (portName, paramTable [, initFunc])
```

Creates a new asynPortDriver with parameters defined in the parameter table,
and optional initialization code. Returns a driver proxy object.

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

<br>

### Parameter Proxy
---

When you access a parameter name on a driver proxy (e.g., `drv.TEMPERATURE`),
you get a parameter proxy object with the following properties:

```lua
drv.PARAM.value          -- read the current parameter value
drv.PARAM.value = x      -- write a new value + call param callbacks
drv.PARAM.name           -- the parameter name string
```

For drivers created with `asyn.driver.new`, you can also bind read and write
callbacks:

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

<br>

### Driver Internal State
---

For drivers created with `asyn.driver.new`, the init function can set
arbitrary fields on the driver proxy via `self`. These fields persist
and are accessible from all callbacks:

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


### Full Example
---

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
