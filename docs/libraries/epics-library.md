---
layout: default
title: epics
parent: Included Libraries
nav_order: 3
---

# Epics Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The epics library provides functions for reading and writing EPICS PVs
from Lua scripts. When a PV exists in the local IOC database, reads and
writes use direct database access for maximum performance. When the PV
is not found locally, Channel Access is used automatically.

```lua
local epics = require("epics")
```


Reading and Writing PVs
------------------------

### epics.get
---

Read the value of a PV.

```
epics.get (PV [, timeout])
epics.get (PV, options)
```

Retrieves the current value of a PV. For scalar PVs, returns a single
value. For array/waveform PVs, returns a Lua table.

```lua
local val   = epics.get("my:ai")
local val   = epics.get("my:ai", 5.0)
local arr   = epics.get("my:waveform", {count=100})
local label = epics.get("my:mbbo", {string=true})
```

| Parameter | Type | Description |
| - | - | - |
| PV | string | The name of the PV to read. |
| timeout | number | Optional. Timeout in seconds. Default: 1.0. |
| options | table | Optional. Named parameters (see below). |

**Options table:**

| Field | Type | Default | Description |
| - | - | - | - |
| timeout | number | 1.0 | Connection and read timeout in seconds. |
| count | integer | all | Maximum number of array elements to fetch. |
| string | boolean | type-dependent | Controls string vs numeric return for enums and char arrays. |

{: .note }
> Char waveforms are returned as Lua strings by default. Use
> `{string=false}` to get a table of byte values instead.

**Returns:** the value on success. For arrays, returns a Lua table.

**Returns:** `nil, "error message"` on failure.

<br>

### epics.put
---

Write a value to a PV.

```
epics.put (PV, value [, timeout])
epics.put (PV, value, options)
```

Writes a value to a PV. When the value is a Lua table, performs an
array write.

```lua
epics.put("my:ao", 42.0)
epics.put("my:ao", 42.0, 5.0)
epics.put("my:waveform", {1.0, 2.0, 3.0})
epics.put("my:stringwf", {"Hello", "World"})
```

| Parameter | Type | Description |
| - | - | - |
| PV | string | The name of the PV to write. |
| value | varies | The value to write. Can be a number, integer, boolean, string, or a Lua table for array writes. |
| timeout | number | Optional. Timeout in seconds. Default: 1.0. |
| options | table | Optional. `{timeout=N}` for custom timeout. |

**Returns:** nothing on success.

**Returns:** an error string on failure.


PV Object
---------

### epics.pv
---

Create a PV proxy object for convenient field access.

```
epics.pv (PV)
```

Returns a PV object that provides dot-syntax access to record fields.
For local PVs, field access uses direct database access; for remote PVs,
Channel Access is used automatically.

```lua
local motor = epics.pv("IOC:m1")

-- Read/write fields via dot syntax
print(motor.RBV)
motor.VAL = 10.0

-- Read/write with options via methods
local val = motor:get("VAL", {timeout=5.0})
motor:put("VAL", 42, {timeout=5.0})

-- Properties
print(motor.name)         -- "IOC:m1"
print(tostring(motor))    -- "IOC:m1"
```

| Parameter | Type | Description |
| - | - | - |
| PV | string | The name of the PV. |

**Returns:** a PV object.

{: .note }
> The reserved names `name`, `get`, and `put` return PV metadata or
> methods. All other keys are treated as EPICS field names.

<br>

### pv.name
---

The PV name as a string (read-only property).

<br>

### pv:get
---

Read a field with custom options.

```
pv:get (field [, options])
```

Reads a field using the same options as `epics.get`.

| Parameter | Type | Description |
| - | - | - |
| field | string | The field name (e.g., `"VAL"`, `"EGU"`). |
| options | table | Optional. `{timeout, count, string}` -- same as `epics.get`. |

**Returns:** the field value, or `nil, "error message"` on failure.

<br>

### pv:put
---

Write a field with custom options.

```
pv:put (field, value [, options])
```

Writes a field using the same options as `epics.put`.

| Parameter | Type | Description |
| - | - | - |
| field | string | The field name. |
| value | varies | The value to write. |
| options | table | Optional. `{timeout}` -- same as `epics.put`. |

**Returns:** nothing on success, error string on failure.


Array Types
-----------

The following table shows how PV field types map to Lua return types
when reading array/waveform PVs:

| PV Field Type | Lua Return Type |
| - | - |
| DBF_CHAR (default) | Lua string |
| DBF_CHAR (string=false) | table of integers |
| DBF_SHORT, DBF_LONG | table of integers |
| DBF_FLOAT, DBF_DOUBLE | table of numbers |
| DBF_STRING | table of strings |
| DBF_ENUM | table of integers (or strings if string=true) |
