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
```

Calls ca_get to retrieve the value of a PV accessible by the host.

Returns the value of the PV on success, or `nil, "error message"` on failure.

| Parameter | Type | Description |
| - | - | - |
| PV       |   string | The name of the PV to request.
| timeout  |   number | Amount of seconds to search for pv before giving a timeout, default is 1.0 (can be fractional).

<br>

### epics.put
---

```
epics.put (PV, value[, timeout])
```

Calls ca_put to set the value of a PV accessible by the host.

Returns `true` on success, or `nil, "error message"` on failure.

| Parameter | Type | Description |
| - | - | - |
| PV       |   string | The name of the PV to request.
| value    |   varies | The new value you want to set the PV to. The type of this parameter should match with the dbtype of the PV requested.
| timeout  |   number | Amount of seconds to wait for connection and completion, default is 1.0 (can be fractional).

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
