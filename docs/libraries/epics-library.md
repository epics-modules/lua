---
layout: default
title: epics
parent: Included Libraries
nav_order: 3
---

# Epics Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The epics library provides Channel Access functions for reading and writing 
EPICS PVs from Lua scripts. A CA context is created automatically on first 
use and cached for the lifetime of the Lua state, so repeated calls do not 
incur context creation overhead.

All functions that can fail return `nil, "error message"` on error, allowing
idiomatic Lua error handling:

```lua
local val, err = epics.get("my:pv")
if not val then
    print("Error: " .. err)
end
```

### epics.get
---

```
epics.get (PV[, timeout])
epics.get (PV, options)
```

Calls ca_get to retrieve the value of a PV accessible by the host. For scalar PVs,
returns a single value. For array/waveform PVs, returns a Lua table of values.

Returns the value on success, or `nil, "error message"` on failure.

The second argument can be either a numeric timeout (for backward compatibility) or
an options table with named parameters:

```lua
-- Simple scalar read:
local val = epics.get("my:ai")

-- With timeout:
local val = epics.get("my:ai", 5.0)

-- With options table:
local val = epics.get("my:ai", {timeout=5.0})

-- Read first 100 elements of a waveform:
local arr = epics.get("my:waveform", {count=100})

-- Read enum as string label instead of numeric index:
local label = epics.get("my:mbbo", {string=true})

-- Read char waveform as table of integers instead of string:
local bytes = epics.get("my:charwf", {string=false})
```

| Parameter | Type | Description |
| - | - | - |
| PV       |   string | The name of the PV to request.
| timeout  |   number | Amount of seconds to search for pv before giving a timeout, default is 1.0 (can be fractional).

**Options table fields:**

| Field | Type | Default | Description |
| - | - | - | - |
| timeout | number | 1.0 | CA connection and read timeout in seconds. |
| count   | integer | all | Maximum number of array elements to fetch. |
| string  | boolean | type-dependent | Controls string vs numeric return for certain types. |

The `string` option affects these field types:

| Field Type | Default | string=true | string=false |
| - | - | - | - |
| DBF_ENUM (scalar) | numeric index | string label | numeric index |
| DBF_CHAR (array) | Lua string | Lua string | table of integers |

**Array return types:**

| PV Field Type | Lua Return Type |
| - | - |
| DBF_CHAR (default) | Lua string |
| DBF_CHAR (string=false) | table of integers |
| DBF_SHORT, DBF_LONG | table of integers |
| DBF_FLOAT, DBF_DOUBLE | table of numbers |
| DBF_STRING | table of strings |
| DBF_ENUM | table of integers (or strings if string=true) |

<br>

### epics.put
---

```
epics.put (PV, value[, timeout])
epics.put (PV, value, options)
```

Calls ca_put to set the value of a PV accessible by the host. When the value
is a Lua table, performs an array put using ca_array_put.

Returns `true` on success, or `nil, "error message"` on failure.

The third argument can be either a numeric timeout or an options table:

```lua
-- Scalar put:
epics.put("my:ao", 42.0)

-- With timeout:
epics.put("my:ao", 42.0, 5.0)

-- With options:
epics.put("my:ao", 42.0, {timeout=5.0})

-- Array put (table of numbers):
epics.put("my:waveform", {1.0, 2.0, 3.0})

-- Array put (table of integers):
epics.put("my:longwf", {1, 2, 3})

-- Array put (table of strings):
epics.put("my:stringwf", {"Hello", "World"})
```

| Parameter | Type | Description |
| - | - | - |
| PV       |   string | The name of the PV to request.
| value    |   varies | The value to write. Can be a number, integer, boolean, string, or a Lua table for array writes. For tables, the element type is determined from the first element.
| timeout  |   number | Amount of seconds to wait for connection and completion, default is 1.0 (can be fractional).

**Options table fields:**

| Field | Type | Default | Description |
| - | - | - | - |
| timeout | number | 1.0 | CA connection and write timeout in seconds. |

<br>

### epics.pv
---

```
epics.pv (PV)
```

Returns a table representing a PV object. Index accesses can be used to retrieve or
change record fields. These changes are completed through ca_get or ca_put.

```lua
local motor = epics.pv("IOC:m1")
print(motor.RBV)        -- reads IOC:m1.RBV via CA
motor.VAL = 10.0        -- writes to IOC:m1.VAL via CA
print(motor:getName())  -- "IOC:m1"
```

| Parameter | Type | Description |
| - | - | - |
| PV   |  string | The name of the PV to request. |
