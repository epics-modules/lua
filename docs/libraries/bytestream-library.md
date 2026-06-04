---
layout: default
title: bytestream
parent: Included Libraries
nav_order: 5
---

# Bytestream Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The bytestream library provides scanf-style parsing and printf-style
formatting for byte stream device communication. It is a pure Lua
library built on top of LPeg and asyn.client.

Format specifiers follow StreamDevice conventions, making it
straightforward to translate protocol files into Lua code.

{: .note }
> The bytestream library is installed to `lib/<arch>/`. Call
> `luaAddModule` in your startup script to make it available via
> `require`.

```lua
local bs = require("bytestream")
```


Format Specifiers
-----------------

All format specifiers use the syntax `%[flags][width][.precision]<specifier>`.

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
| `?` | Lenient: return default on no match | (reserved) |
| `!` | Width is exact, not maximum | (reserved) |

### Width and Precision

On the read side, width limits the maximum characters consumed. With
the `!` flag, width becomes an exact count.

On the write side, width and precision work like `string.format`:

```lua
bs.format("%10d", 42)       -- "        42"
bs.format("%-10d", 42)      -- "42        "
bs.format("%05d", 42)       -- "00042"
bs.format("%.2f", 3.14159)  -- "3.14"
bs.format("%08b", 42)       -- "00101010"
```

### Enumerations

Enum specifiers use `{value0|value1|value2}` syntax with 0-based
indexing. Longest match is tried first to avoid prefix conflicts.

```lua
bs.match("%{off|on|standby}", "standby")    -- returns 2
bs.format("%{off|on|standby}", 1)           -- returns "on"
```


Formatting and Parsing
----------------------

### bytestream.match
---

Parse an input string using a format specifier.

```
bytestream.match (format, input)
```

Extracts values from the input string according to the format.
Multiple conversions produce multiple return values.

```lua
local n      = bs.match("%d", "42")
local x, y   = bs.match("%d %d", "10 20")
local v, u   = bs.match("VOLTS %f %s", "VOLTS 3.14 V")
local val    = bs.match("%*s %d", "skip 42")
```

| Parameter | Type | Description |
| - | - | - |
| format | string | Format specifier with `%` conversions and literal text. |
| input | string | The string to parse. |

**Returns:** the extracted values, or `nil` if the input does not match.

**Raises:** an error if the format specifier is invalid.

<br>

### bytestream.format
---

Format values into a string.

```
bytestream.format (format, ...)
```

Produces an output string from the format specifier and values.

```lua
local s = bs.format("%d", 42)                   -- "42"
local s = bs.format("X=%d Y=%d", 10, 20)        -- "X=10 Y=20"
local s = bs.format("%{off|on|standby}", 2)      -- "standby"
local s = bs.format("%08b", 42)                  -- "00101010"
```

| Parameter | Type | Description |
| - | - | - |
| format | string | Format specifier with `%` conversions and literal text. |
| ... | varies | Values to format, consumed left-to-right. |

**Returns:** the formatted string.

**Raises:** an error if there are not enough arguments or an enum
index is out of range.

<br>

### bytestream.add_format
---

Register a custom format specifier.

```
bytestream.add_format (specifier)
```

Adds a new format letter for use with `match` and `format`. The
specifier table must contain an `identifier` and either or both of
`read` and `write`.

```lua
bs.add_format {
    identifier = "B",
    read = function(flags)
        local lpeg = require("lpeg")
        return lpeg.P("true") * lpeg.Cc(true)
             + lpeg.P("false") * lpeg.Cc(false)
    end,
    write = function(flags)
        return function(value)
            return value and "true" or "false"
        end
    end,
}
```

| Parameter | Type | Description |
| - | - | - |
| specifier | table | Table with `identifier` (string), `read` (function), and/or `write` (function). |


Bytestream Client
------------------

### bytestream.client
---

Create a client for structured device I/O.

```
bytestream.client (portName [, addr])
```

Wraps an `asyn.client` with `:write()` and `:read()` methods that
support format specifiers.

```lua
local dev = bs.client("SERIAL1")
dev.OutTerminator = "\n"
dev.InTerminator  = "\n"
```

| Parameter | Type | Description |
| - | - | - |
| portName | string | A registered asyn port name. |
| addr | number | Optional. The asyn address. Default: 0. |

**Returns:** a bytestream client object.

<br>

### client:write
---

Write data to the port.

```
client:write (data)
client:write (format, ...)
```

Writes a literal string or formats data using `bytestream.format`
before writing. Returns `self` for chaining with `:read()`.

```lua
dev:write("*RST")
dev:write("SET:VOLT %.3f", 3.300)
local val = dev:write("MEAS?"):read("%f")
```

| Parameter | Type | Description |
| - | - | - |
| data/format | string | Literal string, or format specifier if additional arguments follow. |
| ... | varies | Values to format. |

**Returns:** the client object (for chaining).

**Raises:** an error on write failure.

{: .important }
> The `OutTerminator` is appended automatically by the asyn port.
> Do not include terminators in the write data.

<br>

### client:read
---

Read and optionally parse data from the port.

```
client:read ()
client:read (format)
```

Reads from the port. If a format string is given, parses the response
using `bytestream.match` and returns the extracted values.

```lua
local raw    = dev:read()
local temp   = dev:read("%f")
local v, a   = dev:read("%f %f")
local status = dev:write("STAT?"):read("%{off|on|standby}")
```

| Parameter | Type | Description |
| - | - | - |
| format | string | Optional. Format specifier for parsing. |

**Returns:** the parsed values, or the raw string if no format given.

**Raises:** an error if no data is read or the asyn read fails.

<br>

### client:flush
---

Flush the input buffer.

```
client:flush ()
```

**Returns:** the client object (for chaining).

<br>

### Client Properties

Properties delegated to the underlying `asyn.client`:

| Property | Type | Description |
| - | - | - |
| `InTerminator` | string | Get or set the input end-of-string terminator. |
| `OutTerminator` | string | Get or set the output end-of-string terminator. |
| `ReadTimeout` | number | Get or set the read timeout in seconds (default 1.0). |
| `WriteTimeout` | number | Get or set the write timeout in seconds (default 1.0). |
| `portName` | string | Read-only. The port name. |
| `addr` | number | Read-only. The address. |


Usage with DTYP Device Support
------------------------------

The bytestream library is commonly used in DTYP "lua" callbacks for
instrument communication. Errors raised by bytestream are caught by
device support and mapped to record alarm states.

{: .note }
> Call `luaAddModule("$(LUA)")` in your startup script to make
> `require("bytestream")` available in device support states.

### Example: SCPI Temperature Sensor

Each load creates a separate Lua state registered under the port name,
allowing multiple instances without collisions.

```lua
local db = require("db")
local bs = require("bytestream")

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
```

**Startup:**

```lua
luaAddModule("$(LUA)")
drvAsynIPPortConfigure("SENSOR1", "192.168.1.100:5025")
luaLoadFile("sensor.lua", {P="dev1:", PORT="SENSOR1"})
```


Comparison with StreamDevice
----------------------------

| Feature | StreamDevice | bytestream |
| - | - | - |
| Language | Protocol file (.proto) | Lua script |
| Format specifiers | `%f`, `%d`, `%s`, etc. | Same syntax |
| Enum syntax | `%{val0\|val1\|val2}` | Same syntax |
| Separator handling | `Separator` field | Literal text in format string |
| Terminators | `Terminator` field | `client.OutTerminator` / `client.InTerminator` |
| I/O direction | `out`, `in` protocol commands | `client:write(...)`, `client:read(...)` |
| Conditional logic | Limited (`@init`, `@mismatch`) | Full Lua control flow |
