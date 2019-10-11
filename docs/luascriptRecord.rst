luascript - Lua Script Processing Record
========================================

.. contents::
   :depth: 3
..

--------------

Introduction
---------------

The lua script processing record or "luascript" record is derived from
the Calcout record in EPICS base, but replaces the calc operation engine
with the lua scripting language.  The record has 10 string fields
(AA...JJ) and 10 double fields (A...J) whose values are retrieved every
time the record is processed and those values are pushed into lua as
global variables with the same name as the field.

The luascript record has both a VAL and SVAL output field. If the return
operator is used within a lua expression, the returned value is placed
into one of these fields. Booleans or Numbers that are returned get
their value put into the VAL field, while Strings will be put into the
SVAL field.

When writing to a string PV (any of DBF_STRING, DBF_ENUM, DBF_MENU,
DBF_DEVICE, DBF_INLINK, DBF_OUTLINK, DBF_FWDLINK) the record (actually,
device support) writes its string value (SVAL). When writing to any
other kind of PV, the record writes its numeric value (VAL).

To write successfully to a DBF_MENU or DBF_ENUM (for example, the VAL
field of a ``bo`` or ``mbbo`` record) the record's string value must be
one of the possible strings for the PV, or it must an integer specifying
the string number [0..N] for the PV. For example, when writing to a
``bo`` record whose ZNAM is "No" and whose ONAM is "Yes", the string
value must be one of the following: "No", "Yes", "0", or "1". To ensure
that numeric values are converted to integers, set the precision (the
PREC field) to zero.

 

Scan Parameters
---------------

The luascript record has the standard fields for specifying under what
circumstances the record will be processed. These fields are listed in
`Scan Fields, Chapter 2,
2 <http://aps.anl.gov/epics/EpicsDocumentation/AppDevManuals/RecordRef/Recordref-6.html#MARKER-9-2>`__.
In addition, `Scanning Specification, Chapter 1,
1 <http://aps.anl.gov/epics/EpicsDocumentation/AppDevManuals/RecordRef/Recordref-5.html#MARKER-9-2>`__,
explains how these fields are used. Since the luascript record supports
no direct interfaces to hardware, it cannot be scanned on I/O interrupt,
so its SCAN field cannot be ``I/O Intr``.


Read Parameters
---------------

The read parameters for the luascript record consist of 20 input links:
10 to numeric fields (INPA -> A, INPB -> B, . . . INPJ -> J); and 10 to
string fields (INAA -> AA, INBB -> BB, ...INJJ -> JJ). The fields can be
database links, channel access links, or constants. If they are links,
they must specify another record's field. If they are constants, they
will be initialized with the value they are configured with and can be
changed via ``dbPuts``. These fields cannot be hardware addresses. In
addition, the luascript record contains the fields INAV, INBV, . . .
INJV, which indicate the status of the links to numeric fields, and the
fields IAAV, IBBV, . . . IJJV, which indicate the status of the links to
string fields.  These fields indicate whether or not the specified PV
was found and a link to it established. See `Section 5, Operator Display
Parameters <#MARKER-9-2>`__ for an explanation of these fields.

From most types of PV's, the luascript record's string input links fetch
data as strings, and depend on EPICS to convert a PV's native value to
string. But when a luascript record's string input link names a PV whose
data type is an array of DBF_CHAR or DBF_UCHAR, the record fetches up to
39 array elements from the array in their native form, translates the
result using epicsStrSnPrintEscaped(), and truncates the result to fit
into a 40-character string.

 See the EPICS Record Reference Manual for information on how to specify
database links.

.. csv-table:: 
   :header: "Field", "Summary", "Type", "DCT", "Initial", "Access", "Modify", "Rec Proc Monitor"
   :widths: 2, 3, 2, 1, 2, 2, 2, 3

   INPA, Input Link A, INLINK, Yes, 0, Yes, Yes, N/A
   INPB, Input Link B, INLINK, Yes, 0, Yes, Yes, N/A
   ...,  ..., ..., ..., ..., ..., ..., ...
   INPL,  Input Link J,  INLINK, Yes, 0,       Yes,    Yes,    N/A
   INAA,  Input Link AA, INLINK, Yes, 0,       Yes,    Yes,    N/A
   INBB,  Input Link BB, INLINK, Yes, 0,       Yes,    Yes,    N/A
   ...,  ..., ..., ..., ..., ..., ..., ...
   INJJ,  Input Link JJ, INLINK, Yes, 0,       Yes,    Yes,    N/A

Expressions
-----------

The luascript record has a CODE field into which you can enter an
expression for the record to evaluate when it processes. The return
operator can be used to return either a numeric or string variable for
writing to the VAL or SVAL field respectively. Either VAL and SVAL can
also be written to the output link. (If you elect to write an output
value, the record will choose between VAL and SVAL, depending on the
data type of the field at the other end of the output link.)

The CODE expression can also be used to reference a file containing the
description of at least a single lua function. If the CODE field starts
with the symbol '@' followed by the name of said file, the luascript
record will search through a list of directories given by the
environment variable 'LUA_SCRIPT_PATH' (default: current directory) for
the given file. A space character and then the name of a function
defined in the file lets the luascript record know what function to call
when the record processes. Optionally, a set of parameters can be
provided that the function will be called with each processing by
including a comma separated list enclosed by parentheses.

When changing the CODE field, the luascript record's RELO field controls
whether or not the record will recompile the string into a new lua
state, resetting any variables in the global scope. The field is a menu
with three choices:

-  ``Every New File`` -- Recompile only if the file referenced is
   changed, the record can be changed to point to a new function within
   that file without losing any prior state.
-  ``Every New Change`` -- Recompile on any change to the CODE field.
-  ``Every Processing`` -- Recompile before each time the record is
   processed.

There is also the FRLD field which forces the record to recompile a new
lua state when a non-zero value is written to it.

Finally, the ERR field contains a string representation of the last
error encountered during processing.

| 
| The record also has a second set of calculation-related fields
  described in `Section 4, Output Parameters. <#MARKER-9-1>`__
|  

.. csv-table:: 
   :header: "Field", "Summary", "Type", "DCT", "Initial", "Access", "Modify", "Rec Proc Monitor", "PP"
   :widths: 2, 3, 2, 1, 2, 2, 2, 3, 1

   CODE,  Script,                STRING[120],  Yes, 0,       Yes,    Yes,    Yes,              No
   VAL,   Value,                 DOUBLE,       No,  0,       Yes,    Yes,    Yes,              No
   SVAL,  String value,          STRING (40),  No,  0,       Yes,    Yes,    Yes,              No
   RELO,  When to reload state?, Menu,         Yes, 0,       Yes,    Yes,    No,               No
   FRLD,  Force Reload,          Short,        Yes, 0,       Yes,    Yes,    No,               No
   ERR,   Last Error,            String (200), No,  0,       Yes,    Yes,    No,               No

Examples
^^^^^^^^

field(CODE, "return A + B")

-  Sets VAL to the result of A + B

field(CODE, "return AA .. BB")

-  Sets SVAL to the concatenation of AA and BB

field(CODE, "@test.lua example")

-  Runs the function 'example' from the file test.lua with zero
   parameters.

field(CODE, "@test.lua example(1, 'foo')")

-  Runs the function 'example' from the file test.lua with two
   parameters, one a number, the other a string.


Output Parameters
-----------------

These parameters specify and control the output capabilities of the
luascript record. They determine when to write the output, where to
write it, and what the output will be. The OUT link specifies the
Process Variable to which the result will be written. The OOPT field
determines the condition that causes the output link to be written to.
It's a menu field that has six choices:

-  ``Every Time`` -- write output every time record is processed.
-  ``On Change`` -- write output every time VAL/SVAL changes, i.e.,
   every time the result of the expression changes.
-  ``When Zero`` -- when record is processed, write output if VAL is
   zero or if SVAL is an empty string.
-  ``When Non-zero`` -- when record is processed, write output if VAL is
   non-zero or SVAL is a non-empty string.
-  ``Transition to Zero`` -- when record is processed, write output only
   if VAL is zero and last value was non-zero. If SVAL was changed,
   write output only if SVAL is an empty string and the last value was a
   non-empty string.
-  ``Transition to Non-zero`` -- when record is processed, write output
   only if VAL is non-zero and last value was zero. If SVAL was changed,
   write output only if SVAL is a non-empty string and the last value
   was a empty string.
-  ``Never`` -- Don't write output ever.

The SYNC field controls whether the record processes in a synchronous or
asynchronous manner. It is a menu field with two choices:

-  ``Synchronous`` -- process the record's lua code synchronously.
-  ``Asynchronous`` -- process the record's lua code in a separate
   thread.

.. csv-table:: 
   :header: "Field", "Summary", "Type", "DCT", "Initial", "Access", "Modify", "Rec Proc Monitor", "PP"
   :widths: 2, 3, 2, 1, 2, 2, 2, 3, 1

   OUT,   Output Specification,  OUTLINK, Yes, 0,       Yes,    Yes,    N/A,              No
   OOPT,  Output Execute Option, Menu,    Yes, 0,       Yes,    Yes,    No,               No
   SYNC,  Synchronicity,         Menu,    Yes, 0,       Yes,    Yes,    No,               No

The luascript record uses device support to write to the ``OUT`` link.
Soft device supplied with the record is selected with the .dbd
specification

::

    field(DTYP,"Soft Channel") 


Operator Display Parameters
---------------------------

These parameters are used to present meaningful data to the operator.
Some are also meant to represent the status of the record at run-time.
An example of an interactive MEDM display screen that displays the
status of the luascript record is located here.

The HOPR and LOPR fields only refer to the limits of the VAL, HIHI,
HIGH, LOW, and LOLO fields. PREC controls the precision of the VAL
field.

The INAV-INJV and IAAV-IJJV fields indicate the status of the link to
the PVs specified in the INPA-INPJ and INAA-INJJ fields, respectively.
The fields can have three possible values:

========= ==================================================================================
Ext PV NC the PV wasn't found on this IOC and a Channel Access link hasn't been established.
Ext PV OK the PV wasn't found on this IOC and a Channel Access link has been established.
Local PV  the PV was found on this IOC.
Constant  the corresponding link field is a constant.
========= ==================================================================================

The OUTV field indicates the status of the OUT link. It has the same
possible values as the INAV-INJV fields.

See the EPICS Record Reference Manual, for more on the record name
(NAME) and description (DESC) fields.

.. csv-table:: 
   :header: "Field", "Summary", "Type", "DCT", "Initial", "Access", "Modify", "Rec Proc Monitor", "PP"
   :widths: 2, 3, 2, 1, 2, 2, 2, 3, 1

   PREC,  Display Precision,    SHORT,       Yes, 0,       Yes,    Yes,    No,               No
   HOPR,  High Operating Range, FLOAT,       Yes, 0,       Yes,    Yes,    No,               No
   LOPR,  Low Operating Range,  FLOAT,       Yes, 0,       Yes,    Yes,    No,               No
   INAV,  Link Status of INPA,  Menu,        No,  1,       Yes,    No,     No,               No
   INBV,  Link Status of INPB,  Menu,        No,  1,       Yes,    No,     No,               No
   ...,   ...,                  ...,         ..., ...,     ...,    ...,    ...,              ...
   INJV,  Link Status of INPJ,  Menu,        No,  1,       Yes,    No,     No,               No
   OUTV,  OUT PV Status,        Menu,        No,  0,       Yes,    No,     No,               No
   NAME,  Record Name,          STRING [29], Yes, 0,       Yes,    No,     No,               No
   DESC,  Description,          STRING [29], Yes, Null,    Yes,    Yes,    No,               No
   IAAV,  Link Status of INAA,  Menu,        No,  1,       Yes,    No,     No,               No
   IBBV,  Link Status of INBB,  Menu,        No,  1,       Yes,    No,     No,               No
   ...,   ...,                  ...,         ..., ...,     ...,    ...,    ...,              ...
   IJJV,  Link Status of INJJ,  Menu,        No,  1,       Yes,    No,     No,               No


Alarm Parameters
----------------

The possible alarm conditions for the luascript record are the SCAN,
READ, Calculation, and limit alarms. The SCAN and READ alarms are called
by the record support routines. The Calculation alarm is called by the
record processing routine when the CALC expression is an invalid one,
upon which an error message is generated.

 The following alarm parameters which are configured by the user define
the limit alarms for the VAL field and the severity corresponding to
those conditions.

 The HYST field defines an alarm deadband for each limit. See the EPICS
Record Reference Manual for a complete explanation of alarms and these
fields.

.. csv-table:: 
   :header: "Field", "Summary", "Type", "DCT", "Initial", "Access", "Modify", "Rec Proc Monitor", "PP"
   :widths: 2, 3, 2, 1, 2, 2, 2, 3, 1
   
   HIHI,  Hihi Alarm Limit,          FLOAT,  Yes, 0,       Yes,    Yes,    No,               Yes
   HIGH,  High Alarm Limit,          FLOAT,  Yes, 0,       Yes,    Yes,    No,               Yes
   LOW,   Low Alarm Limit,           FLOAT,  Yes, 0,       Yes,    Yes,    No,               Yes
   LOLO,  Lolo Alarm Limit,          FLOAT,  Yes, 0,       Yes,    Yes,    No,               Yes
   HHSV,  Severity for a Hihi Alarm, Menu,   Yes, 0,       Yes,    Yes,    No,               Yes
   HSV,   Severity for a High Alarm, Menu,   Yes, 0,       Yes,    Yes,    No,               Yes
   LSV,   Severity for a Low Alarm,  Menu,   Yes, 0,       Yes,    Yes,    No,               Yes
   LLSV,  Severity for a Lolo Alarm, Menu,   Yes, 0,       Yes,    Yes,    No,               Yes
   HYST,  Alarm Deadband,            DOUBLE, Yes, 0,       Yes,    Yes,    No,               No


Monitor Parameters
------------------

These parameters are used to determine when to send monitors for the
value fields. The monitors are sent when the value field exceeds the
last monitored field by the appropriate deadband, the ADEL for archiver
monitors and the MDEL field for all other types of monitors. If these
fields have a value of zero, every time the value changes, monitors are
triggered; if they have a value of -1, every time the record is scanned,
monitors are triggered.

.. csv-table:: 
   :header: "Field", "Summary", "Type", "DCT", "Initial", "Access", "Modify", "Rec Proc Monitor", "PP"
   :widths: 2, 3, 2, 1, 2, 2, 2, 3, 1

   ADEL,  Archive Deadband,                     DOUBLE, Yes, 0,       Yes,    Yes,    No,               No
   MDEL,  "Monitor, i.e. value change, Deadband", DOUBLE, Yes, 0,       Yes,    Yes,    No,               No


Run-time Parameters
-------------------

These fields are not configurable using a configuration tool and none
are modifiable at run-time. They are used to process the record.

 
Record Support Routines
-----------------------

init_record
^^^^^^^^^^^

For each constant input link, the corresponding value field is
initialized with the constant value if the input link is CONSTANT or a
channel access link is created if the input link is PV_LINK.

The CODE field is processed and either compiled into bytecode directly,
or the record will search for a given file and compile that file into
bytecode.

 

process
^^^^^^^

See section 11.

 

special
^^^^^^^

This is called if CODE is changed.

 

get_precision
^^^^^^^^^^^^^

Retrieves PREC.
 

Record Processing
-----------------

.. _process-1:

process()
^^^^^^^^^

The ``process()`` routine implements the following algorithm:

 

1. Recompile the CODE field if the RELO field is set to "Every
Processing".

 

2. Push the values of all input links to global lua variables.

 

3. Run the compiled code in a separate thread. Process the returned
value from the code to determine if it is a numeric value or a string
value. Update VAL or SVAL accordingly.

 

4. Determine if the Output Execution Option (OOPT) is met. If it is met,
execute the output link (and output event).

 

5. Check to see if monitors should be invoked.

Monitors for A-J and AA-JJ are set whenever values are changed.

 

6. Set PACT FALSE.
