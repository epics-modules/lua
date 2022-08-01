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



**db.registerDatabaseHook** (dbhook)

::
   
   Registers the provided function so that it is invoked each time the dbLoadRecords 
   function is called by the IOC. The callback hook is invoked with two parameters;
   the first being the filepath to the database file being loaded, and the other 
   being a table of the macro definitions provided to dbLoadRecords.

   dbhook    [function]  - The callback function to be invoked



**db.record** ([recordtype,] recordname)

::

   Creates an instance of the dbrecord class, a wrapper around record creation/access.

   recordtype   [string] - The typename of the record (ai, mbbo, calc, etc) Optional. 
                           If the typename is left out, constructor will operate only
                           to find a record, not create one.
						
   recordname   [string] - The name of the record. If the name already exists, the
                           returned instance will refer to the existing record. If
                           there is no record by that name, the constructor will
                           create one.

   Returns a class instance with two instance methods, field and info. Both functions 
   take in two strings as parameters, the first a name and the second a value. 'field'
   attempts to find the record field with a given name and then calls dbPutString to 
   set the value. While 'info' calls dbPutInfo to add a new info field with the given
   name and value to the record.
   
   
   rec = db.record("stringin", "x:y:z")
   rec:field("VAL", "test")
   rec:info("autosave", "VAL")
   
   
   The class instance itself can also be called as a function, taking in a dictionary
   of name-vale pairs. In doing so, the 'field' function is called for each pair, 
   passing through the names and values to the function.

   With lua syntactical sugar, you can chain together the record creation and the
   setting of fields like so:

   db.record("ai", "x:y:z") {
      DTYP = "asynInt32",
      INP = "@asyn(A_PORT,0,1)PARAM_NAME"
   }

