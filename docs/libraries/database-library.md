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

```lua
rec = db.record("stringin", "x:y:z")
rec:field("VAL", "test")
rec:info("autosave", "VAL")
```

The class instance itself can also be called as a function, taking in a dictionary
of name-vale pairs. In doing so, the 'field' function is called for each pair, 
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

### db.list
---

```
db.list ()
```

Returns a list of all the PVs currently defined in the IOC. Each element of the
list is a db.record instance.
