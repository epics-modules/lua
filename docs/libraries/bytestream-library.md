---
layout: default
title: bytestream
parent: Included Libraries
nav_order: 5
---

# Bytestream Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The bytestream library provides scanf-style parsing and printf-style formatting
for byte stream device communication. It is a pure Lua library built on top of
LPeg and asyn.client.

Format specifiers follow StreamDevice conventions, making it straightforward
to translate StreamDevice protocol files into Lua code. The library is installed
as a `.lua` file in the module's `lib/<arch>/` directory and loaded via `require`:

```lua
-- In your startup script, register the lua module's paths first:
luaAddModule("$(LUA)")     -- from iocsh, or luaAddModule(LUA) from Lua

-- Then require works in any Lua state:
local bs = require("bytestream")
```

Dependencies (lpeg and asyn) are loaded lazily on first use -- requiring
bytestream does not force asyn to load until a client is created.

All functions that encounter errors raise them via `error()`. When used from
DTYP "lua" device support, raised errors are caught by `lua_pcall` and mapped
to record alarm states automatically.


Format Specifiers
-----------------

All format specifiers use the syntax:

```
%[flags][width][.precision]<specifier>
```

### Specifiers

| Specifier | Read (match) | Write (format) | Description |
| - | - | - | - |
| `%s` | non-whitespace chars | `tostring(v)` | String |
| `%c` | any characters | first char (or width chars) | Character |
| `%d` | signed decimal | `string.format("%d")` | Signed integer |
| `%u` | unsigned decimal | `string.format("%u")` | Unsigned integer |
| `%o` | octal digits | `string.format("%o")` | Octal integer |
| `%x` | hex digits (with optional `0x` prefix) | `string.format("%x")` | Hexadecimal integer |
| `%f` | fixed-point float | `string.format("%f")` | Fixed-point float |
| `%e` | scientific notation | `string.format("%e")` | Scientific notation |
| `%g` | general float | `string.format("%g")` | General float |
| `%E` | scientific (uppercase) | `string.format("%E")` | Uppercase scientific |
| `%G` | general (uppercase) | `string.format("%G")` | Uppercase general |
| `%b` | binary digits (`01`) | integer to binary string | Binary integer |
| `%r` | raw N bytes (width = count, default 1) | passthrough | Raw bytes |
| `%{a\|b\|c}` | match enum string, return 0-based index | index to enum string | Enumeration |
| `%%` | literal `%` | literal `%` | Escape |

### Flags

| Flag | Read effect | Write effect |
| - | - | - |
| `*` | Match but discard (no capture) | Skip conversion, consume no argument |
| `-` | Allow negative sign on `%o`, `%x` | Left-align output |
| `0` | (none) | Zero-pad output |
| `#` | Allow spaces after sign | (reserved) |
| `?` | Lenient: return default on no match instead of nil | (reserved) |
| `!` | Width is exact, not maximum | (reserved) |

### Width and Precision

On the read side, width limits the maximum number of characters consumed by the
pattern. With the `!` flag, width becomes an exact character count.

On the write side, width and precision work like `string.format`:

```lua
bs.format("%10d", 42)       -- "        42"  (right-aligned in 10 chars)
bs.format("%-10d", 42)      -- "42        "  (left-aligned)
bs.format("%05d", 42)       -- "00042"       (zero-padded)
bs.format("%.2f", 3.14159)  -- "3.14"        (2 decimal places)
bs.format("%08b", 42)       -- "00101010"    (8-bit zero-padded binary)
```

### Enumerations

Enum specifiers use `{value0|value1|value2}` syntax with 0-based indexing:

```lua
-- Read: match string, return index
bs.match("%{off|on|standby}", "standby")       -- returns 2

-- Write: index to string
bs.format("%{off|on|standby}", 1)              -- returns "on"
```

When enum values share a common prefix (e.g., `{on|one|only}`), the longest
match is tried first to avoid false matches.


Formatting and Parsing
----------------------

### bytestream.match
---

```
bytestream.match (format, input)
```

Parses an input string according to the format specifier and returns the
extracted values. Multiple conversions produce multiple return values.

Returns the parsed values on success, or nil if the input does not match
the format. Raises an error if the format specifier itself is invalid.

```lua
local bs = require("bytestream")

-- Single value
local n = bs.match("%d", "42")                        -- 42

-- Multiple values
local x, y = bs.match("%d %d", "10 20")               -- 10, 20

-- Mixed types with literal context
local v, u = bs.match("VOLTS %f %s", "VOLTS 3.14 V")  -- 3.14, "V"

-- Hex with optional prefix
local h = bs.match("%x", "0xFF")                       -- 255

-- Enum
local mode = bs.match("%{off|on}", "on")               -- 1

-- Ignore flag: match but don't capture
local val = bs.match("%*s %d", "skip 42")              -- 42

-- Literal percent
local n = bs.match("%%%d", "%42")                      -- 42
```

| Parameter | Type | Description |
| - | - | - |
| format | string | Format specifier using `%` conversions and literal text. |
| input  | string | The string to parse. |

Compiled patterns are cached internally -- calling `match` with the same
format string multiple times reuses the compiled LPeg grammar.

<br>

### bytestream.format
---

```
bytestream.format (format, ...)
```

Formats values into a string according to the format specifier.

Returns the formatted string. Raises an error if there are not enough
arguments for the number of conversions, or if an enum index is out of range.

```lua
local bs = require("bytestream")

-- Simple formatting
local s = bs.format("%d", 42)                    -- "42"
local s = bs.format("%.2f", 3.14159)             -- "3.14"

-- Multiple values with literal text
local s = bs.format("X=%d Y=%d", 10, 20)         -- "X=10 Y=20"

-- Enum
local s = bs.format("%{off|on|standby}", 2)       -- "standby"

-- Binary
local s = bs.format("%08b", 42)                   -- "00101010"

-- Ignore flag: skip conversion, consume no argument
local s = bs.format("A%*dB%d", 99)                -- "AB99"

-- Literal percent
local s = bs.format("100%%")                       -- "100%"
```

| Parameter | Type | Description |
| - | - | - |
| format | string | Format specifier using `%` conversions and literal text. |
| ...    | varies | Values to format, consumed left-to-right by each conversion. |

Compiled output grammars are cached internally.

<br>

### bytestream.add_format
---

```
bytestream.add_format (specifier)
```

Registers a custom format specifier for use with `match` and `format`.
The specifier table must contain an `identifier` (the format letter) and
either or both of `read` (for match) and `write` (for format).

```lua
local bs = require("bytestream")

-- Add a custom %B specifier for boolean strings
bs.add_format {
    identifier = "B",
    read = function(flags)
        local lpeg = require("lpeg")
        local pattern = lpeg.P("true") * lpeg.Cc(true)
                      + lpeg.P("false") * lpeg.Cc(false)
        return pattern
    end,
    write = function(flags)
        return function(value)
            return value and "true" or "false"
        end
    end,
}

bs.match("%B", "true")        -- true
bs.format("%B", false)         -- "false"
```

| Parameter | Type | Description |
| - | - | - |
| specifier | table | Table with `identifier` (string), `read` (function), and/or `write` (function). |

Note: adding a format specifier invalidates the pattern cache. The new
specifier will be available in subsequent calls to `match` and `format`.


Bytestream Client
-----------------

### bytestream.client
---

```
bytestream.client (portName [, addr])
```

Creates a bytestream client wrapping an `asyn.client` connected to the
specified port. The client provides `:write()` and `:read()` methods with
built-in formatting and parsing.

```lua
local bs = require("bytestream")

local dev = bs.client("SERIAL1")
dev.OutTerminator = "\n"
dev.InTerminator  = "\n"
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr     | number | The asyn address. Optional, default is 0. |

<br>

### client:write
---

```
client:write (data)
client:write (format, ...)
```

Writes data to the port. If called with a single string argument, writes it
as-is. If called with a format string and additional arguments, formats the
data using `bytestream.format` before writing.

Returns the client object (`self`) to allow chaining with `:read()`.

Raises an error on write failure.

```lua
-- Write a literal command
dev:write("*RST")

-- Write a formatted command
dev:write("SET:VOLT %.3f", 3.300)

-- Chain with read
local val = dev:write("MEAS?"):read("%f")
```

| Parameter | Type | Description |
| - | - | - |
| data/format | string | Literal string to write, or a format specifier if additional arguments follow. |
| ...         | varies | Values to format (only when using format mode). |

The `OutTerminator` property is appended automatically by the underlying
asyn port -- do not include terminators in the write data.

<br>

### client:read
---

```
client:read ()
client:read (format)
```

Reads data from the port. If a format string is provided, parses the
response using `bytestream.match` and returns the extracted values.
Without a format string, returns the raw response string.

Returns multiple values when the format contains multiple conversions.

Raises an error if no data is read from the port, or if the underlying
asyn read fails.

```lua
-- Raw read
local raw = dev:read()

-- Parsed read
local temp = dev:read("%f")

-- Multiple values
local volts, amps = dev:read("%f %f")

-- Chained after write
local status = dev:write("STAT?"):read("%{off|on|standby}")
```

| Parameter | Type | Description |
| - | - | - |
| format | string | Optional format specifier for parsing the response. |

<br>

### client:flush
---

```
client:flush ()
```

Flushes the input buffer on the port. Returns the client object for chaining.

<br>

### Client Properties
---

The following properties are delegated to the underlying `asyn.client`:

| Property | Type | Description |
| - | - | - |
| `InTerminator` | string | Get or set the input end-of-string terminator. |
| `OutTerminator` | string | Get or set the output end-of-string terminator. |
| `portName` | string | Read-only. The port name this client is connected to. |
| `addr` | number | Read-only. The address this client is connected to. |

```lua
dev.OutTerminator = "\r\n"
dev.InTerminator  = "\r\n"
print(dev.portName)       -- "SERIAL1"
```


Usage with DTYP Device Support
------------------------------

The bytestream library is most commonly used in DTYP "lua" callback functions
for instrument communication. Errors raised by bytestream (and the underlying
asyn operations) are caught by device support and mapped to record alarm states.

The IOC startup script must register the lua module's paths so that
`require("bytestream")` works in the Lua states created by device support:

```
# st.cmd
< envPaths
luaAddModule("$(LUA)")
```

Or from a Lua startup script:

```lua
-- st.lua
luaAddModule("../..")   -- relative to iocBoot/<ioc>/
```

### Example: SCPI Temperature Sensor

Each load of the script creates a separate Lua state registered under the
port name, so multiple instances for different devices work without collisions.

**Lua script (sensor.lua):**

```lua
local db = require("db")
local bs = require("bytestream")

local P    = P    or "dev:"
local PORT = PORT or "SENSOR"

luaRegisterState(PORT)

local client = bs.client(PORT)
client.OutTerminator = "\n"
client.InTerminator  = "\n"

db.record("ai", P .. "Temperature") {
    DTYP = "lua",
    INP  = "@" .. PORT .. " read_temp()",
    SCAN = "1 second",
    EGU  = "degC",
    PREC = "2",
}

function read_temp()
    return client:write("MEAS:TEMP?"):read("%f")
end

db.record("ao", P .. "Setpoint") {
    DTYP = "lua",
    OUT  = "@" .. PORT .. " write_setpoint()",
    EGU  = "degC",
    PREC = "1",
}

function write_setpoint(record)
    client:write("SET:TEMP %.1f", record.VAL)
end

db.record("longin", P .. "Status") {
    DTYP = "lua",
    INP  = "@" .. PORT .. " read_status()",
    SCAN = "1 second",
}

function read_status()
    return client:write("STAT?"):read("%{off|on|standby}")
end
```

**Startup:**

```lua
luaAddModule("$(LUA)")
drvAsynIPPortConfigure("SENSOR1", "192.168.1.100:5025")
luaLoadFile("sensor.lua", {P="dev1:", PORT="SENSOR1"})

drvAsynIPPortConfigure("SENSOR2", "192.168.1.101:5025")
luaLoadFile("sensor.lua", {P="dev2:", PORT="SENSOR2"})
```

### Example: Multi-Value Response

Some instruments return multiple values in a single response. Use multiple
format conversions to extract them all:

```lua
function read_measurements()
    -- Device responds with: "12.34,56.78,90.12"
    local v1, v2, v3 = client:write("MEAS:ALL?"):read("%f,%f,%f")
    return v1  -- returned as record VAL
end
```

### Example: Ignoring Fields in a Response

Use the `*` flag to match but discard unwanted fields:

```lua
function read_third_value()
    -- Response: "1.0 2.0 3.0 4.0"
    -- Skip first two values, capture the third
    return client:write("READ?"):read("%*f %*f %f")
end
```


Comparison with StreamDevice
----------------------------

The bytestream library follows StreamDevice format specifier conventions,
making it straightforward to translate protocol files into Lua. The key
differences are:

| Feature | StreamDevice | bytestream |
| - | - | - |
| Language | Protocol file (.proto) | Lua script |
| Format specifiers | `%f`, `%d`, `%s`, etc. | Same syntax |
| Enum syntax | `%{val0\|val1\|val2}` | Same syntax |
| Separator handling | `Separator` field | Literal text in format string |
| Terminators | `Terminator` field | `client.OutTerminator` / `client.InTerminator` |
| Error handling | Record alarm on mismatch | `error()` caught by DTYP support |
| I/O direction | `out`, `in` protocol commands | `client:write(...)`, `client:read(...)` |
| Conditional logic | Limited (`@init`, `@mismatch`) | Full Lua control flow |

### StreamDevice Protocol vs. Lua

StreamDevice protocol:
```
getTemperature {
    out "MEAS:TEMP?";
    in "%f";
}
```

Equivalent Lua with bytestream:
```lua
function read_temp()
    return client:write("MEAS:TEMP?"):read("%f")
end
```
