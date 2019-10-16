===================
IOC Shell Functions
===================

**iocsh.** (*arguments…*)

::

   Searches the list of registered IOC shell functions to find a function with the given
   name. If found, the arguments given are converted to the types that the function expects
   and the function is called. While within the luash interpreter, the iocsh library reference
   can be ommitted.

   arguments   [varies] - A list of the arguments you would normally use in the IOC shell.
