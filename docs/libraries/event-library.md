---
layout: default
title: event
parent: Included Libraries
nav_order: 6
---

# Event Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The event library provides synchronization primitives for inter-thread
communication in Lua scripts.

```lua
local event = require("event")
```


Creating Flags
--------------

### event.flag
---

Create or look up an event flag.

```
event.flag ()
event.flag (name)
```

Creates an event flag for signaling between threads. An event flag is
a boolean value that can be set, cleared, and tested atomically.

**Anonymous flags** are created when no name is given. They are local to
the Lua state that created them and are garbage-collected when no longer
referenced.

**Named flags** are created or looked up by name. All calls to
`event.flag("name")` with the same name -- even from different Lua
states -- return a reference to the same underlying flag. Named flags
persist for the lifetime of the IOC.

```lua
local done  = event.flag()              -- anonymous
local ready = event.flag("dataReady")   -- named (shared)
```

| Parameter | Type | Description |
| - | - | - |
| name | string | Optional. Name for a shared flag. If omitted, creates an anonymous flag. |

**Returns:** an event flag object.

<br>

Flag Methods
------------

### flag:set
---

Set the flag.

```
flag:set ()
```

Sets the flag to true. If any thread is blocked in `flag:wait()`, it
will be woken up. Setting an already-set flag is a no-op.

<br>

### flag:clear
---

Clear the flag.

```
flag:clear ()
```

Sets the flag to false.

<br>

### flag:test
---

Test whether the flag is set.

```
flag:test ()
```

**Returns:** `true` if the flag is set, `false` otherwise. Does not
modify the flag.

<br>

### flag:testAndClear
---

Test and atomically clear the flag.

```
flag:testAndClear ()
```

**Returns:** `true` if the flag was set, `false` otherwise. The flag
is cleared regardless.

<br>

### flag:wait
---

Block until the flag is set or a timeout expires.

```
flag:wait (timeout)
```

Blocks the calling thread until the flag is set or the timeout expires.
A timeout of `-1` waits indefinitely. A timeout of `0` tests without
blocking.

The timeout argument is required.

```lua
if flag:wait(5.0) then
    print("Flag was set within 5 seconds")
else
    print("Timed out")
end
```

| Parameter | Type | Description |
| - | - | - |
| timeout | number | Seconds to wait. Use `-1` for indefinite. |

**Returns:** `true` if the flag is set, `false` if the timeout expired.

<br>

Examples
--------

### Basic signaling between threads

```lua
-- shared.lua: loaded via luaLoadFile
local event = require("event")
local ready = event.flag("sensorReady")

-- ... sensor initialization ...

ready:set()
```

```lua
-- main.lua: waits for the signal
local event = require("event")
local ready = event.flag("sensorReady")

if ready:wait(10.0) then
    print("Sensor is ready")
else
    print("Sensor initialization timed out")
end
```

### Polling a flag in a loop

```lua
local event = require("event")
local osi = require("osi")
local stop = event.flag("stopPolling")

while not stop:test() do
    -- do periodic work
    osi.sleep(1.0)
end

print("Stopped")
```
