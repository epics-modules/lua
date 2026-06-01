---
layout: default
title: seq
parent: Included Libraries
nav_order: 7
---

# Sequencer Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The seq library provides a Lua-native state machine sequencer as an
alternative to the EPICS State Notation Language (SNL). Programs are
defined as state machines with transitions driven by PV values,
timers, and event flags.

Programs are registered before iocInit and start automatically in
background threads after iocInit completes. Multiple programs can
run concurrently within the same Lua state using coroutines.

```lua
local seq = require("seq")
```

The seq library is a pure Lua file installed to `lib/<arch>/`. Use
`luaAddModule` to make it available via `require`.


Creating a Program
------------------

### seq.program
---

```
seq.program (name)
seq.program (name, options)
```

Creates a new sequencer program. The program is a container for
states defined with `prog:state()`. The optional options table
can set the poll interval.

```lua
local prog = seq.program("myProgram")
local prog = seq.program("myProgram", { poll = 0.05 })
```

| Parameter | Type | Description |
| - | - | - |
| name | string | Program name (used in log messages and thread names). |
| options | table | Optional. `poll` sets the condition evaluation interval in seconds (default 0.1). |


Defining States
---------------

### prog:state
---

```
prog:state (name, definition)
```

Defines a state in the program. The first state defined becomes the
initial state when the program starts.

The definition table contains named fields for lifecycle hooks and
options, and array entries for transitions:

```lua
prog:state("idle", {
    entry = function() print("Entering idle") end,
    exit  = function() print("Leaving idle") end,

    options = {
        always_enter = true,
    },

    seq.when(function() return voltage.VAL > 5.0 end) {
        action = function() print("Voltage high") end,
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
| `options` | table | Optional. Controls self-transition behavior (see below). |
| Array entries | transitions | One or more `seq.when(...)` transitions. |

### State Options

| Option | Default | Description |
| - | - | - |
| `always_enter` | `false` | Run entry on all transitions, including self-transitions. |
| `always_exit` | `false` | Run exit on all transitions, including self-transitions. |
| `always_reset` | `true` | Reset delay timers on all transitions. Set to `false` to only reset timers on unique (non-self) transitions. |

By default, `entry` and `exit` only run when the state actually
changes (entering from a different state or leaving to a different
state). Self-transitions (where `next` equals the current state) do
not trigger entry/exit unless the corresponding `always_` option is
set.


Transitions
-----------

### seq.when
---

```
seq.when ()
seq.when (condition)
seq.when (delay)
```

Creates a transition. The returned object accepts a table via the
call syntax `{ action=fn, next="state" }`.

The `next` field is required and names the target state. Use `"exit"`
to terminate the program. Use the current state name for a
self-transition.

The `action` field is optional. If provided, the function runs when
the transition fires.

```lua
-- Conditional transition
seq.when(function() return voltage.VAL > 5.0 end) {
    action = function() print("Voltage exceeded threshold") end,
    next = "alarm",
},

-- Unconditional transition (always fires)
seq.when() {
    action = function() initialize() end,
    next = "running",
},

-- Self-transition with delay (polling pattern)
seq.when(seq.delay(0.1)) {
    next = "idle",
},
```

Transitions are evaluated in the order they appear in the state
definition. The first transition whose condition is true fires;
remaining transitions are skipped.

| Condition | Fires when |
| - | - |
| Function | The function returns a truthy value. |
| `seq.delay(seconds)` | The specified time has elapsed since entering the state. |
| None (`seq.when()`) | Always (unconditional). |

<br>

### seq.delay
---

```
seq.delay (seconds)
```

Creates a delay condition for use in transitions. The condition
becomes true when the specified number of seconds have elapsed
since the current state was entered.

Delay timers are reset each time the state is entered, including
on self-transitions (unless `always_reset = false` is set in the
state options).

```lua
seq.when(seq.delay(5.0)) {
    next = "timeout",
},
```

| Parameter | Type | Description |
| - | - | - |
| seconds | number | Delay in seconds. |


Running Programs
----------------

### seq.register
---

```
seq.register (prog)
```

Registers a program for execution. If called before iocInit, the
program starts automatically in a background thread after iocInit
completes. If called after iocInit, the program starts immediately.

Multiple programs can be registered in the same Lua state. They run
as coroutines within a single background thread, taking turns
evaluating their conditions each poll cycle.

```lua
seq.register(prog)
```

| Parameter | Type | Description |
| - | - | - |
| prog | program | A program created with `seq.program`. |

<br>

### prog:stop
---

```
prog:stop ()
```

Signals the program to exit. The program stops at the end of its
current evaluation cycle. This can be called from within an action
function to stop the program from inside.


Error Handling
--------------

If a condition function, action function, or entry/exit function
raises an error, the sequencer logs the error message and continues
running. Condition errors are treated as false (the transition does
not fire). Action and entry/exit errors are logged but the state
transition still proceeds.


Usage Pattern
-------------

The typical usage pattern combines record creation and sequencer
definition in a single file loaded via `luaLoadFile` before iocInit:

```lua
-- myseq.lua
local seq   = require("seq")
local db    = require("db")
local epics = require("epics")

-- Create records (before iocInit)
db.record("ai", P .. "voltage") { PINI = "YES" }
db.record("bo", P .. "light") { ZNAM = "Off", ONAM = "On" }

-- PV objects (accessed after iocInit by callbacks)
local voltage = epics.pv(P .. "voltage")
local light   = epics.pv(P .. "light")

local prog = seq.program("lightControl")

prog:state("off", {
    entry = function() light.VAL = 0 end,

    seq.when(function() return voltage.VAL > 5.0 end) {
        next = "on",
    },
    seq.when(seq.delay(0.1)) {
        next = "off",
    },
})

prog:state("on", {
    entry = function() light.VAL = 1 end,

    seq.when(function() return voltage.VAL <= 5.0 end) {
        next = "off",
    },
    seq.when(seq.delay(0.1)) {
        next = "on",
    },
})

seq.register(prog)
```

```lua
-- st.lua (startup)
luaAddModule("../..")
luaLoadFile("myseq.lua", {P="IOC:"})
iocInit()
-- sequencer starts automatically
```

The `11-Sequencer` example IOC demonstrates this pattern with a
traffic light controller.


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
