The following are additional functions added to the lua runtime interpreter to 
enable access to the epics runtime environment.



**asyn.getStringParam** (*portName, addr, paramName*)  
**asyn.getDoubleParam** (*portName, addr, paramName*)  
**asyn.getIntegerParam** (*portName, addr, paramName*)  

	Fetches the value of an asyn parameter.


	portName   [string]   - The registered asyn port name that contains 
	                        the parameter you are getting.

	addr       [number]   - The asyn address of the parameter.
	
	paramName  [string]   - The name of the parameter to fetch.

	
	returns the value of the asyn parameter
	


**asyn.setStringParam** (*portName, addr, paramName, value*)  
**asyn.setDoubleParam** (*portName, addr, paramName, value*)  
**asyn.setIntegerParam** (*portName, addr, paramName, value*)  

	Sets the value of an asyn parameter.

	
	portName   [string]   - The registered asyn port name that contains 
	                        the parameter you are setting.

	addr       [number]   - The asyn address of the parameter.
	
	paramName  [string]   - The name of the parameter to set.
	
	value      [varies]   - The value to set the parameter to. Type should
	                        match the type of the parameter you are setting.



**asyn.callParamCallbacks** (*portName, addr*)

	Tells an asyn port to call parameter callbacks on changed values.
	
	
	portName   [string]   - A registered asyn port name.

	addr       [number]   - The index of the parameter list to do callbacks on.




**asyn.setOutTerminator** (*terminator*)

	Sets the global variable OutTerminator, which controls asyn write commands
	
	
	terminator [string]   - The string value to append to the end of all asyn
	                        write calls.

**asyn.getOutTerminator** ()
	
	Returns the value of the global variable OutTerminator


**asyn.setInTerminator** (*terminator*)

	Sets the global variable InTerminator, which controls asyn read commands
	
	
	terminator [string]   - The string value to wait for when reading from an
	                        asyn port.


**asyn.getInTerminator** ()
	
	Returns the value of the global variable InTerminator


**asyn.setWriteTimeout** (*timeout*)

	Sets the global variable WriteTimeout, which controls asyn write commands
	
	
	timeout   [number]    - The number of milliseconds for an asyn write command
	                        to wait before failure.


**asyn.getWriteTimeout** ()
	
	Returns the value of the global variable WriteTimeout
							
	

**asyn.setReadTimeout** (*timeout*)

	Sets the global variable ReadTimeout, which controls asyn read commands
	
	
	timeout   [number]    - The number of milliseconds for an asyn read command
	                        to wait before failure.


**asyn.getReadTimeout** ()
	
	Returns the value of the global variable ReadTimeout
							
	

**asyn.write** (*data, portName, addr*)

	Write a string to a given asynOctet port
	
	
	data      [string]    - The string to write to the port. This string will 
	                        automatically have the value of the global variable
	                        OutTerminator appended to it.

	portName   [string]   - A registered asyn port name.

	addr       [number]   - The asyn address of the parameter.
	
	

**asyn.read** (*portName, addr*)

	Read a string from a given asynOctet port


	portName   [string]   - A registered asyn port name.

	addr       [number]   - The asyn address of the parameter.


	returns a string containing all data read from the asynOctet port until encountering 
	the input terminator set by the global variable InTerminator, or until the timeout set
	by the global variable ReadTimeout is reached.



**asyn.writeread** (*data, portName, addr*)

	Writes data to a port and then reads data from that same port.

	
	portName   [string]   - A registered asyn port name.

	addr       [number]   - The asyn address of the parameter.

	
	returns a string containing all data read from the asynOctet port until encountering 
	the input terminator set by the global variable InTerminator, or until the timeout set
	by the global variable ReadTimeout is reached.
	

**asyn.port** (*portName, addr*)

	Returns a table representing a port object. This object has the functions read
	write, and readwrite, which work the same as the functions above, but the port
	and address need not be specified. The port uses the global in and out terminators
	by default, but has the functions setInTerminator and setOutTerminator to
	change that functionality (as well as getInTerminator and getOutTerminator to 
	query those values). 

	
	portName   [string]   - A registered asyn port name.

	addr       [number]   - The asyn address of the parameter.
	

**epics.get** (*PV*)
	
	Calls ca_get to retrieve the value of a PV accessible by the host.
	
	
	PV       [string]     - The name of the PV to request.
	
	
	returns the value of the PV given or Nil if the PV cannot be reached.
	
	

**epics.put** (*PV, value*)

	Calls ca_put to set the value of a PV accessible by the host.
	
	
	PV       [string]     - The name of the PV to request.
	
	value    [varies]     - The new value you want to set the PV to. The type of this
	                        parameter should match with the dbtype of the PV requested.
	
	
	
**epics.sleep** (*seconds*)

	Tells the epics thread running the lua script to sleep for a given time.
	
	
	seconds  [number]     - Amount of seconds to sleep for (can be fractional).

	
**epics.pv** (*PV*)

	Returns a table representing a PV object. Index accesses can be used to retrive or 
	change record fields. These changes are completed through ca_get or ca_put.
	
	PV       [string]    - The name of the PV to request.
