==============================
Database Library Documentation
==============================

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

   db.record("ai", "x:y:z") {
      DTYP = "asynInt32",
      INP = "@asyn(A_PORT,0,1)PARAM_NAME"
   }

