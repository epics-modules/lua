---
layout: default
title: asyn
parent: Included Libraries
nav_order: 1
---


# Asyn Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The asyn library provides functions for communicating with asyn ports,
accessing asynPortDriver parameters, and configuring port connections.

```lua
local asyn = require("asyn")
```


Getting / Setting Parameters
----------------------------

### asyn.getParam
---

Read a parameter value from an asynPortDriver.

```
asyn.getParam (portName [, addr], paramName)
asyn.getStringParam (portName [, addr], paramName)
asyn.getDoubleParam (portName [, addr], paramName)
asyn.getIntegerParam (portName [, addr], paramName)
```

The typed variants force conversion to a specific type. The generic
`getParam` uses the parameter's `asynParamType` to determine the type.

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| paramName | string | The name of the parameter. |

**Returns:** the parameter value.

<br>

### asyn.setParam
---

Write a parameter value to an asynPortDriver.

```
asyn.setParam (portName [, addr], paramName, value)
asyn.setStringParam (portName [, addr], paramName, value)
asyn.setDoubleParam (portName [, addr], paramName, value)
asyn.setIntegerParam (portName [, addr], paramName, value)
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| paramName | string | The name of the parameter. |
| value | varies | The value to set. Type should match the parameter type. |

<br>

### asyn.callParamCallbacks
---

Trigger parameter callbacks on changed values.

```
asyn.callParamCallbacks (portName [, addr, parameter])
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The parameter list index. Default: 0. |
| parameter | string | Optional. A specific parameter. Default: all changed values. |


Reading / Writing Values
------------------------

### asyn.readParam
---

Read a parameter by calling the asyn interface's read function.

```
asyn.readParam (portName [, addr], paramName)
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| paramName | string | The name of the parameter. |

**Returns:** the parameter value.

<br>

### asyn.writeParam
---

Write a parameter by calling the asyn interface's write function.

```
asyn.writeParam (portName [, addr], paramName, value)
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| paramName | string | The name of the parameter. |
| value | varies | The value to write. |


Configuration Parameters
------------------------

### asyn.setOutTerminator / asyn.getOutTerminator
---

Set or get the global output terminator string.

```
asyn.setOutTerminator (terminator)
asyn.getOutTerminator ()
```

The output terminator is appended to all `asyn.write` calls.

| Parameter | Type | Description |
| - | - | - |
| terminator | string | The string to append to writes. |

**Returns:** the current terminator (for get).

<br>

### asyn.setInTerminator / asyn.getInTerminator
---

Set or get the global input terminator string.

```
asyn.setInTerminator (terminator)
asyn.getInTerminator ()
```

The input terminator controls when `asyn.read` stops reading.

| Parameter | Type | Description |
| - | - | - |
| terminator | string | The terminator string. |

**Returns:** the current terminator (for get).

<br>

### asyn.setWriteTimeout / asyn.getWriteTimeout
---

Set or get the global write timeout.

```
asyn.setWriteTimeout (timeout)
asyn.getWriteTimeout ()
```

| Parameter | Type | Description |
| - | - | - |
| timeout | number | Timeout in milliseconds. |

**Returns:** the current timeout (for get).

<br>

### asyn.setReadTimeout / asyn.getReadTimeout
---

Set or get the global read timeout.

```
asyn.setReadTimeout (timeout)
asyn.getReadTimeout ()
```

| Parameter | Type | Description |
| - | - | - |
| timeout | number | Timeout in milliseconds. |

**Returns:** the current timeout (for get).


Debug Information
-----------------

### asyn.setTrace
---

Set trace mask flags on a port.

```
asyn.setTrace (portName [, addr], key, val)
asyn.setTrace (portName [, addr], {key1=val1, ...})
```

Valid keys: `"error"`, `"device"`, `"filter"`, `"driver"`, `"flow"`,
`"warning"` (case insensitive).

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| key | string | Which mask to change. |
| val | boolean | Whether to turn on or off the mask. |

<br>

### asyn.setTraceIO
---

Set trace I/O mask flags on a port.

```
asyn.setTraceIO (portName [, addr], key, val)
asyn.setTraceIO (portName [, addr], {key1=val1, ...})
```

Valid keys: `"nodata"`, `"ascii"`, `"escape"`, `"hex"` (case insensitive).

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| key | string | Which mask to change. |
| val | boolean | Whether to turn on or off the mask. |


Octet Communications
--------------------

### asyn.write
---

Write a string to an asynOctet port.

```
asyn.write (data, portName [, addr, parameter])
```

| Parameter | Type | Description |
| - | - | - |
| data | string | The string to write. The global OutTerminator is appended automatically. |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| parameter | string | Optional. An asyn parameter to write to. |

<br>

### asyn.read
---

Read a string from an asynOctet port.

```
asyn.read (portName [, addr, parameter])
```

Reads until the global InTerminator is encountered or the ReadTimeout
is reached.

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| parameter | string | Optional. An asyn parameter to read from. |

**Returns:** the data read as a string, or `nil` on timeout.

<br>

### asyn.writeread
---

Write data to a port and read the response atomically.

```
asyn.writeread (data, portName [, addr, parameter])
```

| Parameter | Type | Description |
| - | - | - |
| data | string | The string to write. |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| parameter | string | Optional. An asyn parameter. |

**Returns:** the data read as a string, or `nil` on timeout.

<br>

### asyn.setOption
---

Set a driver-specific option on a port.

```
asyn.setOption (portName [, addr], key, val)
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| key | string | The option name. |
| val | string | The option value. |

**Returns:** the asynStatus value.


asynOctetClient Object
----------------------

### asyn.client
---

Create a persistent asynOctet client connection.

```
asyn.client (portName [, addr, parameter])
asyn.client.find (portName [, addr, parameter])
```

Creates a client object with methods for reading, writing, and
configuring the connection. `asyn.client(...)` and
`asyn.client.find(...)` are equivalent.

```lua
local port = asyn.client("SERIAL1")
port.OutTerminator = "\n"
port.InTerminator  = "\n"
port.ReadTimeout   = 5.0
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |
| parameter | string | Optional. An asyn parameter. Default: empty string. |

**Returns:** an asynOctetClient object.

<br>

### Client Properties

| Property | Type | Description |
| - | - | - |
| `portName` | string | Read-only. The port name. |
| `addr` | number | Read-only. The address. |
| `InTerminator` | string | Get or set the input terminator. |
| `OutTerminator` | string | Get or set the output terminator. |
| `ReadTimeout` | number | Get or set the read timeout in seconds (default 1.0). |
| `WriteTimeout` | number | Get or set the write timeout in seconds (default 1.0). |

<br>

### client:read
---

Read a string from the port.

```
client:read ()
```

**Returns:** the data read, or `nil` on timeout.

<br>

### client:write
---

Write a string to the port.

```
client:write (data)
```

| Parameter | Type | Description |
| - | - | - |
| data | string | The string to write. |

<br>

### client:writeread
---

Write data and read the response atomically.

```
client:writeread (data)
```

| Parameter | Type | Description |
| - | - | - |
| data | string | The string to write. |

**Returns:** the data read from the port.

<br>

### client:flush
---

Flush the input buffer.

```
client:flush ()
```

<br>

### client:trace
---

Set trace mask flags on this client's port.

```
client:trace (key, val)
client:trace ({key1=val1, ...})
```

Valid keys: `"error"`, `"device"`, `"filter"`, `"driver"`, `"flow"`,
`"warning"`.

| Parameter | Type | Description |
| - | - | - |
| key | string | Which mask to change. |
| val | boolean | Whether to turn on or off the mask. |

<br>

### client:traceio
---

Set trace I/O mask flags on this client's port.

```
client:traceio (key, val)
client:traceio ({key1=val1, ...})
```

Valid keys: `"nodata"`, `"ascii"`, `"escape"`, `"hex"`.

| Parameter | Type | Description |
| - | - | - |
| key | string | Which mask to change. |
| val | boolean | Whether to turn on or off the mask. |

<br>

### client:setOption
---

Set a driver-specific option.

```
client:setOption (key, val)
```

| Parameter | Type | Description |
| - | - | - |
| key | string | The option name. |
| val | string | The option value. |

<br>

### client[addr]
---

Create a new client at a different address on the same port.

```lua
local port0 = asyn.client("SERIAL1")
local port1 = port0[1]   -- same port, address 1
```

**Returns:** a new asynOctetClient object.


asynPortDriver
--------------

### asyn.driver / asyn.driver.find
---

Find an existing asynPortDriver and return a driver proxy object.

```
asyn.driver (portName)
asyn.driver.find (portName)
```

Access parameters by name through the proxy. `asyn.driver(...)` and
`asyn.driver.find(...)` are equivalent.

```lua
local drv = asyn.driver("MYPORT")

print(drv.TEMPERATURE.value)
drv.SETPOINT.value = 25.0
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asynPortDriver port name. |

**Returns:** a driver proxy object.

For creating new drivers, binding callbacks, and full port driver
documentation, see [Lua Port Drivers](../luaPortDriver).
