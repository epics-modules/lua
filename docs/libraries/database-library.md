---
layout: default
title: db
parent: Included Libraries
nav_order: 2
---


# Database Library
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

The db library provides functions for creating and inspecting EPICS
database records from Lua, replacing traditional `.db` files and
substitution files with programmatic record creation.

```lua
local db = require("db")
```


Record Creation
---------------

### db.record
---

Create or find a database record.

```
db.record ([recordtype,] recordname)
```

Creates an instance of the dbrecord class. If both a type and name are
given and the record does not exist, it is created. If only a name is
given, the constructor finds an existing record.

The returned object can be called as a function with a table of
field name-value pairs to set multiple fields at once:

```lua
db.record("ai", "x:y:z") {
    DTYP = "asynInt32",
    INP  = "@asyn(A_PORT,0,1)PARAM_NAME",
    PREC = "2",
}
```

Fields can also be read and written using dot notation:

```lua
local rec = db.record("stringin", "x:y:z")

rec.VAL  = "test"
rec.DESC = "My record"

print(rec.VAL)    -- "test"
```

| Parameter | Type | Description |
| - | - | - |
| recordtype | string | Optional. The record type (ai, ao, bi, etc). If omitted, finds an existing record by name. |
| recordname | string | The name of the record. |

**Returns:** a dbrecord object.

<br>

### db.loadRecords
---

Load a database file with macro substitutions.

```
db.loadRecords (filename [, macros])
```

Loads an EPICS database file. Macros can be provided as a Lua table
of key-value pairs or as a comma-separated string.

```lua
db.loadRecords("motor.db", {P="ioc:", M="m1", PORT="serial1"})
db.loadRecords("motor.db", "P=ioc:,M=m1,PORT=serial1")
db.loadRecords("simple.db")
```

| Parameter | Type | Description |
| - | - | - |
| filename | string | Path to the database (.db) file to load. |
| macros | table or string | Optional. Macro substitutions. |

**Returns:** 0 on success, non-zero on error.

<br>

### db.loadTemplate
---

Load a template with multiple sets of substitutions.

```
db.loadTemplate (filename, substitutions)
```

Loads a database file multiple times with different macro substitutions,
equivalent to the EPICS `dbLoadTemplate` function.

Three styles of substitution are supported:

**Variable style** -- each entry is a table of named macro values:

```lua
db.loadTemplate("motor.db", {
    {P="ioc:", M="m1", PORT="serial1", ADDR="0"},
    {P="ioc:", M="m2", PORT="serial1", ADDR="1"},
})
```

**Pattern style** -- a `pattern` key names the macro variables:

```lua
db.loadTemplate("motor.db", {
    pattern = {"P", "M", "PORT", "ADDR"},
    {"ioc:", "m1", "serial1", "0"},
    {"ioc:", "m2", "serial1", "1"},
})
```

**Global macros** -- a `global` key provides macros shared by all entries:

```lua
db.loadTemplate("motor.db", {
    global  = {P="ioc:", PORT="serial1"},
    pattern = {"M", "ADDR"},
    {"m1", "0"},
    {"m2", "1"},
})
```

| Parameter | Type | Description |
| - | - | - |
| filename | string | Path to the database file to load. |
| substitutions | table | A list of macro tables. May contain `pattern` and/or `global` keys. |

**Returns:** nothing.

<br>

Record Inspection
-----------------

### db.list
---

List all records in the IOC.

```
db.list ()
```

**Returns:** a table of dbrecord objects, one for each PV defined in
the IOC.

```lua
local records = db.list()
for _, rec in ipairs(records) do
    print(rec.name)
end
```

<br>

Record Object
-------------

The dbrecord object returned by `db.record` and `db.list` provides:

| Property/Method | Description |
| - | - |
| `rec.name` | The record name (read-only property). |
| `rec.type` | The record type, e.g., `"ai"` (read-only property). |
| `rec.FIELD` | Read a field value via dot syntax. |
| `rec.FIELD = value` | Write a field value via dot syntax. |
| `rec:field(name, value)` | Set a field value by name. |
| `rec:info(name, value)` | Add an info tag to the record. |

<br>

Database Hooks
--------------

### db.registerDatabaseHook
---

Register a callback for database loading events.

```
db.registerDatabaseHook (callback)
```

Registers a function that is called each time `dbLoadRecords` is
invoked. The callback receives the file path and a table of macros.

| Parameter | Type | Description |
| - | - | - |
| callback | function | Called with `(filepath, macros_table)`. |

<br>

Static Database Access
----------------------

### db.entry
---

Create a DBENTRY cursor for low-level database inspection.

```
db.entry ()
```

Creates a cursor that can be used with all static database access
functions listed below. The cursor is automatically freed when
garbage collected.

The entry object supports two calling styles:

```lua
local ent = db.entry()

-- Method syntax
ent:findRecord("myrecord")
print(ent:getFieldName())

-- Module function syntax
db.findRecord(ent, "myrecord")
print(db.getFieldName(ent))
```

**Returns:** a dbentry object.

### EPICS Base Functions

The following static database access functions are available, with
names adapted from the C API (the `db` prefix is dropped, and the
next character is lowercased):

* getNRecordTypes, findRecordType, firstRecordType, nextRecordType,
  getRecordTypeName
* getNFields, firstField, nextField, getFieldDbfType, getFieldName,
  getDefault, getPrompt, getPromptGroup
* putRecordAttribute, getRecordAttribute
* getNAliases, getNRecords
* findRecord, firstRecord, nextRecord, getRecordName
* isAlias, createRecord, createAlias, deleteRecord, deleteAliases,
  copyRecord
* findField, foundField, getString, putString, isDefaultValue
* getNMenuChoices, getMenuIndex, putMenuIndex,
  getMenuStringFromIndex, getMenuIndexFromString
* getNLinks, getLinkField
* firstInfo, nextInfo, findInfo, getInfoName, getInfoString,
  putInfoString, putInfo, deleteInfo, getInfo

Functions that return a status code in C return a boolean (`true` for
success, `false` for failure) in Lua.
