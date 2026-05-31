---
layout: default
title: event
parent: Included Libraries
nav_order: 6
---

# Event Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The event library provides synchronization primitives for inter-thread
communication in Lua scripts. It is loaded via `require`:

```lua
local event = require("event")
```


Event Flags
-----------

### event.flag
---

```
event.flag ()
event.flag (name)
```

Creates an event flag for signaling between threads. An event flag is a
boolean value that can be set, cleared, and tested atomically.

**Anonymous flags** are created when no name is given. They are local
to the Lua state that created them and are garbage-collected when no
longer referenced.

**Named flags** are created or looked up by name. All calls to
`event.flag("name")` with the same name -- even from different Lua
states -- return a reference to the same underlying flag. Named flags
persist for the lifetime of the IOC and are never garbage-collected.

```lua
-- Anonymous flag (local to this state)
local done = event.flag()

-- Named flag (shared across states)
local ready = event.flag("dataReady")
```

| Parameter | Type | Description |
| - | - | - |
| name | string | Optional. Name for a shared flag. If omitted, creates an anonymous flag. |

<br>

### flag:set
---

```
flag:set ()
```

Sets the flag. If any thread is blocked in `flag:wait()`, it will be
woken up. Setting an already-set flag is a no-op.

<br>

### flag:clear
---

```
flag:clear ()
```

Clears the flag.

<br>

### flag:test
---

```
flag:test ()
```

Returns `true` if the flag is set, `false` otherwise. Does not modify
the flag.

<br>

### flag:testAndClear
---

```
flag:testAndClear ()
```

Returns `true` if the flag was set, `false` otherwise. Atomically
clears the flag. This is useful for consuming a one-shot signal
without a race between testing and clearing.

<br>

### flag:wait
---

```
flag:wait (timeout)
```

Blocks until the flag is set or the timeout expires. Returns `true` if
the flag is set, `false` if the timeout expired.

A timeout of `-1` waits indefinitely. A timeout of `0` tests without
blocking (equivalent to `flag:test()`).

The timeout argument is required.

| Parameter | Type | Description |
| - | - | - |
| timeout | number | Seconds to wait. Use `-1` for indefinite wait. |

```lua
if flag:wait(5.0) then
    print("Flag was set within 5 seconds")
else
    print("Timed out")
end
```


Examples
--------

### Basic signaling between threads

```lua
-- shared.lua: loaded via luaLoadFile, defines a named flag
local event = require("event")
local ready = event.flag("sensorReady")

-- ... sensor initialization ...

ready:set()  -- signal that initialization is complete
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
