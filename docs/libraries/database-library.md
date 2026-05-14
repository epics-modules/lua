---
layout: default
title: db
parent: Included Libraries
nav_order: 2
---


# Database Library Documentation
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}


Static Database Access
----------------------

### db.entry
---

```
db.entry ()
```

Creates a DBENTRY pointer which can be used with all implemented static database 
access functions. dbFreeEntry will be called on the pointer automatically when the 
entry is garbage collected.

<br>

### EPICS Base Functions

The following list of static database access functions are implemented, largely unchanged from their
C API. Naming conventions have been changed to drop the initial "db" prefix and for the next character
to be lowercase; so, for example, dbGetFieldName would become the module function getFieldName. 

Functions which would return a status code, like dbFindRecord, will instead return a boolean representing
success or failure (true or false respectively). Where a DBENTRY pointer is required as a parameter, instead
instances of the dbentry class will be used, created with the aforementioned db.entry function.

* getNRecordTypes
* findRecordType
* firstRecordType
* nextRecordType
* getRecordTypeName
* getNFields
* firstField
* nextField
* getFieldDbfType
* getFieldName
* getDefault
* getPrompt
* getPromptGroup
* putRecordAttribute
* getRecordAttribute
* getNAliases
* getNRecords
* findRecord
* firstRecord
* nextRecord
* getRecordName
* isAlias
* createRecord
* createAlias
* deleteRecord
* deleteAliases
* copyRecord
* findField
* foundField
* getString
* putString
* isDefaultValue
* getNMenuChoices
* getMenuIndex
* putMenuIndex
* getMenuStringFromIndex
* getMenuIndexFromString
* getNLinks
* getLinkField
* firstInfo
* nextInfo
* findInfo
* getInfoName
* getInfoString
* putInfoString
* putInfo
* deleteInfo
* getInfo


<br>

### db.registerDatabaseHook
---

```
db.registerDatabaseHook (dbhook)
```

Registers the provided function so that it is invoked each time the dbLoadRecords 
function is called by the IOC. The callback hook is invoked with two parameters;
the first being the filepath to the database file being loaded, and the other 
being a table of the macro definitions provided to dbLoadRecords.

| Parameter | Type | Description |
| - | - | - | 
| dbhook  |  function |  The callback function to be invoked |

<br>

### db.record
---

```
db.record ([recordtype,] recordname)
```

Creates an instance of the dbrecord class, a wrapper around record creation/access.

Returns a class instance with four instance methods, name, type, field, and info. 
'name' and 'type' are accessor methods that will return the record name and the
RTYP of the record. 

'field' and 'info' are both functions that take in two strings as parameters, the 
first being a name and the second a value. 'field' attempts to find the record 
field with the given name and then calls dbPutString to set the value. While 'info' 
calls dbPutInfo to add a new info field with the given name and value to the record.

Record fields can also be read and written using dot notation:

```lua
rec = db.record("stringin", "x:y:z")

-- Set fields
rec.VAL = "test"
rec.DESC = "My record"

-- Read fields
print(rec.VAL)     --> "test"
print(rec.DESC)    --> "My record"

-- Methods still work
rec:field("VAL", "test")
rec:info("autosave", "VAL")
```

The class instance itself can also be called as a function, taking in a dictionary
of name-value pairs. In doing so, the 'field' function is called for each pair, 
passing through the names and values to the function.

With lua syntactical sugar, you can chain together the record creation and the
setting of fields like so:

```lua
db.record("ai", "x:y:z") {
    DTYP = "asynInt32",
    INP = "@asyn(A_PORT,0,1)PARAM_NAME"
}
```

| Parameter | Type | Description |
| - | - | - |
| recordtype  | string | The typename of the record (ai, mbbo, calc, etc) Optional. If the typename is left out, constructor will operate only to find a record, not create one. |
| recordname  | string | The name of the record. If the name already exists, the returned instance will refer to the existing record. If there is no record by that name, the constructor will create one. |

<br>

### db.loadRecords
---

```
db.loadRecords (filename [, macros])
```

Loads an EPICS database file with optional macro substitutions. Macros can be
provided as a Lua table of key-value pairs, which is more convenient than the
traditional comma-separated string format.

```lua
-- Using a Lua table for macros
db.loadRecords("motor.db", {P="ioc:", M="m1", PORT="serial1", ADDR="0"})

-- Using a plain string (also supported)
db.loadRecords("motor.db", "P=ioc:,M=m1,PORT=serial1,ADDR=0")

-- No macros
db.loadRecords("simple.db")
```

Returns 0 on success, non-zero on error.

| Parameter | Type | Description |
| - | - | - |
| filename | string | Path to the database (.db) file to load |
| macros | table or string | Optional. Macro substitutions as a table of key=value pairs or a comma-separated string |

<br>

### db.loadTemplate
---

```
db.loadTemplate (filename, substitutions)
```

Loads a database file multiple times with different sets of macro substitutions,
equivalent to the EPICS `dbLoadTemplate` function and `.substitutions` file format.
This is useful for creating many similar record instances from a single template.

Three styles of substitution are supported:

**Variable style** -- each entry is a table of named macro values:

```lua
db.loadTemplate("motor.db", {
    {P="ioc:", M="m1", PORT="serial1", ADDR="0"},
    {P="ioc:", M="m2", PORT="serial1", ADDR="1"},
    {P="ioc:", M="m3", PORT="serial2", ADDR="0"},
})
```

**Pattern style** -- a `pattern` key names the macro variables, and each subsequent
entry provides positional values. This mirrors the `pattern` directive in EPICS
substitution files:

```lua
db.loadTemplate("motor.db", {
    pattern = {"P", "M", "PORT", "ADDR"},
    {"ioc:", "m1", "serial1", "0"},
    {"ioc:", "m2", "serial1", "1"},
    {"ioc:", "m3", "serial2", "0"},
})
```

**Global macros** -- a `global` key provides macros that are merged into every
instance. This mirrors the `global` directive in EPICS substitution files:

```lua
db.loadTemplate("motor.db", {
    global = {P="ioc:", PORT="serial1"},
    {M="m1", ADDR="0"},
    {M="m2", ADDR="1"},
    {M="m3", ADDR="2"},
})
```

Global and pattern styles can be combined:

```lua
db.loadTemplate("motor.db", {
    global  = {P="ioc:"},
    pattern = {"M", "PORT", "ADDR", "DESC"},
    {"m1", "serial1", "0", "Sample X"},
    {"m2", "serial1", "1", "Sample Y"},
    {"m3", "serial2", "0", "Detector Z"},
})
```

Because this is Lua, you can use loops and computed values to generate
substitution tables programmatically:

```lua
local motors = {}
for i = 1, 10 do
    table.insert(motors, {
        P="ioc:", M="m"..i, PORT="serial1",
        ADDR=tostring(i-1), DESC="Motor "..i
    })
end
db.loadTemplate("motor.db", motors)
```

Internally, `db.loadTemplate` calls `dbLoadRecords` for each substitution entry,
so the standard `dbLoadRecordsHook` mechanism fires for each instance.

| Parameter | Type | Description |
| - | - | - |
| filename | string | Path to the database (.db or .template) file to load |
| substitutions | table | A list of macro tables. May optionally contain `pattern` and/or `global` keys. |

<br>

### db.list
---

```
db.list ()
```

Returns a list of all the PVs currently defined in the IOC. Each element of the
list is a db.record instance.
