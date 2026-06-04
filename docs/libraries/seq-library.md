---
layout: default
title: seq
parent: Included Libraries
nav_order: 7
---

# Sequencer Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The seq library provides a Lua-native state machine sequencer as an
alternative to the EPICS State Notation Language (SNL). Programs define
states with transitions driven by PV values, timers, and event flags.

Programs are registered before iocInit and start automatically in
background threads after iocInit completes. Multiple programs can
run concurrently within the same Lua state using coroutines.

{: .note }
> The seq library is a pure Lua file installed to `lib/<arch>/`. Call
> `luaAddModule` in your startup script to make it available via
> `require`.

```lua
local seq = require("seq")
```


Creating a Program
------------------

### seq.program
---

Create a new sequencer program.

```
seq.program (name)
seq.program (name, options)
```

Creates a program object. Define states with `prog:state()`, then
register with `seq.register()`.

```lua
local prog = seq.program("myProgram")
local prog = seq.program("myProgram", { poll = 0.05 })
```

| Parameter | Type | Description |
| - | - | - |
| name | string | Program name (used in log messages and thread names). |
| options | table | Optional. `poll` sets the evaluation interval in seconds (default 0.1). |

**Returns:** a program object.

<br>

Defining States
---------------

### prog:state
---

Define a state in the program.

```
prog:state (name, definition)
```

Defines a named state. The first state defined becomes the initial
state when the program starts.

The definition table contains named fields for lifecycle hooks and
options, and array entries for transitions:

```lua
prog:state("idle", {
    entry = function() print("Entering idle") end,
    exit  = function() print("Leaving idle") end,

    options = { always_enter = true },

    seq.when(function() return voltage.VAL > 5.0 end) {
        action = function() print("High!") end,
        next = "alarm",
    },

    seq.when(seq.delay(0.1)) {
        next = "idle",
    },
})
```

| Field | Type | Description |
| - | - | - |
| `entry` | function | Optional. Runs when entering this state. |
| `exit` | function | Optional. Runs when leaving this state. |
| `options` | table | Optional. Controls self-transition behavior. |

**Returns:** the program object (for chaining).

<br>

### State Options

Options control behavior on self-transitions (where `next` equals the
current state name):

| Option | Default | Description |
| - | - | - |
| `always_enter` | `false` | Run entry on all transitions, including self-transitions. |
| `always_exit` | `false` | Run exit on all transitions, including self-transitions. |
| `always_reset` | `true` | Reset delay timers on all transitions. Set to `false` to only reset on unique transitions. |

By default, `entry` and `exit` only run when the state actually changes.

<br>

Transitions
-----------

### seq.when
---

Create a transition.

```
seq.when ()
seq.when (condition)
seq.when (delay)
```

Returns a transition builder. Call it with a table containing `action`
(optional) and `next` (required) to complete the transition:

```lua
seq.when(function() return voltage.VAL > 5.0 end) {
    action = function() print("Threshold exceeded") end,
    next = "alarm",
},
```

Transitions are evaluated in order. The first whose condition is true
fires; remaining transitions are skipped.

| Condition | Fires when |
| - | - |
| Function | The function returns a truthy value. |
| `seq.delay(seconds)` | The specified time has elapsed since entering the state. |
| None (`seq.when()`) | Always (unconditional). |

**Returns:** a transition builder object (call it with `{ next="..." }`).

<br>

### seq.delay
---

Create a delay condition for transitions.

```
seq.delay (seconds)
```

The condition becomes true when the specified time has elapsed since
the current state was entered. Timers reset on each state entry,
including self-transitions (unless `always_reset = false`).

```lua
seq.when(seq.delay(5.0)) {
    next = "timeout",
},
```

| Parameter | Type | Description |
| - | - | - |
| seconds | number | Delay in seconds. |

**Returns:** a delay condition object.

<br>

Running Programs
----------------

### seq.register
---

Register a program for execution after iocInit.

```
seq.register (prog)
```

If called before iocInit, the program starts automatically after
iocInit completes. If called after iocInit, starts immediately.

Multiple programs can be registered in the same Lua state. They
run as coroutines within a single background thread.

```lua
seq.register(prog)
```

| Parameter | Type | Description |
| - | - | - |
| prog | program | A program created with `seq.program`. |

<br>

### prog:stop
---

Signal the program to stop.

```
prog:stop ()
```

The program exits at the end of its current evaluation cycle.

<br>

Error Handling
--------------

If a condition, action, or entry/exit function raises an error, the
sequencer logs the error message and continues running. Condition
errors are treated as false (the transition does not fire).

<br>

Usage Pattern
-------------

The typical pattern combines record creation and sequencer definition
in a single file loaded via `luaLoadFile` before iocInit:

```lua
-- myseq.lua
local seq   = require("seq")
local db    = require("db")
local epics = require("epics")

db.record("ai", P .. "voltage") { PINI = "YES" }
db.record("bo", P .. "light") { ZNAM = "Off", ONAM = "On" }

local voltage = epics.pv(P .. "voltage")
local light   = epics.pv(P .. "light")

local prog = seq.program("lightControl")

prog:state("off", {
    entry = function() light.VAL = 0 end,
    seq.when(function() return voltage.VAL > 5.0 end) {
        next = "on",
    },
    seq.when(seq.delay(0.1)) { next = "off" },
})

prog:state("on", {
    entry = function() light.VAL = 1 end,
    seq.when(function() return voltage.VAL <= 5.0 end) {
        next = "off",
    },
    seq.when(seq.delay(0.1)) { next = "on" },
})

seq.register(prog)
```

```lua
-- st.lua
luaAddModule("../..")
luaLoadFile("myseq.lua", {P="IOC:"})
iocInit()
```

The `11-Sequencer` example IOC demonstrates this pattern with a
traffic light controller and a voltage monitor.

<br>

Comparison with SNL
-------------------

| Feature | SNL | seq |
| - | - | - |
| Language | SNL (.st/.stt) | Lua |
| Compiler | snc to C to binary | None (interpreted) |
| States | `state name { }` | `prog:state("name", { })` |
| Transitions | `when (cond) { } state next` | `seq.when(cond) { next = "next" }` |
| Timers | `delay(seconds)` | `seq.delay(seconds)` |
| PV access | `assign`/`monitor`/`pvGet`/`pvPut` | `epics.pv` |
| Concurrency | Multiple state sets per program | Multiple `seq.program` per state |
| Event flags | `evflag`/`efSet`/`efTest` | `event.flag` |
| Record creation | Separate .db file | `db.record` in same file |
