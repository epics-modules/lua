==========================
Asyn Library Documentation
==========================

Getting / Setting Parameters
----------------------------

* **asyn.getParam** (*portName[, addr], paramName*)  
* **asyn.getStringParam** (*portName[, addr], paramName*)  
* **asyn.getDoubleParam** (*portName[, addr], paramName*)  
* **asyn.getIntegerParam** (*portName[, addr], paramName*)  

::

   Fetches the value of an asyn parameter. These work like the asynPortDriver
   functions of the same name, retrieving the value from the param list.


   portName    [string] - The registered asyn port name that contains
                          the parameter you are getting.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.

   paramName   [string] - The name of the parameter to fetch.


   Returns the value of the asyn parameter as the type specified, if no
   type was specified, uses the asynParamType of the parameter to determine       


* **asyn.setParam** (*portName[, addr], paramName*)
* **asyn.setStringParam** (*portName[, addr], paramName, value*)
* **asyn.setDoubleParam** (*portName[, addr], paramName, value*)
* **asyn.setIntegerParam** (*portName[, addr], paramName, value*)

::

   Sets the value of an asyn parameter. These work like the asynPortDriver
   functions of the same name, saving the value in the param list.


   portName    [string] - The registered asyn port name that contains
                          the parameter you are setting.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.

   paramName   [string] - The name of the parameter to set.

   value       [varies] - The value to set the parameter to. Type should
                          match the type of the parameter you are setting.

						
**asyn.callParamCallbacks** (*portName[, addr, parameter]*)

::

   Tells an asyn port to call parameter callbacks on changed values.


   portName    [string] - A registered asyn port name.

   addr        [number] - The index of the parameter list to do callbacks on. Optional,
                          default value is 0.
       
   parameter   [string] - A specific parameter to do callbacks on. Optional,
                          default is to perform callbacks on all values that have
                          been changed.
						
						
Reading / Writing Values
------------------------
						
						
**asyn.readParam** (*portName[, addr], paramName*)

::

   Calls the read function of the correct asyn interface

   portName    [string] - The registered asyn port name that contains
                          the parameter you are getting.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.

   paramName   [string] - The name of the parameter to fetch.


   Returns the value of the asyn parameter as the type specified, if no
   type was specified, uses the asynParamType of the parameter to determine

						
**asyn.writeParam** (*portName[, addr], paramName, value*)

::

   Calls the write function of the correct asyn interface


   portName    [string] - The registered asyn port name that contains
                          the parameter you are setting.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.

   paramName   [string] - The name of the parameter to set.

   value       [varies] - The value to write to the parameter. Type should
                          match the type of the parameter you are setting.

Configuration Parameters
------------------------
						
**asyn.setOutTerminator** (*terminator*)

::

   Sets the global variable OutTerminator, which controls asyn write commands


   terminator  [string] - The string value to append to the end of all asyn
                          write calls.

**asyn.getOutTerminator** ()

::

   Returns the value of the global variable OutTerminator

**asyn.setInTerminator** (*terminator*)

::

   Sets the global variable InTerminator, which controls asyn read commands


   terminator  [string] - The string value to wait for when reading from an
                          asyn port.

**asyn.getInTerminator** ()

::

   Returns the value of the global variable InTerminator

**asyn.setWriteTimeout** (*timeout*)

::

   Sets the global variable WriteTimeout, which controls asyn write commands


   timeout     [number] - The number of milliseconds for an asyn write command
                          to wait before failure.

**asyn.getWriteTimeout** ()

::

   Returns the value of the global variable WriteTimeout

**asyn.setReadTimeout** (*timeout*)

::

   Sets the global variable ReadTimeout, which controls asyn read commands


   timeout     [number] - The number of milliseconds for an asyn read command
                          to wait before failure.

**asyn.getReadTimeout** ()

::

   Returns the value of the global variable ReadTimeout

Debug Information
-----------------

* **asyn.setTrace** (*portName[, addr], key, val*) 
* **asyn.setTrace** (*portName[, addr], {key1=val1, …}*)

::

   Turns on or off asyn's tracing for a mask on a given port. Valid keys are 
   "error", "device", "filter", "driver", "flow", and "warning", case insensitive.
       
       
   portName    [string]  - A registered asyn port name.

   addr        [number]  - The asyn address of the parameter. Optional,
                           default value is 0.

   key         [string]  - Which mask to change

   val         [boolean] - Whether to turn on or off the mask

       

* **asyn.setTraceIO** (*portName[, addr], key, val*) 
* **asyn.setTraceIO** (*portName[, addr], {key1=val1, …}*)

::

   Turns on or off asyn's tracing for a mask on a given port. Valid keys are 
   "nodata", "ascii", "escape", and "hex", case insensitive.


   portName    [string]  - A registered asyn port name.

   addr        [number]  - The asyn address of the parameter. Optional,
                           default value is 0.

   key         [string]  - Which mask to change

   val         [boolean] - Whether to turn on or off the mask

Octet Communications
--------------------
	
**asyn.write** (*data, portName[, addr, parameter]*)

::

   Write a string to a given asynOctet port


   data        [string] - The string to write to the port. This string will
                          automatically have the value of the global variable
                          OutTerminator appended to it.

   portName    [string] - A registered asyn port name.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.
       
   parameter   [string] - An asyn parameter to write to. Optional.

**asyn.read** (*portName[, addr, parameter]*)

::

   Read a string from a given asynOctet port


   portName    [string] - A registered asyn port name.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.
       
   parameter   [string] - An asyn parameter to read from. Optional.


   Returns a string containing all data read from the asynOctet port until encountering
   the input terminator set by the global variable InTerminator, or until the timeout set
   by the global variable ReadTimeout is reached.

**asyn.writeread** (*data, portName[, addr, parameter]*)

::

   Writes data to a port and then reads data from that same port.


   portName    [string] - A registered asyn port name.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.
       
   parameter   [string] - An asyn parameter to read and write to. Optional.


   Returns a string containing all data read from the asynOctet port until encountering
   the input terminator set by the global variable InTerminator, or until the timeout set
   by the global variable ReadTimeout is reached.

**asyn.setOption** (*portName[, addr], key, val*)

::

   Sets driver-specific options

   
   portName    [string] - A registered asyn port name.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.

   key         [string] - The name of the option you are setting.

   val         [string] - The value to set the option to.


   Returns the asynStatus value of the asynSetOption call.

asynOctetClient Object
----------------------

**asyn.client** (*portName[, addr, parameter]*)

::

   Returns a table representing an asynOctetClient object. This object has the functions 
   read, write, and readwrite, which work the same as the functions above, but the port
   and address need not be specified. The client copies the global in and out terminators
   at creation, but you can also set the table's InTerminator and/or OutTerminator fields 
   manually to a different value. 


   portName    [string] - A registered asyn port name.

   addr        [number] - The asyn address of the parameter. Optional,
                          default value is 0.
       
   parameter   [string] - A specific asyn parameter. Optional.


* **client:trace** (*key, val*) 
* **client:trace** (*{key1=val1, …}*)

::

   Turns on or off asyn's tracing for a given mask on the port this client is connected 
   to. Valid keys are "error", "device", "filter", "driver", "flow", and "warning", case
   insensitive.

   key         [string]  - Which mask to change

   val         [boolean] - Whether to turn on or off the mask

       

* **client:traceio** (*key, val*) 
* **client:traceio** (*{key1=val1, …}*)

::

   Turns on or off asyn's tracing for a given mask on the port this client is connected 
   to. Valid keys are "nodata", "ascii", "escape", and "hex", case insensitive.

   key         [string]  - Which mask to change

   val         [boolean] - Whether to turn on or off the mask

**client:setOption** (*key, val*)

::

   Sets an asynOption for the port this client is connected to.

   key         [string] - The name of the option you are setting.

   val         [string] - The value to set the option to.


   Returns the asynStatus of the asynSetOption call.
	
asynPortDriver Object
---------------------

**asyn.driver** (*portName*)

::

   Returns a table representing an asynPortDriver object. You can read to and write to
   keys in the table and the table will try to resolve the names as asyn parameters,
   calling getParam or setParam as necessary. The table also indexes the addresses that
   the asynPortDriver implements, so driver[1]["VAL"] gets the VAL param associated
   with address 1, rather than the default 0.

   portName    [string] - A registered asynPortDriver port name

* **driver:readParam** (*paramName*) 
* **driver:writeParam** (*paramName, value*)

::

   Calls the read or write function of the correct asyn interface based upon
   the asynParamType of the parameter being written to or read from.

   paramName   [string] - The name of a parameter in the driver

   value       [varies] - The new value to have the driver write (for writeParam)


   Returns the value the the driver returns from the read function (for readParam)
       
       
