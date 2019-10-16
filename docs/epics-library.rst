===============
Epics Functions
===============

**epics.get** (*PV*)

::

   Calls ca_get to retrieve the value of a PV accessible by the host.


   PV          [string] - The name of the PV to request.


   Returns the value of the PV given or Nil if the PV cannot be reached.

**epics.put** (*PV, value*)

::

   Calls ca_put to set the value of a PV accessible by the host.


   PV          [string] - The name of the PV to request.

   value       [varies] - The new value you want to set the PV to. The type of this
                          parameter should match with the dbtype of the PV requested.

**epics.sleep** (*seconds*)

::

   Tells the epics thread running the lua script to sleep for a given time.


   seconds     [number] - Amount of seconds to sleep for (can be fractional).

**epics.pv** (*PV*)

::

   Returns a table representing a PV object. Index accesses can be used to retrive or
   change record fields. These changes are completed through ca_get or ca_put.

   PV          [string] - The name of the PV to request.
