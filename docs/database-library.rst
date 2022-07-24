==============================
Database Library Documentation
==============================

**db.entry** ()

::

   Creates a DBENTRY pointer which can be used with all implemented static database 
   access functions. dbFreeEntry will be called on the pointer automatically when the 
   entry is garbage collected.


Static Database Access
----------------------

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




**db.record** (recordtype, recordname)

::

   Adds a record of the given type and name to the IOC


   recordtype   [string] - The typename of the record to create (ai, mbbo, calc, etc)

   recordname   [string] - The name to create the record with


   Returns a table that can be called as a function that takes a single table as input.
   Each key-value pair in that input table are treated as a field definition, with the
   key as the name of the field, and the value in the table as the value to set the
   field to.

   With lua syntactical sugar, the generation and calling of the table can be combined.
   An example of this would be:

   db.record("ai
      DTYP = "asynInt32",
      INP = "@asyn(A_PORT,0,1)PARAM_NAME"
   }

