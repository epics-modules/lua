---
layout: default
title: iocsh
parent: Included Libraries
nav_order: 4
---

# IOC Shell Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The iocsh library provides access to EPICS environment variables and
iocsh-registered functions from Lua. In the Lua shell, this functionality
is embedded into the global environment automatically -- you can reference
environment variables and iocsh functions by name without loading the
library explicitly.

```lua
local iocsh = require("iocsh")
```


Environment and Function Lookup
--------------------------------

### iocsh.\<item\>
---

Look up an environment variable or iocsh function by name.

```
iocsh.<item> [(arguments)]
```

First checks for an EPICS environment variable matching the name. If
none is found, searches for a registered iocsh function. Returns `nil`
if neither is found.

```lua
-- Access an environment variable
local arch = iocsh.EPICS_HOST_ARCH

-- Call an iocsh function
iocsh.dbLoadRecords("my.db", "P=test:")

-- In the Lua shell, the iocsh prefix is optional:
EPICS_HOST_ARCH
dbLoadRecords("my.db", "P=test:")
```

{: .note }
> When using the Lua shell, iocsh lookups are built into the global
> environment. Any name that isn't a Lua variable is automatically
> checked as an environment variable or iocsh function. The explicit
> `iocsh.` prefix is only needed when using `require("iocsh")` in
> scripts outside the shell.

| Parameter | Type | Description |
| - | - | - |
| item | string | The name of an environment variable or iocsh function. |
| arguments | varies | If referencing an iocsh function, the arguments to pass to it. |

**Returns:** the environment variable value (as a string), the iocsh
function reference, or `nil` if not found.
